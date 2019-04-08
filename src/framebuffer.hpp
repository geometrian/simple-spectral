#pragma once

#include "stdafx.hpp"


class Framebuffer final {
	public:
		size_t const res[2];

		class Tile final { public:
			size_t pos[2];
			size_t res[2];
		};

	private:
		sRGBA* _pixels;

	public:
		explicit Framebuffer(size_t const res[2]) : res{res[0],res[1]} {
			_pixels = new sRGBA[res[1]*res[0]];
			for (size_t j=0;j<res[1];++j) {
				for (size_t i=0;i<res[0];++i) {
					if (((i/TILE_SIZE)^(j/TILE_SIZE))%2==0) {
						_pixels[j*res[0]+i] = sRGBA( 1.0f,1.0f,1.0f, 0.5f );
					} else {
						_pixels[j*res[0]+i] = sRGBA( 0.0f,0.0f,0.0f, 0.5f );
					}
				}
			}
		}
		~Framebuffer() {
			delete[] _pixels;
		}

		sRGBA const& operator()(size_t i,size_t j) const { return _pixels[j*res[0]+i]; }
		sRGBA&       operator()(size_t i,size_t j)       { return _pixels[j*res[0]+i]; }

		void save(std::string const& path) const;

		#ifdef SUPPORT_WINDOWED
		void draw() const;
		#endif
};
