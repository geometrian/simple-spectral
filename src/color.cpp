#include "color.hpp"


namespace Color {


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

	glm::vec3 S_rgb = glm::transpose(glm::mat3x3(X_rgb,Y_rgb,Z_rgb)) * XYZ_W;

	glm::mat3x3 M = glm::transpose(glm::mat3x3(
		S_rgb * X_rgb,
		S_rgb * Y_rgb,
		S_rgb * Z_rgb
	));

	return M;
}


struct _Data final {
	Spectrum D65;

	Spectrum std_obs_xbar;
	Spectrum std_obs_ybar;
	Spectrum std_obs_zbar;

	glm::vec3 XYZ_D65;

	glm::mat3x3 matr_XYZtoRGB;
} _data;



void init() {
	std::vector<std::vector<float>> data = load_spectral_data("../data/standard-observer.csv");
	if (data.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

	_data.D65 = 


	std::vector<std::vector<float>> data = load_spectral_data("../data/standard-observer.csv");
	if (data.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

	_std_obs_xbar = Spectrum( data[0], 380,780 );
	_std_obs_ybar = Spectrum( data[1], 380,780 );
	_std_obs_zbar = Spectrum( data[2], 380,780 );
}



Spectrum _std_obs_xbar;
Spectrum _std_obs_ybar;
Spectrum _std_obs_zbar;

glm::mat3x3 _matr_XYZtoRGB = calc_matr_RGBtoXYZ(
	glm::vec2(0.64f,0.33f), glm::vec2(0.30f,0.60f), glm::vec2(0.15f,0.06f),
	XYZ_D65
);





}
