#pragma once

#include "stdafx.hpp"

#include "color.hpp"
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
			if (str_contains(arg,"=")) {
				std::vector<std::string> components = str_split(arg,"=",1);

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
	if (options->scene_name=="cornell"); else {
		fprintf(stderr,"Unrecognized scene \"%s\"!  (Supported scenes: \"cornell\")\n",options->scene_name.c_str());
		throw -3;
	}

	std::string strs_res[2];
	strs_res[0] = get_arg_req("--width", "-w");
	strs_res[1] = get_arg_req("--height","-h");
	try {
		options->res[0] = str_to_pos(strs_res[0]);
		options->res[1] = str_to_pos(strs_res[1]);
	} catch (int) {
		fprintf(stderr,"Invalid width or height!\n");
		throw;
	}

	std::string str_spp = get_arg_req("--samples", "-spp");
	try {
		options->spp = str_to_pos(str_spp);
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
}

int main(int argc, char* argv[]) {
	Renderer::Options options;
	try {
		_parse_arguments( argv,static_cast<size_t>(argc), &options );
	} catch (int) {
		_print_usage();
		return -1;
	}

	Color::init();

	Renderer renderer(options);
	#ifdef SUPPORT_WINDOWED
	if (options.open_window) {
		GLFWwindow* window;

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

		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

		window = glfwCreateWindow(
			static_cast<int>(options.res[0]), static_cast<int>(options.res[1]),
			"simple-spectral",
			nullptr,
			nullptr
		);

		glfwSetKeyCallback(window, _callback_key);

		glfwMakeContextCurrent(window);
		#ifdef _DEBUG
			//glDebugMessageCallback(callback_err_gl,nullptr);
		#endif

		//glfwSwapInterval(0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		renderer.render_start();
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			renderer.framebuffer.draw();

			glfwSwapBuffers(window);
		}
		renderer.render_stop();
		renderer.render_wait();

		glfwDestroyWindow(window);

		glfwTerminate();
	} else {
	#endif
		renderer.render_start();
		renderer.render_wait ();
	#ifdef SUPPORT_WINDOWED
	}
	#endif

	return 0;
}
