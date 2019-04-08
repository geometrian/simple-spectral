#include "color.hpp"


namespace Color {


struct Data* data;


static glm::mat3x3 calc_matr_RGBtoXYZ(
	glm::vec2 const& xy_r, glm::vec2 const& xy_g, glm::vec2 const& xy_b,
	glm::vec3 const& XYZ_W
) {
	//http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html

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


void   init() {
	data = new struct Data;

	{
		std::vector<std::vector<float>> tmp = load_spectral_data("data/d65-300+5+780.csv");
		if (tmp.size()==1); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

		data->D65 = Spectrum( tmp[0], 300,780 );
	}

	{
		#if   CIE_OBSERVER == 1931
			std::vector<std::vector<float>> tmp = load_spectral_data("data/cie1931-xyzbar-380+5+780.csv");
			if (tmp.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			data->std_obs_xbar.spec = Spectrum( tmp[0], 380,780 );
			data->std_obs_ybar.spec = Spectrum( tmp[1], 380,780 );
			data->std_obs_zbar.spec = Spectrum( tmp[2], 380,780 );
		#elif CIE_OBSERVER == 2006
			std::vector<std::vector<float>> tmp = load_spectral_data("data/cie2006-xyzbar-390+1+830.csv");
			if (tmp.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			data->std_obs_xbar.spec = Spectrum( tmp[0], 390,830 );
			data->std_obs_ybar.spec = Spectrum( tmp[1], 390,830 );
			data->std_obs_zbar.spec = Spectrum( tmp[2], 390,830 );
		#else
			#error
		#endif

		data->std_obs_xbar.integral = Spectrum::integrate(data->std_obs_xbar.spec);
		data->std_obs_ybar.integral = Spectrum::integrate(data->std_obs_ybar.spec);
		data->std_obs_zbar.integral = Spectrum::integrate(data->std_obs_zbar.spec);
	}

	{
		#if   CIE_OBSERVER == 1931
			std::vector<std::vector<float>> tmp = load_spectral_data("data/cie1931-basis-bt709-380+5+780.csv");
			if (tmp.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			data->basis_bt709.r = Spectrum( tmp[0], 380,780 );
			data->basis_bt709.g = Spectrum( tmp[1], 380,780 );
			data->basis_bt709.b = Spectrum( tmp[2], 380,780 );
		#elif CIE_OBSERVER == 2006
			std::vector<std::vector<float>> tmp = load_spectral_data("data/cie2006-basis-bt709-390+1+780.csv");
			if (tmp.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			data->basis_bt709.r = Spectrum( tmp[0], 390,780 );
			data->basis_bt709.g = Spectrum( tmp[1], 390,780 );
			data->basis_bt709.b = Spectrum( tmp[2], 390,780 );
		#else
			#error
		#endif
	}

	{
		data->matr_RGBtoXYZ = calc_matr_RGBtoXYZ(
			glm::vec2(0.64f,0.33f), glm::vec2(0.30f,0.60f), glm::vec2(0.15f,0.06f), //BT.709
			spectrum_to_ciexyz(data->D65)
		);
		data->matr_XYZtoRGB = glm::inverse(data->matr_RGBtoXYZ);
	}
}
void deinit() {
	delete data;
}


}
