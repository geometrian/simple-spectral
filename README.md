# Simple Spectral

This is a simple multithreaded spectral pathtracer implementing the algorithm (linear combination of
bases) described in our [EGSR 2019](http://egsr2019.icube.unistra.fr/program.html) paper "Spectral
Primary Decomposition for Rendering with sRGB Reflectance"
[[Mallett and Yuksel 2019]](https://graphics.geometrian.com/research/spectral-primaries.html).  Also
implemented are [[Meng et al. 2015]](https://onlinelibrary.wiley.com/doi/abs/10.1111/cgf.12676),
[[Jakob and Hanika 2019]](https://rgl.epfl.ch/publications/Jakob2019Spectral), and a classic RGB
renderer.

The spectral rendering uses hero-wavelength sampling on a simple pathtracer.  Our implementation
supports both the CIE 1931 and 2006 standard observers.

## Setup

Dependencies are intended to be minimal and easy (with the exception of GLM, all required
dependencies are included):

- [GLM](https://github.com/g-truc/glm)           (required)
- [GLFW](https://github.com/glfw/glfw)           (optional, adds windowing support)
- [lodepng](https://github.com/lvandeve/lodepng) (required, **included**)
- [Meng et al.      2015]                        (required, **included**)
- [Jakob and Hanika 2019]                        (required, **included**)

Setup is basic CMake:

1. `cd <path/to/>simple-spectral/`
2. `mkdir build`
3. `cd build/`
4. `cmake ..`
5. `cmake --build .`

There are some handy options in ["simple-spectral/src/stdafx.hpp"](src/stdafx.hpp) near the top of
the file.  Some parameters are only exposed this way for simplicity or performance.

## Program Invocation

Usage, including command-line options, can be found by simply running the binary with no
arguments.

However, for convenience, basic usage looks something like:

	<binary> --scene=cornell-srgb -w=512 -h=512 -spp=64 --output=output.png --window

The available scenes are "cornell" (original
[Cornell box](http://www.graphics.cornell.edu/online/box/)), "cornell-srgb" (adjusted materials;
Figure 5.a), and "plane-srgb" (Figure 1).

## Acknowledgments

We would like to thank [Meng et al. 2015] and [Jakob and Hanika 2019], both of which make their code
easy to use.  Also lodepng, for existing.  More acknowledgments can be found within the paper.
