#include "renderer.hpp"

#include "util/color.hpp"
#include "util/math-helpers.hpp"

#include "geometry.hpp"
#include "material.hpp"
#include "scene.hpp"



Renderer::Renderer(Options const& options) :
	options(options),
	framebuffer(options.res)
{
	//Load the scene
	if        (options.scene_name=="cornell"     ) {
		scene = Scene::get_new_cornell     ();
		#ifndef EXPLICIT_LIGHT_SAMPLING
			fprintf(stderr,"Warning: Cornell converges much faster with explicit light sampling!  (See \"stdafx.hpp\" to enable.)\n");
		#endif
	} else if (options.scene_name=="cornell-srgb") {
		scene = Scene::get_new_cornell_srgb();
		#ifndef EXPLICIT_LIGHT_SAMPLING
			fprintf(stderr,"Warning: Cornell converges much faster with explicit light sampling!  (See \"stdafx.hpp\" to enable.)\n");
		#endif
	} else if (options.scene_name=="plane-srgb"  ) {
		scene = Scene::get_new_plane_srgb  ();
		#ifdef EXPLICIT_LIGHT_SAMPLING
			fprintf(stderr,"Warning: Plane converges much faster without explicit light sampling!  (See \"stdafx.hpp\" to disable.)\n");
		#endif
	} else {
		fprintf(stderr,
			"Unrecognized scene \"%s\"!  (Supported scenes: \"cornell\", \"cornell-srgb\", \"plane-srgb\")\n",
			options.scene_name.c_str()
		);
		throw -3;
	}

	//Allocate space for threads
	#if 0
		fprintf(stderr,"Warning: only using one thread!\n");
		_threads.resize(1);
	#else
		_threads.resize(std::thread::hardware_concurrency());
	#endif
}
Renderer::~Renderer() {
	//Cleanup scene
	delete scene;
}

void Renderer::_print_progress() const {
	//Prints `secs` as a count of days, hours, minutes, and seconds.
	auto pretty_print_time = [](double secs) -> void {
		double days  = std::floor(secs/86400.0);
		secs -= 86400.0 * days;
		double hours = std::floor(secs/ 3600.0);
		secs -=  3600.0 * hours;
		double mins  = std::floor(secs/   60.0);
		secs -=    60.0 * mins;

		if (days>0.0) printf("%d days + ",static_cast<int>(days));
		printf(
			"%02d:%02d:%06.3f",
			static_cast<int>(hours),
			static_cast<int>(mins ),
			secs
		);
	};

	std::chrono::steady_clock::time_point time_now = std::chrono::steady_clock::now();

	double time_since_start = static_cast<double>(
		std::chrono::duration_cast<std::chrono::nanoseconds>(time_now-_time_start).count()
	) * 1.0e-9;

	//Fraction of the tiles that have been rendered (or are being rendered).  So, roughly the
	//	overall fraction of the render that is completed.
	double part = static_cast<double>(_num_tiles_start-_tiles.size()) / static_cast<double>(_num_tiles_start);

	if (part<1.0) {
		if (part>0.0) {
			//Middle of render.  Print fraction and expected time based on a simple extrapolation.
			double expected_time_total = time_since_start / part;
			double expected_time_remaining = expected_time_total - time_since_start;
			printf("\rRender %.3f%% (ETA ",part*100.0);
			pretty_print_time(expected_time_remaining);
			printf(")           ");
			fflush(stdout);
		} else {
			//Start of render.
			printf("\rRender started                               ");
		}
	} else {
		//End of render.  Print elapsed time.
		printf("\rRender completed in ");
		pretty_print_time(time_since_start);
		printf("             \n");
	}
}

#ifdef RENDER_MODE_SPECTRAL
CIEXYZ_A_32F Renderer::_render_sample(Math::RNG& rng, size_t i,size_t j)
#else
lRGB_A_F32   Renderer::_render_sample(Math::RNG& rng, size_t i,size_t j)
#endif
{
	//Render sample within pixel (`i`,`j`).

	//	Location within the framebuffer
	//		Compute with `double`-precision, which helps for large framebuffers.
	glm::dvec2 subpixel(rand_1d(rng),rand_1d(rng));
	glm::dvec2 framebuffer_st(
		(static_cast<double>(i)+subpixel.x) / static_cast<double>(framebuffer.res[0]),
		(static_cast<double>(j)+subpixel.y) / static_cast<double>(framebuffer.res[1])
	);
	//	Normalized device coordinates (to borrow OpenGL terminology)
	glm::dvec2 framebuffer_ndc = framebuffer_st*2.0 - glm::dvec2(1.0);

	//	Camera ray through pixel.  This is a pinhole camera model (bad) with the framebuffer
	//		semantically in-front of the center of projection (bogus).  Nevertheless, it is just-
	//		about the simplest-possible camera model.
	//		Note: this, and especially the normalize, must be computed in `double`-precision.  GLM
	//			decides to use inverse square root to normalize, which means lots of precision is
	//			lost, leading to stair-step artifacts.
	Dir camera_ray_dir;
	{
		glm::dvec4 point = scene->camera.matr_PV_inv * glm::dvec4( framebuffer_ndc, 0.0, 1.0 );
		point /= point.w;
		camera_ray_dir = Dir(glm::normalize( glm::dvec3(point) - glm::dvec3(scene->camera.pos) ));
	}

	#ifdef RENDER_MODE_SPECTRAL
	//	Hero wavelength sampling.
	//		First, the spectrum is divided into some number of regions.  Then, the hero wavelength
	//			is selected randomly from the first region.
	nm lambda_0 = LAMBDA_MIN + rand_1f(rng)*LAMBDA_STEP;
	//		Subsequent wavelengths are defined implicitly as multiples of `LAMBDA_STEP` above
	//			`lambda_0`.  The vector of these wavelengths are the wavelengths that the light
	//			transport is computed along.
	#endif

	//	Main radiance-gathering function used for recursive path tracing
	bool hit_anything = false;
	#ifdef RENDER_MODE_SPECTRAL
	std::function<SpectralRadiance::HeroSample(Ray const&,bool,unsigned,PrimBase const*)> L = [&](
		Ray const& ray, bool last_was_delta, unsigned depth, PrimBase const* ignore
	) -> SpectralRadiance::HeroSample
	#else
	std::function<RGB_Radiance(Ray const&,bool,unsigned,PrimBase const*)> L = [&](
		Ray const& ray, bool last_was_delta, unsigned depth, PrimBase const* ignore
	) -> RGB_Radiance
	#endif
	{
		#ifdef RENDER_MODE_SPECTRAL
		SpectralRadiance::HeroSample radiance(0);
		#else
		RGB_Radiance                 radiance(0);
		#endif

		HitRecord hitrec;
		if (scene->intersect( ray,&hitrec, ignore )) {
			hit_anything = true;

			//Emission
			#ifdef EXPLICIT_LIGHT_SAMPLING
			//Only add if could not have been sampled on previous)
			if (last_was_delta&&(!options.indirect_only||depth>0u)) {
			#endif
				auto emitted_radiance = hitrec.prim->material->evaluate_emission( hitrec.st, SPECTRAL_ONLY(lambda_0 COMMA) -ray.dir );
				radiance += emitted_radiance;
			#ifdef EXPLICIT_LIGHT_SAMPLING
			}
			#endif

			//If more rays are allowed . . .
			if (depth+1u<MAX_DEPTH) {
				//Hit position of ray
				Pos hit_pos = ray.at(hitrec.dist);

				#ifdef EXPLICIT_LIGHT_SAMPLING
				//Direct lighting
				if (!options.indirect_only||depth>0u) {
					//Get random ray toward random light
					Dir shad_ray_dir;
					PrimBase const* light;
					float shad_pdf;
					scene->get_rand_toward_light( rng, hit_pos, &shad_ray_dir,&light,&shad_pdf );

					float n_dot_l = glm::dot(shad_ray_dir,hitrec.normal);
					if (n_dot_l>0.0f) {
						//Cast the shadow ray
						Ray ray_shad = { hit_pos, shad_ray_dir };
						HitRecord hitrec_shad;
						scene->intersect(ray_shad,&hitrec_shad,hitrec.prim);

						if (hitrec_shad.prim == light) {
							//If the only thing we hit was the light we were shooting at, then we're
							//	not shadowed.  Add the radiance contribution.

							//	Emitted radiance
							auto emitted_radiance = hitrec_shad.prim->material->evaluate_emission(
								hitrec_shad.st, SPECTRAL_ONLY(lambda_0 COMMA) -shad_ray_dir
							);

							//	Evaluation of BSDF
							struct MaterialBase::BSDF_Evaluation evalbsdf = {
								hitrec.st, SPECTRAL_ONLY(lambda_0 COMMA)
								-ray.dir, hitrec.normal, shad_ray_dir,
								{}
							};
							hitrec.prim->material->evaluate_bsdf(&evalbsdf);

							//	Monte Carlo radiance estimate
							radiance += emitted_radiance * n_dot_l * evalbsdf.f_s / shad_pdf;
						}
					}
				}
				#endif

				//Indirect lighting
				//	Random sample from BSDF
				struct MaterialBase::BSDF_Interaction sampbsdf = {
					hitrec.st, SPECTRAL_ONLY(lambda_0 COMMA)
					-ray.dir, hitrec.normal, Dir(qNaN), qNaN, rng,
					{}
				};
				hitrec.prim->material->interact_bsdf(&sampbsdf);
				//	Recurse in sampled direction if BSDF is nonzero
				if (glm::dot(sampbsdf.f_s,sampbsdf.f_s)>0.0f) {
					//	And if the direction has nonzero contribution via the geometry term.
					float n_dot_l;
					if (std::isfinite(sampbsdf.pdf_w_i)) {
						n_dot_l = glm::dot(sampbsdf.w_i,hitrec.normal);
					} else {
						//		Dirac δ function.  BSDFs that are δ functions are posed having an
						//			inverse geometry term so that it cancels out in the rendering
						//			equation.  Instead of doing that, it's more numerically precise
						//			to just ignore the geometry term entirely.
						n_dot_l = 1.0f;
						sampbsdf.pdf_w_i = 1.0f;
					}
					if (n_dot_l>0.0f) {
						//Trace the ray recursively and use in Monte-Carlo estimate of rendering
						//	equation.
						Ray ray_next = { hit_pos, sampbsdf.w_i };
						radiance += L(ray_next,false,depth+1u,hitrec.prim) * n_dot_l * sampbsdf.f_s / sampbsdf.pdf_w_i;
					}
				}
			}
		}

		return radiance;
	};

	Ray ray_camera = { scene->camera.pos, camera_ray_dir };
	auto pixel_rad_est = L(ray_camera,true,0u,nullptr);

	//Value of Monte-Carlo estimator for the radiant flux incident on the pixel due to paths of any
	//	length.
	#ifdef FLAT_FIELD_CORRECTION
		auto pixel_flux_est = pixel_rad_est;
	#else
		auto pixel_flux_est = pixel_rad_est * glm::dot( camera_ray_dir, scene->camera.dir );
	#endif

	#ifdef RENDER_MODE_SPECTRAL
		//Convert each wavelength sample to CIE XYZ and average.
		CIEXYZ_32F ciexyz_avg = Color::specradflux_to_ciexyz( pixel_flux_est, lambda_0 );

		return CIEXYZ_A_32F( ciexyz_avg,     hit_anything?1.0f:0.0f );
	#else
		//Die inside.
		return lRGB_A_F32  ( pixel_flux_est, hit_anything?1.0f:0.0f );
	#endif
}
void       Renderer::_render_pixel (Math::RNG& rng, size_t i,size_t j) {
	#ifdef RENDER_MODE_SPECTRAL
		/*
		Accumulate samples into CIE XYZ instead of a spectrum (probably `SpectralRadiantFlux`).
		This way we avoid quantization artifacts and a large memory overhead per-pixel (in a more-
		sophisticated renderer, the pixel data would be stored for longer, e.g. to do nontrivial
		reconstruction filtering), for the (small) cost of having to do the conversion to CIE XYZ
		for each sample.

		Note accumulating must be into a 64-bit value is necessary to have adequate precision for
		high sample counts.  We also scale the values before accumulating them to keep the precision
		in a better range.
		*/

		CIEXYZ_A_64F avg( 0,0,0, 0 );
		for (size_t k=0;k<options.spp;++k) {
			avg += _render_sample(rng, i,j) * 0.001f;
		}
		avg *= 1000.0 / static_cast<double>(options.spp);

		framebuffer(i,j) = sRGB_A_F32( Color::ciexyz_to_srgb(CIEXYZ_32F(avg)), avg.a );
	#else
		lRGB_A_F64   avg( 0,0,0, 0 );
		for (size_t k=0;k<options.spp;++k) {
			avg += _render_sample(rng, i,j);
		}
		avg /= static_cast<double>(options.spp);

		framebuffer(i,j) = sRGB_A_F32( Color::lrgb_to_srgb  (lRGB_F32  (avg)), avg.a );
	#endif
}
void Renderer::_render_threadwork() {
	//Add ourself to the count of rendering threads
	uint32_t thread_index = _num_rendering++;

	/*
	Random number generator for each thread.  Note that this must be per-thread data; making it
	threadsafe and shared would be too slow, and making it simply shared (which is, unfortunately,
	what many simplistic implementations do with e.g. `rand()`) risks producing bogus results due to
	race conditions.

	There are several ways to make thread local variables (such as e.g. C++ `thread_local`), but
	just making a local variable in the thread function is probably the clearest, if not the
	cleanest.
	*/
	Math::RNG rng;
	/*
	Seed the RNG with some kind of data.  Since many RNGs will produce similar starting sequences
	given similar seeds, and it is desirable for different threads to have different sequences, it
	is desirable for seeds to vary substantially between threads.

	A simple way to do this is to scramble the thread's index with a hash function, though on some
	platforms `std::hash` is the identity so we have to implement this ourselves to ensure it
	actually works.  Passing zero for the seed is also not allowed, so we choose `1` if that's the
	case.  Note that that potential increment comes after the hashing, so it is not particularly
	likely to result in two threads receiving the same seed.
	*/
	size_t seed = get_hashed(thread_index);
	if (seed==0) ++seed;
	rng.seed(static_cast<Math::RNG::result_type>(seed));

	//Main render thread loop
	while (_render_continue) {
		//Atomically pull the next tile of un-rendered pixels off the list of un-rendered tiles.  If
		//	there are none, terminate the loop.  Also print the progress (inside the mutex so that
		//	it's threadsafe) every 10ms.

		Framebuffer::Tile tile;

		std::chrono::steady_clock::time_point time_now = std::chrono::steady_clock::now();

		_tiles_mutex.lock();

		if (!_tiles.empty()) {
			float time_since_last_print = static_cast<float>(
				std::chrono::duration_cast<std::chrono::nanoseconds>(time_now-_time_last_print).count()
			) * 1.0e-9f;
			if (time_since_last_print>0.01f) {
				_print_progress();
				_time_last_print = time_now;
			}

			tile = _tiles.back();
			_tiles.pop_back();

			_tiles_mutex.unlock();
		} else {
			_tiles_mutex.unlock();

			//	Minor optimization
			_render_continue = false;

			break;
		}

		//Render each pixel of the tile
		for (size_t j=tile.pos[1];j<tile.pos[1]+tile.res[1];++j) {
			for (size_t i=tile.pos[0];i<tile.pos[0]+tile.res[0];++i) {
				_render_pixel(rng, i,j);
			}
		}
	}

	//Remove ourself from the count of rendering threads
	assert(_num_rendering>0u);
	--_num_rendering;

	//If we're the last thread to finish, no threads can be touching the image anymore.  We are
	//	responsible for removing any un-rendered tiles (such as if the render was aborted) and
	//	saving the image to disk.
	if (_num_rendering==0u) {
		_tiles.clear();

		_print_progress();

		framebuffer.save(options.output_path);
	}
}
void Renderer::render_start() {
	//Create the list of tiles of un-rendered pixels
	assert(_tiles.empty());
	for (size_t j=0;j<options.res[1];j+=TILE_SIZE) {
		for (size_t i=0;i<options.res[0];i+=TILE_SIZE) {
			_tiles.push_back({
				{ i, j },
				{ std::min(options.res[0]-i,TILE_SIZE), std::min(options.res[1]-j,TILE_SIZE) }
			});
		}
	}
	//	Rearrange so that the lower tiles are at the end of the list (and thereby are pulled off and
	//		rendered first).
	std::reverse(_tiles.begin(),_tiles.end());

	//Starting information for timing
	_num_tiles_start = _tiles.size();
	_time_start      = std::chrono::steady_clock::now();
	_time_last_print = _time_start - std::chrono::seconds(1);

	//Create render threads (which also starts them working)
	_num_rendering = 0u;
	_render_continue = true;
	for (std::thread*& thread : _threads) {
		thread = new std::thread( &Renderer::_render_threadwork, this );
	}
}
void Renderer::render_wait () {
	//Wait for each render thread to terminate and clean up
	for (std::thread* thread : _threads) {
		thread->join();
		delete thread;
	}
	assert(_num_rendering==0u);
}
