#pragma once

#include "../stdafx.hpp"



namespace Math {



class SphericalTriangle final {
	public:
		//Vertices
		Pos A;
		Pos B;
		Pos C;

		float a,b,c; //Side lengths (on the surface of the sphere); also equal to the angle they subtend with respect to the sphere's center.
		float sin_a,sin_b,sin_c; //Sine of angles subtended by sides from center of sphere.
		float cos_a,cos_b,cos_c; //Cosine of angles subtended by sides from center of sphere.

		radians alpha,beta,gamma; //Vertex angles at A, B, and C, respectively.
		float cos_alpha,cos_beta,cos_gamma; //Cosine of angles at vertices.

		//Area (on the surface of the sphere); also equal to the solid angle subtended.
		union {
			float surface_area;
			float solid_angle;
			float spherical_excess;
		};

	public:
		//Position on the spherical triangle (unit vectors).
		SphericalTriangle(Pos const& A, Pos const& B, Pos const& C);
		~SphericalTriangle() = default;
};



}
