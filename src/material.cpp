#include "material.hpp"

#include "util/lodepng/lodepng.h"
#include "util/color.hpp"

#include "util/math-helpers.hpp"



sRGB_ReflectanceTexture::sRGB_ReflectanceTexture(std::string const& path) {
	//Load data from file
	std::vector<unsigned char> out;
	unsigned w, h;
	lodepng::decode( out, w,h, path, LCT_RGB );
	if (!out.empty()); else {
		fprintf(stderr,"Could not load texture \"%s\"\n",path.c_str());
		throw -1;
	}

	//Set resolution
	res[0] = w;
	res[1] = h;

	//Allocate pixels
	_data = new sRGB_U8[res[1]*res[0]];

	//Copy loaded data into pixels
	memcpy(_data,out.data(),3*res[1]*res[0]);
}
sRGB_ReflectanceTexture::sRGB_ReflectanceTexture(sRGB_ReflectanceTexture const& other) :
	res{other.res[0],other.res[1]}
{
	//Allocate pixels
	_data = new sRGB_U8[res[1]*res[0]];

	//Copy `other`'s data into pixels
	memcpy(_data,other._data,3*res[1]*res[0]);
}
sRGB_ReflectanceTexture::~sRGB_ReflectanceTexture() {
	//Clean up pixel data
	delete[] _data;
}

#ifdef RENDER_MODE_SPECTRAL
SpectralReflectance::HeroSample sRGB_ReflectanceTexture::sample( size_t i,size_t j, nm lambda_0 ) const
#else
RGB_Reflectance                 sRGB_ReflectanceTexture::sample( size_t i,size_t j              ) const
#endif
{
	//Load sRGB data and convert to floating-point.
	sRGB_U8 const& srgb_u8 = _data[j*res[0]+i];
	sRGB_F32 srgb = sRGB_F32(srgb_u8.r,srgb_u8.g,srgb_u8.b)*(1.0f/255.0f);

	//Undo the gamma transform to get ℓRGB.
	RGB_Reflectance lrgb = Color::srgb_to_lrgb(srgb);

	#ifdef RENDER_MODE_SPECTRAL
	//Sample the reflection spectrum corresponding to this ℓRGB triple.  See paper for details on
	//	what "corresponding" means.
	return Color::lrgb_to_specrefl(lrgb,lambda_0);
	#else
	return                         lrgb;
	#endif
}
#ifdef RENDER_MODE_SPECTRAL
SpectralReflectance::HeroSample sRGB_ReflectanceTexture::sample( ST const& st,      nm lambda_0 ) const
#else
RGB_Reflectance                 sRGB_ReflectanceTexture::sample( ST const& st                   ) const
#endif
{
	#if 1
		//Convert from ST space to UV space.
		UV uv = st * glm::vec2(res[0],res[1]);

		//Convert from UV space to index space
		glm::vec2 index = glm::vec2(
			         uv.x,
			res[1] - uv.y
		);

		//Convert to integer (clamped nearest-neighbor sample)
		int i = static_cast<int>(std::floor(index.x));
		int j = static_cast<int>(std::floor(index.y));
		i = glm::clamp( i, 0,static_cast<int>(res[0]-1) );
		j = glm::clamp( j, 0,static_cast<int>(res[1]-1) );

		//Sample the texture
		#ifdef RENDER_MODE_SPECTRAL
		return sample( static_cast<size_t>(i),static_cast<size_t>(j), lambda_0 );
		#else
		return sample( static_cast<size_t>(i),static_cast<size_t>(j)           );
		#endif
	#else
		//Return the ST coordinates as a spectral reflectance.
		return Color::lrgb_to_specrefl(lRGB_F32(st,0),lambda_0);
	#endif
}


bool MaterialBase::is_emissive() const {
	#ifdef RENDER_MODE_SPECTRAL
		return SpectralRadiance::integrate(emission) > 0.0f;
	#else
		return emission.r>0.0f || emission.g>0.0f || emission.b>0.0f;
	#endif
}


MaterialSimpleAlbedoBase::~MaterialSimpleAlbedoBase() {
	#ifdef RENDER_MODE_SPECTRAL
		if (mode==MODE::CONSTANT) delete albedo.constant;
		else                      delete albedo.texture;
	#else
		delete albedo.texture;
	#endif
}


void MaterialLambertian::evaluate_bsdf(struct BSDF_Evaluation*  evaluation ) const /*override*/ {
	#ifdef RENDER_MODE_SPECTRAL
		if (mode==MODE::CONSTANT) evaluation->f_s=(*albedo.constant      )[               evaluation->lambda_0];
		else                      evaluation->f_s=  albedo.texture->sample(evaluation->st,evaluation->lambda_0);
	#else
		if (mode==MODE::CONSTANT) evaluation->f_s=albedo.constant;
		else                      evaluation->f_s=albedo.texture->sample(evaluation->st);
	#endif
	evaluation->f_s /= Constants::pi<float>;
}
void MaterialLambertian::interact_bsdf(struct BSDF_Interaction* interaction) const /*override*/ {
	//Importance-sample the geometry term
	interaction->w_i = Math::rand_coshemi(interaction->rng,&interaction->pdf_w_i);
	interaction->w_i = Math::get_rotated_to(interaction->w_i,interaction->N);

	#ifdef RENDER_MODE_SPECTRAL
		if (mode==MODE::CONSTANT) interaction->f_s=(*albedo.constant      )[                interaction->lambda_0];
		else                      interaction->f_s=  albedo.texture->sample(interaction->st,interaction->lambda_0);
	#else
		if (mode==MODE::CONSTANT) interaction->f_s=albedo.constant;
		else                      interaction->f_s=albedo.texture->sample(interaction->st);
	#endif
	interaction->f_s /= Constants::pi<float>;
}


void MaterialMirror::evaluate_bsdf(struct BSDF_Evaluation*  evaluation ) const /*override*/ {
	//Impossible to hit a Dirac δ function.
	#ifdef RENDER_MODE_SPECTRAL
		evaluation->f_s = SpectralRadiance::HeroSample(0.0f);
	#else
		evaluation->f_s = RGB_RecipSR                 (0.0f);
	#endif
}
void MaterialMirror::interact_bsdf(struct BSDF_Interaction* interaction) const /*override*/ {
	//Importance-sample the Dirac δ function
	interaction->w_i = Math::reflect(interaction->w_o,interaction->N);
	interaction->pdf_w_i = INF;

	//Note: value represents a Dirac δ function.
	#ifdef RENDER_MODE_SPECTRAL
		if (mode==MODE::CONSTANT) interaction->f_s=(*albedo.constant      )[                interaction->lambda_0];
		else                      interaction->f_s=  albedo.texture->sample(interaction->st,interaction->lambda_0);
	#else
		if (mode==MODE::CONSTANT) interaction->f_s=albedo.constant;
		else                      interaction->f_s=albedo.texture->sample(interaction->st);
	#endif
}
