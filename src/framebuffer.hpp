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

		#ifdef SUPPORT_WINDOWED
		void draw() const {
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
};
