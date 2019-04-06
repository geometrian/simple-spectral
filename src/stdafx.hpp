#pragma once

//C Standard Library
#include <cassert>
//#include <cinttypes>
#include <cstdio>

//C++ Standard Library
//#include <map>
//#include <set>
#include <algorithm>
#include <array>
#include <functional>
#include <fstream>
#include <map>
#include <mutex>
#include <random>
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

#define EPS 0.001f


template <typename T> constexpr T PI = T(3.14159265358979323846L); //π


typedef glm::vec3 CIEXYZ;
typedef glm::vec3 lRGB;
typedef glm::vec3 sRGB;
typedef glm::vec4 sRGBA;

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

template <typename T> T lerp( T val0,T val1, float blend ) {
	return val0*(1.0f-blend) + val1*blend;
}
/*template <typename T> T clamp( T val, T low,T high ) {
	if (val>low) {
		if (val<high) return val;
		else          return high;
	} else {
		return low;
	}
}*/

class Ray final {
	public:
		Pos orig;
		Dir dir;
};

class PrimitiveBase;

class HitRecord final {
	public:
		PrimitiveBase const* prim;
		Dir normal;
		float dist;
};
