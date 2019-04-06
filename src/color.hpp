#pragma once

#include "stdafx.hpp"

#include "spectrum.hpp"


namespace Color {


struct _Data final {
	Spectrum D65;

	Spectrum std_obs_xbar;
	Spectrum std_obs_ybar;
	Spectrum std_obs_zbar;

	glm::vec3 XYZ_D65;

	glm::mat3x3 matr_XYZtoRGB;
} _data;


void init();


inline CIEXYZ spectralsample_to_ciexyz(nm lambda_0, Spectrum::Sample radiant_flux) {
	Spectrum::Sample xbars = _std_obs_xbar[lambda_0];
	Spectrum::Sample ybars = _std_obs_ybar[lambda_0];
	Spectrum::Sample zbars = _std_obs_zbar[lambda_0];
	float X = glm::dot( xbars, radiant_flux );
	float Y = glm::dot( ybars, radiant_flux );
	float Z = glm::dot( zbars, radiant_flux );
	return CIEXYZ(X,Y,Z);
}

inline sRGB xyz_to_srgb(CIEXYZ const& xyz) {
	lRGB lrgb = _matr_XYZtoRGB * xyz;

	sRGB srgb = sRGB(
		lrgb.r<0.0031308f ? 12.92f*lrgb.r : 1.055f*std::pow(lrgb.r,1.0f/2.4f)-0.055f,
		lrgb.g<0.0031308f ? 12.92f*lrgb.g : 1.055f*std::pow(lrgb.g,1.0f/2.4f)-0.055f,
		lrgb.b<0.0031308f ? 12.92f*lrgb.b : 1.055f*std::pow(lrgb.b,1.0f/2.4f)-0.055f
	);

	return srgb;
}


}
