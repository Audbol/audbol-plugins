#pragma once

#include <algorithm>
#include <vector>

namespace suite::dsp {

// Circular delay buffer, power-of-two sized. at(d) / read(d) return the
// sample written d pushes ago, so call them before push() for the current
// sample (or after it, counting the new sample as d = 1).
class DelayLine
{
public:
	void init (int minLength)
	{
		int size = 1;
		while (size < minLength + 4)
			size <<= 1;
		buf.assign (size, 0.0f);
		mask = size - 1;
		pos = 0;
	}
	void clear () { std::fill (buf.begin (), buf.end (), 0.0f); }
	int capacity () const { return static_cast<int> (buf.size ()) - 4; }
	void push (float x)
	{
		buf[pos] = x;
		pos = (pos + 1) & mask;
	}
	float at (int d) const { return buf[(pos - d) & mask]; }
	// Fractional read with 4-point Hermite interpolation; d >= 2.
	double read (double d) const
	{
		const int i = static_cast<int> (d);
		const double f = d - i;
		const double xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
		const double c1 = 0.5 * (x1 - xm1);
		const double c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
		const double c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);
		return ((c3 * f + c2) * f + c1) * f + x0;
	}

private:
	std::vector<float> buf;
	int mask {0};
	int pos {0};
};

} // namespace suite::dsp
