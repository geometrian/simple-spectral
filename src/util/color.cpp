#include "color.hpp"



namespace Color {



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

	//Load D65
	{
		std::vector<std::vector<float>> tmp = load_spectral_data("data/d65-300+5+780.csv");
		if (tmp.size()==1); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

		data->D65_orig = SpectrumUnspecified( tmp[0], 300,780 );

		kelvin temp_d65 = 6500.0f;
		//In 1968, the "c₂" (second radiation constant) in Planck's law was amended from
		//	"1.438×10⁻² m·K" (which my source does not record any more precisely) to its current
		//	value.  This affected data defined before then, and we correct for it by multiplying the
		//	new value over the old.  This produces a slightly hotter temperature.
		temp_d65 *= (Constants::h<float> * Constants::c<float> / Constants::k_B<float>) / 1.438e-2f;

		assert(data->D65_orig[560_nm][0]==100.0f); //TODO
		//Factor of "100" to scale back to "1", and factor of "1000" to convert from "W" to "kW".
		data->D65_rad = data->D65_orig * ( 0.00001f * _planck(560_nm,temp_d65) );
	}

	//Load CIE standard observer functions
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
			specradflux_to_ciexyz(data->D65_rad)
		);
		data->matr_XYZtoRGB = glm::inverse(data->matr_RGBtoXYZ);
	}
}
void deinit() {
	delete data;
}



}
