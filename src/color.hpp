#pragma once

#include "stdafx.hpp"

#include "spectrum.hpp"


namespace Color {


struct Data final {
	Spectrum D65;

	struct StandardObserver final {
		Spectrum spec;
		float integral;
	};
	struct StandardObserver std_obs_xbar;
	struct StandardObserver std_obs_ybar;
	struct StandardObserver std_obs_zbar;

	struct {
		Spectrum r;
		Spectrum g;
		Spectrum b;
	} basis_bt709;

	glm::mat3x3 matr_RGBtoXYZ;
	glm::mat3x3 matr_XYZtoRGB;
};
extern struct Data* data;


void   init();
void deinit();


inline CIEXYZ spectrum_to_ciexyz(Spectrum const& spectrum) {
	float X = Spectrum::integrate(spectrum,data->std_obs_xbar.spec);
	float Y = Spectrum::integrate(spectrum,data->std_obs_ybar.spec);
	float Z = Spectrum::integrate(spectrum,data->std_obs_zbar.spec);
	return CIEXYZ(X,Y,Z);
}
inline CIEXYZ spectralsample_to_ciexyz(nm lambda_0, Spectrum::Sample const& radiant_flux) {
	Spectrum::Sample xbars = data->std_obs_xbar.spec[lambda_0];
	Spectrum::Sample ybars = data->std_obs_ybar.spec[lambda_0];
	Spectrum::Sample zbars = data->std_obs_zbar.spec[lambda_0];
	//Dirac-delta function integration
	float X = glm::dot( xbars, radiant_flux ) * data->std_obs_xbar.integral;
	float Y = glm::dot( ybars, radiant_flux ) * data->std_obs_ybar.integral;
	float Z = glm::dot( zbars, radiant_flux ) * data->std_obs_zbar.integral;
	return CIEXYZ(X,Y,Z);
}

//Note that the conversion from lRGB to sRGB and vice-versa are not simple power laws!  The
//	standard-compliant versions are not even much-more complicated.
inline sRGB lrgb_to_srgb(lRGB const& lrgb) {
	return sRGB(
		lrgb.r<0.0031308f ? 12.92f*lrgb.r : 1.055f*std::pow(lrgb.r,1.0f/2.4f)-0.055f,
		lrgb.g<0.0031308f ? 12.92f*lrgb.g : 1.055f*std::pow(lrgb.g,1.0f/2.4f)-0.055f,
		lrgb.b<0.0031308f ? 12.92f*lrgb.b : 1.055f*std::pow(lrgb.b,1.0f/2.4f)-0.055f
	);
}
inline lRGB srgb_to_lrgb(sRGB const& srgb) {
	return lRGB(
		srgb.r<0.04045f ? srgb.r/12.92f : std::pow((srgb.r+0.055f)/1.055f,2.4f),
		srgb.g<0.04045f ? srgb.g/12.92f : std::pow((srgb.g+0.055f)/1.055f,2.4f),
		srgb.b<0.04045f ? srgb.b/12.92f : std::pow((srgb.b+0.055f)/1.055f,2.4f)
	);
}

inline Spectrum::Sample lrgb_to_spectralsample(lRGB const& lrgb, nm lambda_0) {
	Spectrum::Sample result =
		lrgb.r * data->basis_bt709.r[lambda_0] +
		lrgb.g * data->basis_bt709.g[lambda_0] +
		lrgb.b * data->basis_bt709.b[lambda_0]
	;
	return result;
}

inline sRGB xyz_to_srgb(CIEXYZ const& xyz) {
	lRGB lrgb = data->matr_XYZtoRGB * xyz;
	return lrgb_to_srgb(lrgb);
}


}
