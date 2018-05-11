import os
import tarfile
import os
import shutil
import json

jobID = 0
directoryToCheck = "../Storage/"
databaseDirectory = "../Database/"

def makeFileDirectory():
	setOfFiles = set()
	originalLength = 0
	while(1):
		for tarFile in os.listdir(directoryToCheck):
			sTarFile = str(tarFile)
			if(sTarFile not in setOfFiles and (sTarFile.lower().endswith(".tar"))):
				setOfFiles.add(str(tarFile))
				indexOfEnd = sTarFile.find(".tar")
				nameOfFile = sTarFile[:indexOfEnd]
				untarFile(directoryToCheck + sTarFile)
				makeDatabaseEntry(nameOfFile)



def makeDatabaseEntry(nameOfFile):
  global jobID
  pathToMake = databaseDirectory + str(jobID)
  os.mkdir(pathToMake)
  (exeArray, textArray) = parseTextFile(directoryToCheck+nameOfFile+"/"+"metaData.txt")
  firstFileToMake = str(exeArray[0])

  for i in xrange(len(exeArray)+1):
    os.mkdir(pathToMake + "/" + str(i))

  for textFile in os.listdir(directoryToCheck+nameOfFile):
    if textFile in textArray:
      shutil.move(directoryToCheck+nameOfFile+"/"+textFile, pathToMake+"/"+"0/")

  for i in xrange(len(exeArray)):
    shutil.move(directoryToCheck+nameOfFile+"/"+str(exeArray[i]), pathToMake+"/"+str(i)+"/")
  os.remove(directoryToCheck+nameOfFile+".tar")
  shutil.rmtree(directoryToCheck+nameOfFile)
  jobID += 1


def parseTextFile(path):
	with open(path, "r") as myfile:
		datastore = json.load(myfile)
		exeArray = datastore["executableArray"]
		textArray = datastore["textFileArray"]
	return (exeArray, textArray)


def untarFile(path):
  untarCmd = "tar -C" + directoryToCheck + " -xvf " + path
  os.system(untarCmd)

makeFileDirectory()
