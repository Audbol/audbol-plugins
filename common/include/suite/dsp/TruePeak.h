#pragma once

#include <algorithm>
#include <cmath>

namespace suite::dsp {

// 4x oversampled peak estimator (ITU-R BS.1770 style). process() returns the
// largest absolute value among the input and three interpolated points; the
// result describes the signal around `kDelay` samples ago.
class TruePeakDetector
{
public:
	static constexpr int kPhases = 4;
	static constexpr int kTapsPerPhase = 12;
	static constexpr int kDelay = 6; // ~ (kPhases * kTapsPerPhase - 1) / 2 / kPhases

	TruePeakDetector ()
	{
		constexpr int n = kPhases * kTapsPerPhase;
		constexpr double pi = 3.14159265358979323846;
		const double center = (n - 1) * 0.5;
		double h[n];
		for (int i = 0; i < n; ++i)
		{
			const double x = (i - center) / kPhases;
			const double sinc = std::abs (x) < 1e-12 ? 1.0 : std::sin (pi * x) / (pi * x);
			const double w = 0.42 - 0.5 * std::cos (2 * pi * i / (n - 1)) + 0.08 * std::cos (4 * pi * i / (n - 1));
			h[i] = sinc * w;
		}
		for (int p = 0; p < kPhases; ++p)
		{
			double sum = 0.0;
			for (int j = 0; j < kTapsPerPhase; ++j)
				sum += h[j * kPhases + p];
			for (int j = 0; j < kTapsPerPhase; ++j)
				coef[p][j] = h[j * kPhases + p] / sum; // unity DC gain per phase
		}
	}

	void reset ()
	{
		std::fill (std::begin (hist), std::end (hist), 0.0);
		pos = 0;
	}

	double process (double x)
	{
		pos = (pos + kTapsPerPhase - 1) % kTapsPerPhase;
		hist[pos] = x;
		double peak = std::abs (hist[(pos + kDelay) % kTapsPerPhase]);
		for (int p = 0; p < kPhases; ++p)
		{
			double y = 0.0;
			for (int j = 0; j < kTapsPerPhase; ++j)
				y += coef[p][j] * hist[(pos + j) % kTapsPerPhase];
			peak = std::max (peak, std::abs (y));
		}
		return peak;
	}

private:
	double coef[kPhases][kTapsPerPhase] {};
	double hist[kTapsPerPhase] {};
	int pos {0};
};

} // namespace suite::dsp
