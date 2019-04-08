#pragma once

//C Standard Library
#include <cassert>
//#include <cinttypes>
#include <cstdio>

//C++ Standard Library
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
//#include <string>
#include <thread>
#include <vector>

#define GLM_FORCE_SIZE_T_LENGTH
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#ifdef SUPPORT_WINDOWED
	#include <GLFW/glfw3.h>
#endif


#define TILE_SIZE size_t(4)

#define SAMPLE_WAVELENGTHS size_t(4)

#define MAX_DEPTH 10u

#define LAMBDA_MIN nm(380.0f)
#define LAMBDA_MAX nm(780.0f)
#define LAMBDA_STEP ( (LAMBDA_MAX-LAMBDA_MIN) / static_cast<float>(SAMPLE_WAVELENGTHS) )

#define EPS 0.00001f

#if 1
	#define CIE_OBSERVER 1931
#else
	#define CIE_OBSERVER 2006
#endif

#define FLAT_FIELD_CORRECTION


template <typename T> constexpr T PI = T(3.14159265358979323846L); //π


typedef glm::vec3 CIEXYZ;
typedef glm::vec3 lRGB;
typedef glm::vec3 sRGB;
typedef glm::vec4 sRGBA;

struct PixelRGB8 final {
	uint8_t r, g, b;
};
static_assert(sizeof(struct PixelRGB8)==3,"Implementation error!");

typedef glm::vec3 Pos;
typedef glm::vec3 Dir;

typedef float nm;


inline bool str_contains(std::string const& main, std::string const& test) {
	return main.find(test) != std::string::npos;
}
inline std::vector<std::string> str_split(std::string const& main, std::string const& test, size_t max_splits=~size_t(0)) {
	std::vector<std::string> result;
	size_t num_splits = 0;
	size_t offset = 0;
	LOOP:
		size_t loc = main.find(test,offset);
		if (loc!=main.npos) {
			assert(offset<=loc);
			result.emplace_back(main.substr(offset,loc-offset));
			offset = loc + test.size();
			if (++num_splits<max_splits) goto LOOP;
		}
	result.emplace_back(main.substr(offset));
	return result;
}
inline int      str_to_int (std::string const& str) {
	size_t i;
	int value = std::stoi(str,&i);
	if (i==str.length()) return value;
	throw -1; //Contained non-number values
}
inline unsigned str_to_nneg(std::string const& str) {
	int val = str_to_int(str);
	if (val>=0) return static_cast<unsigned>(val);
	throw -2; //Negative
}
inline unsigned str_to_pos (std::string const& str) {
	int val = str_to_int(str);
	if (val>0) return static_cast<unsigned>(val);
	throw -2; //Not strictly positive
}

template <typename type> inline size_t get_hashed(type const& item) {
	std::hash<type> hasher;
	return hasher(item);
}
template <typename type> inline size_t get_hashed(type const& item, size_t combine_with) {
	//http://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
	return combine_with^( get_hashed(item) + 0x9E3779B9 + (combine_with<<6) + (combine_with>>2) );
}

class Ray final {
	public:
		Pos orig;
		Dir dir;

	public:
		Pos at(float dist) const {
			return orig + dist*dir;
		}
};

class PrimBase;

class HitRecord final {
	public:
		PrimBase const* prim;

		Dir normal;
		glm::vec2 st;

		float dist;
};

class SphereBound final {
	public:
		Pos center;
		float radius;
};
