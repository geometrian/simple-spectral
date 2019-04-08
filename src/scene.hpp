#pragma once

#include "stdafx.hpp"

#include "rng.hpp"
#include "spectrum.hpp"


class sRGB_Texture final {
	private:
		size_t res[2];

		PixelRGB8* _data;

	public:
		sRGB_Texture(std::string const& path);
		~sRGB_Texture();

		Spectrum::Sample sample( size_t i,size_t j,   nm lambda_0 ) const;
		Spectrum::Sample sample( glm::vec2 const& st, nm lambda_0 ) const;
};


class MaterialBase {
	protected:
		MaterialBase() = default;
	public:
		virtual ~MaterialBase() = default;

		virtual Spectrum::Sample sample_emission(glm::vec2 const& st, nm lambda_0) const = 0;
		virtual Spectrum::Sample sample_brdf    (glm::vec2 const& st, nm lambda_0) const = 0;

		virtual bool is_emissive() const = 0;
};

class MaterialLambertianTexture final : public MaterialBase {
	private:
		sRGB_Texture _texture;

	public:
		MaterialLambertianTexture(std::string const& path) : MaterialBase(), _texture(path) {}
		virtual ~MaterialLambertianTexture() = default;

		virtual Spectrum::Sample sample_emission(glm::vec2 const& /*st*/, nm /*lambda_0*/) const override { return Spectrum::Sample(0); }
		virtual Spectrum::Sample sample_brdf    (glm::vec2 const&   st,   nm   lambda_0  ) const override {
			return _texture.sample(st,lambda_0) / PI<float>;
		}

		virtual bool is_emissive() const override { return false; }
};
class MaterialLambertianSpectral final : public MaterialBase {
	public:
		//Emission (Lambertian emitter, radiance, default zeroes)
		Spectrum emission;

		//Reflectance (percentage, default ones)
		Spectrum reflectance;

	public:
		MaterialLambertianSpectral();
		virtual ~MaterialLambertianSpectral() = default;

		virtual Spectrum::Sample sample_emission(glm::vec2 const& /*st*/, nm lambda_0) const override {
			return emission   [lambda_0];
		}
		virtual Spectrum::Sample sample_brdf    (glm::vec2 const& /*st*/, nm lambda_0) const override {
			return reflectance[lambda_0] / PI<float>;
		}

		virtual bool is_emissive() const override {
			return Spectrum::integrate(emission) > 0.0f;
		}
};


class Vertex final {
	public:
		Pos pos;
		glm::vec2 st;
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
		static Scene* get_new_srgb        ();

		void get_rand_toward_light(Math::RNG& rng, Pos const& from, Dir* dir,PrimBase const** light,float* pdf );

		bool intersect(Ray const& ray, HitRecord* hitrec) const;
};
