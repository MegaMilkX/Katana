import glob
from shutil import copyfile
from string import Template
import os
import kt_config

def listProjectFiles(dir):
    mask = "/*." + kt_config.PROJECT_EXT
    projectList = glob.glob(dir + mask)
    projectCount = len(projectList)
    if(projectCount == 0):
        print("No " + mask + " files found in working directory. Abort.")
        stop
    elif(projectCount > 1):
        print("Project directory should contain only one " + mask + " file")
        return

    return projectList

def findProjectFile(dir):
    mask = "/*." + kt_config.PROJECT_EXT
    projectList = glob.glob(dir + mask)
    projectCount = len(projectList)
    if(projectCount == 0):
        print("No " + mask + " files found in working directory.")
        return
    projectFileName = projectList[0]
    return projectFileName

def createFileFromTemplate(tplFilename, tgtDir, newFilename, replaceList, overwrite=True):
    if not overwrite:
        if os.path.isfile(tgtDir + "/" + newFilename):
            return
    f = open(tplFilename)
    src = Template(f.read())
    tgtF = open(tgtDir + "/" + newFilename, 'w+')
    tgtF.write(src.safe_substitute(replaceList))

def createProjectFileFromTemplate(tplFilename, tgtDir):
    projName = os.path.basename(tgtDir) # TODO: Said to work differently on unix
    tgtFilename = tgtDir + "/" + projName + "." + kt_config.PROJECT_EXT
    copyfile(tplFilename, tgtFilename)
    return tgtFilename
