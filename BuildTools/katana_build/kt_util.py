import glob
from shutil import copyfile
from string import Template
import os
import kt_config

def copyDirNoReplace(src, dst):
    if not os.path.isdir(dst):
        os.mkdir(dst)
    
    src_list = os.listdir(src)
    dst_list = os.listdir(dst)
    for fname in src_list:
        full_fname = os.path.join(src, fname)
        full_dst_fname = os.path.join(dst, fname)
        if os.path.isfile(full_fname):
            if fname in dst_list:
                continue
            if not os.path.isfile(full_dst_fname): # Don't replace existing files
                copyfile(full_fname, full_dst_fname)
        elif os.path.isdir(full_fname):
            copyDirNoReplace(full_fname, full_dst_fname)

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
