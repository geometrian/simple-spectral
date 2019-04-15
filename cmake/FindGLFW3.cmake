#Credits omitted for review

set(_glfw3_HEADER_SEARCH_DIRS
	"/usr/include"
	"/usr/local/include"
	"${CMAKE_SOURCE_DIR}/include"
	"C:/Program Files (x86)/glfw/include"
	"C:/Program Files (x86)/Windows Kits/10/Lib/user/glfw/include")

set(_glfw3_LIB_SEARCH_DIRS
	"/usr/lib"
	"/usr/local/lib"
	"${CMAKE_SOURCE_DIR}/lib"
	"C:/Program Files (x86)/glfw/lib-msvc110"
	"C:/Program Files (x86)/Windows Kits/10/Lib/user/glfw/build/x64/RelWithDebInfo")

# Check environment for root search directory
set( _glfw3_ENV_ROOT $ENV{GLFW3_ROOT} )
if( NOT GLFW3_ROOT AND _glfw3_ENV_ROOT )
	set(GLFW3_ROOT ${_glfw3_ENV_ROOT} )
endif()

# Put user specified location at beginning of search
if( GLFW3_ROOT )
	list( INSERT _glfw3_HEADER_SEARCH_DIRS 0 "${GLFW3_ROOT}/include" )
	list( INSERT _glfw3_LIB_SEARCH_DIRS 0 "${GLFW3_ROOT}/lib" )
endif()

# Search for the header
FIND_PATH(GLFW3_INCLUDE_DIR "GLFW/glfw3.h" PATHS ${_glfw3_HEADER_SEARCH_DIRS} )
# Search for the library
FIND_LIBRARY(GLFW3_LIBRARY NAMES glfw3 glfw PATHS ${_glfw3_LIB_SEARCH_DIRS} )
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLFW3 DEFAULT_MSG GLFW3_LIBRARY GLFW3_INCLUDE_DIR)
