# Simple Spectral

This is a simple multithreaded spectral pathtracer implementing our algorithm (linear combination of
bases), Meng et al. 2015, and a classic RGB renderer.  The spectral rendering uses hero-wavelength
sampling on a simple pathtracer.  Our implementation supports both the CIE 1931 and 2006 standard
observers.

Setup:

	Dependencies are intended to be minimal and easy:
		• GLM         (required)
		• GLFW        (optional, adds windowing support)
		• lodepng     (required, included)
		• Meng et al. (required, included)

	Setup is basic CMake:
		(1) "cd <path/to/>simple-spectral/"
		(2) "mkdir build"
		(3) "cd build/"
		(4) "cmake .."
		(3) "cmake --build ."

		There are some handy options in "simple-spectral/src/stdafx.hpp" near the top of the file.
		Some parameters are only exposed this way for simplicity or performance.

Program invocations:
	These can be found by simply running the program with no arguments.  However, basic usage looks
	something like:

		<binary> --scene=cornell-srgb -w=1024 -h=1024 -spp=64 --output=output.png --window

	The available scenes are "cornell" (original Cornell box), "cornell-srgb" (adjusted materials),
	and "plane-srgb", which is the plane setup in Figure 1.

Acknowledgments:
	Full acknowledgments are omitted for review, but for now we would like to thank Meng et al. and
	lodepng, both of which made their code available.
