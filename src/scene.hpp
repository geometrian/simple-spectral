#pragma once

#include "stdafx.hpp"

#include "spectrum.hpp"


class MaterialBase {
	protected:
		MaterialBase() = default;
	public:
		virtual ~MaterialBase() = default;

		virtual Spectrum::Sample sample_emission(nm lambda_0) const = 0;
		virtual Spectrum::Sample sample_brdf    (nm lambda_0) const = 0;
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

		virtual Spectrum::Sample sample_emission(nm lambda_0) const override {
			return emission   [lambda_0];
		}
		virtual Spectrum::Sample sample_brdf    (nm lambda_0) const override {
			return reflectance[lambda_0] / PI<float>;
		}
};


class PrimitiveBase {
	public:
		MaterialBase* material;

	protected:
		PrimitiveBase() = default;
		explicit PrimitiveBase(MaterialBase* material) : material(material) {}
	public:
		virtual ~PrimitiveBase() = default;

		virtual bool intersect(Ray const& ray, HitRecord* hitrec) const = 0;
};

class Tri final : public PrimitiveBase {
	public:
		Pos verts[3];
		Dir normal;

	public:
		Tri() = default;
		Tri(
			MaterialBase* material,
			Pos const& pos0, Pos const& pos1, Pos const& pos2
		) :
			PrimitiveBase(material),
			verts{pos0,pos1,pos2},
			normal(glm::normalize(glm::cross( verts[1]-verts[0], verts[2]-verts[0] )))
		{}
		virtual ~Tri() = default;

		virtual bool intersect(Ray const& ray, HitRecord* hitrec) const override;
};
class Quad final : public PrimitiveBase {
	public:
		Tri tri0;
		Tri tri1;

	public:
		Quad() = default;
		Quad(
			MaterialBase* material,
			Pos const& pos00, Pos const& pos10, Pos const& pos11, Pos const& pos01
		) :
			PrimitiveBase(material),
			tri0( material, pos00,pos10,pos11 ),
			tri1( material, pos00,pos11,pos01 )
		{}
		virtual ~Quad() = default;

		virtual bool intersect(Ray const& ray, HitRecord* hitrec) const override;
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

		std::vector<PrimitiveBase*> primitives;

	private:
		Scene() = default;
	public:
		~Scene();

	private:
		void _init();
	public:
		static Scene* get_new_cornell();

		bool intersect(Ray const& ray, HitRecord* hitrec) const;
};
