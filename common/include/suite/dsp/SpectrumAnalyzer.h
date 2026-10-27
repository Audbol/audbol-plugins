#pragma once

#include "suite/dsp/FFT.h"

#include <complex>
#include <vector>

namespace suite::dsp {

// UI-side spectrum analyzer. Audio is pushed in (from the processor tap) and
// compute() renders a smoothed, slope-compensated dB curve for an arbitrary
// set of display frequencies (one per pixel column).
class SpectrumAnalyzer
{
public:
	SpectrumAnalyzer ();

	void setSampleRate (double sr);
	double getSampleRate () const { return sampleRate; }

	void push (const float* samples, int count);
	void clear ();

	// Slope added in dB/octave around 1 kHz. 4.5 makes typical mixes look
	// roughly flat; pink noise then rises by ~1.5 dB/oct (3.0 makes it flat).
	void setTilt (float dbPerOctave) { tilt = dbPerOctave; }
	void setReleaseDbPerSecond (float v) { releaseDbPerSec = v; }

	// Recomputes the FFT and updates `outDb` (resized to freqs.size()).
	void compute (const std::vector<float>& freqs, std::vector<float>& outDb, float dtSeconds);

	bool hasNewData () const { return samplesSinceCompute > 0; }

private:
	void resizeFor (double sr);

	double sampleRate {48000.0};
	int order {12};
	FFT fft;
	std::vector<float> ring;
	int writePos {0};
	int samplesSinceCompute {0};
	std::vector<float> window;
	float windowGain {1.0f};
	std::vector<std::complex<float>> buffer;
	std::vector<float> power;
	float tilt {4.5f};
	float releaseDbPerSec {36.0f};
};

} // namespace suite::dsp
