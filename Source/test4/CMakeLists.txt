cmake_minimum_required (VERSION 2.6)
project (test)

foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

file(GLOB_RECURSE GENBINFILES ../common/gen/*[!.h])

foreach(gen_bin_file ${GENBINFILES})
	get_filename_component(DIR ${gen_bin_file} DIRECTORY)
	get_filename_component(FNAME ${gen_bin_file} NAME)
	execute_process(
		COMMAND "C:/MyDocuments/Projects/Aurora3/BuildTools/bin2header/bin2header.exe" ${FNAME}
		WORKING_DIRECTORY ${DIR}
	)
endforeach(gen_bin_file)

file(GLOB_RECURSE SRCFILES 
../common/*.cpp;
../common/*.c;
../common/*.cxx;
*.cpp;
*.c;
*.cxx)
add_executable(test ${SRCFILES})

target_include_directories(test PRIVATE 
	../../../lib/glfw/include
	../../../lib/rttr/src
	../../../lib/assimp-master/include
	../../../lib/assimp-master/build/include
	../../../lib/bullet3-2.86.1/src
)
target_link_directories(test PRIVATE 
	../../../lib/glfw/lib
	../../../lib/rttr/lib
	../../../lib/assimp-master/build/code/Release
	../../../lib/assimp-master/build/contrib/zlib/Release
	../../../lib/bullet3-2.86.1/bin
)
target_link_libraries(test 
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