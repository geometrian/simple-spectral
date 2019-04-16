#pragma once

#include "stdafx.hpp"

#include "util/random.hpp"

#include "framebuffer.hpp"



class Scene;

class Renderer final {
	public:
		//Render options
		class Options final { public:
			std::string scene_name;

			size_t res[2]; //Resolution of image
			size_t spp;    //Samples per pixel

			bool indirect_only; //Whether only indirect illumination should be rendered

			std::string output_path;

			#ifdef SUPPORT_WINDOWED
			bool open_window;
			#endif
		};
		Options const options;

		Framebuffer framebuffer;

		Scene* scene;

	private:
		//Concurrent list of pixel tiles in the framebuffer that remain to be rendered
		std::mutex _tiles_mutex;
		std::vector<Framebuffer::Tile> _tiles;

		//Worker threads
		std::vector<std::thread*> _threads;
		//Number of threads currently rendering
		std::atomic<uint32_t> _num_rendering;

		//Internal data used for calculating statistics
		size_t _num_tiles_start;
		std::chrono::steady_clock::time_point _time_start;
		std::chrono::steady_clock::time_point _time_last_print;

		//Whether the render should continue
		bool volatile _render_continue;

	public:
		explicit Renderer(Options const& options);
		~Renderer();

	private:
		//Prints the status of an ongoing render.
		void _print_progress() const;

		//Calculate a single sample for pixel (`i`,`j`).
		#ifdef RENDER_MODE_SPECTRAL
		CIEXYZ_A_32F _render_sample(Math::RNG& rng, size_t i,size_t j);
		#else
		lRGB_A_F32   _render_sample(Math::RNG& rng, size_t i,size_t j);
		#endif
		//Calculate all samples for pixel (`i`,`j`) and store the reconstructed value into the
		//	framebuffer.  Called internally by the thread worker.
		void       _render_pixel (Math::RNG& rng, size_t i,size_t j);
		//Member function called by each thread
		void _render_threadwork();
	public:
		//Creates the worker threads and sets them rendering
		void render_start();
		//Tells the worker threads to abort the render
		void render_stop () { _render_continue=false; }
		//Waits for the worker threads to terminate
		void render_wait ();

		bool is_rendering() const { return _num_rendering>0u; }
};
