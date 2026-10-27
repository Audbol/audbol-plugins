#include "suite/dsp/SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>

namespace suite::dsp {

namespace {
constexpr float kFloorDb = -120.0f;
}

SpectrumAnalyzer::SpectrumAnalyzer () { resizeFor (sampleRate); }

void SpectrumAnalyzer::setSampleRate (double sr)
{
	if (sr > 0.0 && sr != sampleRate)
		resizeFor (sr);
}

void SpectrumAnalyzer::resizeFor (double sr)
{
	sampleRate = sr;
	// Keep roughly 10 Hz resolution regardless of sample rate.
	order = sr > 100000.0 ? 14 : sr > 50000.0 ? 13 : 12;
	fft.setOrder (order);
	const int n = fft.size ();

	ring.assign (n, 0.0f);
	writePos = 0;
	buffer.resize (n);
	power.assign (n / 2 + 1, 0.0f);

	window.resize (n);
	double sum = 0.0;
	for (int i = 0; i < n; ++i)
	{
		window[i] = float (0.5 - 0.5 * std::cos (2.0 * 3.14159265358979323846 * i / (n - 1))); // Hann
		sum += window[i];
	}
	windowGain = float (2.0 / sum); // full-scale sine reads 0 dB
}

void SpectrumAnalyzer::clear ()
{
	std::fill (ring.begin (), ring.end (), 0.0f);
	writePos = 0;
}

void SpectrumAnalyzer::push (const float* samples, int count)
{
	const int n = static_cast<int> (ring.size ());
	for (int i = 0; i < count; ++i)
	{
		ring[writePos] = samples[i];
		writePos = (writePos + 1) % n;
	}
	samplesSinceCompute += count;
}

void SpectrumAnalyzer::compute (const std::vector<float>& freqs, std::vector<float>& outDb,
                                float dt)
{
	const int n = fft.size ();
	const int bins = n / 2;

	for (int i = 0; i < n; ++i)
		buffer[i] = {ring[(writePos + i) % n] * window[i], 0.0f};
	fft.forward (buffer.data ());
	for (int i = 0; i <= bins; ++i)
		power[i] = std::norm (buffer[i % n]) * windowGain * windowGain;
	samplesSinceCompute = 0;

	if (outDb.size () != freqs.size ())
		outDb.assign (freqs.size (), kFloorDb);

	const double binHz = sampleRate / n;
	const float release = releaseDbPerSec * dt;

	for (size_t c = 0; c < freqs.size (); ++c)
	{
		// Bin range covered by this column (geometric midpoints to neighbours).
		const double f = freqs[c];
		const double fLo = c > 0 ? std::sqrt (f * freqs[c - 1]) : f;
		const double fHi = c + 1 < freqs.size () ? std::sqrt (f * freqs[c + 1]) : f;
		const double bLo = fLo / binHz;
		const double bHi = fHi / binHz;

		float p = 0.0f;
		if (bHi - bLo >= 1.0)
		{
			// Several bins per column: take the peak so tones keep their height.
			int i0 = std::max (1, int (std::ceil (bLo)));
			int i1 = std::min (bins, int (std::floor (bHi)));
			for (int i = i0; i <= i1; ++i)
				p = std::max (p, power[i]);
		}
		else
		{
			// Fewer bins than columns: interpolate.
			double b = std::clamp (f / binHz, 1.0, double (bins - 1));
			int i = int (b);
			float t = float (b - i);
			p = power[i] * (1.0f - t) + power[i + 1] * t;
		}

		float db = p > 1e-14f ? 10.0f * std::log10 (p) : kFloorDb;
		db += tilt * float (std::log2 (f / 1000.0));
		db = std::max (db, kFloorDb);

		float& prev = outDb[c];
		prev = db >= prev ? db : std::max (db, prev - release);
	}
}

} // namespace suite::dsp
