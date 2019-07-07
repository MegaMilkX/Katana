import os

import kt_util
import kt_config

def createCMakeLists(projName, srcDir):
    print(kt_config.ENGINE_PATH)
    replaceList = { 
        'PROJECT_NAME':projName, 
        'BUILD_TOOLS_DIR':kt_config.BUILD_TOOLS_PATH,
        'ENGINE_SRC_DIR':kt_config.ENGINE_SOURCE_PATH,
        'INCLUDE_PATHS':'\n\t'.join(kt_config.INCLUDE_PATHS),
        'LIB_PATHS':'\n\t'.join(kt_config.LIB_PATHS)
    }
    kt_util.createFileFromTemplate(kt_config.CMAKELISTS_TEMPLATE, srcDir, "CMakeLists.txt", replaceList)
