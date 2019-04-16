#pragma once

#include "stdafx.hpp"

#include "util/random.hpp"



class MaterialBase;

//Encapsulates a simple scene
class Scene final {
	public:
		//Camera parameters.  A simple pinhole camera projection model is used, similar to classic
		//	OpenGL.
		struct {
			//Position, look-at, and up vectors (same meaning as `gluLookat(...)`).
			Pos pos;
			Dir dir;
			Dir up;

			//Screen resolution
			size_t res[2];
			//Near and far clipping planes (not really relevant; used for projection matrix).
			float near, far;
			//Vertical field of view (°)
			float vfov_deg;

			//Projection, view, and inverse-projection-times-view matrices.
			glm::mat4x4 matr_P;
			glm::mat4x4 matr_V;
			glm::mat4x4 matr_PV_inv;
		} camera;

		//Backing store of materials.  Map of their names onto the materials themselves.
		std::map<std::string,MaterialBase*> materials;

		//Backing store of all primitives.
		std::vector<PrimBase*> primitives;
		//Convenience view of all primitives that have emissive materials (i.e. are lights).
		std::vector<PrimBase*> lights;

	private:
		Scene() = default;
	public:
		~Scene();

	private:
		//Common method to precompute some scene data.
		void _init();
	public:
		//Construct new scenes from hard-coded parameters.
		//	Cornell box with original data
		static Scene* get_new_cornell     ();
		//	Cornell box with some walls replaced by white and others by textures
		static Scene* get_new_cornell_srgb();
		//	Camera exactly looking at plane in white environment box
		static Scene* get_new_plane_srgb  ();

		//Get a random direction `dir` from `from` to a randomly chosen light returned in `light`.
		//	The probability density of choosing this direction is returned in `pdf`.
		void get_rand_toward_light(Math::RNG& rng, Pos const& from, Dir* dir,PrimBase const** light,float* pdf );

		//Intersect ray `ray` with the scene.  Returns whether anything was hit, with data in
		//	`hitrec`.  `ignore` can be passed to ignore hits from that primitive.
		bool intersect(Ray const& ray, HitRecord* hitrec, PrimBase const* ignore=nullptr) const;
};
