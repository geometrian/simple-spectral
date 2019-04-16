#pragma once

#include "stdafx.hpp"

#include "util/color.hpp"
#include "util/string.hpp"

#include "framebuffer.hpp"
#include "renderer.hpp"



#ifdef SUPPORT_WINDOWED
#ifdef _DEBUG
inline static void _callback_err_glfw(int /*error*/, char const* description) {
	fprintf(stderr, "Error: %s\n", description);
}
#endif

inline static void _callback_key(GLFWwindow* window, int key,int /*scancode*/, int action, int mods) {
	if (action==GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				break;
			default:
				break;
		}
	}
}
#endif

inline static void _print_usage() {
	printf(
		"Simple Spectral: a simple spectral renderer for demonstration purposes\n"
		"  Required arguments:\n"
		"    `--scene=<name>`/`-s=<name>`\n"
		"          Render the given built-in scene (valid scenes: \"cornell\").\n"
		"    `--width=<width>`/`-w=<width>`\n"
		"          Set the width of the render.\n"
		"    `--height=<height>`/`-h=<height>`\n"
		"          Set the height of the render.\n"
		"    `--samples=<samples>`/`-spp=<samples>`\n"
		"          Set the number of samples per pixel.\n"
		"    `--output=<output-image-path>\n`/`-o=<output-image-path>`\n"
		"          Set the path to the output image.\n"
		#ifdef SUPPORT_WINDOWED
		"  Optional arguments:\n"
		"    `--window`/`-w`\n"
		"          Opens a window to display the ongoing render.\n"
		#endif
	);
}

inline static void _parse_arguments( char const*const argv[],size_t length, Renderer::Options* options ) {
	std::vector<std::string> args;
	for (size_t i=0;i<length;++i) args.emplace_back(argv[i]);

	auto get_arg     = [&](std::string const& name, std::string const& shortname="") -> std::string {
		for (auto iter=args.begin(); iter!=args.end(); ++iter) {
			std::string const& arg = *iter;
			if (Str::contains(arg,"=")) {
				std::vector<std::string> components = Str::split(arg,"=",1);

				if ( components[0]!=name && components[0]!=shortname );
				else {
					std::string result = arg;
					args.erase(iter);
					return components[1];
				}
			} else if ( arg==name || arg==shortname ) {
				args.erase(iter);
				return name;
			}
		}
		throw -2; //Argument not found
	};
	auto get_arg_req = [&](std::string const& name, std::string const& shortname="") -> std::string {
		try {
			return get_arg(name,shortname);
		} catch (int) {
			fprintf(stderr,"Required argument `%s`",name.c_str());
			if (!shortname.empty()) fprintf(stderr,"/`%s`",shortname.c_str());
			fprintf(stderr," not found!\n");
			throw;
		}
	};

	options->scene_name = get_arg_req("--scene","-s");
	if      (options->scene_name=="cornell"     );
	else if (options->scene_name=="cornell-srgb");
	else if (options->scene_name=="srgb"        );
	else {
		fprintf(stderr,"Unrecognized scene \"%s\"!  (Supported scenes: \"cornell\", \"cornell-srgb\", \"srgb\")\n",options->scene_name.c_str());
		throw -3;
	}

	std::string strs_res[2];
	strs_res[0] = get_arg_req("--width", "-w");
	strs_res[1] = get_arg_req("--height","-h");
	try {
		options->res[0] = Str::to_pos(strs_res[0]);
		options->res[1] = Str::to_pos(strs_res[1]);
	} catch (int) {
		fprintf(stderr,"Invalid width or height!\n");
		throw;
	}

	std::string str_spp = get_arg_req("--samples", "-spp");
	try {
		options->spp = Str::to_pos(str_spp);
	} catch (int) {
		fprintf(stderr,"Invalid number of samples!\n");
		throw;
	}

	options->output_path = get_arg_req("--output", "-o");

	#ifdef SUPPORT_WINDOWED
	std::string str_win;
	try {
		str_win = get_arg("--window", "-w");
		options->open_window = true;
	} catch (...) {
		options->open_window = false;
	}
	if (options->open_window) {
		if (str_win=="--window");
		else {
			fprintf(stderr,"`--window`/`-w` does not take a value!\n");
			throw -1;
		}
	}
	#endif

	if (args.size()>1) {
		fprintf(stderr,"Warning: ignoring extraneous argument(s):\n");
		for (size_t i=1;i<args.size();++i) {
			fprintf(stderr,"  \"%s\"\n",args[i].c_str());
		}
	}
}

int main(int argc, char* argv[]) {
	#if defined _WIN32 && defined _DEBUG
		_CrtSetDbgFlag(0xFFFFFFFF);
	#endif

	{
		//Attempt to parse arguments for render
		Renderer::Options options;
		try {
			_parse_arguments( argv,static_cast<size_t>(argc), &options );
		} catch (int) {
			_print_usage();
			return -1;
		}

		#ifdef RENDER_MODE_SPECTRAL
		//Initialize color data
		Color::init();
		#endif

		//Create renderer
		Renderer renderer(options);
		#ifdef SUPPORT_WINDOWED
		if (options.open_window) {
			//Set up window and rendering parameters
			GLFWwindow* window;
			{
				#ifdef _DEBUG
					glfwSetErrorCallback(_callback_err_glfw);
				#endif

				glfwInit();

				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
				#ifdef _DEBUG
					glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
				#endif

				glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

				#ifdef WITH_TRANSPARENT_FRAMEBUFFER
					glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
				#endif

				window = glfwCreateWindow(
					static_cast<int>(options.res[0]), static_cast<int>(options.res[1]),
					"simple-spectral",
					nullptr,
					nullptr
				);

				glfwSetKeyCallback(window, _callback_key);

				glfwMakeContextCurrent(window);
				#ifdef _DEBUG
					//glDebugMessageCallback(_callback_err_gl,nullptr);
				#endif

				//glfwSwapInterval(0);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			}

			//Start rendering
			renderer.render_start();

			//Display loop for the ongoing or completed render
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();

				renderer.framebuffer.draw();

				glfwSwapBuffers(window);
			}

			//Stop the renderer if it hasn't been already.
			renderer.render_stop();
			renderer.render_wait();

			//Clean up
			glfwDestroyWindow(window);

			glfwTerminate();
		} else {
		#endif
			//Start rendering
			renderer.render_start();

			//Wait for completion
			renderer.render_wait ();
		#ifdef SUPPORT_WINDOWED
		}
		#endif

		#ifdef RENDER_MODE_SPECTRAL
		//Clean up color data
		Color::deinit();
		#endif
	}

	#if defined _WIN32 && defined _DEBUG
		if (_CrtDumpMemoryLeaks()) {
			fprintf(stderr,"Memory leaks detected!\n");
			printf("Press ENTER to exit.\n");
			getchar();
		}
	#endif

	//Return to OS
	return 0;
}
