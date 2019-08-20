import argparse
import os
import errno
import subprocess

try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser  # ver. < 3.0

import kt_config
import kt_util

import kt_cmake

parser = argparse.ArgumentParser()
parser.add_argument('path', metavar='PROJECT_PATH', help='path to a project directory')
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
projName = os.path.splitext(os.path.basename(projFile))[0]
projDir = args.path.replace('\\','/')

if not projFile:
	print("INFO | Project config doesn't exist. Creating a default one...")
	tgtFilename = kt_util.createProjectFileFromTemplate(kt_config.PROJECT_TEMPLATE, args.path)
	print("INFO | Created a default project config.")
	print("INFO | Please edit '" + tgtFilename + "' and run init_project.py again")
else:
	print("INFO | Refreshing the project directory...")
	config = ConfigParser()
	config.read(projFile)

	replaceList = {
		'BUILD_TOOLS_PATH':kt_config.BUILD_TOOLS_PATH
	}
	kt_util.createFileFromTemplate(
		kt_config.REFRESH_PROJECT_PY_TPL,
		projDir, 'refresh_project.py', replaceList
	)

	srcDir = config.get("General", "source_dir")
	srcDir = projDir + "/" + srcDir
	assetDir = config.get("General", "asset_dir")
	assetDir = projDir + "/" + assetDir
	
	if not os.path.isdir(srcDir):
		os.mkdir(srcDir)
	if not os.path.isdir(assetDir):
		os.mkdir(assetDir)
	if not os.path.isdir(srcDir + '/../cmake'):
		os.mkdir(srcDir + '/../cmake')
	if not os.path.isdir(srcDir + '/../.vscode'):
		os.mkdir(srcDir + '/../.vscode')
	
	# === CPP starter files ====
	replaceList = {
		'GAME_MODE_NAME':projName + '_MainMode'
	}
	kt_util.createFileFromTemplate(
		kt_config.MAIN_CPP_TPL,
		srcDir, 'main.cpp', replaceList, False
	)

	# === VSCODE ====
	replaceList = {
		'BUILD_SCRIPTS_PATH':kt_config.BUILD_SCRIPTS_PATH,
		'PROJECT_PATH':projDir
	}
	kt_util.createFileFromTemplate(
		kt_config.VSCODE_TASKS_JSON_TPL,
		projDir + "/.vscode", 'tasks.json', replaceList
	)
	
	INCLUDE_PATHS = kt_config.INCLUDE_PATHS
	INCLUDE_PATHS.append(kt_config.ENGINE_SOURCE_PATH + '/common')
	INCLUDE_PATHS.append(kt_config.ENGINE_SOURCE_PATH + '/katana')
	replaceList={
		'INCLUDE_PATHS':',\n'.join('"{0}"'.format(l) for l in kt_config.INCLUDE_PATHS)
	}
	kt_util.createFileFromTemplate(
		kt_config.VSCODE_C_CPP_PROPERTIES_JSON_TPL,
		projDir + '/.vscode', 'c_cpp_properties.json', replaceList
	)

	# === CMakeLists ====	
	kt_cmake.createCMakeLists(
		projName,
		srcDir
	)
	
	# === Configs ====
	cfgDir = config.get("General", "config_dir")
	cfgDir = projDir + "/" + cfgDir
	if not os.path.isdir(cfgDir):
		os.mkdir(cfgDir)
	# TODO: Create input bindings xml if doesn't exist

	# === Generate cmake stuff ====
	subprocess.check_call(
		"cmake -S \".\" -B \"../cmake\"",
		cwd=srcDir
	)

	print("INFO | Done")
