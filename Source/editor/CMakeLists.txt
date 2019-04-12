cmake_minimum_required (VERSION 2.6)
project (scene_editor)

foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

file(GLOB_RECURSE SRCFILES 
../common_old/*.cpp;
../common_old/*.c;
../common_old/*.cxx;
*.cpp;
*.c;
*.cxx)
add_executable(scene_editor ${SRCFILES})

target_include_directories(scene_editor PRIVATE 
	../../../lib/glfw/include
	../../../lib/rttr/src
	../../../lib/assimp-master/include
	../../../lib/assimp-master/build/include
	../../../lib/bullet3-2.86.1/src
)
target_link_directories(scene_editor PRIVATE 
	../../../lib/glfw/lib
	../../../lib/rttr/lib
	../../../lib/assimp-master/build/code/Release
	../../../lib/assimp-master/build/contrib/zlib/Release
	../../../lib/bullet3-2.86.1/bin
)
target_link_libraries(scene_editor 
	shlwapi.lib
	glfw3.lib 
	OpenGL32.lib
	librttr_core.lib
	assimp.lib
	zlibstatic.lib
	BulletCollision_vs2010.lib
	BulletDynamics_vs2010.lib
	LinearMath_vs2010.lib
)