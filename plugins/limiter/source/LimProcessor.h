#pragma once

#include "LimParams.h"

#include "suite/dsp/Smoother.h"
#include "suite/dsp/TruePeak.h"
#include "suite/vst/DynamicsTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace lim {

// Sliding-window minimum in O(1) amortized time (monotonic queue).
class SlidingMin
{
public:
	void resize (int window)
	{
		size = window;
		values.assign (window + 1, 1.0);
		indices.assign (window + 1, 0);
		reset ();
	}
	void reset ()
	{
		head = tail = 0;
		count = 0;
		n = 0;
	}
	double push (double v)
	{
		const int cap = static_cast<int> (values.size ());
		while (count > 0 && values[(tail + cap - 1) % cap] >= v)
		{
			tail = (tail + cap - 1) % cap;
			--count;
		}
		values[tail] = v;
		indices[tail] = n;
		tail = (tail + 1) % cap;
		++count;
		while (indices[head] <= n - size)
		{
			head = (head + 1) % cap;
			--count;
		}
		++n;
		return values[head];
	}

private:
	std::vector<double> values;
	std::vector<int64_t> indices;
	int size {1}, head {0}, tail {0}, count {0};
	int64_t n {0};
};

// Running mean over a fixed window.
class BoxFilter
{
public:
	void resize (int window)
	{
		buf.assign (window, 1.0);
		sum = window;
		pos = 0;
	}
	void reset () { resize (static_cast<int> (buf.size ())); }
	double push (double v)
	{
		sum += v - buf[pos];
		buf[pos] = v;
		if (++pos == static_cast<int> (buf.size ()))
		{
			pos = 0;
			sum = 0.0; // re-sum once per cycle to stop floating-point drift
			for (double b : buf)
				sum += b;
		}
		return sum / buf.size ();
	}

private:
	std::vector<double> buf;
	double sum {0};
	int pos {0};
};

class LimProcessor : public suite::ProcessorBase
{
public:
	LimProcessor ();
	static Steinberg::FUnknown* create (void*);

	Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) override;

protected:
	void prepare (double sampleRate, int32_t maxBlockSize) override;
	void reset () override;
	void paramChanged (uint32_t id, double plain) override;
	void processAudio (const float* const* in, float* const* out, int32_t numSamples) override;
	uint32_t latencySamples () const override { return latency; }

private:
	void updateRelease ();

	double ceilingDb {-0.3};
	double releaseMs {60};
	bool autoRelease {true};
	double link {1.0};
	bool truePeak {true};

	int lookahead {144};
	uint32_t latency {150};
	double releaseCoef {0}, fastCoef {0}, slowCoef {0}, autoCoef {0};
	suite::dsp::Smoother inputGain, ceiling;

	struct Channel
	{
		suite::dsp::TruePeakDetector tp;
		std::vector<double> detDelay; // aligns sample-peak detection with true-peak
		std::vector<float> audioDelay;
		int detPos {0}, audioPos {0};
		double env {1.0}, slow {1.0};
		SlidingMin hold;
		BoxFilter box;
	} ch[2];

	suite::AudioTap tap {this, suite::kNumDynStreams};
	std::vector<float> tapIn, tapOut, tapGr;
};

} // namespace lim
