#pragma once

#include "stdafx.hpp"

#include "util/random.hpp"



class MaterialBase;



class Vertex final {
	public:
		//Vertex coordinate
		Pos pos;

		//ST texture coordinate
		//	Note: if you expected UV, see the definition of `ST` for explanation.
		ST st;
};



//Encapsulates a geometric primitive
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

		//Get a random direction `dir` from `from` toward the primitive.  The probability density of
		//	choosing this direction is returned in `pdf`.
		virtual void get_rand_toward(Math::RNG& rng, Pos const& from, Dir* dir,float* pdf) const = 0;

		virtual SphereBound get_bound() const = 0;
};


//Triangle primitive
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

		virtual void get_rand_toward(Math::RNG& rng, Pos const& from, Dir* dir,float* pdf) const override;

		virtual SphereBound get_bound() const override;
};

//Quadrilateral primitive
//	Note: should be planar (or else must update `.intersect(...)`).
//	Implemented as two triangle primitives, although some operations can be optimized slightly.
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

		virtual void get_rand_toward(Math::RNG& rng, Pos const& from, Dir* dir,float* pdf) const override;

		virtual SphereBound get_bound() const override;
};
