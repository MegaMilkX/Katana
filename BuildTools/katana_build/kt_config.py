import os

INCLUDE_PATHS=[
    'C:/MyDocuments/Projects/lib/glfw/include',
    'C:/MyDocuments/Projects/lib/rttr/src',
    'C:/MyDocuments/Projects/lib/assimp-master/include',
    'C:/MyDocuments/Projects/lib/assimp-master/build/include',
    'C:/MyDocuments/Projects/lib/bullet3-2.86.1/src'
]
LIB_PATHS=[
    'C:/MyDocuments/Projects/lib/glfw/lib',
    'C:/MyDocuments/Projects/lib/rttr/lib',
    'C:/MyDocuments/Projects/lib/assimp-master/build/code/Release',
    'C:/MyDocuments/Projects/lib/assimp-master/build/contrib/zlib/Release',
    'C:/MyDocuments/Projects/lib/bullet3-2.86.1/bin'
]

# Necessary tools
CMAKE_PATH="C:/Program Files/CMake/bin"
MSBUILD_PATH=""

# ==============
ENGINE_PATH = os.path.dirname(os.path.realpath(__file__))
ENGINE_PATH = os.path.abspath(os.path.join(ENGINE_PATH, os.pardir))
ENGINE_PATH = os.path.abspath(os.path.join(ENGINE_PATH, os.pardir))
ENGINE_PATH = ENGINE_PATH.replace('\\', '/')

BUILD_TOOLS_PATH = ENGINE_PATH + '/' + 'BuildTools'
BUILD_SCRIPTS_PATH = BUILD_TOOLS_PATH + '/katana_build'
ENGINE_SOURCE_PATH = ENGINE_PATH + '/' + 'Source'

# Misc.
PROJECT_EXT="katana"
PROJECT_TEMPLATE = BUILD_SCRIPTS_PATH + "/template/project.tpl"
CMAKELISTS_TEMPLATE = BUILD_SCRIPTS_PATH + "/template/CMakeLists.tpl"
REFRESH_PROJECT_PY_TPL = BUILD_SCRIPTS_PATH + "/template/script/refresh_project.py.tpl"
MAIN_CPP_TPL = BUILD_SCRIPTS_PATH + "/template/cpp/main.cpp.tpl"

VSCODE_TASKS_JSON_TPL = BUILD_SCRIPTS_PATH + "/template/vscode/tasks.json.tpl"
VSCODE_C_CPP_PROPERTIES_JSON_TPL = BUILD_SCRIPTS_PATH + "/template/vscode/c_cpp_properties.json.tpl"