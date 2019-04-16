#include "scene.hpp"

#include "util/color.hpp"

#include "geometry.hpp"
#include "material.hpp"



Scene::~Scene() {
	for (auto iter : materials) delete iter.second;

	for (PrimBase const* iter : primitives) delete iter;
}

void Scene::_init() {
	//Compute camera matrices.
	camera.matr_P = glm::perspectiveFov(
		glm::radians(camera.vfov_deg),
		static_cast<float>(camera.res[0]), static_cast<float>(camera.res[1]),
		camera.near, camera.far
	);
	camera.matr_V = glm::lookAt( camera.pos, camera.pos+camera.dir, camera.up );
	camera.matr_PV_inv = glm::inverse( camera.matr_P * camera.matr_V );

	//Make a list of all the lights so that we can sample them later.
	for (PrimBase* prim : primitives) {
		if (prim->is_light) lights.emplace_back(prim);
	}
	assert(!lights.empty());
}
Scene* Scene::get_new_cornell     () {
	//http://www.graphics.cornell.edu/online/box/data.html
	Scene* result = new Scene;

	{
		result->camera.pos = Pos(278,273,-800);
		result->camera.dir = glm::normalize(Dir(0,0,1));
		result->camera.up  = Dir(0,1,0);

		result->camera.res[0] = 512;
		result->camera.res[1] = 512;
		result->camera.near=0.1f; result->camera.far=1.0f;
		result->camera.vfov_deg = 39.0f;
		//focal = 0.035f;
		//size
	}

	{
		#ifdef RENDER_MODE_SPECTRAL
			std::vector<std::vector<float>> data = load_spectral_data("data/scenes/cornell/white-green-red.csv");
			if (data.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			MaterialLambertian* white_back      = new MaterialLambertian;
			*white_back->albedo.constant = SpectralReflectance( data[0], 400,700 );
			//*white_back->albedo.constant = SpectralReflectance( 1.0f );

			MaterialLambertian* white_blocks    = new MaterialLambertian(*white_back);

			MaterialLambertian* white_floorceil = new MaterialLambertian(*white_back);

			MaterialLambertian* green           = new MaterialLambertian;
			*green->albedo.constant      = SpectralReflectance( data[1], 400,700 );

			MaterialLambertian* red             = new MaterialLambertian;
			*red->albedo.constant        = SpectralReflectance( data[2], 400,700 );
		#else
			MaterialLambertian* white_back      = new MaterialLambertian;
			white_back->albedo.constant  = RGB_Reflectance(1,1,1);

			MaterialLambertian* white_blocks    = new MaterialLambertian(*white_back);

			MaterialLambertian* white_floorceil = new MaterialLambertian(*white_back);

			MaterialLambertian* green           = new MaterialLambertian;
			green->     albedo.constant  = RGB_Reflectance(0,1,0);

			MaterialLambertian* red             = new MaterialLambertian;
			red->       albedo.constant  = RGB_Reflectance(1,0,0);
		#endif

		result->materials["white-back"     ] = white_back;
		result->materials["white-blocks"   ] = white_blocks;
		result->materials["white-floorceil"] = white_floorceil;
		result->materials["green"          ] = green;
		result->materials["red"            ] = red;
	}

	{
		MaterialLambertian* light = new MaterialLambertian;
		#ifdef RENDER_MODE_SPECTRAL
			std::vector<std::vector<float>> data = load_spectral_data("data/scenes/cornell/light.csv");
			if (data.size()==1); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			light->emission         = SpectralRadiance( data[0], 400,700 ) * 200.0f;
			//light->emission         = Color::data->D65_rad * 0.5f;
			//light->emission         = Color::data->D65_rad * 5.0f;
			*light->albedo.constant = SpectralReflectance( 0.78f );
		#else
			light->emission         = RGB_Radiance(1,1,1) * 200.0f;
			light->albedo.constant  = RGB_Reflectance( 0.78f );
		#endif

		result->materials["light"] = light;
	}

	{
		//Floor
		result->primitives.emplace_back(new PrimQuad(result->materials["white-floorceil"],
			{ Pos( 552.8f, 0.0f,   0.0f ), ST(1,0) },
			{ Pos(   0.0f, 0.0f,   0.0f ), ST(0,0) },
			{ Pos(   0.0f, 0.0f, 559.2f ), ST(0,1) },
			{ Pos( 549.6f, 0.0f, 559.2f ), ST(1,1) }
		));

		#if 0
			//Shift the light downward a bit

			//Light (note moved down 0.1 so not on ceiling)
			result->primitives.emplace_back(new PrimQuad(result->materials["light"],
				{ Pos( 343.0f, 548.7f, 227.0f ), ST(1,0) },
				{ Pos( 343.0f, 548.7f, 332.0f ), ST(1,1) },
				{ Pos( 213.0f, 548.7f, 332.0f ), ST(0,1) },
				{ Pos( 213.0f, 548.7f, 227.0f ), ST(0,0) }
			));

			//Ceiling
			result->primitives.emplace_back(new PrimQuad(result->materials["white-floorceil"],
				{ Pos( 556.0f, 548.8f,   0.0f ), ST(1,0) },
				{ Pos( 556.0f, 548.8f, 559.2f ), ST(1,1) },
				{ Pos(   0.0f, 548.8f, 559.2f ), ST(0,1) },
				{ Pos(   0.0f, 548.8f,   0.0f ), ST(0,0) }
			));
		#else
			/*
			Actually cut a hole in the ceiling for the light:

				A-------B    Left <-----+
				| E---F |               |
				| |   | |               |
				| G---H |               v
				C-------D             Front
			*/

			Pos A = Pos(   0.0f, 548.8f, 559.2f );
			Pos B = Pos( 556.0f, 548.8f, 559.2f );
			Pos C = Pos(   0.0f, 548.8f,   0.0f );
			Pos D = Pos( 556.0f, 548.8f,   0.0f );
			Pos E = Pos( 213.0f, 548.8f, 332.0f );
			Pos F = Pos( 343.0f, 548.8f, 332.0f );
			Pos G = Pos( 213.0f, 548.8f, 227.0f );
			Pos H = Pos( 343.0f, 548.8f, 227.0f );

			/*
			This reduces variance for explicit light sampling significantly because rays can't get
			onto the ceiling behind the light and sample the large solid angle (near-hemisphere)
			toward it.
			*/

			//Light (H,F,E,G)
			result->primitives.emplace_back(new PrimQuad(result->materials["light"],
				{ H, ST(1,0) },
				{ F, ST(1,1) },
				{ E, ST(0,1) },
				{ G, ST(0,0) }
			));

			//Ceiling
			result->primitives.emplace_back(new PrimQuad(result->materials["white-floorceil"],
				{ D, ST(0,0) },
				{ B, ST(0,0) },
				{ F, ST(0,0) },
				{ H, ST(0,0) }
			));
			result->primitives.emplace_back(new PrimQuad(result->materials["white-floorceil"],
				{ B, ST(0,0) },
				{ A, ST(0,0) },
				{ E, ST(0,0) },
				{ F, ST(0,0) }
			));
			result->primitives.emplace_back(new PrimQuad(result->materials["white-floorceil"],
				{ A, ST(0,0) },
				{ C, ST(0,0) },
				{ G, ST(0,0) },
				{ E, ST(0,0) }
			));
			result->primitives.emplace_back(new PrimQuad(result->materials["white-floorceil"],
				{ C, ST(0,0) },
				{ D, ST(0,0) },
				{ H, ST(0,0) },
				{ G, ST(0,0) }
			));
		#endif

		//Back wall
		result->primitives.emplace_back(new PrimQuad(result->materials["white-back"],
			{ Pos( 549.6f,   0.0f, 559.2f ), ST(0,0) },
			{ Pos(   0.0f,   0.0f, 559.2f ), ST(1,0) },
			{ Pos(   0.0f, 548.8f, 559.2f ), ST(1,1) },
			{ Pos( 556.0f, 548.8f, 559.2f ), ST(0,1) }
		));

		//Right wall
		result->primitives.emplace_back(new PrimQuad(result->materials["green"],
			{ Pos( 0.0f,   0.0f, 559.2f ), ST(1,0) },
			{ Pos( 0.0f,   0.0f,   0.0f ), ST(0,0) },
			{ Pos( 0.0f, 548.8f,   0.0f ), ST(0,1) },
			{ Pos( 0.0f, 548.8f, 559.2f ), ST(1,1) }
		));

		//Left wall
		result->primitives.emplace_back(new PrimQuad(result->materials["red"  ],
			{ Pos( 552.8f,   0.0f,   0.0f ), ST(0,0) },
			{ Pos( 549.6f,   0.0f, 559.2f ), ST(1,0) },
			{ Pos( 556.0f, 548.8f, 559.2f ), ST(1,1) },
			{ Pos( 556.0f, 548.8f,   0.0f ), ST(0,1) }
		));

		//Short block
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 130.0f, 165.0f,  65.0f ), ST(0,0) },
			{ Pos(  82.0f, 165.0f, 225.0f ), ST(0,0) },
			{ Pos( 240.0f, 165.0f, 272.0f ), ST(0,0) },
			{ Pos( 290.0f, 165.0f, 114.0f ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 290.0f,   0.0f, 114.0f ), ST(0,0) },
			{ Pos( 290.0f, 165.0f, 114.0f ), ST(0,0) },
			{ Pos( 240.0f, 165.0f, 272.0f ), ST(0,0) },
			{ Pos( 240.0f,   0.0f, 272.0f ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 130.0f,   0.0f,  65.0f ), ST(0,0) },
			{ Pos( 130.0f, 165.0f,  65.0f ), ST(0,0) },
			{ Pos( 290.0f, 165.0f, 114.0f ), ST(0,0) },
			{ Pos( 290.0f,   0.0f, 114.0f ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos(  82.0f,   0.0f, 225.0f ), ST(0,0) },
			{ Pos(  82.0f, 165.0f, 225.0f ), ST(0,0) },
			{ Pos( 130.0f, 165.0f,  65.0f ), ST(0,0) },
			{ Pos( 130.0f,   0.0f,  65.0f ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 240.0f,   0.0f, 272.0f ), ST(0,0) },
			{ Pos( 240.0f, 165.0f, 272.0f ), ST(0,0) },
			{ Pos(  82.0f, 165.0f, 225.0f ), ST(0,0) },
			{ Pos(  82.0f,   0.0f, 225.0f ), ST(0,0) }
		));

		//Tall block
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 423.0f, 330.0f, 247.0f ), ST(0,0) },
			{ Pos( 265.0f, 330.0f, 296.0f ), ST(0,0) },
			{ Pos( 314.0f, 330.0f, 456.0f ), ST(0,0) },
			{ Pos( 472.0f, 330.0f, 406.0f ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 423.0f,   0.0f, 247.0f ), ST(0,0) },
			{ Pos( 423.0f, 330.0f, 247.0f ), ST(0,0) },
			{ Pos( 472.0f, 330.0f, 406.0f ), ST(0,0) },
			{ Pos( 472.0f,   0.0f, 406.0f ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 472.0f,   0.0f, 406.0f ), ST(0,0) },
			{ Pos( 472.0f, 330.0f, 406.0f ), ST(0,0) },
			{ Pos( 314.0f, 330.0f, 456.0f ), ST(0,0) },
			{ Pos( 314.0f,   0.0f, 456.0f ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 314.0f,   0.0f, 456.0f ), ST(0,0) },
			{ Pos( 314.0f, 330.0f, 456.0f ), ST(0,0) },
			{ Pos( 265.0f, 330.0f, 296.0f ), ST(0,0) },
			{ Pos( 265.0f,   0.0f, 296.0f ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["white-blocks"],
			{ Pos( 265.0f,   0.0f, 296.0f ), ST(0,0) },
			{ Pos( 265.0f, 330.0f, 296.0f ), ST(0,0) },
			{ Pos( 423.0f, 330.0f, 247.0f ), ST(0,0) },
			{ Pos( 423.0f,   0.0f, 247.0f ), ST(0,0) }
		));
	}

	result->_init();

	return result;
}
Scene* Scene::get_new_cornell_srgb() {
	Scene* result = Scene::get_new_cornell();

	MaterialBase* mtl_tex = new MaterialLambertian("data/scenes/crystal-lizard-512.png"); float lightsc=30.0f;
	//MaterialBase* mtl_tex = new MaterialLambertian("data/scenes/test-img.png"); float lightsc=20.0f;
	result->materials["srgb"] = mtl_tex;

	MaterialLambertian* mtl_white1 = new MaterialLambertian;
	#ifdef RENDER_MODE_SPECTRAL
		*mtl_white1->albedo.constant = SpectralReflectance( 1.0f );
	#else
		 mtl_white1->albedo.constant = RGB_Reflectance    ( 1.0f );
	#endif
	result->materials["white1"] = mtl_white1;

	for (PrimBase* prim : result->primitives) {
		if      (prim->material==result->materials["white-blocks"   ]) prim->material=mtl_white1;
		else if (prim->material==result->materials["white-floorceil"]) prim->material=mtl_white1;
		else if (prim->material==result->materials["red"            ]) prim->material=mtl_tex;
	}

	static_cast<MaterialLambertian*>(result->materials["light"])->emission =
		#ifdef RENDER_MODE_SPECTRAL
			Color::data->D65_rad * lightsc
		#else
			RGB_Radiance(1,1,1)  * lightsc;
		#endif
	;

	return result;
}
Scene* Scene::get_new_plane_srgb  () {
	Scene* result = new Scene;

	{
		result->camera.pos = Pos(0,0,5);
		//result->camera.pos = Pos(50,30,20);
		result->camera.dir = glm::normalize(Pos(0)-result->camera.pos);
		result->camera.up  = Dir(0,1,0);

		result->camera.res[0] = 512;
		result->camera.res[1] = 512;
		result->camera.near=0.1f; result->camera.far=1.0f;
		result->camera.vfov_deg = glm::degrees(2.0f*std::atan2( 1.0f, result->camera.pos.z ));
	}

	{
		MaterialLambertian* mtl_light = new MaterialLambertian;
		#ifdef RENDER_MODE_SPECTRAL
			*mtl_light->albedo.constant = SpectralReflectance(0.0f);
			 mtl_light->emission = Color::data->D65_rad;
		#else
			 mtl_light->albedo.constant = RGB_Reflectance    (0.0f);
			 mtl_light->emission = RGB_Radiance(1,1,1);
		#endif
		result->materials["light"] = mtl_light;

		//Either `MaterialMirror` or `MaterialLambertian` produces the same results, but the mirror
		//	converges much faster because the ray direction is not a random variable.
		#if 1 //Lizard texture
			MaterialSimpleAlbedoBase* mtl_tex = new MaterialMirror("data/scenes/crystal-lizard-4096.png");
		#else //A helpful 64⨯64 test image I made
			MaterialSimpleAlbedoBase* mtl_tex = new MaterialMirror("data/scenes/test-img.png");
		#endif
		result->materials["tex"] = mtl_tex;
	}

	{
		result->primitives.emplace_back(new PrimQuad(result->materials["tex"],
			{ Pos( -1, -1, 0 ), ST(0,0) },
			{ Pos(  1, -1, 0 ), ST(1,0) },
			{ Pos(  1,  1, 0 ), ST(1,1) },
			{ Pos( -1,  1, 0 ), ST(0,1) }
		));

		float size = 10.0f;
		result->primitives.emplace_back(new PrimQuad(result->materials["light"],
			{ Pos( -size, -size,  size ), ST(0,0) },
			{ Pos( -size, -size, -size ), ST(0,0) },
			{ Pos( -size,  size, -size ), ST(0,0) },
			{ Pos( -size,  size,  size ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["light"],
			{ Pos(  size, -size, -size ), ST(0,0) },
			{ Pos(  size, -size,  size ), ST(0,0) },
			{ Pos(  size,  size,  size ), ST(0,0) },
			{ Pos(  size,  size, -size ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["light"],
			{ Pos( -size, -size,  size ), ST(0,0) },
			{ Pos(  size, -size,  size ), ST(0,0) },
			{ Pos(  size, -size, -size ), ST(0,0) },
			{ Pos( -size, -size, -size ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["light"],
			{ Pos(  size,  size,  size ), ST(0,0) },
			{ Pos( -size,  size,  size ), ST(0,0) },
			{ Pos( -size,  size, -size ), ST(0,0) },
			{ Pos(  size,  size, -size ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["light"],
			{ Pos( -size, -size, -size ), ST(0,0) },
			{ Pos(  size, -size, -size ), ST(0,0) },
			{ Pos(  size,  size, -size ), ST(0,0) },
			{ Pos( -size,  size, -size ), ST(0,0) }
		));
		result->primitives.emplace_back(new PrimQuad(result->materials["light"],
			{ Pos(  size, -size,  size ), ST(0,0) },
			{ Pos( -size, -size,  size ), ST(0,0) },
			{ Pos( -size,  size,  size ), ST(0,0) },
			{ Pos(  size,  size,  size ), ST(0,0) }
		));
	}

	result->_init();

	return result;
}

void Scene::get_rand_toward_light(Math::RNG& rng, Pos const& from, Dir* dir,PrimBase const** light,float* pdf ) {
	*light = lights[ rand_choice(rng,lights.size()) ];

	#if 0 //Sample the bounding sphere of the light
		SphereBound bound = (*light)->get_bound();

		Dir vec_to_sph_cen = bound.center - from;

		*dir = Math::rand_toward_sphere( rng, vec_to_sph_cen,bound.radius, pdf );
	#else //Sample the primitive directly
		(*light)->get_rand_toward( rng, from, dir,pdf );
	#endif

	*pdf /= static_cast<float>(lights.size());
}

bool Scene::intersect(Ray const& ray, HitRecord* hitrec, PrimBase const* ignore/*=nullptr*/) const {
	hitrec->prim = nullptr;
	hitrec->dist = INF;

	bool hit = false;
	for (PrimBase const* prim : primitives) {
		if (prim!=ignore) {
			hit |= prim->intersect(ray, hitrec);
		}
	}

	return hit;
}
