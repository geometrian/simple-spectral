#pragma once

#include "stdafx.hpp"

#include "util/random.hpp"



//TODO: comment

class MaterialBase;

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

		bool intersect(Ray const& ray, HitRecord* hitrec, PrimBase const* ignore=nullptr) const;
};
