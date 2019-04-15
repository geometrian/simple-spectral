#pragma once

#include "../stdafx.hpp"

#include "../spectrum.hpp"



namespace Color {



#ifdef RENDER_MODE_SPECTRAL

//Encapsulates color data required by the renderer.
struct _Data final {
	//CIE standard observer functions "x̄(λ)", "ȳ(λ)", and "z̄(λ)".  These are spectral data, computed
	//	as a linear transformation of the CIE RGB color matching functions, which themselves are
	//	based off of perceptual user studies.  They underpin the CIE XYZ color space which links
	//	spectral power distributions to RGB spaces (among other things).
	SpectrumUnspecified std_obs_xbar;
	SpectrumUnspecified std_obs_ybar;
	SpectrumUnspecified std_obs_zbar;

	//CIE standard illuminant D65.
	//	The original data has no radiometric meaning, since its intensity has been normalized by the
	//		CIE so that at wavelength 560nm, the value is exactly "100.0".
	SpectrumUnspecified D65_orig;
	CIEXYZ_32F          D65_orig_XYZ;
	//	However, we can scale the values so that the intensity represents spectral radiance by using
	//		Planck's law.  Since the output color space (BT.709) is normalized to D65 anyway,
	//		normalizing is not important for correctness.  However, we prefer it because it makes
	//		the values computing during path tracing have physical meaning in-terms of real-world
	//		units.
	SpectralRadiance    D65_rad;
	CIEXYZ_32F          D65_rad_XYZ;

	//Basis for spectral reflectance computed using our algorithm.  Given any BT.709 "(R,G,B)"
	//	triple that is linear (pre-gamma) and normalized (in the range "[0,1]"), i.e., ℓRGB ("linear
	//	RGB"), the spectral reflectance given by:
	//
	//		"R" `basis_bt709.r` + "G" `basis_bt709.g` + "B" `basis_bt709.b`
	//
	//	will also be a valid reflection spectrum.  Furthermore, when the reflectance represents the
	//	albedo of a Lambertian surface illuminated with no interreflection in a white (D65) furnace
	//	test, the pre-gamma normalized pixel value corresponding to the point on the surface will be
	//	"(R,G,B)" back again.  See paper for more details.
	struct {
		SpectralReflectance r;
		SpectralReflectance g;
		SpectralReflectance b;
	} basis_bt709;

	//Conversion matrix from BT.709 RGB to CIE XYZ
	glm::mat3x3 matr_RGBtoXYZ;
	//Conversion matrix from CIE XYZ to BT.709 ℓRGB
	glm::mat3x3 matr_XYZtoRGB;
};
extern struct _Data* data;

//Initialization of global color data
void   init();
//Cleanup of global color data
void deinit();

#endif



//Conversion from/to linear (pre-gamma), normalized BT.709 RGB (i.e., ℓRGB) to/from post-gamma,
//	normalized BT.709 RGB (i.e., floating-point sRGB).  Note that the conversion from ℓRGB to sRGB
//	and vice-versa are not simple power laws!  The standard-compliant versions are not even much-
//	more complicated.
inline sRGB_F32 lrgb_to_srgb(lRGB_F32 const& lrgb) {
	return sRGB_F32(
		lrgb.r<0.0031308f ? 12.92f*lrgb.r : 1.055f*std::pow(lrgb.r,1.0f/2.4f)-0.055f,
		lrgb.g<0.0031308f ? 12.92f*lrgb.g : 1.055f*std::pow(lrgb.g,1.0f/2.4f)-0.055f,
		lrgb.b<0.0031308f ? 12.92f*lrgb.b : 1.055f*std::pow(lrgb.b,1.0f/2.4f)-0.055f
	);
}
inline lRGB_F32 srgb_to_lrgb(sRGB_F32 const& srgb) {
	return lRGB_F32(
		srgb.r<0.04045f ? srgb.r/12.92f : std::pow((srgb.r+0.055f)/1.055f,2.4f),
		srgb.g<0.04045f ? srgb.g/12.92f : std::pow((srgb.g+0.055f)/1.055f,2.4f),
		srgb.b<0.04045f ? srgb.b/12.92f : std::pow((srgb.b+0.055f)/1.055f,2.4f)
	);
}



#ifdef RENDER_MODE_SPECTRAL

//Calculate the CIE XYZ tristimulus value for the given spectral radiant flux `spec_rad_flux`.  Note
//	radiant flux (i.e. radiant power) is what the eye is sensitive to, not e.g. radiance.  A camera
//	is sensitive to radiant energy (radiant flux integrated over a shutter interval).
inline CIEXYZ_32F specradflux_to_ciexyz(SpectralRadiantFlux             const& spec_rad_flux             ) {
	float X = SpectrumUnspecified::integrate( spec_rad_flux, data->std_obs_xbar );
	float Y = SpectrumUnspecified::integrate( spec_rad_flux, data->std_obs_ybar );
	float Z = SpectrumUnspecified::integrate( spec_rad_flux, data->std_obs_zbar );
	return CIEXYZ_32F(X,Y,Z);
}
//Calculate the estimated CIE XYZ tristimulus value for the given hero-wavelength sample of spectral
//	radiant flux `spec_rad_flux` with hero wavelength `lambda_0`.  As-above, note that the eye is
//	sensitive to radiant flux.
inline CIEXYZ_32F specradflux_to_ciexyz(SpectralRadiantFlux::HeroSample const& spec_rad_flux, nm lambda_0) {
	//TODO: optimize with hardware-supported dot products

	//The hero samples times the corresponding CIE standard observer function values give a sample
	//	from the product of the notional spectrum the hero sample was taken from and the standard
	//	observer functions.
	SpectrumUnspecified::HeroSample value_sample_xbar_times_flux = data->std_obs_xbar[lambda_0] * spec_rad_flux;
	SpectrumUnspecified::HeroSample value_sample_ybar_times_flux = data->std_obs_ybar[lambda_0] * spec_rad_flux;
	SpectrumUnspecified::HeroSample value_sample_zbar_times_flux = data->std_obs_zbar[lambda_0] * spec_rad_flux;

	//Monte Carlo estimate of the integral of that product over each wavelength band.
	SpectrumUnspecified::HeroSample value_montecarlo_est_subintegrals_X = value_sample_xbar_times_flux * LAMBDA_STEP;
	SpectrumUnspecified::HeroSample value_montecarlo_est_subintegrals_Y = value_sample_ybar_times_flux * LAMBDA_STEP;
	SpectrumUnspecified::HeroSample value_montecarlo_est_subintegrals_Z = value_sample_zbar_times_flux * LAMBDA_STEP;

	//Summing them gives the Monte Carlo estimate of the integral of that product over the whole
	//	spectrum.  That is, this is the Monte Carlo estimate of the product of the notional spectrum
	//	the hero sample was taken from and the corresponding CIE standard observer function.
	float X=0.0f; for (size_t i=0;i<SAMPLE_WAVELENGTHS;++i) X+=value_montecarlo_est_subintegrals_X[i];
	float Y=0.0f; for (size_t i=0;i<SAMPLE_WAVELENGTHS;++i) Y+=value_montecarlo_est_subintegrals_Y[i];
	float Z=0.0f; for (size_t i=0;i<SAMPLE_WAVELENGTHS;++i) Z+=value_montecarlo_est_subintegrals_Z[i];

	//Done
	return CIEXYZ_32F(X,Y,Z);
}

//Conversion from a linear (pre-gamma), normalized BT.709 RGB (i.e., ℓRGB) triple representing a
//	reflectance to a hero sample from a reflectance spectrum corresponding to it (corresponding, in
//	the sense that D65 (the white point of BT.709) hemispherically integrated over a Lambertian
//	surface of that reflectance will appear as that RGB triple on the screen; see paper for
//	details).  The conversion is just a linear combination of three basis spectra with the triple's
//	values as weights.
SpectralReflectance::HeroSample lrgb_to_specrefl(lRGB_F32 const& lrgb, nm lambda_0);

//Direct conversion from CIE XYZ to post-gamma, normalized BT.709 RGB (i.e. sRGB).
sRGB_F32 ciexyz_to_srgb(CIEXYZ_32F const& xyz);

#endif



}
