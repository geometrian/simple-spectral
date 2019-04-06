#pragma once

#include "stdafx.hpp"

#include "framebuffer.hpp"
#include "rng.hpp"


class PrimitiveBase;

class Scene;

class Renderer final {
	public:
		class Options final { public:
			std::string scene_name;

			size_t res[2]; //Resolution of image
			size_t spp;    //Samples per pixel

			std::string output_path;

			#ifdef SUPPORT_WINDOWED
			bool open_window;
			#endif
		};
		Options const options;

		Framebuffer framebuffer;

		Scene* scene;

	private:
		std::mutex _tiles_mutex;
		std::vector<Framebuffer::Tile> _tiles;

		std::vector<std::thread*> _threads;

		bool volatile _render_continue;

	public:
		explicit Renderer(Options const& options);
		~Renderer();

	private:
		CIEXYZ _render_sample(Math::RNG& rng, size_t i,size_t j);
		void   _render_pixel (Math::RNG& rng, size_t i,size_t j);
		void _render_threadwork();
	public:
		void render_start();
		void render_stop () { _render_continue=false; }
		void render_wait ();
};
