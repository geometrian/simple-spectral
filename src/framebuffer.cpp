#include "framebuffer.hpp"

#include "lodepng/lodepng.h"


void Framebuffer::save(std::string const& path) const {
	struct PixelRGB8* pixels = new struct PixelRGB8[res[1]*res[0]];
	for (size_t j=0;j<res[1];++j) {
		for (size_t i=0;i<res[0];++i) {
			PixelRGB8&   dst = pixels [(res[1]-1-j)*res[0]+i];
			sRGBA const& src = _pixels[          j *res[0]+i];

			sRGB srgb = sRGB( src );
			srgb = glm::clamp( 255.0f*srgb, sRGB(0),sRGB(255) );
			dst.r = static_cast<uint8_t>(std::round(srgb.r));
			dst.g = static_cast<uint8_t>(std::round(srgb.g));
			dst.b = static_cast<uint8_t>(std::round(srgb.b));
		}
	}

	lodepng::encode(
		path,
		reinterpret_cast<uint8_t*>(pixels),
		static_cast<unsigned>(res[0]), static_cast<unsigned>(res[1]),
		LCT_RGB
	);

	delete[] pixels;
}

#ifdef SUPPORT_WINDOWED
void Framebuffer::draw() const {
	//glClearColor( 1.0f,1.0f,1.0f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	/*glViewport(0,0,static_cast<GLsizei>(res[0]),static_cast<GLsizei>(res[1]));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( 0.0,static_cast<GLdouble>(res[0]), 0.0,static_cast<GLdouble>(res[1]), -1.0,1.0 );

	glColor3f( 0.5f,0.5f,0.5f );
	glEnable(GL_POLYGON_STIPPLE);
	GLubyte pattern[128];
	for (size_t j=0;j<4;++j) for (size_t i=0;i<4;++i) {
		GLubyte value = ((i^j)&0b1)>0 ? 0x00 : 0xFF;
		for (size_t jj=0;jj<8;++jj) pattern[(8*j+jj)*4+i]=value;
	}
	glPolygonStipple(pattern);
	glRecti(0,0,static_cast<GLint>(res[0])-1,static_cast<GLint>(res[1])-1);
	glDisable(GL_POLYGON_STIPPLE);
	glColor3f(1,1,1);*/

	glDrawPixels(
		static_cast<GLsizei>(res[0]), static_cast<GLsizei>(res[1]),
		GL_RGBA, GL_FLOAT, _pixels
	);
}
#endif
