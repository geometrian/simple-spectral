#include "renderer.hpp"

#include "color.hpp"
#include "math-helpers.hpp"
#include "scene.hpp"


Renderer::Renderer(Options const& options) :
	options(options),
	framebuffer(options.res)
{
	if        (options.scene_name=="cornell"     ) {
		scene = Scene::get_new_cornell     ();
	} else if (options.scene_name=="cornell-srgb") {
		scene = Scene::get_new_cornell_srgb();
	} else if (options.scene_name=="d65sphere"   ) {
		assert(false);
	} else if (options.scene_name=="srgb"        ) {
		scene = Scene::get_new_srgb        ();
	} else {
		fprintf(stderr,"Unrecognized scene \"%s\"!  (Supported scenes: \"cornell\", \"cornell-srgb\", \"d65sphere\", \"srgb\")\n",options.scene_name.c_str());
		throw -3;
	}

	for (size_t j=0;j<options.res[1];j+=TILE_SIZE) {
		for (size_t i=0;i<options.res[0];i+=TILE_SIZE) {
			_tiles.push_back({
				{ i, j },
				{ std::min(options.res[0]-i,TILE_SIZE), std::min(options.res[1]-j,TILE_SIZE) }
			});
		}
	}
	std::reverse(_tiles.begin(),_tiles.end());

	#if 0
		_threads.resize(1);
	#else
		_threads.resize(std::thread::hardware_concurrency());
	#endif

	//float X = Spectrum::integrate(Color::data->D65, Color::data->std_obs_xbar.spec);
	//float Y = Spectrum::integrate(Color::data->D65, Color::data->std_obs_ybar.spec);
	//float Z = Spectrum::integrate(Color::data->D65, Color::data->std_obs_zbar.spec);

	_render_continue = true;
}
Renderer::~Renderer() {
	delete scene;
}

CIEXYZ Renderer::_render_sample(Math::RNG& rng, size_t i,size_t j) {
	//i=256; j=430;

	glm::vec2 framebuffer_st(
		(static_cast<float>(i)+rand_1f(rng)) / static_cast<float>(framebuffer.res[0]),
		(static_cast<float>(j)+rand_1f(rng)) / static_cast<float>(framebuffer.res[1])
	);
	glm::vec2 framebuffer_ndc = framebuffer_st*2.0f - glm::vec2(1.0f);

	Dir camera_ray_dir;
	{
		glm::vec4 point = scene->camera.matr_PV_inv * glm::vec4( framebuffer_ndc, 0.0f, 1.0f );
		point /= point.w;
		camera_ray_dir = glm::normalize( Pos(point) - scene->camera.pos );
	}

	//Path tracing with explicit light sampling.
	//#define EXPLICIT_LIGHT_SAMPLING

	//	Hero wavelength sampling.
	//		First, the spectrum is divided into some number of regions.  Then, the hero wavelength
	//			is selected randomly from the first region.
	nm lambda_0 = LAMBDA_MIN + rand_1f(rng)*LAMBDA_STEP;
	//		Subsequent wavelengths are defined implicitly as multiples of `LAMBDA_STEP` above
	//			`lambda_0`.  The vector of these wavelengths are the wavelengths that the light
	//			transport is computed along.

	std::function<Spectrum::Sample(Ray const&,bool,unsigned)> L = [&](
		Ray const& ray, bool last_was_delta, unsigned depth
	) -> Spectrum::Sample
	{
		Spectrum::Sample radiance(0);

		HitRecord hitrec;
		if (scene->intersect(ray,&hitrec)) {
			//Emission
			#ifdef EXPLICIT_LIGHT_SAMPLING
			//Only add if could not have been sampled on previous)
			if (last_was_delta) {
			#endif
				Spectrum::Sample emitted_radiance = hitrec.prim->material->sample_emission(hitrec.st,lambda_0);

				radiance += emitted_radiance;
			#ifdef EXPLICIT_LIGHT_SAMPLING
			}
			#endif

			if (depth<MAX_DEPTH) {
				Pos hit_pos = ray.at(hitrec.dist);

				Spectrum::Sample brdf = hitrec.prim->material->sample_brdf(hitrec.st,lambda_0);

				#ifdef EXPLICIT_LIGHT_SAMPLING
				//Direct lighting
				{
					Dir shad_ray_dir;
					PrimBase const* light;
					float shad_pdf;

					scene->get_rand_toward_light(rng,hit_pos,&shad_ray_dir,&light,&shad_pdf);
					float n_dot_l = glm::dot(shad_ray_dir,hitrec.normal);
					if (n_dot_l>0.0f) {
						Ray ray_shad = { hit_pos, shad_ray_dir };
						HitRecord hitrec_shad;
						scene->intersect(ray_shad,&hitrec_shad);
						if (hitrec_shad.prim == light) {
							Spectrum::Sample emitted_radiance = light->material->sample_emission(hitrec_shad.st,lambda_0);

							radiance += emitted_radiance * n_dot_l * brdf / shad_pdf;
						}
					}
				}
				#endif

				//Indirect lighting
				if (glm::dot(brdf,brdf)>0.0f) {
					float pdf_dir;
					Dir ray_next_dir = Math::rand_coshemi(rng,&pdf_dir);
					ray_next_dir = Math::get_rotated_to(ray_next_dir,hitrec.normal);

					Ray ray_next = { hit_pos, ray_next_dir };

					float n_dot_l = glm::dot(ray_next_dir,hitrec.normal);
					if (n_dot_l>0.0f) {
						radiance += L(ray_next,false,depth+1u) * n_dot_l * brdf / pdf_dir;
					}
				}
			}
		}

		return radiance;
	};

	Ray ray_camera = { scene->camera.pos, camera_ray_dir };
	Spectrum::Sample pixel_rad_est_infdepth = L(ray_camera,true,0u);

	#if 0
	//Value of Monte-Carlo estimator for the radiance incident on the pixel due to paths of any
	//	length.
	Spectrum::Sample pixel_rad_est_infdepth(0);

	unsigned depth = 0u;

	//	Note: pinhole camera model (bad) with the framebuffer in front of the center of projection
	//		(utterly bogus).  This model is simple, wrong, and so commonplace it amounts to a
	//		standard.
	Spectrum::Sample throughput(1);
	Ray ray = { scene->camera.pos, camera_ray_dir };
	float pdf = 1.0f;
	bool last_was_delta = true;

	while (depth<MAX_DEPTH) {
		HitRecord hitrec;
		if (!scene->intersect(ray,&hitrec)) break;

		//Emission (only add if could not have been sampled on previous)
		if (false) {//(last_was_delta) {
			Spectrum::Sample emitted_radiance = hitrec.prim->material->sample_emission(lambda_0);

			Spectrum::Sample pixel_rad_est_currdepth = emitted_radiance * throughput / pdf;
			pixel_rad_est_infdepth += pixel_rad_est_currdepth;
		}

		ray.orig += ray.dir * hitrec.dist;

		Spectrum::Sample brdf = hitrec.prim->material->sample_brdf(lambda_0);
		throughput *= brdf;

		//Direct lighting
		if (depth==6) {
			Dir shad_ray_dir;
			PrimBase const* light;
			float shad_pdf;

			scene->get_rand_toward_light(rng,ray.orig,&shad_ray_dir,&light,&shad_pdf);

			Ray ray_shad = { ray.orig, shad_ray_dir };
			HitRecord hitrec_shad;
			scene->intersect(ray_shad,&hitrec_shad);
			if (hitrec_shad.prim == light) {
				Spectrum::Sample emitted_radiance = light->material->sample_emission(lambda_0);

				//Value of Monte-Carlo estimator for the radiance incident on the pixel due to paths
				//	of length `depth`+1.
				Spectrum::Sample pixel_rad_est_currdepth = emitted_radiance * throughput / (pdf*shad_pdf);
				pixel_rad_est_infdepth += pixel_rad_est_currdepth;
			}
		}

		//Indirect lighting (iterate)
		{
			float pdf_dir;
			ray.dir = Math::rand_coshemi(rng,&pdf_dir);
			ray.dir = Math::get_rotated_to(ray.dir,hitrec.normal);
			pdf *= pdf_dir;
			last_was_delta = false;
			++depth;
		}

		//break;
	}
	#endif

	//Value of Monte-Carlo estimator for the radiant flux incident on the pixel due to paths of any
	//	length.
	#ifdef FLAT_FIELD_CORRECTION
		Spectrum::Sample pixel_flux_est_infdepth = pixel_rad_est_infdepth;
	#else
		Spectrum::Sample pixel_flux_est_infdepth = pixel_rad_est_infdepth * glm::dot( camera_ray_dir, scene->camera.dir );
	#endif

	//Convert each wavelength sample to CIE XYZ and average.
	CIEXYZ ciexyz_avg = Color::spectralsample_to_ciexyz( lambda_0, pixel_flux_est_infdepth );

	return ciexyz_avg;
}
void   Renderer::_render_pixel (Math::RNG& rng, size_t i,size_t j) {
	//Accumulate into CIE XYZ instead of a `Spectrum`.  This way we avoid quantization artifacts and
	//	a large memory overhead per-pixel (in a more-sophisticated renderer, the pixel data would be
	//	stored for longer, e.g. to do nontrivial reconstruction filtering), for the (small) cost of
	//	having to do the conversion to XYZ for each sample.

	CIEXYZ avg(0,0,0);

	for (size_t k=0;k<options.spp;++k) {
		avg += _render_sample(rng, i,j);
	}
	avg /= static_cast<float>(options.spp);

	framebuffer(i,j) = sRGBA(Color::xyz_to_srgb(avg),1.0f);
}
void Renderer::_render_threadwork() {
	uint32_t thread_index = _num_rendering++;

	Math::RNG rng;
	rng.seed(static_cast<Math::RNG::result_type>( get_hashed(thread_index) ));

	while (_render_continue) {
		Framebuffer::Tile tile;

		_tiles_mutex.lock();

		if (!_tiles.empty()) {
			tile = _tiles.back();
			_tiles.pop_back();

			_tiles_mutex.unlock();
		} else {
			_tiles_mutex.unlock();

			_render_continue = false;

			break;
		}

		for (size_t j=tile.pos[1];j<tile.pos[1]+tile.res[1];++j) {
			for (size_t i=tile.pos[0];i<tile.pos[0]+tile.res[0];++i) {
				_render_pixel(rng, i,j);
			}
		}
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	assert(_num_rendering>0u);
	--_num_rendering;

	if (_num_rendering==0u) {
		framebuffer.save(options.output_path);
	}
}
void Renderer::render_start() {
	_num_rendering = 0u;
	for (std::thread*& thread : _threads) {
		thread = new std::thread( &Renderer::_render_threadwork, this );
	}
}
void Renderer::render_wait () {
	for (std::thread* thread : _threads) {
		thread->join();
		delete thread;
	}
	assert(_num_rendering==0u);
}
