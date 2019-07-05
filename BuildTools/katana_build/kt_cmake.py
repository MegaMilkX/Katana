import os

import kt_util
import kt_config

def createCMakeLists(projName, srcDir):
    enginePath = os.path.dirname(os.path.realpath(__file__))
    enginePath = os.path.abspath(os.path.join(enginePath, os.pardir))
    enginePath = os.path.abspath(os.path.join(enginePath, os.pardir))
    enginePath = enginePath.replace('\\', '/')
    print(enginePath)
    replaceList = { 
        'PROJECT_NAME':projName, 
        'BUILD_TOOLS_DIR':enginePath + '/' + "BuildTools", # TODO
        'ENGINE_SRC_DIR':enginePath + '/' + 'Source'
    }
    kt_util.createFileFromTemplate(kt_config.CMAKELISTS_TEMPLATE, srcDir, "CMakeLists.txt", replaceList)
