#include "scene.hpp"


MaterialLambertianSpectral::MaterialLambertianSpectral() :
	emission   ( std::vector<float>(size_t(2),0.0f), LAMBDA_MIN,LAMBDA_MAX ),
	reflectance( std::vector<float>(size_t(2),1.0f), LAMBDA_MIN,LAMBDA_MAX )
{}


bool Tri:: intersect(Ray const& ray, HitRecord* hitrec) const /*override*/ {
	//Robust intersection:
	//	http://jcgt.org/published/0002/01/05/paper.pdf
	//	TODO: vectorize!

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
	Pos const A = verts[0] - ray.orig;
	Pos const B = verts[1] - ray.orig;
	Pos const C = verts[2] - ray.orig;

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
	float dist = T * det_recip;
	assert(!std::isnan(dist));
	if (dist>=EPS) {
		if (dist>=hitrec->dist);
		else {
			//glm::vec3 bary = UVW * det_recip;
			hitrec->prim   = this;
			hitrec->normal = normal;
			hitrec->dist   = dist;
			return true;
		}
	}

	return false;
}


bool Quad::intersect(Ray const& ray, HitRecord* hitrec) const /*override*/ {
	
	if (tri0.intersect(ray,hitrec)) return true;

	if (tri1.intersect(ray,hitrec)) return true;

	return false;
}


Scene::~Scene() {
	for (auto iter : materials) delete iter.second;

	for (PrimitiveBase const* iter : primitives) delete iter;
}

void Scene::_init() {
	camera.matr_P = glm::perspectiveFov(
		glm::radians(camera.fov),
		static_cast<float>(camera.res[0]), static_cast<float>(camera.res[1]),
		camera.near, camera.far
	);
	camera.matr_V = glm::lookAt( camera.pos, camera.pos+camera.dir, camera.up );
	camera.matr_PV_inv = glm::inverse( camera.matr_P * camera.matr_V );
}
Scene* Scene::get_new_cornell() {
	//http://www.graphics.cornell.edu/online/box/data.html
	Scene* result = new Scene;

	{
		result->camera.pos = Pos(278,273,-800);
		result->camera.dir = glm::normalize(Dir(0,0,1));
		result->camera.up  = Dir(0,1,0);

		result->camera.res[0] = 512;
		result->camera.res[1] = 512;
		result->camera.near=0.1f; result->camera.far=1.0f;
		result->camera.fov = 41.0f;
		//result->camera.focal = 0.035f;

		//size?
	}

	{
		std::vector<std::vector<float>> data = load_spectral_data("../data/scenes/cornell/white-green-red.csv");
		if (data.size()==3); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

		MaterialLambertianSpectral* white = new MaterialLambertianSpectral;
		white->reflectance = Spectrum( data[0], 400,700 );
		result->materials["white"] = white;

		MaterialLambertianSpectral* green = new MaterialLambertianSpectral;
		green->reflectance = Spectrum( data[1], 400,700 );
		result->materials["green"] = green;

		MaterialLambertianSpectral* red   = new MaterialLambertianSpectral;
		red->  reflectance = Spectrum( data[2], 400,700 );
		result->materials["red"  ] = red;
	}

	{
		std::vector<std::vector<float>> data = load_spectral_data("../data/scenes/cornell/light.csv");
		if (data.size()==1); else { fprintf(stderr,"Invalid data in file!\n"); throw -1; }

		MaterialLambertianSpectral* light = new MaterialLambertianSpectral;
		light->emission    = Spectrum( data[0], 400,700 );
		light->reflectance = Spectrum( std::vector<float>(size_t(2),0.78f), 380,780 );
		result->materials["light"] = light;
	}

	{
		//Floor
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 552.8f, 0.0f,   0.0f ),
			Pos(   0.0f, 0.0f,   0.0f ),
			Pos(   0.0f, 0.0f, 559.2f ),
			Pos( 549.6f, 0.0f, 559.2f )
		));

		//Light (note moved down 0.01 so not on ceiling)
		result->primitives.emplace_back(new Quad(result->materials["light"],
			Pos( 343.0f, 548.79f, 227.0f ),
			Pos( 343.0f, 548.79f, 332.0f ),
			Pos( 213.0f, 548.79f, 332.0f ),
			Pos( 213.0f, 548.79f, 227.0f )
		));

		//Ceiling
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 556.0f, 548.8f,   0.0f ),
			Pos( 556.0f, 548.8f, 559.2f ),
			Pos(   0.0f, 548.8f, 559.2f ),
			Pos(   0.0f, 548.8f,   0.0f )
		));

		//Back wall
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 549.6f,   0.0f, 559.2f ),
			Pos(   0.0f,   0.0f, 559.2f ),
			Pos(   0.0f, 548.8f, 559.2f ),
			Pos( 556.0f, 548.8f, 559.2f )
		));

		//Right wall
		result->primitives.emplace_back(new Quad(result->materials["green"],
			Pos( 0.0f,   0.0f, 559.2f ),
			Pos( 0.0f,   0.0f,   0.0f ),
			Pos( 0.0f, 548.8f,   0.0f ),
			Pos( 0.0f, 548.8f, 559.2f )
		));

		//Left wall
		result->primitives.emplace_back(new Quad(result->materials["red"  ],
			Pos( 552.8f,   0.0f,   0.0f ),
			Pos( 549.6f,   0.0f, 559.2f ),
			Pos( 556.0f, 548.8f, 559.2f ),
			Pos( 556.0f, 548.8f,   0.0f )
		));

		//Short block
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 130.0f, 165.0f,  65.0f ),
			Pos(  82.0f, 165.0f, 225.0f ),
			Pos( 240.0f, 165.0f, 272.0f ),
			Pos( 290.0f, 165.0f, 114.0f )
		));
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 290.0f,   0.0f, 114.0f ),
			Pos( 290.0f, 165.0f, 114.0f ),
			Pos( 240.0f, 165.0f, 272.0f ),
			Pos( 240.0f,   0.0f, 272.0f )
		));
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 130.0f,   0.0f,  65.0f ),
			Pos( 130.0f, 165.0f,  65.0f ),
			Pos( 290.0f, 165.0f, 114.0f ),
			Pos( 290.0f,   0.0f, 114.0f )
		));
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos(  82.0f,   0.0f, 225.0f ),
			Pos(  82.0f, 165.0f, 225.0f ),
			Pos( 130.0f, 165.0f,  65.0f ),
			Pos( 130.0f,   0.0f,  65.0f )
		));
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 240.0f,   0.0f, 272.0f ),
			Pos( 240.0f, 165.0f, 272.0f ),
			Pos(  82.0f, 165.0f, 225.0f ),
			Pos(  82.0f,   0.0f, 225.0f )
		));

		//Tall block
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 423.0f, 330.0f, 247.0f ),
			Pos( 265.0f, 330.0f, 296.0f ),
			Pos( 314.0f, 330.0f, 456.0f ),
			Pos( 472.0f, 330.0f, 406.0f )
		));
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 423.0f,   0.0f, 247.0f ),
			Pos( 423.0f, 330.0f, 247.0f ),
			Pos( 472.0f, 330.0f, 406.0f ),
			Pos( 472.0f,   0.0f, 406.0f )
		));
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 472.0f,   0.0f, 406.0f ),
			Pos( 472.0f, 330.0f, 406.0f ),
			Pos( 314.0f, 330.0f, 456.0f ),
			Pos( 314.0f,   0.0f, 456.0f )
		));
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 314.0f,   0.0f, 456.0f ),
			Pos( 314.0f, 330.0f, 456.0f ),
			Pos( 265.0f, 330.0f, 296.0f ),
			Pos( 265.0f,   0.0f, 296.0f )
		));
		result->primitives.emplace_back(new Quad(result->materials["white"],
			Pos( 265.0f,   0.0f, 296.0f ),
			Pos( 265.0f, 330.0f, 296.0f ),
			Pos( 423.0f, 330.0f, 247.0f ),
			Pos( 423.0f,   0.0f, 247.0f )
		));
	}

	result->_init();

	return result;
}

bool Scene::intersect(Ray const& ray, HitRecord* hitrec) const {
	hitrec->prim = nullptr;
	hitrec->dist = std::numeric_limits<float>::infinity();

	bool hit = false;
	for (PrimitiveBase const* prim : primitives) {
		hit |= prim->intersect(ray, hitrec);
	}

	return hit;
}
