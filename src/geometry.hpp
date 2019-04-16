#pragma once

#include "stdafx.hpp"

#include "util/random.hpp"



//TODO: comment

class MaterialBase;

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
		PrimBase(TYPE type, MaterialBase* material);
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
