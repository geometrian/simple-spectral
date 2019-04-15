#include "framebuffer.hpp"

#include "util/lodepng/lodepng.h"



Framebuffer::Framebuffer(size_t const res[2]) :
	res{res[0],res[1]}
{
	//Create pixel buffer
	_pixels = new sRGB_A_F32[res[1]*res[0]];

	//Fill it with a checkerboard pattern
	for (size_t j=0;j<res[1];++j) {
		for (size_t i=0;i<res[0];++i) {
			#ifdef WITH_TRANSPARENT_FRAMEBUFFER
				if (((i/TILE_SIZE)^(j/TILE_SIZE))%2==0) {
					_pixels[j*res[0]+i] = sRGB_A_F32( sRGB_F32(1.0f), 0.5f );
				} else {
					_pixels[j*res[0]+i] = sRGB_A_F32( sRGB_F32(0.0f), 0.5f );
				}
			#else
				if (((i/TILE_SIZE)^(j/TILE_SIZE))%2==0) {
					_pixels[j*res[0]+i] = sRGB_A_F32( sRGB_F32(0.7f), 1.0f );
				} else {
					_pixels[j*res[0]+i] = sRGB_A_F32( sRGB_F32(0.3f), 1.0f );
				}
			#endif
		}
	}
}
Framebuffer::~Framebuffer() {
	//Clean up pixel buffer
	delete[] _pixels;
}

void Framebuffer::save(std::string const& path) const {
	//Construct temporary buffer
	sRGB_U8* pixels = new sRGB_U8[res[1]*res[0]];

	//Fill it with pixels byte-quantized from the internal storage.  While we are doing that, note
	//	we also flip the order of the scanlines so that they are from top to bottom, since this is
	//	the order the PNG saver expects.
	for (size_t j=0;j<res[1];++j) {
		for (size_t i=0;i<res[0];++i) {
			//Source and destination pixel; note vertical flip
			sRGB_U8&          dst = pixels [(res[1]-1-j)*res[0]+i];
			sRGB_A_F32 const& src = _pixels[          j *res[0]+i];

			//Remove alpha
			sRGB_F32 srgb = sRGB_F32( src );

			//Convert to bytes
			srgb = glm::clamp( 255.0f*srgb, sRGB_F32(0),sRGB_F32(255) );
			dst.r = static_cast<uint8_t>(std::round(srgb.r));
			dst.g = static_cast<uint8_t>(std::round(srgb.g));
			dst.b = static_cast<uint8_t>(std::round(srgb.b));
		}
	}

	//Save the image to disk
	lodepng::encode(
		path,
		reinterpret_cast<uint8_t*>(pixels),
		static_cast<unsigned>(res[0]), static_cast<unsigned>(res[1]),
		LCT_RGB
	);

	//Cleanup
	delete[] pixels;
}

#ifdef SUPPORT_WINDOWED
void Framebuffer::draw() const {
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glDrawPixels(
		static_cast<GLsizei>(res[0]), static_cast<GLsizei>(res[1]),
		GL_RGBA, GL_FLOAT, _pixels
	);
}
#endif
