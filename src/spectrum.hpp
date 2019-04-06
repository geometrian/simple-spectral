#pragma once

#include "stdafx.hpp"


class Spectrum final {
	public:
		typedef glm::vec<SAMPLE_WAVELENGTHS,float> Sample;

	private:
		std::vector<float> _data;
		nm _low, _high;

		float _sc;

	public:
		Spectrum() = default;
		Spectrum(std::vector<float> const& data, nm low,nm high) :
			_data(data), _low(low),_high(high)
		{
			if (data.size()>=2); else {
				fprintf(stderr,"Must have at-least two elements in sampled spectrum!\n");
				throw -1;
			}

			_sc = static_cast<float>(data.size()-1) / (_high-_low);
		}
		~Spectrum() = default;

	private:
		float _sample_linear(nm lambda) const {
			//This could definitely be optimized.

			float i = (lambda-_low)*_sc;
			float i0f = std::floor(i);
			float frac = i - i0f;
			int i0 = static_cast<int>(i0f);
			int i1 = i0 + 1;

			float val0;
			if (i0>=0&&i0<_data.size()) {
				val0 = _data[static_cast<size_t>(i0)];
			} else {
				val0 = 0.0f;
			}
			float val1;
			if (i1>=0&&i1<_data.size()) {
				val1 = _data[static_cast<size_t>(i1)];
			} else {
				val1 = 0.0f;
			}

			return lerp( val0,val1, frac );
		}
	public:
		Sample operator[](nm lambda_0) const {
			Sample result;
			for (size_t i=0;i<SAMPLE_WAVELENGTHS;++i) {
				result[i] = _sample_linear(lambda_0+i*LAMBDA_STEP);
			}
			return result;
		}
};


std::vector<std::vector<float>> load_spectral_data(std::string const& csv_path);
