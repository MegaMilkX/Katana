cmake_minimum_required (VERSION 3.15)
project ($PROJECT_NAME)

if(MSVC)
  add_definitions(/MP)
  #add_definitions(/incremental)
  #add_definitions(/Debug:fastlink)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# TODO: Fix. Files with h at the end not being processed
file(GLOB_RECURSE GENBINFILES $ENGINE_SRC_DIR/common/gen/*[!.h])

foreach(gen_bin_file ${GENBINFILES})
	get_filename_component(DIR ${gen_bin_file} DIRECTORY)
	get_filename_component(FNAME ${gen_bin_file} NAME)
	execute_process(
		COMMAND "$BUILD_TOOLS_DIR/bin2header/bin2header.exe" ${FNAME}
		WORKING_DIRECTORY ${DIR}
	)
endforeach(gen_bin_file)

file(GLOB_RECURSE COMMON_SRCFILES
	$ENGINE_SRC_DIR/common/*.cpp;
	$ENGINE_SRC_DIR/common/*.c;
	$ENGINE_SRC_DIR/common/*.cxx;
)
file(GLOB_RECURSE SRCFILES 
	$ENGINE_SRC_DIR/katana/*.cpp;
	$ENGINE_SRC_DIR/katana/*.c;
	$ENGINE_SRC_DIR/katana/*.cxx;
	*.cpp;
	*.c;
	*.cxx
)
file(GLOB_RECURSE EDITOR_SRCFILES 
	$ENGINE_SRC_DIR/editor/*.cpp;
	$ENGINE_SRC_DIR/editor/*.c;
	$ENGINE_SRC_DIR/editor/*.cxx;
	*.cpp;
	*.c;
	*.cxx
)
file(GLOB_RECURSE THUMB_BUILDER_SRCFILES 
	$ENGINE_SRC_DIR/thumb_builder/*.cpp;
	$ENGINE_SRC_DIR/thumb_builder/*.c;
	$ENGINE_SRC_DIR/thumb_builder/*.cxx;
	*.cpp;
	*.c;
	*.cxx
)
add_library(common STATIC ${COMMON_SRCFILES})
add_executable($PROJECT_NAME ${SRCFILES} $ENGINE_SRC_DIR/resource.rc)
add_executable(${PROJECT_NAME}_editor ${EDITOR_SRCFILES} $ENGINE_SRC_DIR/resource.rc)
add_executable(thumb_builder ${THUMB_BUILDER_SRCFILES} $ENGINE_SRC_DIR/resource.rc)

set_target_properties(
	$PROJECT_NAME PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../build/debug"
)
set_target_properties(
	${PROJECT_NAME}_editor PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../build/debug"
)
set_target_properties(
	thumb_builder PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../build/debug"
)


target_include_directories(common PRIVATE 
	$ENGINE_SRC_DIR/common/
	$ENGINE_SRC_DIR/common/lib/
	$ENGINE_SRC_DIR/lib/glfw/include/
	$ENGINE_SRC_DIR/lib/assimp/include/
	$ENGINE_SRC_DIR/lib/assimp/build/include/
	$ENGINE_SRC_DIR/lib/bullet3-2.88/src/
	$ENGINE_SRC_DIR/lib/freetype-2.10.0/include/
	$ENGINE_SRC_DIR/lib/angelscript/include/
	$INCLUDE_PATHS
)
target_link_directories(common PRIVATE 
	$LIB_PATHS
)

target_include_directories($PROJECT_NAME PRIVATE 
	$ENGINE_SRC_DIR/common/
	$ENGINE_SRC_DIR/common/lib/
	$ENGINE_SRC_DIR/lib/glfw/include/
	$ENGINE_SRC_DIR/lib/assimp/include/
	$ENGINE_SRC_DIR/lib/assimp/build/include/
	$ENGINE_SRC_DIR/lib/bullet3-2.88/src/
	$ENGINE_SRC_DIR/katana/
	$ENGINE_SRC_DIR/lib/freetype-2.10.0/include/
	$ENGINE_SRC_DIR/lib/angelscript/include/
	$INCLUDE_PATHS
)
target_link_directories($PROJECT_NAME PRIVATE 
	$LIB_PATHS
)
target_link_libraries($PROJECT_NAME 
	shlwapi.lib
	OpenGL32.lib
	hid.lib
	setupapi.lib
	BulletCollision
	BulletDynamics
	LinearMath
	glfw
	assimp
	zlibstatic
	freetype
	angelscript
	common
)

target_include_directories(${PROJECT_NAME}_editor PRIVATE 
	$ENGINE_SRC_DIR/common/
	$ENGINE_SRC_DIR/common/lib/
	$ENGINE_SRC_DIR/lib/glfw/include/
	$ENGINE_SRC_DIR/lib/assimp/include/
	$ENGINE_SRC_DIR/lib/assimp/build/include/
	$ENGINE_SRC_DIR/lib/bullet3-2.88/src/
	$ENGINE_SRC_DIR/lib/freetype-2.10.0/include/
	$ENGINE_SRC_DIR/lib/angelscript/include/
	$INCLUDE_PATHS
)
target_link_directories(${PROJECT_NAME}_editor PRIVATE 
	$LIB_PATHS
)
target_link_libraries(${PROJECT_NAME}_editor
	shlwapi.lib
	OpenGL32.lib
	hid.lib
	setupapi.lib
	BulletCollision
	BulletDynamics
	LinearMath
	glfw
	assimp
	zlibstatic
	freetype
	angelscript
	common
)

target_include_directories(thumb_builder PRIVATE 
	$ENGINE_SRC_DIR/common/
	$ENGINE_SRC_DIR/common/lib/
	$ENGINE_SRC_DIR/lib/glfw/include/
	$ENGINE_SRC_DIR/lib/assimp/include/
	$ENGINE_SRC_DIR/lib/assimp/build/include/
	$ENGINE_SRC_DIR/lib/bullet3-2.88/src/
	$ENGINE_SRC_DIR/lib/freetype-2.10.0/include/
	$ENGINE_SRC_DIR/lib/angelscript/include/
	$INCLUDE_PATHS
)
target_link_directories(thumb_builder PRIVATE 
	$LIB_PATHS
)
target_link_libraries(thumb_builder
	shlwapi.lib
	OpenGL32.lib
	hid.lib
	setupapi.lib
	BulletCollision
	BulletDynamics
	LinearMath
	glfw
	assimp
	zlibstatic
	freetype
	angelscript
	common
)

target_compile_definitions(common PRIVATE 
	_CRT_SECURE_NO_WARNINGS
	NOMINMAX
	_GLFW_WIN32
)
target_compile_definitions(${PROJECT_NAME} PRIVATE 
	_CRT_SECURE_NO_WARNINGS
	NOMINMAX
	_GLFW_WIN32
)
target_compile_definitions(${PROJECT_NAME}_editor PRIVATE 
	_CRT_SECURE_NO_WARNINGS
	NOMINMAX
	_GLFW_WIN32
	KT_EDITOR
)
target_compile_definitions(thumb_builder PRIVATE 
	_CRT_SECURE_NO_WARNINGS
	NOMINMAX
	_GLFW_WIN32
)


add_dependencies(${PROJECT_NAME}_editor thumb_builder)


option(BUILD_SHARED_LIBS OFF)
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_INSTALL OFF)
option(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)
add_subdirectory(
	$ENGINE_SRC_DIR/lib/glfw
	$ENGINE_SRC_DIR/lib/glfw/build
)


option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_BUILD_ZLIB ON)
option(BUILD_SHARED_LIBS OFF)
add_subdirectory(
	$ENGINE_SRC_DIR/lib/assimp
	$ENGINE_SRC_DIR/lib/assimp/build
)


option(BUILD_BULLET2_DEMOS OFF)
option(BUILD_BULLET3 OFF)
option(BUILD_CPU_DEMOS OFF)
option(BUILD_EXTRAS OFF)
option(BUILD_OPENGL3_DEMOS OFF)
option(BUILD_PYBULLET OFF)
option(BUILD_SHARED_LIBS OFF)
option(BUILD_UNIT_TESTS OFF)
option(BULLET2_USE_THREAD_LOCKS OFF) # Threading
option(USE_GLUT OFF)
option(USE_GRAPHICAL_BENCHMARK OFF)
option(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)
add_subdirectory(
	$ENGINE_SRC_DIR/lib/bullet3-2.88
	$ENGINE_SRC_DIR/lib/bullet3-2.88/build
)


add_subdirectory(
	$ENGINE_SRC_DIR/lib/freetype-2.10.0
	$ENGINE_SRC_DIR/lib/freetype-2.10.0/build
)

option(BUILD_SHARED_LIBS OFF)
option(AS_NO_EXCEPTIONS OFF)
add_subdirectory(
	$ENGINE_SRC_DIR/lib/angelscript/projects/cmake
	$ENGINE_SRC_DIR/lib/angelscript/build
)

