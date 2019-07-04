import argparse
import os
import errno

try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser  # ver. < 3.0

import kt_config
import kt_util

parser = argparse.ArgumentParser()
parser.add_argument('path', metavar='DIRECTORY', help='path to a preferably empty directory')
args = parser.parse_args()

if not args:
	print("Failed to parse command arguments")
	raise

try:
	os.mkdir(args.path)
except OSError as ex:
	if ex.errno != errno.EEXIST:
		print("INFO | Project directory already exists. Ok.")
		raise
	pass

projFile = kt_util.findProjectFile(args.path)

if not projFile:
	print("INFO | Project config doesn't exist. Creating a default one...")
	tgtFilename = kt_util.createProjectFileFromTemplate(kt_config.PROJECT_TEMPLATE, args.path)
	print("INFO | Created a default project config.")
	print("INFO | Please edit '" + tgtFilename + "' and run init_project.py again")
else:
	print("INFO | Refreshing the project directory...")
	config = ConfigParser()
	config.read(projFile)

	srcDir = config.get("General", "source")
	srcDir = args.path + "/" + srcDir
	if not os.path.isdir(srcDir):
		os.mkdir(srcDir)
	replaceList = { 
		'PROJECT_NAME':os.path.splitext(os.path.basename(projFile))[0], 
		'BUILD_TOOLS_DIR':srcDir, # TODO
		'ENGINE_SRC_DIR':srcDir # TODO
	}
	kt_util.createFileFromTemplate(kt_config.CMAKELISTS_TEMPLATE, srcDir, "CMakeLists.txt", replaceList)

	cfgDir = config.get("General", "config")
	cfgDir = args.path + "/" + cfgDir
	if not os.path.isdir(cfgDir):
		os.mkdir(cfgDir)
	# TODO: Create input bindings xml if doesn't exist

	print("INFO | Done")
