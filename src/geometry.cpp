#include "geometry.hpp"

#include "material.hpp"



PrimBase::PrimBase(TYPE type, MaterialBase* material) :
	type(type), material(material), is_light(material->is_emissive())
{}


bool PrimTri:: intersect(Ray const& ray, HitRecord* hitrec) const /*override*/ {
	//Robust ray-triangle intersection.  See:
	//	http://jcgt.org/published/0002/01/05/paper.pdf

	//Dimension where the ray direction is maximal
	Dir abs_dir = glm::abs(ray.dir);
	size_t kx, ky, kz;
	if (abs_dir.x>abs_dir.y) {
		if (abs_dir.x>abs_dir.z) {
			kz=0; kx=1; ky=2;
		} else {
			kz=2; kx=0; ky=1;
		}
	} else {
		if (abs_dir.y>abs_dir.z) {
			kz=1; kx=2; ky=0;
		} else {
			kz=2; kx=0; ky=1;
		}
	}
	if (ray.dir[kz]<0) std::swap(kx,ky); //Winding order

	//Calculate shear constants
	float Sx = ray.dir[kx] / ray.dir[kz]; //* ray.dir_inv[kz];
	float Sy = ray.dir[ky] / ray.dir[kz]; //* ray.dir_inv[kz];
	float Sz =        1.0f / ray.dir[kz]; //  ray.dir_inv[kz];

	//Vertices relative to ray origin
	Pos const A = verts[0].pos - ray.orig;
	Pos const B = verts[1].pos - ray.orig;
	Pos const C = verts[2].pos - ray.orig;

	//Shear and scale of vertices
	glm::vec3 ABC_kx = glm::vec3(A[kx],B[kx],C[kx]);
	glm::vec3 ABC_ky = glm::vec3(A[ky],B[ky],C[ky]);
	glm::vec3 ABC_kz = glm::vec3(A[kz],B[kz],C[kz]);
	glm::vec3 ABCx = ABC_kx - Sx*ABC_kz; //Ax, Bx, Cx
	glm::vec3 ABCy = ABC_ky - Sy*ABC_kz; //Ay, By, Cy

	//Scaled barycentric coordinates and edge tests
	glm::vec3 UVW = glm::cross( ABCy, ABCx );
	float const& U = UVW[0];
	float const& V = UVW[1];
	float const& W = UVW[2];
	if (U!=0.0f && V!=0.0f && W!=0.0f) {
		if ( (U<0.0f||V<0.0f||W<0.0f) && (U>0.0f||V>0.0f||W>0.0f) ) return false;
	} else {
		glm::dvec3 UVWd = glm::cross( glm::dvec3(ABCy), glm::dvec3(ABCx) );
		double const& Ud = UVWd[0];
		double const& Vd = UVWd[1];
		double const& Wd = UVWd[2];

		if ( (Ud<0.0||Vd<0.0||Wd<0.0) && (Ud>0.0||Vd>0.0||Wd>0.0) ) return false;

		UVW = glm::vec3(UVWd);
	}

	//Determinant
	float det = U + V + W;
	if (std::abs(det)>EPS);
	else return false;

	//Calculate scaled z-coordinates of vertices and use them to calculate the hit distance.
	glm::vec3 ABCz = Sz * ABC_kz;
	float const T = U*ABCz.x + V*ABCz.y + W*ABCz.z;

	//	Check signs of `det` and `T` match
	uint32_t det_u; memcpy(&det_u,&det, sizeof(float));
	uint32_t T_u;   memcpy(&T_u,  &T,   sizeof(float));
	bool different = ((det_u&0x80000000u) ^ (T_u&0x80000000u)) > 0;
	if (different) return false;

	//Normalize U, V, W, and T, then return
	float det_recip = 1 / det;
	Dist dist = T * det_recip;
	assert(!std::isnan(dist));
	if (dist>=EPS && dist<hitrec->dist) {
		hitrec->prim   = this;

		glm::vec3 bary = UVW * det_recip;
		hitrec->normal = normal;
		hitrec->st     = bary.x*verts[0].st + bary.y*verts[1].st + bary.z*verts[2].st;

		hitrec->dist   = dist;

		return true;
	}

	return false;
}


bool PrimQuad::intersect(Ray const& ray, HitRecord* hitrec) const /*override*/ {
	if (tri0.intersect(ray,hitrec)) goto HIT;
	if (tri1.intersect(ray,hitrec)) goto HIT;
	return false;

	HIT:
	hitrec->prim = this;
	return true;
}
