#include "renderer.hpp"

#include "color.hpp"
#include "math-helpers.hpp"
#include "scene.hpp"


Renderer::Renderer(Options const& options) :
	options(options),
	framebuffer(options.res)
{
	if (options.scene_name=="cornell") {
		scene = Scene::get_new_cornell();
	} else {
		fprintf(stderr,"Unrecognized scene \"%s\"!  (Supported scenes: \"cornell\")\n",options.scene_name.c_str());
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

	//_threads.resize(1);
	_threads.resize(std::thread::hardware_concurrency());

	_render_continue = true;
}
Renderer::~Renderer() {
	delete scene;
}

CIEXYZ Renderer::_render_sample(Math::RNG& rng, size_t i,size_t j) {
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

	//Naïve path tracing

	//	Hero wavelength sampling.
	//		First, the spectrum is divided into some number of regions.  Then, the hero wavelength
	//			is selected randomly from the first region.
	nm lambda_0 = LAMBDA_MIN + rand_1f(rng)*LAMBDA_STEP;
	//		Subsequent wavelengths are defined implicitly as multiples of `LAMBDA_STEP` above
	//			`lambda_0`.  The vector of these wavelengths are the wavelengths that the light
	//			transport is computed along.

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

	while (depth<MAX_DEPTH) {
		HitRecord hitrec;
		if (!scene->intersect(ray,&hitrec)) break;

		Spectrum::Sample emitted_radiance = hitrec.prim->material->sample_emission(lambda_0);
		Spectrum::Sample brdf             = hitrec.prim->material->sample_brdf    (lambda_0);

		//Value of Monte-Carlo estimator for the radiance incident on the pixel due to paths of
		//	length `depth`.
		Spectrum::Sample pixel_rad_est_currdepth = emitted_radiance * throughput / pdf;

		pixel_rad_est_infdepth += pixel_rad_est_currdepth;

		throughput *= brdf;

		ray.orig += ray.dir * hitrec.dist;

		float pdf_dir;
		ray.dir = rand_coshemi(rng,&pdf_dir);
		ray.dir = Math::get_rotated_to(ray.dir,hitrec.normal);
		pdf *= pdf_dir;

		break;
	}

	//Value of Monte-Carlo estimator for the radiant flux incident on the pixel due to paths of any
	//	length.
	Spectrum::Sample pixel_flux_est_infdepth = pixel_rad_est_infdepth * glm::dot( camera_ray_dir, scene->camera.dir );

	//Convert each wavelength sample to CIE XYZ and average.
	CIEXYZ ciexyz_avg = Color::spectralsample_to_ciexyz( lambda_0, pixel_flux_est_infdepth );

	return ciexyz_avg;
}
void   Renderer::_render_pixel (Math::RNG& rng, size_t i,size_t j) {
	//Accumulate into CIE XYZ instead of a `Spectrum`.  This way we avoid quantization
	//	artifacts and a large memory overhead per-pixel (in a more-sophisticated renderer, the
	//	pixel data would be stored for longer, e.g. to do nontrivial reconstruction filtering),
	//	for the (small) cost of having to do the conversion to XYZ for each sample.

	CIEXYZ avg(0,0,0);

	for (size_t k=0;k<options.spp;++k) {
		avg += _render_sample(rng, i,j);
	}
	avg /= static_cast<float>(options.spp);

	framebuffer(i,j) = sRGBA(Color::xyz_to_srgb(avg),1.0f);
}
void Renderer::_render_threadwork() {
	Math::RNG rng;

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
}
void Renderer::render_start() {
	for (std::thread*& thread : _threads) {
		thread = new std::thread( &Renderer::_render_threadwork, this );
	}
}
void Renderer::render_wait () {
	for (std::thread* thread : _threads) {
		thread->join();
		delete thread;
	}
}
