#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

namespace suite::dsp {

// 4x polyphase oversampler with a linear-phase Kaiser-windowed FIR.
// Passband to ~0.4 fs, stopband from ~0.6 fs (aliases above 0.4 fs fold to
// inaudible frequencies). Round-trip latency is exactly kLatency base-rate
// samples.
class Oversampler4x
{
public:
	static constexpr int kFactor = 4;
	static constexpr int kTaps = 129;
	static constexpr int kPhaseTaps = (kTaps + kFactor - 1) / kFactor; // 33
	static constexpr int kLatency = (kTaps - 1) / kFactor;             // 32

	Oversampler4x ()
	{
		constexpr double pi = 3.14159265358979323846;
		constexpr double beta = 8.0;
		const double center = (kTaps - 1) * 0.5;
		const double fc = 0.5 / kFactor; // cycles per oversampled sample
		double sum = 0.0;
		for (int i = 0; i < kTaps; ++i)
		{
			const double x = i - center;
			const double sinc = x == 0.0 ? 2.0 * fc : std::sin (2.0 * pi * fc * x) / (pi * x);
			const double r = x / center;
			const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / besselI0 (beta);
			h[i] = sinc * w;
			sum += h[i];
		}
		for (auto& v : h)
			v /= sum; // unity DC gain
		for (int k = 0; k < kFactor; ++k)
			for (int j = 0; j < kPhaseTaps; ++j)
			{
				const int i = k + kFactor * j;
				phase[k][j] = i < kTaps ? h[i] * kFactor : 0.0;
			}
		reset ();
	}

	void reset ()
	{
		std::fill (std::begin (inHist), std::end (inHist), 0.0);
		std::fill (std::begin (osHist), std::end (osHist), 0.0);
		inPos = osPos = 0;
	}

	// The histories are mirrored rings (every sample is written twice, one
	// ring length apart) so the filter loops read contiguous memory without
	// wrap-around arithmetic and can vectorize.

	// One base-rate sample in, kFactor oversampled samples out.
	void up (double x, double* out)
	{
		inPos = (inPos + kInRing - 1) % kInRing;
		inHist[inPos] = inHist[inPos + kInRing] = x;
		const double* hist = inHist + inPos; // hist[j] = x[n - j]
		for (int k = 0; k < kFactor; ++k)
		{
			double y = 0.0;
			for (int j = 0; j < kPhaseTaps; ++j)
				y += phase[k][j] * hist[j];
			out[k] = y;
		}
	}

	// kFactor oversampled samples in, one base-rate sample out.
	double down (const double* in)
	{
		for (int k = 0; k < kFactor; ++k)
		{
			osPos = (osPos + kOsRing - 1) % kOsRing;
			osHist[osPos] = osHist[osPos + kOsRing] = in[k];
		}
		// Align to the first sample of the group (index 4n) so the total
		// latency is a whole number of base samples.
		const double* hist = osHist + osPos + (kFactor - 1);
		double y = 0.0;
		for (int i = 0; i < kTaps; ++i)
			y += h[i] * hist[i];
		return y;
	}

private:
	static double besselI0 (double x)
	{
		double sum = 1.0, term = 1.0;
		for (int k = 1; k < 32; ++k)
		{
			term *= (x / (2.0 * k)) * (x / (2.0 * k));
			sum += term;
		}
		return sum;
	}

	static constexpr int kInRing = kPhaseTaps;
	static constexpr int kOsRing = kTaps + kFactor; // reads reach kFactor - 1 past kTaps

	double h[kTaps] {};
	double phase[kFactor][kPhaseTaps] {};
	double inHist[2 * kInRing] {};
	double osHist[2 * kOsRing] {};
	int inPos {0}, osPos {0};
};

} // namespace suite::dsp
