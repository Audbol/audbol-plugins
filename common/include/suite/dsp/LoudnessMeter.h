#pragma once

#include "suite/dsp/TruePeak.h"

#include <array>
#include <cstdint>

namespace suite::dsp {

// Stereo loudness measurement per ITU-R BS.1770-4 / EBU R128 / EBU 3342:
// K-weighting, momentary (400 ms) and short-term (3 s) loudness updated
// every 100 ms, gated integrated loudness, loudness range, and true peak.
// Integrated and range use 0.1 LU histograms, so they cover a whole session
// in constant memory.
class LoudnessMeter
{
public:
	static constexpr double kFloor = -70.0; // LUFS; also the absolute gate

	void prepare (double sampleRate);
	void reset ();           // everything
	void resetIntegrated (); // integrated, range and max true peak only

	// One stereo sample. Returns true when a 100 ms update completed.
	bool push (double left, double right);

	double momentary () const { return momentaryLufs; }
	double shortTerm () const { return shortTermLufs; }
	double integrated () const { return integratedLufs; }
	double range () const { return rangeLu; }
	double truePeakMax () const;   // dBTP since the last resetIntegrated
	double truePeakRecent () const; // dBTP of the last 100 ms

private:
	struct Biquad
	{
		double b0 {1}, b1 {0}, b2 {0}, a1 {0}, a2 {0};
		double z1 {0}, z2 {0};
		double process (double x)
		{
			const double y = b0 * x + z1;
			z1 = b1 * x - a1 * y + z2;
			z2 = b2 * x - a2 * y;
			return y;
		}
	};

	struct Histogram
	{
		static constexpr int kBins = 1000; // -70 .. +30 LUFS in 0.1 LU steps
		std::array<uint32_t, kBins> count {};
		std::array<double, kBins> energy {};
		void clear ()
		{
			count.fill (0);
			energy.fill (0.0);
		}
		void add (double lufs, double e);
		double gatedMean (double gate) const;  // loudness of energy above gate
		double percentile (double gate, double p) const;
	};

	void hop ();

	double sampleRate {48000.0};
	Biquad shelf[2], highpass[2];
	TruePeakDetector truePeak[2];

	int hopSamples {4800};
	int hopCount {0};
	double hopEnergy {0};
	std::array<double, 30> subBlocks {}; // last 3 s of 100 ms mean-square energies
	int subBlockPos {0};
	int subBlocksFilled {0};

	Histogram integratedHist, rangeHist;
	double momentaryLufs {kFloor}, shortTermLufs {kFloor}, integratedLufs {kFloor}, rangeLu {0.0};
	double peakHop {0}, peakRecent {0}, peakMax {0};
};

} // namespace suite::dsp
