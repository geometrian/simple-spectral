#include "framebuffer.hpp"

#include "util/lodepng/lodepng.h"
#include "util/color.hpp"
#include "util/string.hpp"



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
	if        (Str::endswith(path,".csv")) {
		//Save floating-point image in CSV file

		//	Open file
		FILE* file = fopen(path.c_str(),"wb");
		assert(file!=nullptr);

		for (size_t j=0;j<res[1];++j) {
			for (size_t i=0;;++i) {
				sRGB_A_F32 const& srgba = _pixels[ j*res[0] + i ];
				lRGB_F32 lrgb = Color::srgb_to_lrgb(sRGB_F32(srgba));

				fprintf(file, "%g,%g,%g",
					static_cast<double>(lrgb.r),
					static_cast<double>(lrgb.g),
					static_cast<double>(lrgb.b)
				);
				if (i<res[0]-1) fputc(',',file);
				else          { fputc('\n',file); break; }
			}
		}

		//	Done
		fclose(file);
	} else if (Str::endswith(path,".hdr")) {
		//Save HDR RADIANCE image

		//	Open file
		FILE* file = fopen(path.c_str(),"wb");
		assert(file!=nullptr);

		//	Write header
		fprintf(file,
			"#?RADIANCE\n"
			"FORMAT=32-bit_rle_rgbe\n"
			"EXPOSURE=1.0\n"
			"SOFTWARE=simple-spectral\n"
			"\n"
			"-Y %zu +X %zu\n",
			res[1], res[0]
		);

		//	Write data
		for (size_t j=0;j<res[1];++j) {
			for (size_t i=0;i<res[0];++i) {
				sRGB_A_F32 const& srgba = _pixels[ (res[1]-1-j)*res[0] + i ];
				lRGB_F32 lrgb = Color::srgb_to_lrgb(sRGB_F32(srgba));

				float v = std::max(lrgb.r,std::max(lrgb.g,lrgb.b));

				if (v < 1.0e-32f) {
					uint32_t zero = 0u;
					fwrite( &zero, sizeof(uint32_t),1u, file );
				} else {
					int e;
					v = std::frexp(v,&e) * 256.0f/v;
					e += 128;

					glm::vec3 rgb = glm::round(lrgb*v);
					uint8_t data[4] = {
						static_cast<uint8_t>(glm::clamp( static_cast<int>(std::round(rgb[0])), 0,255 )),
						static_cast<uint8_t>(glm::clamp( static_cast<int>(std::round(rgb[1])), 0,255 )),
						static_cast<uint8_t>(glm::clamp( static_cast<int>(std::round(rgb[2])), 0,255 )),
						static_cast<uint8_t>(                                        e                )
					};
					fwrite( data, sizeof(uint8_t),4, file );
				}
			}
		}

		//	Done
		fclose(file);
	} else if (Str::endswith(path,".pfm")) {
		//Save PFM image

		//	Open file
		FILE* file = fopen(path.c_str(),"wb");
		assert(file!=nullptr);

		//	Write header
		fprintf(file,
			"PF\n"
			"%zu %zu\n"
			"-1.0\n",
			res[0], res[1]
		);

		//	Write data
		for (size_t j=0;j<res[1];++j) {
			for (size_t i=0;i<res[0];++i) {
				//		It's unclear, but the data is supposed to be stored in bottom-to-top order
				//			in the file, unlike NetPBM.  Note that some reference data gets this
				//			wrong!
				sRGB_A_F32 const& srgba = _pixels[ (res[1]-1-j)*res[0] + i ];
				lRGB_F32 lrgb = Color::srgb_to_lrgb(sRGB_F32(srgba));
				fwrite( &lrgb, sizeof(float),3, file );
			}
		}

		//	Done
		fclose(file);
	} else {
		//Save PNG image

		//	Construct temporary buffer
		sRGB_A_U8* pixels = new sRGB_A_U8[res[1]*res[0]];

		//	Fill it with pixels byte-quantized from the internal storage.  While we are doing that,
		//		note we also flip the order of the scanlines so that they are from top to bottom,
		//		since this is the order the PNG saver expects.
		for (size_t j=0;j<res[1];++j) {
			for (size_t i=0;i<res[0];++i) {
				//Source and destination pixel; note vertical flip
				sRGB_A_U8&        dst   = pixels [(res[1]-1-j)*res[0]+i];
				sRGB_A_F32 const& srgba = _pixels[          j *res[0]+i];

				//Convert to bytes
				sRGB_A_F32 srgba_clipped = glm::clamp( 255.0f*srgba, sRGB_A_F32(0),sRGB_A_F32(255) );
				dst.r = static_cast<uint8_t>(std::round(srgba_clipped.r));
				dst.g = static_cast<uint8_t>(std::round(srgba_clipped.g));
				dst.b = static_cast<uint8_t>(std::round(srgba_clipped.b));
				dst.a = static_cast<uint8_t>(std::round(srgba_clipped.a));
			}
		}

		//	Save the image to disk
		lodepng::encode(
			path,
			reinterpret_cast<uint8_t*>(pixels),
			static_cast<unsigned>(res[0]), static_cast<unsigned>(res[1]),
			LCT_RGBA
		);

		//	Cleanup
		delete[] pixels;
	}
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
