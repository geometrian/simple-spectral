#pragma once

#include "../stdafx.hpp"


namespace Math {



template <typename T> inline T lerp( T val0,T val1, float blend ) {
	return val0*(1.0f-blend) + val1*blend;
}

inline void get_basis( Dir const& basis_y, Dir*__restrict basis_x,Dir*__restrict basis_z ) {
	//http://jcgt.org/published/0006/01/01/paper.pdf

	float sign = std::copysignf(1.0f,basis_y.z);

	float a = -1.0f / (sign + basis_y.z);
	float b = basis_y.x * basis_y.y * a;

	*basis_x = Dir(
		1.0f + sign*basis_y.x*basis_y.x*a,
		sign*b,
		-sign*basis_y.x
	);
	*basis_z = Dir(
		b,
		sign + basis_y.y*basis_y.y*a,
		-basis_y.y
	);
}

inline Dir get_rotated_to(Dir const& dir, Dir const& normal) {
	Dir basis_x, basis_z;
	get_basis(normal, &basis_x,&basis_z);
	return dir.x*basis_x + dir.y*normal + dir.z*basis_z;
}



}
