#pragma once

#include <cmath>
#include <complex>
#include <vector>

namespace suite::dsp {

// In-place iterative radix-2 complex FFT with cached twiddles.
class FFT
{
public:
	explicit FFT (int order = 12) { setOrder (order); }

	void setOrder (int order)
	{
		n = 1 << order;
		twiddles.resize (n / 2);
		for (int i = 0; i < n / 2; ++i)
			twiddles[i] = std::polar (1.0f, float (-2.0 * 3.14159265358979323846 * i / n));
		bitrev.resize (n);
		for (int i = 0; i < n; ++i)
		{
			int r = 0;
			for (int b = 0; b < order; ++b)
				r |= ((i >> b) & 1) << (order - 1 - b);
			bitrev[i] = r;
		}
	}

	int size () const { return n; }

	void forward (std::complex<float>* data) const
	{
		for (int i = 0; i < n; ++i)
			if (i < bitrev[i])
				std::swap (data[i], data[bitrev[i]]);

		for (int len = 2; len <= n; len <<= 1)
		{
			const int half = len >> 1;
			const int step = n / len;
			for (int i = 0; i < n; i += len)
			{
				for (int j = 0; j < half; ++j)
				{
					auto t = data[i + j + half] * twiddles[j * step];
					data[i + j + half] = data[i + j] - t;
					data[i + j] += t;
				}
			}
		}
	}

private:
	int n {0};
	std::vector<std::complex<float>> twiddles;
	std::vector<int> bitrev;
};

} // namespace suite::dsp
