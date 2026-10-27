#pragma once

// Topology-preserving-transform state variable filters (A. Simper, Cytomic).
// They stay stable and click-free under fast parameter modulation, and the
// same design code gives the exact digital response for drawing curves.

#include <algorithm>
#include <cmath>
#include <complex>

namespace suite::dsp {

constexpr double kPi = 3.14159265358979323846;

enum class FilterType : int
{
	Bell,
	LowShelf,
	HighShelf,
	LowCut,
	HighCut,
	Notch,
	BandPass,
	TiltShelf,
	kCount
};

inline bool filterTypeHasGain (FilterType t)
{
	return t == FilterType::Bell || t == FilterType::LowShelf || t == FilterType::HighShelf ||
	       t == FilterType::TiltShelf;
}

inline bool filterTypeHasSlope (FilterType t)
{
	return t == FilterType::LowCut || t == FilterType::HighCut;
}

// Cut slopes in dB/octave and the filter order each one needs.
constexpr int kSlopeOrders[] = {1, 2, 3, 4, 6, 8, 12, 16};
constexpr int kNumSlopes = 8;

struct SvfCoeffs
{
	double g {0}, k {0}, a1 {0}, a2 {0}, a3 {0};
	double m0 {1}, m1 {0}, m2 {0};

	void setGK (double gIn, double kIn)
	{
		g = gIn;
		k = kIn;
		a1 = 1.0 / (1.0 + g * (g + k));
		a2 = g * a1;
		a3 = g * a2;
	}

	std::complex<double> response (double w) const // w = tan(pi f / fs)
	{
		std::complex<double> s (0.0, w / g);
		auto d = s * s + k * s + 1.0;
		return m0 + m1 * s / d + m2 / d;
	}
};

struct OnePoleCoeffs
{
	double g {0}, G {0};
	bool highpass {false};

	std::complex<double> response (double w) const
	{
		std::complex<double> s (0.0, w / g);
		return highpass ? s / (s + 1.0) : 1.0 / (s + 1.0);
	}
};

struct SvfState
{
	double ic1 {0}, ic2 {0};

	void reset () { ic1 = ic2 = 0; }

	inline double process (double v0, const SvfCoeffs& c)
	{
		double v3 = v0 - ic2;
		double v1 = c.a1 * ic1 + c.a2 * v3;
		double v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
		ic1 = 2.0 * v1 - ic1;
		ic2 = 2.0 * v2 - ic2;
		return c.m0 * v0 + c.m1 * v1 + c.m2 * v2;
	}
};

struct OnePoleState
{
	double s {0};

	void reset () { s = 0; }

	inline double process (double x, const OnePoleCoeffs& c)
	{
		double v = (x - s) * c.G;
		double lp = v + s;
		s = lp + v;
		return c.highpass ? x - lp : lp;
	}
};

// A complete band: up to 8 biquad sections plus an optional one-pole.
struct BandDesign
{
	static constexpr int kMaxSections = 8;

	SvfCoeffs sections[kMaxSections];
	int numSections {0};
	OnePoleCoeffs onePole;
	bool hasOnePole {false};
	double outputScale {1.0};

	double magnitude (double freq, double sampleRate) const
	{
		double w = std::tan (kPi * std::min (freq, sampleRate * 0.4999) / sampleRate);
		std::complex<double> h (outputScale, 0.0);
		for (int i = 0; i < numSections; ++i)
			h *= sections[i].response (w);
		if (hasOnePole)
			h *= onePole.response (w);
		return std::abs (h);
	}
};

inline BandDesign designBand (FilterType type, double freq, double q, double gainDb, int slopeIndex,
                              double sampleRate)
{
	BandDesign d;
	freq = std::clamp (freq, 1.0, sampleRate * 0.49);
	q = std::max (q, 0.025);
	const double w = std::tan (kPi * freq / sampleRate);
	const double A = std::pow (10.0, gainDb / 40.0);

	auto& c = d.sections[0];
	d.numSections = 1;

	switch (type)
	{
		case FilterType::Bell:
			c.setGK (w, 1.0 / (q * A));
			c.m0 = 1.0;
			c.m1 = c.k * (A * A - 1.0);
			c.m2 = 0.0;
			break;
		case FilterType::LowShelf:
			c.setGK (w / std::sqrt (A), 1.0 / q);
			c.m0 = 1.0;
			c.m1 = c.k * (A - 1.0);
			c.m2 = A * A - 1.0;
			break;
		case FilterType::HighShelf:
		case FilterType::TiltShelf:
			c.setGK (w * std::sqrt (A), 1.0 / q);
			c.m0 = A * A;
			c.m1 = c.k * (1.0 - A) * A;
			c.m2 = 1.0 - A * A;
			if (type == FilterType::TiltShelf)
				d.outputScale = 1.0 / A; // pivot: -g/2 below, +g/2 above
			break;
		case FilterType::Notch:
			c.setGK (w, 1.0 / q);
			c.m0 = 1.0;
			c.m1 = -c.k;
			c.m2 = 0.0;
			break;
		case FilterType::BandPass:
			c.setGK (w, 1.0 / q);
			c.m0 = 0.0;
			c.m1 = c.k; // unity gain at the peak
			c.m2 = 0.0;
			break;
		case FilterType::LowCut:
		case FilterType::HighCut:
		{
			const bool hp = type == FilterType::LowCut;
			const int order = kSlopeOrders[std::clamp (slopeIndex, 0, kNumSlopes - 1)];
			const int pairs = order / 2;
			d.numSections = pairs;
			d.hasOnePole = (order & 1) != 0;
			// Butterworth pole Qs; the user's Q adds resonance to the sharpest pair.
			for (int i = 0; i < pairs; ++i)
			{
				const double theta = (order & 1) ? (i + 1) * kPi / order : (2 * i + 1) * kPi / (2.0 * order);
				double secQ = 1.0 / (2.0 * std::cos (theta));
				if (i == pairs - 1)
					secQ *= q / 0.70710678;
				auto& s = d.sections[i];
				s.setGK (w, 1.0 / secQ);
				if (hp)
				{
					s.m0 = 1.0;
					s.m1 = -s.k;
					s.m2 = -1.0;
				}
				else
				{
					s.m0 = 0.0;
					s.m1 = 0.0;
					s.m2 = 1.0;
				}
			}
			if (d.hasOnePole)
			{
				d.onePole.g = w;
				d.onePole.G = w / (1.0 + w);
				d.onePole.highpass = hp;
			}
			break;
		}
		case FilterType::kCount: break;
	}
	return d;
}

// Runs a BandDesign on one channel.
struct BandState
{
	SvfState sections[BandDesign::kMaxSections];
	OnePoleState onePole;

	void reset ()
	{
		for (auto& s : sections)
			s.reset ();
		onePole.reset ();
	}

	inline double process (double x, const BandDesign& d)
	{
		for (int i = 0; i < d.numSections; ++i)
			x = sections[i].process (x, d.sections[i]);
		if (d.hasOnePole)
			x = onePole.process (x, d.onePole);
		return x * d.outputScale;
	}
};

} // namespace suite::dsp
