#include "scene.hpp"

#include "util/lodepng/lodepng.h"
#include "util/color.hpp"



sRGB_ReflectanceTexture::sRGB_ReflectanceTexture(std::string const& path) {
	//Load data from file
	std::vector<unsigned char> out;
	unsigned w, h;
	lodepng::decode( out, w,h, path, LCT_RGB );
	if (!out.empty()); else {
		fprintf(stderr,"Could not load texture \"%s\"\n",path.c_str());
		throw -1;
	}

	//Set resolution
	res[0] = w;
	res[1] = h;

	//Allocate pixels
	_data = new sRGB_U8[res[1]*res[0]];

	//Copy loaded data into pixels
	memcpy(_data,out.data(),3*res[1]*res[0]);
}
sRGB_ReflectanceTexture::~sRGB_ReflectanceTexture() {
	//Clean up pixel data
	delete[] _data;
}

#ifdef RENDER_MODE_SPECTRAL
SpectralReflectance::HeroSample sRGB_ReflectanceTexture::sample( size_t i,size_t j, nm lambda_0 ) const
#else
RGB_Reflectance                 sRGB_ReflectanceTexture::sample( size_t i,size_t j              ) const
#endif
{
	//Load sRGB data and convert to floating-point.
	sRGB_U8 const& srgb_u8 = _data[j*res[0]+i];
	sRGB_F32 srgb = sRGB_F32(srgb_u8.r,srgb_u8.g,srgb_u8.b)*(1.0f/255.0f);

	//Undo the gamma transform to get ℓRGB.
	RGB_Reflectance lrgb = Color::srgb_to_lrgb(srgb);

	#ifdef RENDER_MODE_SPECTRAL
	//Sample the reflection spectrum corresponding to this ℓRGB triple.  See paper for details on
	//	what "corresponding" means.
	return Color::lrgb_to_specrefl(lrgb,lambda_0);
	#else
	return                         lrgb;
	#endif
}
#ifdef RENDER_MODE_SPECTRAL
SpectralReflectance::HeroSample sRGB_ReflectanceTexture::sample( ST const& st,      nm lambda_0 ) const
#else
RGB_Reflectance                 sRGB_ReflectanceTexture::sample( ST const& st                   ) const
#endif
{
	#if 1
		//Convert from ST space to UV space.
		UV uv = st * glm::vec2(res[0],res[1]);

		//Convert from UV space to index space
		glm::vec2 index = glm::vec2(
			         uv.x,
			res[1] - uv.y
		);

		//Convert to integer (clamped nearest-neighbor sample)
		int i = static_cast<int>(std::floor(index.x));
		int j = static_cast<int>(std::floor(index.y));
		i = glm::clamp( i, 0,static_cast<int>(res[0]-1) );
		j = glm::clamp( j, 0,static_cast<int>(res[1]-1) );

		//Sample the texture
		#ifdef RENDER_MODE_SPECTRAL
		return sample( static_cast<size_t>(i),static_cast<size_t>(j), lambda_0 );
		#else
		return sample( static_cast<size_t>(i),static_cast<size_t>(j)           );
		#endif
	#else
		//Return the ST coordinates as a spectral reflectance.
		return Color::lrgb_to_specrefl(lRGB_F32(st,0),lambda_0);
	#endif
}



bool PrimTri:: intersect(Ray const& ray, HitRecord* hitrec) const /*override*/ {
	//Robust ray-triangle intersection.  See:
	//	http://jcgt.org/published/0002/01/05/paper.pdf

	//Dimension where the ray direction is maximal
	Dir abs_dir = glm::abs(ray.dir);
	size_t kx, ky, kz;
	if (abs_dir.x>abs_dir.y) {
		if (abs_dir.x>abs_dir.z) {
			kz=0; kx=1; ky=2;
		} else {
			kz=2; kx=0; ky=1;
		}
	} else {
		if (abs_dir.y>abs_dir.z) {
			kz=1; kx=2; ky=0;
		} else {
			kz=2; kx=0; ky=1;
		}
	}
	if (ray.dir[kz]<0) std::swap(kx,ky); //Winding order

	//Calculate shear constants
	float Sx = ray.dir[kx] / ray.dir[kz]; //* ray.dir_inv[kz];
	float Sy = ray.dir[ky] / ray.dir[kz]; //* ray.dir_inv[kz];
	float Sz =        1.0f / ray.dir[kz]; //  ray.dir_inv[kz];

	//Vertices relative to ray origin
	Pos const A = verts[0].pos - ray.orig;
	Pos const B = verts[1].pos - ray.orig;
	Pos const C = verts[2].pos - ray.orig;

	//Shear and scale of vertices
	glm::vec3 ABC_kx = glm::vec3(A[kx],B[kx],C[kx]);
	glm::vec3 ABC_ky = glm::vec3(A[ky],B[ky],C[ky]);
	glm::vec3 ABC_kz = glm::vec3(A[kz],B[kz],C[kz]);
	glm::vec3 ABCx = ABC_kx - Sx*ABC_kz; //Ax, Bx, Cx
	glm::vec3 ABCy = ABC_ky - Sy*ABC_kz; //Ay, By, Cy

	//Scaled barycentric coordinates and edge tests
	glm::vec3 UVW = glm::cross( ABCy, ABCx );
	float const& U = UVW[0];
	float const& V = UVW[1];
	float const& W = UVW[2];
	if (U!=0.0f && V!=0.0f && W!=0.0f) {
		if ( (U<0.0f||V<0.0f||W<0.0f) && (U>0.0f||V>0.0f||W>0.0f) ) return false;
	} else {
		glm::dvec3 UVWd = glm::cross( glm::dvec3(ABCy), glm::dvec3(ABCx) );
		double const& Ud = UVWd[0];
		double const& Vd = UVWd[1];
		double const& Wd = UVWd[2];

		if ( (Ud<0.0||Vd<0.0||Wd<0.0) && (Ud>0.0||Vd>0.0||Wd>0.0) ) return false;

		UVW = glm::vec3(UVWd);
	}

	//Determinant
	float det = U + V + W;
	if (std::abs(det)>EPS);
	else return false;

	//Calculate scaled z-coordinates of vertices and use them to calculate the hit distance.
	glm::vec3 ABCz = Sz * ABC_kz;
	float const T = U*ABCz.x + V*ABCz.y + W*ABCz.z;

	//	Check signs of `det` and `T` match
	uint32_t det_u; memcpy(&det_u,&det, sizeof(float));
	uint32_t T_u;   memcpy(&T_u,  &T,   sizeof(float));
	bool different = ((det_u&0x80000000u) ^ (T_u&0x80000000u)) > 0;
	if (different) return false;

	//Normalize U, V, W, and T, then return
	float det_recip = 1 / det;
	Dist dist = T * det_recip;
	assert(!std::isnan(dist));
	if (dist>=EPS && dist<hitrec->dist) {
		hitrec->prim   = this;

		glm::vec3 bary = UVW * det_recip;
		hitrec->normal = normal;
		hitrec->st     = bary.x*verts[0].st + bary.y*verts[1].st + bary.z*verts[2].st;

		hitrec->dist   = dist;

		return true;
	}

	return false;
}


bool PrimQuad::intersect(Ray const& ray, HitRecord* hitrec) const /*override*/ {
	if (tri0.intersect(ray,hitrec)) goto HIT;
	if (tri1.intersect(ray,hitrec)) goto HIT;
	return false;

	HIT:
	hitrec->prim = this;
	return true;
}



Scene::~Scene() {
	for (auto iter : materials) delete iter.second;

	for (PrimBase const* iter : primitives) delete iter;
}

//TODO: comment

void Scene::_init() {
	camera.matr_P = glm::perspectiveFov(
		glm::radians(camera.fov),
		static_cast<float>(camera.res[0]), static_cast<float>(camera.res[1]),
		camera.near, camera.far
	);
	camera.matr_V = glm::lookAt( camera.pos, camera.pos+camera.dir, camera.up );
	camera.matr_PV_inv = glm::inverse( camera.matr_P * camera.matr_V );

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
		result->camera.fov = 39.0f;
		//result->camera.focal = 0.035f;

		//size?
	}

	{
		#ifdef RENDER_MODE_SPECTRAL
			std::vector<std::vector<float>> data = load_spectral_data("data/scenes/cornell/white-green-red.csv");
			if (data.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			MaterialLambertianSpectral* white_back      = new MaterialLambertianSpectral;
			white_back->reflectance = SpectralReflectance( data[0], 400,700 );
			//white_back->reflectance = SpectralReflectance( 1.0f );

			MaterialLambertianSpectral* white_blocks    = new MaterialLambertianSpectral(*white_back);

			MaterialLambertianSpectral* white_floorceil = new MaterialLambertianSpectral(*white_back);

			MaterialLambertianSpectral* green           = new MaterialLambertianSpectral;
			green->     reflectance = SpectralReflectance( data[1], 400,700 );

			MaterialLambertianSpectral* red             = new MaterialLambertianSpectral;
			red->       reflectance = SpectralReflectance( data[2], 400,700 );
		#else
			MaterialLambertianRGB*      white_back      = new MaterialLambertianRGB;
			white_back->reflectance = RGB_Reflectance(1,1,1);

			MaterialLambertianRGB*      white_blocks    = new MaterialLambertianRGB(*white_back);

			MaterialLambertianRGB*      white_floorceil = new MaterialLambertianRGB(*white_back);

			MaterialLambertianRGB*      green           = new MaterialLambertianRGB;
			green->     reflectance = RGB_Reflectance(0,1,0);

			MaterialLambertianRGB*      red             = new MaterialLambertianRGB;
			red->       reflectance = RGB_Reflectance(1,0,0);
		#endif

		result->materials["white-back"     ] = white_back;
		result->materials["white-blocks"   ] = white_blocks;
		result->materials["white-floorceil"] = white_floorceil;
		result->materials["green"          ] = green;
		result->materials["red"            ] = red;
	}

	{
		#ifdef RENDER_MODE_SPECTRAL
			std::vector<std::vector<float>> data = load_spectral_data("data/scenes/cornell/light.csv");
			if (data.size()==1); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

			MaterialLambertianSpectral* light = new MaterialLambertianSpectral;
			light->emission    = SpectralRadiance( data[0], 400,700 ) * 200.0f;
			//light->emission    = Color::data->D65_rad * 0.5f;
			//light->emission    = Color::data->D65_rad * 5.0f;
			light->reflectance = SpectralReflectance( 0.78f );
		#else
			MaterialLambertianRGB*      light = new MaterialLambertianRGB;
			light->emission    = RGB_Radiance(1,1,1) * 200.0f;
			light->reflectance = RGB_Reflectance( 0.78f );
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

	//MaterialBase* mtl_tex = new MaterialLambertianTexture("data/scenes/allrgb-512.png"); float lightsc=40.0f;
	MaterialBase* mtl_tex = new MaterialLambertianTexture("data/scenes/crystal-lizard-512.png"); float lightsc=30.0f;
	//MaterialBase* mtl_tex = new MaterialLambertianTexture("data/scenes/test-img.png"); float lightsc=20.0f;
	result->materials["srgb"] = mtl_tex;

	#ifdef RENDER_MODE_SPECTRAL
		MaterialLambertianSpectral* mtl_white1 = new MaterialLambertianSpectral;
		mtl_white1->reflectance = SpectralReflectance( 1.0f );
	#else
		MaterialLambertianRGB*      mtl_white1 = new MaterialLambertianRGB;
		mtl_white1->reflectance = RGB_Reflectance    ( 1.0f );
	#endif
	result->materials["white1"] = mtl_white1;

	for (PrimBase* prim : result->primitives) {
		if      (prim->material==result->materials["white-back"     ]) prim->material=mtl_tex;
		else if (prim->material==result->materials["white-blocks"   ]) prim->material=mtl_white1;
		else if (prim->material==result->materials["white-floorceil"]) prim->material=mtl_white1;
		else if (prim->material==result->materials["green"          ]) prim->material=mtl_white1;
		else if (prim->material==result->materials["red"            ]) prim->material=mtl_tex;
	}

	#ifdef RENDER_MODE_SPECTRAL
		static_cast<MaterialLambertianSpectral*>(result->materials["light"])->emission = Color::data->D65_rad * lightsc;
	#else
		static_cast<MaterialLambertianRGB*     >(result->materials["light"])->emission = RGB_Radiance(1,1,1)  * lightsc;
	#endif

	return result;
}
Scene* Scene::get_new_srgb        () {
	Scene* result = new Scene;

	{
		result->camera.pos = Pos(0,0,5);
		//result->camera.pos = Pos(50,30,20);
		result->camera.dir = glm::normalize(Pos(0)-result->camera.pos);
		result->camera.up  = Dir(0,1,0);

		result->camera.res[0] = 512;
		result->camera.res[1] = 512;
		result->camera.near=0.1f; result->camera.far=1.0f;
		result->camera.fov = glm::degrees(2.0f*std::atan2( 1.0f, result->camera.pos.z ));
		//result->camera.fov = 45.0f;
	}

	{
		#ifdef RENDER_MODE_SPECTRAL
			MaterialLambertianSpectral* mtl_light = new MaterialLambertianSpectral;
			mtl_light->reflectance = SpectralReflectance(0.0f);
			mtl_light->emission = Color::data->D65_rad;// * 0.5f;
		#else
			MaterialLambertianRGB*      mtl_light = new MaterialLambertianRGB;
			mtl_light->reflectance = RGB_Reflectance    (0.0f);
			mtl_light->emission = RGB_Radiance(1,1,1);
		#endif
		result->materials["light"] = mtl_light;

		//MaterialLambertianTexture* mtl_tex = new MaterialLambertianTexture("data/scenes/test-img.png");
		MaterialLambertianTexture* mtl_tex = new MaterialLambertianTexture("data/scenes/crystal-lizard-4096.png");
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
	hitrec->dist = std::numeric_limits<float>::infinity();

	bool hit = false;
	for (PrimBase const* prim : primitives) {
		if (prim!=ignore) {
			hit |= prim->intersect(ray, hitrec);
		}
	}

	return hit;
}
