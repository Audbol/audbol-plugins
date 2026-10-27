#include "suite/dsp/LoudnessMeter.h"

#include <algorithm>
#include <cmath>

namespace suite::dsp {

namespace {

constexpr double kPi = 3.14159265358979323846;

double energyToLufs (double e) { return e > 0.0 ? -0.691 + 10.0 * std::log10 (e) : -1000.0; }
double lufsToEnergy (double l) { return std::pow (10.0, (l + 0.691) / 10.0); }

} // namespace

void LoudnessMeter::prepare (double sr)
{
	sampleRate = sr;
	// K-weighting from its analog prototype, valid at any sample rate
	// (same parameters as libebur128). Stage 1: high shelf, ~+4 dB.
	{
		const double f0 = 1681.974450955533, g = 3.999843853973347, q = 0.7071752369554196;
		const double k = std::tan (kPi * f0 / sr);
		const double vh = std::pow (10.0, g / 20.0);
		const double vb = std::pow (vh, 0.4996667741545416);
		const double a0 = 1.0 + k / q + k * k;
		for (auto& f : shelf)
		{
			f.b0 = (vh + vb * k / q + k * k) / a0;
			f.b1 = 2.0 * (k * k - vh) / a0;
			f.b2 = (vh - vb * k / q + k * k) / a0;
			f.a1 = 2.0 * (k * k - 1.0) / a0;
			f.a2 = (1.0 - k / q + k * k) / a0;
		}
	}
	// Stage 2: RLB high-pass at ~38 Hz.
	{
		const double f0 = 38.13547087602444, q = 0.5003270373238773;
		const double k = std::tan (kPi * f0 / sr);
		const double a0 = 1.0 + k / q + k * k;
		for (auto& f : highpass)
		{
			f.b0 = 1.0;
			f.b1 = -2.0;
			f.b2 = 1.0;
			f.a1 = 2.0 * (k * k - 1.0) / a0;
			f.a2 = (1.0 - k / q + k * k) / a0;
		}
	}
	hopSamples = std::max (1, static_cast<int> (std::round (sr * 0.1)));
	reset ();
}

void LoudnessMeter::reset ()
{
	for (int c = 0; c < 2; ++c)
	{
		shelf[c].z1 = shelf[c].z2 = 0.0;
		highpass[c].z1 = highpass[c].z2 = 0.0;
		truePeak[c].reset ();
	}
	hopCount = 0;
	hopEnergy = 0.0;
	subBlocks.fill (0.0);
	subBlockPos = subBlocksFilled = 0;
	momentaryLufs = shortTermLufs = kFloor;
	peakHop = peakRecent = 0.0;
	resetIntegrated ();
}

void LoudnessMeter::resetIntegrated ()
{
	integratedHist.clear ();
	rangeHist.clear ();
	integratedLufs = kFloor;
	rangeLu = 0.0;
	peakMax = 0.0;
}

bool LoudnessMeter::push (double left, double right)
{
	const double x[2] = {left, right};
	for (int c = 0; c < 2; ++c)
	{
		const double k = highpass[c].process (shelf[c].process (x[c]));
		hopEnergy += k * k; // channel weights are 1.0 for L and R
		peakHop = std::max (peakHop, truePeak[c].process (x[c]));
	}
	if (++hopCount < hopSamples)
		return false;
	hop ();
	return true;
}

void LoudnessMeter::hop ()
{
	subBlocks[subBlockPos] = hopEnergy / hopCount;
	subBlockPos = (subBlockPos + 1) % static_cast<int> (subBlocks.size ());
	subBlocksFilled = std::min (subBlocksFilled + 1, static_cast<int> (subBlocks.size ()));
	hopEnergy = 0.0;
	hopCount = 0;
	peakRecent = peakHop;
	peakMax = std::max (peakMax, peakHop);
	peakHop = 0.0;

	auto meanOfLast = [this] (int n) {
		double sum = 0.0;
		for (int i = 1; i <= n; ++i)
			sum += subBlocks[(subBlockPos - i + subBlocks.size ()) % subBlocks.size ()];
		return sum / n;
	};

	// Momentary: 400 ms block, 75 % overlap. Each one feeds the integrated gate.
	if (subBlocksFilled >= 4)
	{
		const double e = meanOfLast (4);
		momentaryLufs = std::max (kFloor, energyToLufs (e));
		integratedHist.add (energyToLufs (e), e);
		integratedLufs = integratedHist.gatedMean (-10.0);
	}
	// Short-term: 3 s block, feeds the loudness range.
	if (subBlocksFilled >= static_cast<int> (subBlocks.size ()))
	{
		const double e = meanOfLast (static_cast<int> (subBlocks.size ()));
		shortTermLufs = std::max (kFloor, energyToLufs (e));
		rangeHist.add (energyToLufs (e), e);
		const double lo = rangeHist.percentile (-20.0, 0.10);
		const double hi = rangeHist.percentile (-20.0, 0.95);
		rangeLu = hi > lo ? hi - lo : 0.0;
	}
}

double LoudnessMeter::truePeakMax () const
{
	return peakMax > 0.0 ? 20.0 * std::log10 (peakMax) : -150.0;
}

double LoudnessMeter::truePeakRecent () const
{
	return peakRecent > 0.0 ? 20.0 * std::log10 (peakRecent) : -150.0;
}

// --- Histogram ---------------------------------------------------------------

void LoudnessMeter::Histogram::add (double lufs, double e)
{
	if (lufs <= kFloor) // absolute gate
		return;
	const int bin = std::clamp (static_cast<int> ((lufs - kFloor) * 10.0), 0, kBins - 1);
	++count[bin];
	energy[bin] += e;
}

double LoudnessMeter::Histogram::gatedMean (double relativeGate) const
{
	// Mean energy above the absolute gate -> relative threshold -> mean
	// energy of the blocks at or above it.
	double sum = 0.0;
	uint64_t n = 0;
	for (int i = 0; i < kBins; ++i)
	{
		sum += energy[i];
		n += count[i];
	}
	if (n == 0)
		return kFloor;
	const double threshold = energyToLufs (sum / n) + relativeGate;
	const int first = std::clamp (static_cast<int> (std::ceil ((threshold - kFloor) * 10.0)), 0, kBins);
	sum = 0.0;
	n = 0;
	for (int i = first; i < kBins; ++i)
	{
		sum += energy[i];
		n += count[i];
	}
	return n ? std::max (kFloor, energyToLufs (sum / n)) : kFloor;
}

double LoudnessMeter::Histogram::percentile (double relativeGate, double p) const
{
	double sum = 0.0;
	uint64_t n = 0;
	for (int i = 0; i < kBins; ++i)
	{
		sum += energy[i];
		n += count[i];
	}
	if (n == 0)
		return kFloor;
	const double threshold = energyToLufs (sum / n) + relativeGate;
	const int first = std::clamp (static_cast<int> (std::ceil ((threshold - kFloor) * 10.0)), 0, kBins);
	uint64_t total = 0;
	for (int i = first; i < kBins; ++i)
		total += count[i];
	if (total == 0)
		return kFloor;
	const double target = p * (total - 1);
	uint64_t cumulative = 0;
	for (int i = first; i < kBins; ++i)
	{
		cumulative += count[i];
		if (cumulative > target)
			return kFloor + (i + 0.5) / 10.0; // bin centre
	}
	return kFloor + (kBins - 0.5) / 10.0;
}

} // namespace suite::dsp
