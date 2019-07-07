
import subprocess
import os

os.system('python $BUILD_TOOLS_PATH/katana_build/init_project.py ' + os.path.dirname(os.path.realpath(__file__)))

try:
    raw_input("Press enter to exit ;)")
except:
    input("Press enter to exit ;)")