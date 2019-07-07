import os
import argparse
import subprocess
try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser  # ver. < 3.0

import kt_util
import kt_config

parser = argparse.ArgumentParser()
parser.add_argument('path', metavar='PROJECT_PATH', help='path to a project directory or a config file')
args = parser.parse_args()

projFilename = ""
projDir = ""

if os.path.isdir(args.path):
    projFilename = kt_util.findProjectFile(args.path)
    projDir = args.path
elif os.path.isfile(args.path):
    projFilename = args.path
    projDir = os.path.dirname(args.path)

config = ConfigParser()
config.read(projFilename)

srcDir = config.get("General", "source_dir")
srcDir = projDir + "/" + srcDir

subprocess.check_call(
    "cmake --build \"../cmake\" --config RelWithDebInfo", 
    cwd=srcDir
)
