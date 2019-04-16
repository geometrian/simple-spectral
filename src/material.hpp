#pragma once

#include "stdafx.hpp"

#include "util/random.hpp"

#include "spectrum.hpp"



//Texture defining reflectance data
//	The data is stored in sRGB texels, but using our algorithm (see paper for details) can be
//	sampled with hero wavelength sampling, returning spectral reflectance on-the-fly.
class sRGB_ReflectanceTexture final {
	public:
		//Resolution
		size_t res[2];

	private:
		//Internal data storage stored in scanlines from top to bottom
		sRGB_U8* _data;

	public:
		explicit sRGB_ReflectanceTexture(std::string const& path);
		sRGB_ReflectanceTexture(sRGB_ReflectanceTexture const& other);
		~sRGB_ReflectanceTexture();

	#ifdef RENDER_MODE_SPECTRAL
		//Return hero wavelength sample of the texture at the coordinates given by pixel index
		//	(`i`,`j`) for the hero wavelength `lambda_0`.  Note that scanlines are stored top-to-
		//	bottom.
		SpectralReflectance::HeroSample sample( size_t i,size_t j, nm lambda_0 ) const;
		//Return hero wavelength sample of the texture at the coordinates given by ST coordinate
		//	`st` for the hero wavelength `lambda_0`.
		SpectralReflectance::HeroSample sample( ST const& st,      nm lambda_0 ) const;
	#else
		//Return RGB sample of the texture at the coordinates given by pixel index (`i`,`j`).  Note
		//	that scanlines are stored top-to-bottom.
		RGB_Reflectance                 sample( size_t i,size_t j              ) const;
		//Return RGB sample of the texture at the coordinates given by ST coordinate `st`.
		RGB_Reflectance                 sample( ST const& st                   ) const;
	#endif
};



//Material base class
class MaterialBase {
	public:
		//Emission (default zeros).
		#ifdef RENDER_MODE_SPECTRAL
			SpectralRadiance emission;
		#else
			RGB_Radiance     emission;
		#endif

		//Encapsulates the state of a BSDF evaluation (that is, given the input, output, and normal
		//	vectors, return the BSDF's value).
		struct BSDF_Evaluation  final {
			ST const st;
			#ifdef RENDER_MODE_SPECTRAL
			nm const lambda_0;
			#endif

			Dir const w_o;
			Dir const N;
			Dir const w_i;

			#ifdef RENDER_MODE_SPECTRAL
				SpectralRadiance::HeroSample f_s;
			#else
				RGB_RecipSR                  f_s;
			#endif
		};
		//Encapsulates the state of a BSDF interaction (that is, given the output and normal
		//	vectors, return a randomly sampled input vector, the PDF of choosing it, and the BSDF's
		//	value).
		struct BSDF_Interaction final {
			ST const st;
			#ifdef RENDER_MODE_SPECTRAL
			nm const lambda_0;
			#endif

			Dir const w_o;
			Dir const N;
			Dir w_i; float pdf_w_i; Math::RNG& rng;

			#ifdef RENDER_MODE_SPECTRAL
				SpectralRadiance::HeroSample f_s;
			#else
				RGB_RecipSR                  f_s;
			#endif
		};

	protected:
		MaterialBase() : emission(0.0f) {}
	public:
		virtual ~MaterialBase() = default;

	#ifdef RENDER_MODE_SPECTRAL
		SpectralRadiance::HeroSample evaluate_emission(ST const& st, nm lambda_0, Dir const& w_0) const {
			return emission[lambda_0];
		}
	#else
		RGB_Radiance                 evaluate_emission(ST const& st,              Dir const& w_0) const {
			return emission;
		}
	#endif
		virtual void evaluate_bsdf(struct BSDF_Evaluation*  evaluation ) const = 0;
		virtual void interact_bsdf(struct BSDF_Interaction* interaction) const = 0;

		bool is_emissive() const;
};

//Material with a simple albedo that can be either a constant (spectrum or RGB, determined by the
//	rendering mode) or keyed by an sRGB texture.
class MaterialSimpleAlbedoBase : public MaterialBase {
	public:
		enum MODE { CONSTANT, TEXTURE } const mode;
		union Albedo final {
			#ifdef RENDER_MODE_SPECTRAL
			SpectralReflectance*const     constant;
			#else
			RGB_Reflectance               constant;
			#endif
			sRGB_ReflectanceTexture*const texture;

		#ifdef RENDER_MODE_SPECTRAL
			Albedo(                                ) : constant(new SpectralReflectance    (1.0f  )) {}
			Albedo(SpectralReflectance const* other) : constant(new SpectralReflectance    (*other)) {}
		#else
			Albedo(                                ) : rgb     (    RGB_Reflectance        (1.0f  )) {}
			Albedo(RGB_Reflectance const& other    ) : rgb     (    RGB_Reflectance        ( other)) {}
		#endif
			Albedo(std::string             const& path ) : texture(new sRGB_ReflectanceTexture(path  )) {}
			Albedo(sRGB_ReflectanceTexture const* other) : texture(new sRGB_ReflectanceTexture(*other)) {}
		} albedo;

	protected:
		//Constant albedo
		         MaterialSimpleAlbedoBase(                                     ) : mode(MODE::CONSTANT), albedo(    ) {}
		//Albedo keyed by sRGB texture
		explicit MaterialSimpleAlbedoBase(std::string              const& path ) : mode(MODE::TEXTURE ), albedo(path) {}
		//Copy from another material
		         MaterialSimpleAlbedoBase(MaterialSimpleAlbedoBase const& other) :
			mode(other.mode), albedo(mode==MODE::CONSTANT?Albedo(other.albedo.constant):Albedo(other.albedo.texture))
		{}
	public:
		virtual ~MaterialSimpleAlbedoBase();
};

//Lambertian material
class MaterialLambertian final : public MaterialSimpleAlbedoBase {
	public:
		//Lambertian material with emission (default zeros) and reflectance (default ones).
		MaterialLambertian(                       ) : MaterialSimpleAlbedoBase(    ) {}
		//Lambertian material with emission (default zeros) and spectral reflectance given by image
		//	loaded from sRGB texture specified by `path`.
		MaterialLambertian(std::string const& path) : MaterialSimpleAlbedoBase(path) {}
		virtual ~MaterialLambertian() = default;

		virtual void evaluate_bsdf(struct BSDF_Evaluation*  evaluation ) const override;
		virtual void interact_bsdf(struct BSDF_Interaction* interaction) const override;
};

//Mirror material
class MaterialMirror final : public MaterialSimpleAlbedoBase {
	public:
		//Lambertian material with emission (default zeros) and reflectance (default ones).
		MaterialMirror(                       ) : MaterialSimpleAlbedoBase(    ) {}
		//Lambertian material with emission (default zeros) and spectral reflectance given by image
		//	loaded from sRGB texture specified by `path`.
		MaterialMirror(std::string const& path) : MaterialSimpleAlbedoBase(path) {}
		virtual ~MaterialMirror() = default;

		virtual void evaluate_bsdf(struct BSDF_Evaluation*  evaluation ) const override;
		virtual void interact_bsdf(struct BSDF_Interaction* interaction) const override;
};
