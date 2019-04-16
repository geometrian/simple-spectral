#pragma once

#include "../stdafx.hpp"



namespace Str {



inline bool contains(std::string const& main, std::string const& test) {
	return main.find(test) != std::string::npos;
}

inline bool startswith(std::string const& main, std::string const& test) {
	return main.compare( 0,test.size(), test ) == 0;
}
inline bool endswith  (std::string const& main, std::string const& test) {
	if (main.length()>=test.length()) {
		return main.compare( main.length()-test.length(),test.length(), test ) == 0;
	} else {
		return false;
	}
}

inline std::vector<std::string> split(std::string const& main, std::string const& test, size_t max_splits=~size_t(0)) {
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

inline int      to_int (std::string const& str) {
	size_t i;
	int value = std::stoi(str,&i);
	if (i==str.length()) return value;
	throw -1; //Contained non-number values
}
inline unsigned to_nneg(std::string const& str) {
	int val = to_int(str);
	if (val>=0) return static_cast<unsigned>(val);
	throw -2; //Negative
}
inline unsigned to_pos (std::string const& str) {
	int val = to_int(str);
	if (val>0) return static_cast<unsigned>(val);
	throw -2; //Not strictly positive
}



}
