#pragma once

#include "stdafx.hpp"



//Encapsulates the renderer's framebuffer
class Framebuffer final {
	public:
		//Resolution of framebuffer
		size_t const res[2];

		//Rectangular region of pixels (useful for dividing up rendering work).
		class Tile final {
			public:
				//Index of top-left pixel
				size_t pos[2];

				//Width and height of rectangular region
				size_t res[2];
		};

	private:
		//Pixel storage, stored as a flat array of pixels stored in OpenGL order (i.e., scanlines
		//	ordered bottom to top) for efficiency if drawing is enabled.
		sRGB_A_F32* _pixels;

	public:
		explicit Framebuffer(size_t const res[2]);
		~Framebuffer();

		//Get access to the framebuffer's pixel at coordinate (`i`,`j`).
		sRGB_A_F32 const& operator()(size_t i,size_t j) const { return _pixels[j*res[0]+i]; }
		sRGB_A_F32&       operator()(size_t i,size_t j)       { return _pixels[j*res[0]+i]; }

		//Save the framebuffer's contents to the given path `path`.
		void save(std::string const& path) const;

		#ifdef SUPPORT_WINDOWED
		//Draw the framebuffer to the current OpenGL window.
		void draw() const;
		#endif
};
