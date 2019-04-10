#pragma once

#include "stdafx.hpp"



#ifdef RENDER_MODE_SPECTRAL



//Encapsulates a spectrum defined by sequence of "n" values over a wavelength range "[λₘᵢₙ,λₘₐₓ]".
class _Spectrum final {
	public:
		//Sample value at some wavelength "λ₀" (specified by context), with "n-1" additional sample
		//	values "λ₁" through "λₙ₋₁" at wavelengths "λᵢ = λ₀ + i (λₘₐₓ-λₘᵢₙ)/n".  I.e., hero
		//	wavelength sampling.
		typedef glm::vec<SAMPLE_WAVELENGTHS,float> HeroSample;

	private:
		//Sequence of values defining the spectrum.  The first value is at wavelength "λₘᵢₙ" (i.e.
		//	`._low`).  The last value is at wavelength `._high` (i.e. "λₘₐₓ").  The rest are evenly
		//	spaced in-between.
		std::vector<float> _data;
		nm _low, _high;

		//Values used internally.  Equal to "(λₘₐₓ-λₘᵢₙ)/n" and "n/(λₘₐₓ-λₘᵢₙ)".
		float _delta_lambda;
		float _delta_lambda_recip;

		//Method used to reconstruct the spectrum when sampled.
		typedef float(_Spectrum::*SpectrumSampler)(nm)const;
		SpectrumSampler _sampler = &_Spectrum::_sample_linear;

	public:
		//Empty spectrum (invalid)
		_Spectrum() = default;
		//Spectrum is constant value of `data` over range [`LAMBDA_MIN`,`LAMBDA_MAX`].
		explicit _Spectrum(float data);
		//Spectrum takes values given by `data` sampled over the wavelength range [`low`,`high`].
		_Spectrum(std::vector<float> const& data, nm low,nm high);
		~_Spectrum() = default;

		//Set the reconstruction method.
		void set_filter_nearest() { _sampler=&_Spectrum::_sample_nearest; }
		void set_filter_linear () { _sampler=&_Spectrum::_sample_linear;  }

		//Sample the spectrum given the hero wavelength "λ₀" given by `lambda_0`.
	private:
		float _sample_nearest(nm lambda) const;
		float _sample_linear (nm lambda) const;
	public:
		HeroSample operator[](nm lambda_0) const;

		//Multiplication of this spectrum by a constant scalar `sc`, returning a new spectrum.
		_Spectrum operator*(float sc) const;

		//Compute the integral of the spectrum with respect to wavelength.
		static float integrate(_Spectrum const& spec                         );
		static float integrate(_Spectrum const& spec0, _Spectrum const& spec1);
		//static _HeroSample integrate_sub(_Spectrum const& spec);
};

//Spectral radiance (note units: kW·sr⁻¹·m⁻²·nm⁻¹)
typedef _Spectrum SpectralRadiance;
//Spectral radiant flux (note units: kW·nm⁻¹)
typedef _Spectrum SpectralRadiantFlux;
//Spectral reciprocal-steradians (sr⁻¹)
typedef _Spectrum SpectralRecipSR;
//Spectral reflectance (dimensionless, between "0" and "1")
typedef _Spectrum SpectralReflectance;
//Unspecified meaning (probably bogus dimensions)
typedef _Spectrum SpectrumUnspecified;



//Loads spectral data from a CSV file.  The data is in rows, and is therefore returned as a list of
//	column vectors.
std::vector<std::vector<float>> load_spectral_data(std::string const& csv_path);



#endif
