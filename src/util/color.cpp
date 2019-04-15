#include "color.hpp"

#ifdef RENDER_MODE_SPECTRAL_MENG
	#include "../meng-et-al.-2015/spectrum_grid.h"
#endif



namespace Color {



#ifdef RENDER_MODE_SPECTRAL



//Generates the conversion matrix for a given RGB space.  Although you can look up the matrix for
//	many RGB spaces (including BT.709), it is better to compute it from first principles, so as to
//	avoid roundoff error and take into account any updated data.  The algorithm is simple, anyway.
//	This code was directly inspired from:
//		http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
static glm::mat3x3 _calc_matr_RGBtoXYZ(
	glm::vec2 const& xy_r, glm::vec2 const& xy_g, glm::vec2 const& xy_b,
	glm::vec3 const& XYZ_W
) {
	glm::vec3 x_rgb( xy_r.x, xy_g.x, xy_b.x );
	glm::vec3 y_rgb( xy_r.y, xy_g.y, xy_b.y );

	glm::vec3 X_rgb = x_rgb / y_rgb;
	glm::vec3 Y_rgb(1);
	glm::vec3 Z_rgb = ( glm::vec3(1) - x_rgb - y_rgb ) / y_rgb;

	glm::vec3 S_rgb = glm::inverse(glm::transpose(glm::mat3x3(X_rgb,Y_rgb,Z_rgb))) * XYZ_W;

	glm::mat3x3 M = glm::transpose(glm::mat3x3(
		S_rgb * X_rgb,
		S_rgb * Y_rgb,
		S_rgb * Z_rgb
	));

	return M;
}

//Planck's law (spectral radiance in W·sr⁻¹·m⁻²·nm⁻¹).  See definition:
//	https://en.wikipedia.org/wiki/Planck%27s_law#The_law
static float _planck(nm lambda_nm, kelvin temp) {
	Dist lambda_m = lambda_nm * 1.0e-9f;

	//First radiation constant "2 h c²"
	float c_1L = 2.0f * Constants::h<float> * Constants::c<float>*Constants::c<float>;
	//Second radiation constant "h c / k_B"
	float c_2  =  Constants::h<float> * Constants::c<float> / Constants::k_B<float>;

	float numer = c_1L;
	float denom = std::pow(lambda_m,5.0f) * (
		std::exp( c_2 / (lambda_m*temp) ) -
		1.0f
	);
	float value = numer / denom;

	return value * 1.0e-9f;
}



struct _Data* data;

void   init() {
	data = new struct _Data;

	//Load CIE standard observer functions
	//	Note that this must come before any computations of CIE XYZ.
	{
		#if   CIE_OBSERVER == 1931
			std::vector<std::vector<float>> tmp = load_spectral_data("data/cie1931-xyzbar-380+5+780.csv");
			if (tmp.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			data->std_obs_xbar = SpectrumUnspecified( tmp[0], 380,780 );
			data->std_obs_ybar = SpectrumUnspecified( tmp[1], 380,780 );
			data->std_obs_zbar = SpectrumUnspecified( tmp[2], 380,780 );
		#elif CIE_OBSERVER == 2006
			std::vector<std::vector<float>> tmp = load_spectral_data("data/cie2006-xyzbar-390+1+830.csv");
			if (tmp.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			data->std_obs_xbar = SpectrumUnspecified( tmp[0], 390,830 );
			data->std_obs_ybar = SpectrumUnspecified( tmp[1], 390,830 );
			data->std_obs_zbar = SpectrumUnspecified( tmp[2], 390,830 );
		#else
			#error
		#endif
	}

	//Load D65
	{
		std::vector<std::vector<float>> tmp = load_spectral_data("data/d65-300+5+780.csv");
		if (tmp.size()==1); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

		data->D65_orig = SpectrumUnspecified( tmp[0], 300,780 );
		data->D65_orig_XYZ = specradflux_to_ciexyz(data->D65_orig);

		kelvin temp_d65 = 6500.0f;
		//In 1968, the "c₂" (second radiation constant) in Planck's law was amended from
		//	"1.438×10⁻² m·K" (which my source does not record any more precisely) to its current
		//	value.  This affected data defined before then, and we correct for it by multiplying the
		//	new value over the old.  This produces a slightly hotter temperature.
		temp_d65 *= (Constants::h<float> * Constants::c<float> / Constants::k_B<float>) / 1.438e-2f;

		//Convert D65 to a radiometric version (spectral radiance) instead of a spectrum normalized
		//	arbitrarily to 100 at 560nm.  There's no technical reason to do this (the numbers work
		//	out either way), but doing it means that we're tracing units with a physical meaning.  
		assert(data->D65_orig[560_nm][0]==100.0f);
		//	Factor of "100" to scale back to "1", and factor of "1000" to convert from "W" to "kW".
		data->D65_rad = data->D65_orig * ( 0.00001f * _planck(560_nm,temp_d65) );
		data->D65_rad_XYZ = specradflux_to_ciexyz(data->D65_rad);
	}

	//Load spectral basis functions.  See our paper for details.
	{
		#if   CIE_OBSERVER == 1931
			std::vector<std::vector<float>> tmp = load_spectral_data("data/cie1931-basis-bt709-380+5+780.csv");
			if (tmp.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			data->basis_bt709.r = SpectralReflectance( tmp[0], 380,780 );
			data->basis_bt709.g = SpectralReflectance( tmp[1], 380,780 );
			data->basis_bt709.b = SpectralReflectance( tmp[2], 380,780 );
		#elif CIE_OBSERVER == 2006
			std::vector<std::vector<float>> tmp = load_spectral_data("data/cie2006-basis-bt709-390+1+780.csv");
			if (tmp.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			data->basis_bt709.r = SpectralReflectance( tmp[0], 390,780 );
			data->basis_bt709.g = SpectralReflectance( tmp[1], 390,780 );
			data->basis_bt709.b = SpectralReflectance( tmp[2], 390,780 );
		#else
			#error
		#endif
	}

	//Calculate RGB to XYZ (and vice-versa) conversion matrices.
	{
		data->matr_RGBtoXYZ = _calc_matr_RGBtoXYZ(
			glm::vec2(0.64f,0.33f), glm::vec2(0.30f,0.60f), glm::vec2(0.15f,0.06f), //BT.709
			data->D65_rad_XYZ
		);
		data->matr_XYZtoRGB = glm::inverse(data->matr_RGBtoXYZ);
	}
}
void deinit() {
	delete data;
}



#if   defined RENDER_MODE_SPECTRAL_OURS
SpectralReflectance::HeroSample lrgb_to_specrefl(lRGB_F32 const& lrgb, nm lambda_0) {
	return SpectralReflectance::HeroSample(
		lrgb.r * data->basis_bt709.r[lambda_0] +
		lrgb.g * data->basis_bt709.g[lambda_0] +
		lrgb.b * data->basis_bt709.b[lambda_0]
	);
}
#elif defined RENDER_MODE_SPECTRAL_MENG
SpectralReflectance::HeroSample lrgb_to_specrefl(lRGB_F32 const& lrgb, nm lambda_0) {
	/*
	This is the matrix Meng et al. have in their code.

	It is apparently based on somewhat-dated values, and they are listed imprecisely.  This amounts
	to a slight inaccuracy, but we maintain it for consistency with their results (since their
	method, like other methods, is not round-trip preserving, the matrix used matters).

	The matrix is also missing the scaling factor by the Y of D65 (almost certainly because the
	scaling is commonly, albeit less-correctly, computed separately).

	The scaling by 100 is necessary, presumably because their code expects XYZ values scaled to D65.
	*/

	CIEXYZ_32F xyz_rel = glm::transpose(glm::mat3x3(
		 0.41231515f, 0.3576f, 0.1805f,
		 0.2126f,     0.7152f, 0.0722f,
		 0.01932727f, 0.1192f, 0.95063333f
	)) * 100.0f * lrgb;

	SpectralReflectance::HeroSample result;
	for (size_t i=0;i<SAMPLE_WAVELENGTHS;++i) {
		result[i] = spectrum_xyz_to_p( lambda_0+i*LAMBDA_STEP, &xyz_rel[0] );
	}

	return result;
}
#else
	#error
#endif

#if   defined RENDER_MODE_SPECTRAL_OURS
sRGB_F32 ciexyz_to_srgb(CIEXYZ_32F const& xyz) {
	lRGB_F32 lrgb = data->matr_XYZtoRGB * xyz;
	return lrgb_to_srgb(lrgb);
}
#elif defined RENDER_MODE_SPECTRAL_MENG
sRGB_F32 ciexyz_to_srgb(CIEXYZ_32F const& xyz) {
	//This is the inverse matrix Meng et al. use in their code.  We again preserve it, for
	//	consistency.  See also comments in `lrgb_to_specrefl(...)`.
	CIEXYZ_32F xyz_rel = xyz / data->D65_rad_XYZ.y;
	lRGB_F32 lrgb = glm::transpose(glm::mat3x3(
			3.24156456f, -1.53766524f, -0.49870224f,
		-0.96920119f,  1.87588535f,  0.04155324f,
			0.05562416f, -0.20395525f,  1.05685902f
	)) * xyz_rel;
	return lrgb_to_srgb(lrgb);
}
#else
	#error
#endif



#endif



}
