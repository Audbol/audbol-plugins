#pragma once

#include <algorithm>
#include <cmath>

namespace suite::dsp {

// One-pole exponential smoother. Reaches ~99% of a step after `timeMs`.
class Smoother
{
public:
	void setup (double sampleRate, double timeMs)
	{
		double samples = std::max (1.0, sampleRate * timeMs * 0.001);
		coef = 1.0 - std::exp (-4.6 / samples);
	}

	void reset (double v) { current = target = v; }
	void setTarget (double v) { target = v; }
	double getTarget () const { return target; }
	double getCurrent () const { return current; }
	bool isSmoothing () const { return std::abs (target - current) > epsilon; }

	double next ()
	{
		current += (target - current) * coef;
		if (std::abs (target - current) <= epsilon)
			current = target;
		return current;
	}

	// Advance n samples at once (for control-rate updates).
	double skip (int n)
	{
		if (!isSmoothing ())
			return current;
		current = target + (current - target) * std::pow (1.0 - coef, n);
		if (std::abs (target - current) <= epsilon)
			current = target;
		return current;
	}

private:
	static constexpr double epsilon = 1e-6;
	double current {0.0};
	double target {0.0};
	double coef {1.0};
};

inline double dbToGain (double db) { return std::pow (10.0, db * 0.05); }
inline double gainToDb (double g, double floorDb = -150.0)
{
	return g > 0.0 ? std::max (floorDb, 20.0 * std::log10 (g)) : floorDb;
}

} // namespace suite::dsp
