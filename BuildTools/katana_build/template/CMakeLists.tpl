cmake_minimum_required (VERSION 2.6)
project ($PROJECT_NAME)

if(MSVC)
  add_definitions(/MP)
  add_definitions(/incremental)
  add_definitions(/Debug:fastlink)
endif()

foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

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
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/../build/debug"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../build/debug"
)

set_target_properties(
	${PROJECT_NAME}_editor PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/../build/debug"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/../build/release"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../build/debug"
)

target_include_directories($PROJECT_NAME PRIVATE 
	$ENGINE_SRC_DIR/common/
	$ENGINE_SRC_DIR/katana/
	$INCLUDE_PATHS
)
target_link_directories($PROJECT_NAME PRIVATE 
	$LIB_PATHS
)
target_link_libraries($PROJECT_NAME 
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

target_include_directories(${PROJECT_NAME}_editor PRIVATE 
	$ENGINE_SRC_DIR/common/
	$INCLUDE_PATHS
)
target_link_directories(${PROJECT_NAME}_editor PRIVATE 
	$LIB_PATHS
)
target_link_libraries(${PROJECT_NAME}_editor
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

target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
target_compile_definitions(${PROJECT_NAME}_editor PRIVATE NOMINMAX)

