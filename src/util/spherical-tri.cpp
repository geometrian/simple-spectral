#include "spherical-tri.hpp"



namespace Math {



//Largest `float` which is strictly less than "π".
inline static float _underestimate_pi() {
	static_assert(sizeof(float)==sizeof(uint32_t),"Implementation error!");
	uint32_t u = 0x40490FDAu;
	float result;
	std::memcpy(&result,&u,sizeof(uint32_t));
	return result;
}

SphericalTriangle::SphericalTriangle(Pos const& A, Pos const& B, Pos const& C) :
	A(A), B(B), C(C)
{
	cos_a = glm::clamp( glm::dot(B,C), -1.0f,1.0f );
	cos_b = glm::clamp( glm::dot(A,C), -1.0f,1.0f );
	cos_c = glm::clamp( glm::dot(A,B), -1.0f,1.0f );

	a = std::acos(cos_a);
	b = std::acos(cos_b);
	c = std::acos(cos_c);
	//The result should be in [0,π], but in-practice it can be slightly outside of this.  In-
	//	particular, `std::acos(-1)`>π, causing the sine below to be negative.
	a = glm::clamp( a, 0.0f,_underestimate_pi() );
	b = glm::clamp( b, 0.0f,_underestimate_pi() );
	c = glm::clamp( c, 0.0f,_underestimate_pi() );
	assert(
		a>=0 && a<Constants::pi<float> &&
		b>=0 && a<Constants::pi<float> &&
		c>=0 && a<Constants::pi<float>
	);

	sin_a = std::sin(a);
	sin_b = std::sin(b);
	sin_c = std::sin(c);
	assert(sin_a>=0&&sin_b>=0&&sin_c>=0);

	/*
	Need to compute the angles at vertices and the surface area.  The surface area can be easily
	computed from the angles (see http://mathworld.wolfram.com/SphericalTriangle.html), but the
	angles themselves are tricky.

	The calculations should be robust to when any combination of A, B, and C are near to each other
	or nearly (or exactly) collinear.
	*/

	float numer0 = cos_a - cos_b*cos_c;
	float numer1 = cos_b - cos_c*cos_a;
	float numer2 = cos_c - cos_a*cos_b;
	//`numer{0,1,2}` theoretically in range [-1,1], but may be outside it.  Enforce after division
	//	(clamps below).
	float denom0 = sin_b*sin_c;
	float denom1 = sin_c*sin_a;
	float denom2 = sin_a*sin_b;
	assert(denom0>=0&&denom1>=0&&denom2>=0);

	if (denom0>0 && denom1>0 && denom2>0) {
		cos_alpha = glm::clamp( numer0/denom0, -1.0f,1.0f );
		cos_beta  = glm::clamp( numer1/denom1, -1.0f,1.0f );
		cos_gamma = glm::clamp( numer2/denom2, -1.0f,1.0f );
		alpha = glm::clamp( std::acos(cos_alpha), 0.0f,_underestimate_pi() );
		beta  = glm::clamp( std::acos(cos_beta ), 0.0f,_underestimate_pi() );
		gamma = glm::clamp( std::acos(cos_gamma), 0.0f,_underestimate_pi() );
		assert(alpha>=0&&beta>=0&&gamma>=0);

		surface_area = alpha + beta + gamma - Constants::pi<float>;
		if (surface_area>=0); else surface_area=0; //Numerical issues
	} else {
		surface_area = 0;

		if (sin_a > 0) {
			if (sin_b > 0) {
				if (sin_c > 0) {
					//Too imprecise.  Numerical issues are what made something zero above.
					goto DEGENERATE_OR_TOO_IMPRECISE;
				} else {
					//Only `c` is 0 or π.
					cos_alpha=cos_beta =1; alpha=beta =Constants::pi<float>*0.5f;
					cos_gamma=glm::clamp(numer2/denom2,-1.0f,1.0f); gamma=std::acos(cos_gamma);
				}
			} else {
				if (sin_c > 0) {
					//Only `b` is 0 or π.
					cos_alpha=cos_gamma=1; alpha=gamma=Constants::pi<float>*0.5f;
					cos_beta =glm::clamp(numer1/denom1,-1.0f,1.0f); beta =std::acos(cos_beta );
				} else {
					//Both `b` and `c` are 0 or π.
					goto DEGENERATE_OR_TOO_IMPRECISE;
				}
			}
		} else {
			if (sin_b > 0) {
				if (sin_c > 0) {
					//Only `a` is 0 or π.
					cos_beta =cos_gamma=1; beta =gamma=Constants::pi<float>*0.5f;
					cos_alpha=glm::clamp(numer0/denom0,-1.0f,1.0f); alpha=std::acos(cos_alpha);
				} else {
					//Both `a` and `c` are 0 or π.
					goto DEGENERATE_OR_TOO_IMPRECISE;
				}
			} else {
				//`a`, `b`, and possibly `c` are 0 or π.
				goto DEGENERATE_OR_TOO_IMPRECISE;
			}
		}

		return;

		DEGENERATE_OR_TOO_IMPRECISE:
			//Degenerate:
			//	At least two of `sin_{a,b,c}` are zero (this means, theoretically, all three are
			//	zero) and the triangle is degenerate.
			//Imprecise:
			//	Numerical issues have made the result too small to compute accurately, despite all
			//	values being in sensible, correct domains.
			cos_alpha=cos_beta=cos_gamma = alpha=beta=gamma = std::numeric_limits<float>::quiet_NaN();
	}
}



}
