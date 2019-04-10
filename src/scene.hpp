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



//Material
class MaterialBase {
	protected:
		MaterialBase() = default;
	public:
		virtual ~MaterialBase() = default;

	#ifdef RENDER_MODE_SPECTRAL
		virtual SpectralRadiance::HeroSample sample_emission(ST const& st, nm lambda_0) const = 0;
		virtual SpectralRecipSR:: HeroSample sample_brdf    (ST const& st, nm lambda_0) const = 0;
	#else
		virtual RGB_Radiance                 sample_emission(ST const& st             ) const = 0;
		virtual RGB_RecipSR                  sample_brdf    (ST const& st             ) const = 0;
	#endif

		virtual bool is_emissive() const = 0;
};

//Lambertian material with albedo defined by texture
class MaterialLambertianTexture final : public MaterialBase {
	private:
		sRGB_ReflectanceTexture _albedo_texture;

	public:
		MaterialLambertianTexture(std::string const& path) : MaterialBase(), _albedo_texture(path) {}
		virtual ~MaterialLambertianTexture() = default;

	#ifdef RENDER_MODE_SPECTRAL
		virtual SpectralRadiance::HeroSample sample_emission(ST const& /*st*/, nm /*lambda_0*/) const override {
			return SpectralRadiance::HeroSample(0);
		}
		virtual SpectralRecipSR:: HeroSample sample_brdf    (ST const&   st,   nm   lambda_0  ) const override {
			return _albedo_texture.sample(st,lambda_0) / Constants::pi<float>;
		}
	#else
		virtual RGB_Radiance                 sample_emission(ST const& st                     ) const override {
			return RGB_Reflectance             (0);
		}
		virtual RGB_RecipSR                  sample_brdf    (ST const& st                     ) const override {
			return _albedo_texture.sample(st         ) / Constants::pi<float>;
		}
	#endif

		virtual bool is_emissive() const override { return false; }
};
#ifdef RENDER_MODE_SPECTRAL
//Lambertian material with albedo (and potential Lambertian emission) defined by spectrum
class MaterialLambertianSpectral final : public MaterialBase {
	public:
		//Emission (default zeroes).
		SpectralRadiance emission;

		//Reflectance (default ones)
		SpectralReflectance reflectance;

	public:
		MaterialLambertianSpectral() : emission(0.0f), reflectance(1.0f) {}
		virtual ~MaterialLambertianSpectral() = default;

		virtual SpectralRadiance::HeroSample sample_emission(ST const& /*st*/, nm lambda_0) const override {
			return emission   [lambda_0];
		}
		virtual SpectralRecipSR:: HeroSample sample_brdf    (ST const& /*st*/, nm lambda_0) const override {
			return reflectance[lambda_0] / Constants::pi<float>;
		}

		virtual bool is_emissive() const override {
			return SpectralRadiance::integrate(emission) > 0.0f;
		}
};
#else
//Lambertian material with albedo (and potential Lambertian emission) defined by RGB
class MaterialLambertianRGB      final : public MaterialBase {
	public:
		//Emission (default zeroes).
		RGB_Radiance emission;

		//Reflectance (default ones)
		RGB_Reflectance reflectance;

	public:
		MaterialLambertianRGB() : emission(0.0f), reflectance(1.0f) {}
		virtual ~MaterialLambertianRGB() = default;

		virtual RGB_Radiance                 sample_emission(ST const& /*st*/             ) const override {
			return emission;
		}
		virtual RGB_RecipSR                  sample_brdf    (ST const& /*st*/             ) const override {
			return reflectance / Constants::pi<float>;
		}

		virtual bool is_emissive() const override {
			return emission.r>0.0f || emission.g>0.0f || emission.b>0.0f;
		}
};
#endif



//TODO: comment

class Vertex final {
	public:
		Pos pos;
		ST st;
};

class PrimBase {
	public:
		enum class TYPE {
			TRI,
			QUAD
		};
		TYPE type;

		MaterialBase* material;

		bool is_light;

	protected:
		PrimBase() = default;
		explicit PrimBase(TYPE type, MaterialBase* material) :
			type(type), material(material), is_light(material->is_emissive())
		{}
	public:
		virtual ~PrimBase() = default;

		virtual bool intersect(Ray const& ray, HitRecord* hitrec) const = 0;

		virtual void get_rand_toward(Math::RNG& rng, Pos const& from, Dir* dir,float* pdf) const = 0;

		virtual SphereBound get_bound() const = 0;
};

class PrimTri final : public PrimBase {
	public:
		Vertex verts[3];
		Dir normal;

	public:
		PrimTri() = default;
		PrimTri(
			MaterialBase* material,
			Vertex const& vert0, Vertex const& vert1, Vertex const& vert2
		) :
			PrimBase(TYPE::TRI,material),
			verts{vert0,vert1,vert2},
			normal(glm::normalize(glm::cross( verts[1].pos-verts[0].pos, verts[2].pos-verts[0].pos )))
		{}
		virtual ~PrimTri() = default;

		virtual bool intersect(Ray const& ray, HitRecord* hitrec) const override;

		virtual void get_rand_toward(Math::RNG& rng, Pos const& from, Dir* dir,float* pdf) const override {
			Math::SphericalTriangle tri(
				glm::normalize( verts[0].pos - from ),
				glm::normalize( verts[1].pos - from ),
				glm::normalize( verts[2].pos - from )
			);
			*dir = Math::rand_toward_sphericaltri( rng, tri );
			*pdf = 1.0f / tri.surface_area;
		}

		virtual SphereBound get_bound() const override {
			Pos centroid = (verts[0].pos+verts[1].pos+verts[2].pos)*(1.0f/3.0f);
			float max_dist = 0.0f;
			for (size_t i=0;i<3;++i) max_dist=std::max(max_dist,glm::length(verts[i].pos-centroid));
			return { centroid, max_dist };
		}
};
class PrimQuad final : public PrimBase {
	public:
		PrimTri tri0;
		PrimTri tri1;

	public:
		PrimQuad() = default;
		PrimQuad(
			MaterialBase* material,
			Vertex const& vert00, Vertex const& vert10, Vertex const& vert11, Vertex const& vert01
		) :
			PrimBase(TYPE::QUAD,material),
			tri0( material, vert00,vert10,vert11 ),
			tri1( material, vert00,vert11,vert01 )
		{}
		virtual ~PrimQuad() = default;

		virtual bool intersect(Ray const& ray, HitRecord* hitrec) const override;

		virtual void get_rand_toward(Math::RNG& rng, Pos const& from, Dir* dir,float* pdf) const override {
			( rand_1f(rng)<=0.5f ? tri0 : tri1 ).get_rand_toward(rng,from,dir,pdf);
			*pdf *= 0.5f;
		}

		virtual SphereBound get_bound() const override {
			Pos centroid = (tri0.verts[0].pos+tri0.verts[1].pos+tri0.verts[2].pos+tri1.verts[2].pos)*0.25f;
			float max_dist = std::max({
				glm::length(tri0.verts[0].pos-centroid),
				glm::length(tri0.verts[1].pos-centroid),
				glm::length(tri0.verts[2].pos-centroid),
				glm::length(tri1.verts[2].pos-centroid)
			});
			return { centroid, max_dist };
		}
};



class Scene final {
	public:
		struct {
			Pos pos;
			Dir dir;
			Dir up;

			size_t res[2];
			float near, far;
			float fov;
			//float focal;

			glm::mat4x4 matr_P;
			glm::mat4x4 matr_V;
			glm::mat4x4 matr_PV_inv;
		} camera;

		std::map<std::string,MaterialBase*> materials;

		std::vector<PrimBase*> primitives;
		std::vector<PrimBase*> lights;

	private:
		Scene() = default;
	public:
		~Scene();

	private:
		void _init();
	public:
		static Scene* get_new_cornell     ();
		static Scene* get_new_cornell_srgb();
		static Scene* get_new_d65sphere   ();
		static Scene* get_new_srgb        ();

		void get_rand_toward_light(Math::RNG& rng, Pos const& from, Dir* dir,PrimBase const** light,float* pdf );

		bool intersect(Ray const& ray, HitRecord* hitrec, PrimBase const* ignore=nullptr) const;
};
