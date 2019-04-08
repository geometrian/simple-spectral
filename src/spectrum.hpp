#pragma once

#include "stdafx.hpp"


class Spectrum final {
	public:
		typedef glm::vec<SAMPLE_WAVELENGTHS,float> Sample;

	private:
		std::vector<float> _data;
		nm _low, _high;

		float _sc;

		typedef float(Spectrum::*SpectrumSampler)(nm)const;
		SpectrumSampler _sampler = &Spectrum::_sample_linear;

	public:
		Spectrum() = default;
		explicit Spectrum(float data);
		Spectrum(std::vector<float> const& data, nm low,nm high);
		~Spectrum() = default;

		void set_filter_nearest() { _sampler=&Spectrum::_sample_nearest; }
		void set_filter_linear () { _sampler=&Spectrum::_sample_linear;  }

	private:
		float _sample_nearest(nm lambda) const;
		float _sample_linear (nm lambda) const;
	public:
		Sample operator[](nm lambda_0) const;

		Spectrum operator*(float sc) const {
			Spectrum result = *this;
			for (float& f : result._data) f*=sc;
			return result;
		}

		static float integrate(Spectrum const& spec);
		static float integrate(Spectrum const& spec0, Spectrum const& spec1);
};


std::vector<std::vector<float>> load_spectral_data(std::string const& csv_path);
