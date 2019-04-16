#pragma once



//Includes

//	C Standard Library
#include <cassert>
#include <cstdio>

//	C++ Standard Library
#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <fstream>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

//	GLM
#define GLM_FORCE_SIZE_T_LENGTH
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#ifdef SUPPORT_WINDOWED
	//	GLFW
	#include <GLFW/glfw3.h>
#endif



//Configuration

//	(Note also usage of user-defined literals, defined below.)

//	Use explicit light sampling when path tracing.
#define EXPLICIT_LIGHT_SAMPLING

//	Maximum depth of path trace integrator (including shadow rays).
#define MAX_DEPTH 10u

//	Work items during the path trace are square tiles of pixels.  This is their width and height.
#define TILE_SIZE 4_zu

//	If enabled, compensates for the cosine-factor falloff due to viewing rays leaving the camera
//		sensor at an angle by brightening those areas by an inverse factor.  This is quite typical
//		for real-world cameras (indeed, many people don't know this is even necessary).
#define FLAT_FIELD_CORRECTION

//	Epsilon, used for a variety of numerical tests.
#define EPS 0.001f

//	Whether to use spectral rendering (correct), and if so which variant to use (our paper or the
//		work by Meng et al.) or RGB mode (what many people do instead).
#if 1
	#define RENDER_MODE_SPECTRAL
	#if 1
		#define RENDER_MODE_SPECTRAL_OURS
	#else
		#define RENDER_MODE_SPECTRAL_MENG
	#endif

	//		The CIE observer standard to use.  The 1931 version is the CIE 1931 2° standard
	//			observer, and the 2006 version is the CIE 2006 10° standard observer.  The former is
	//			based on 1920s experiments and is well-established.  The latter is based on updated
	//			data, a wider field of view, and a denser sampling.  The latter is probably what one
	//			should be using.
	#if 0
		#define CIE_OBSERVER 1931
	#else
		#define CIE_OBSERVER 2006
	#endif

	//		Number of wavelengths sampled by a single sample.  When more than one is used, hero
	//			wavelength sampling is done.
	#define SAMPLE_WAVELENGTHS 4_zu
#else
	#define RENDER_MODE_RGB
#endif

#ifdef SUPPORT_WINDOWED
	//Whether to make the un-rendered pixels partially transparent.  Disabled by default because on
	//	Windows 10, current GLFW has a bug which prevents it from working correctly:
	//		https://github.com/glfw/glfw/issues/1237
	//#define WITH_TRANSPARENT_FRAMEBUFFER
#endif



//Computed values

#ifdef RENDER_MODE_SPECTRAL
	#if defined RENDER_MODE_SPECTRAL_MENG && CIE_OBSERVER!=1931
		#error "Meng et al. only support the CIE 1931 standard observer!"
	#endif

	//	Shortest and longest wavelengths, in nanometers, considered during the rendering.  It only
	//		makes sense to sample wavelengths where the observer can see anything (and maybe then,
	//		perhaps even slightly less than that, since at the extreme wavelengths we cannot see
	//		very well).  The values here are simply the ranges of the respective observer functions.
	#if   CIE_OBSERVER == 1931
		#define LAMBDA_MIN 380_nm
		#define LAMBDA_MAX 780_nm
	#elif CIE_OBSERVER == 2006
		#define LAMBDA_MIN 390_nm
		#define LAMBDA_MAX 830_nm
	#else
		#error "Implementation error!"
	#endif
#endif



//Common Types

#ifdef RENDER_MODE_SPECTRAL
	//	CIE XYZ tristimulus values
	typedef glm::vec3 CIEXYZ_32F;
	typedef glm::vec4 CIEXYZ_A_32F;

	//	Nanometers
	typedef float nm;
	constexpr inline float operator""_nm(long double        x) { return static_cast<float>(x); }
	constexpr inline float operator""_nm(unsigned long long x) { return static_cast<float>(x); }

	//	Kelvin scale (kelvin, K)
	typedef float kelvin;
#endif

//	BT.709 color
//		Linear (pre-gamma), floating-point
typedef glm::vec3 lRGB_F32;
//		Post-gamma, floating-point
typedef glm::vec3 sRGB_F32;
//		Post-gamma RGB and linear alpha, floating-point
typedef glm::vec4 sRGB_A_F32;
//		Post-gamma, byte
class sRGB_U8 final { public: uint8_t r, g, b;      };
static_assert(sizeof(sRGB_U8  )==3,"Implementation error!");
class sRGB_A_U8 final { public: uint8_t r, g, b, a; };
static_assert(sizeof(sRGB_A_U8)==4,"Implementation error!");

//	Position
typedef glm::vec3 Pos;
//	Direction
typedef glm::vec3 Dir;
//	Distance
typedef float Dist;

//	Angle
typedef float radians;

//	ST and UV coordinates.  Many graphics people call mesh texture coordinates "UV"s, but strictly
//		speaking they're "ST"-coordinates.  That is, they are normalized to the [0,1] range.  The
//		"UV" space is actually the coordinates within the texture space, and is not normalized by
//		the resolution of the texture.
typedef glm::vec2 ST;
typedef glm::vec2 UV;

#ifndef RENDER_MODE_SPECTRAL
	//Absolute madness, I say
	typedef lRGB_F32 RGB_Radiance;
	typedef lRGB_F32 RGB_RadiantFlux;
	typedef lRGB_F32 RGB_RecipSR;
#endif
typedef lRGB_F32 RGB_Reflectance;



//Common stuff

namespace Constants {

//	π
//		Value is given to perfect machine precision with 80-bit `long double`.
template <typename T> constexpr T pi = T(3.14159265358979323846L);

//	k_B, the Boltzmann constant (J⋅K⁻¹)
//		Value is given to known precision.
template <typename T> constexpr T k_B = T(1.38064852e-23L);

//	h, the Planck constant (J⋅s)
//		Value is a precise definition.
template <typename T> constexpr T h = T(6.62607015e-34L);

//	c, the speed of light (m⋅s⁻¹)
//		Value is a precise definition.
template <typename T> constexpr T c = T(299'792'458.0L);

}

//	Ray structure
class Ray final {
	public:
		Pos orig;
		Dir dir;

	public:
		Pos at(Dist dist) const { return orig + dist*dir; }
};

//	Hit record
class PrimBase;
class HitRecord final {
	public:
		PrimBase const* prim;

		Dir normal;
		ST st;

		Dist dist;
};

//	Bounding sphere
class SphereBound final {
	public:
		Pos center;
		Dist radius;
};

//	Hash functions
template <typename type> inline size_t get_hashed(type const& item                     ) {
	std::hash<type> hasher;
	return hasher(item);
}
template <typename type> inline size_t get_hashed(type const& item, size_t combine_with) {
	//http://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
	return combine_with^( get_hashed(item) + 0x9E3779B9 + (combine_with<<6) + (combine_with>>2) );
}

//	Definition of `size_t` literal
constexpr inline size_t operator""_zu(unsigned long long x) { return static_cast<size_t>(x); }



//Helpful macros

//	Needed for technical reasons when using certain macros
#define COMMA ,

//	Constants.  TODO: move to `Constants::`?
#define qNaN std::numeric_limits<float>::quiet_NaN()
#define INF  std::numeric_limits<float>::infinity();

#ifdef RENDER_MODE_SPECTRAL
	//	The size of the wavelength band each wavelength in a hero sample is responsible for
	#define LAMBDA_STEP ( (LAMBDA_MAX-LAMBDA_MIN) / static_cast<nm>(SAMPLE_WAVELENGTHS) )

	//	Code that's only present when doing spectral rendering
	#define SPECTRAL_ONLY(CODE) CODE
#else
	#define SPECTRAL_ONLY(CODE)
#endif
