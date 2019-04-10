#include "random.hpp"

#include "math-helpers.hpp"



namespace Math {



Dir rand_sphere(RNG& rng, float* pdf) {
	*pdf = static_cast<float>( 1.0 / (4.0*Constants::pi<double>) );

	//Pick a random z-coordinate, then pick a random point on the circle.  This works out to be
	//	evenly sampled.

	float z = 2.0f*rand_1f(rng) - 1.0f;
	assert(z>=-1.0f&&z<=1.0f);
	float radius_circle = std::sqrt( 1.0f - z*z );

	radians angle = rand_1f(rng) * static_cast<float>(2.0*Constants::pi<double>);

	float c = std::cos(angle);
	float s = std::sin(angle);

	return Dir(radius_circle*c,radius_circle*s,z);
}

Dir rand_coshemi(RNG& rng, float* pdf) {
	Dir result;
	do {
		radians angle = rand_1f(rng) * (2.0f*Constants::pi<float>);
		float c = std::cos(angle);
		float s = std::sin(angle);

		float radius_sq = rand_1f(rng);
		float radius = std::sqrt(radius_sq);

		result = Dir(
			radius * c,
			std::sqrt( 1 - radius_sq ),
			radius * s
		);
		*pdf = result[1];
	} while (*pdf<=EPS);
	*pdf *= 1.0f / Constants::pi<float>;

	return result;
}

Dir rand_toward_sphere(RNG& rng, Dir const& vec_to_sph_cen,float sph_radius, float*__restrict pdf) {
	//Note: needs to be at least double-precision.

	double l = glm::length(glm::dvec3(vec_to_sph_cen));
	if (l<static_cast<double>(sph_radius)) {
		//We're starting inside the sphere.  Every direction hits.

		return rand_sphere(rng,pdf);
	} else {
		//We're outside or on the sphere.

		//Sample a slightly smaller sphere than the one given, just to ensure that the direction
		//	really will hit the real sphere.

		double l_recip = 1.0 / l;

		double radius2 = static_cast<double>(sph_radius) * 0.99999;

		double opp_over_hyp = radius2 * l_recip;
		assert(opp_over_hyp>0.0&&opp_over_hyp<1.0);
		double theta = std::asin(opp_over_hyp); //arcsin(r/l).  Half the vertex angle of the cone.

		//Compute area on the implicit unit sphere centered at the ray origin (the spherical angle
		//	we're sampling over, not on the explicit sphere we're throwing samples toward).
		//	Note: cos(θ) = cos(arcsin(opp_over_hyp)) = sqrt(1-opp_over_hyp*opp_over_hyp)
		double cos_theta = std::sqrt(1.0 - opp_over_hyp*opp_over_hyp);
		//	Note: This formula is simply derived from the area of a spherical cap:
		//		A = 2 π r h, where r := 1 and h = 1-cos(θ).
		double area = (2.0*Constants::pi<double>)*( 1.0 - cos_theta );

		*pdf = static_cast<float>( 1.0 / area );

		//Generate random vector within cone.  This can be computed in `float`, but we might as well
		//	continue with `double` since we have a lot of the stuff we need already . . .
		double y = rand_1d(rng)*(1.0-cos_theta) + cos_theta;
		assert(y>=cos_theta&&y<=1.0);
		double phi = rand_1d(rng) * (2.0*Constants::pi<double>);
		double radius = std::sqrt( 1.0 - y*y );

		double c = std::cos(phi);
		double s = std::sin(phi);
		Dir result = Dir(glm::dvec3( radius*c, y, radius*s ));
		result = get_rotated_to(
			result,
			vec_to_sph_cen * static_cast<float>(l_recip)
		);
		return result;
	}
}

Dir rand_toward_sphericaltri(RNG& rng, SphericalTriangle const& tri) {
	//From "Stratified sampling of spherical triangles" by James Arvo:
	//	http://www.graphics.cornell.edu/pubs/1995/Arv95c.pdf

	float r0 = rand_1f(rng);
	float r1 = rand_1f(rng);

	float sin_alpha = std::sin(tri.alpha);
	assert(sin_alpha>=0);
	float q;
	if (sin_alpha>0) {
		float random_area = r0 * tri.surface_area;
		radians angle = random_area - tri.alpha;
		float s = std::sin(angle);
		float t = std::cos(angle);
		float u = t - tri.cos_alpha;
		float v = s + sin_alpha*tri.cos_c;
		q = ( (v*t-u*s)*tri.cos_alpha - v )/( (v*s+u*t)*sin_alpha );
	} else {
		//Triangle is degenerate.  However, this allows us to compute the desired cosine by
		//	interpolating the angle linearly.
		//	TODO: we should be able to simplify the following in this case; in-particular, `r1`
		//		is no longer necessary.
		q = cos( tri.b * r0 );
	}
	q = glm::clamp( q, -1.0f,1.0f ); //Numerical issues

	auto func_bar = [](Pos const& x,Pos const& y) -> Dir {
		Dir dir = x - glm::dot(x,y)*y;
		float lensq = glm::dot(dir,dir);
		if (lensq==0.0f) return glm::vec3(0);
		return dir * glm::inversesqrt(lensq);
	};

	glm::vec3 C_hat = q*tri.A + sqrt(1-q*q)*func_bar(tri.C,tri.A);

	float z = 1.0f - r1*( 1.0f - glm::dot(C_hat,tri.B) );
	z = glm::clamp( z, -1.0f,1.0f ); //Numerical issues

	Dir result = z*tri.B + sqrt(1-z*z)*func_bar(C_hat,tri.B);
	assert(!std::isnan(result.x)&&!std::isnan(result.y)&&!std::isnan(result.z));
	return result;
}



}
