cmake_minimum_required (VERSION 3.15)
project ($PROJECT_NAME)

if(MSVC)
  add_definitions(/MP)
  #add_definitions(/incremental)
  #add_definitions(/Debug:fastlink)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#foreach(flag_var
#        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
#        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
#    if(${flag_var} MATCHES "/MD")
#        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
#    endif(${flag_var} MATCHES "/MD")
#	if(${flag_var} MATCHES "/MDd")
#        string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
#    endif(${flag_var} MATCHES "/MDd")
#endforeach(flag_var)

file(GLOB_RECURSE GENBINFILES $ENGINE_SRC_DIR/common/gen/*[!.h])

foreach(gen_bin_file ${GENBINFILES})
	get_filename_component(DIR ${gen_bin_file} DIRECTORY)
	get_filename_component(FNAME ${gen_bin_file} NAME)
	execute_process(
		COMMAND "$BUILD_TOOLS_DIR/bin2header/bin2header.exe" ${FNAME}
		WORKING_DIRECTORY ${DIR}
	)
endforeach(gen_bin_file)

file(GLOB_RECURSE SRCFILES 
$ENGINE_SRC_DIR/common/*.cpp;
$ENGINE_SRC_DIR/common/*.c;
$ENGINE_SRC_DIR/common/*.cxx;
$ENGINE_SRC_DIR/katana/*.cpp;
$ENGINE_SRC_DIR/katana/*.c;
$ENGINE_SRC_DIR/katana/*.cxx;
*.cpp;
*.c;
*.cxx)
file(GLOB_RECURSE EDITOR_SRCFILES 
$ENGINE_SRC_DIR/common/*.cpp;
$ENGINE_SRC_DIR/common/*.c;
$ENGINE_SRC_DIR/common/*.cxx;
$ENGINE_SRC_DIR/editor/*.cpp;
$ENGINE_SRC_DIR/editor/*.c;
$ENGINE_SRC_DIR/editor/*.cxx;
*.cpp;
*.c;
*.cxx)
add_executable($PROJECT_NAME ${SRCFILES} $ENGINE_SRC_DIR/resource.rc)
add_executable(${PROJECT_NAME}_editor ${EDITOR_SRCFILES} $ENGINE_SRC_DIR/resource.rc)

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

target_include_directories($PROJECT_NAME PRIVATE 
	$ENGINE_SRC_DIR/common/
	$ENGINE_SRC_DIR/common/lib/
	$ENGINE_SRC_DIR/lib/glfw/include/
	$ENGINE_SRC_DIR/lib/assimp/include/
	$ENGINE_SRC_DIR/lib/assimp/build/include/
	$ENGINE_SRC_DIR/lib/bullet3-2.88/src/
	$ENGINE_SRC_DIR/katana/
	$INCLUDE_PATHS
)
target_link_directories($PROJECT_NAME PRIVATE 
	$LIB_PATHS
)
target_link_libraries($PROJECT_NAME 
	shlwapi.lib
	OpenGL32.lib
	BulletCollision
	BulletDynamics
	LinearMath
	glfw
	assimp
	zlibstatic
)

target_include_directories(${PROJECT_NAME}_editor PRIVATE 
	$ENGINE_SRC_DIR/common/
	$ENGINE_SRC_DIR/common/lib/
	$ENGINE_SRC_DIR/lib/glfw/include/
	$ENGINE_SRC_DIR/lib/assimp/include/
	$ENGINE_SRC_DIR/lib/assimp/build/include/
	$ENGINE_SRC_DIR/lib/bullet3-2.88/src/
	$INCLUDE_PATHS
)
target_link_directories(${PROJECT_NAME}_editor PRIVATE 
	$LIB_PATHS
)
target_link_libraries(${PROJECT_NAME}_editor
	shlwapi.lib
	OpenGL32.lib
	BulletCollision
	BulletDynamics
	LinearMath
	glfw
	assimp
	zlibstatic
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
)


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

