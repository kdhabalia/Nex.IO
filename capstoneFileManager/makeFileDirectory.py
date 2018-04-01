import os
import tarfile
import os
import shutil
import json

jobID = 0

def makeFileDirectory():
	directoryToCheck = "../clientTempStore/"
	setOfFiles = {[]}
	originalLength = 0
	while(1):
		for tarFile in os.listdir(directoryToCheck):
			sTarFile = str(tarFile)
			if(sTarFile not in setOfFiles and (sTarFile.lower().endswith(".tar"))):
				setOfFiles.add(str(tarFile))
				indexOfEnd = sTarFile.find(".tar")
				nameOfFile = sTarFile[:indexOfEnd]
				untarFile(directoryToCheck + "/" + sTarFile)
				makeDatabaseEntry(nameOfFile)



def makeDatabaseEntry(nameOfFile):
	databaseDirectory = "../clientFileDatabase/"
	pathToMake = "../clientFileDatabase/" + str(jobID)
	os.mkdir(pathToMake)
	(exeArray, textArray) = parseTextFile("../clientTempStore/"+nameOfFile+"/"+"metaData.txt")
	firstFileToMake = str(exeArray[0])

	for i in xrange(len(exeArray)):
		os.mkdir(pathToMake + "/" + str(exeArray[i]))

	for textFile in os.listdir("../clientTempStore/"+nameOfFile):
		if textFile in textArray:
			shutil.move("../clientTempStore/"+nameOfFile+"/"+textFile, pathToMake+"/"+str(exeArray[0])+"/")

	for i in xrange(len(exeArray)):
		shutil.move("../clientTempStore/"+nameOfFile+"/"+str(exeArray[i]), pathToMake+"/"+str(exeArray[i])+"/")
	jobID += 1


def parseTextFile(path):
	with open(path, "r") as myfile:
		datastore = json.load(myfile)
		exeArray = datastore["executableArray"]
		textArray = datastore["textFileArray"]
	return (exeArray, textArray)


def untarFile(path):
	tar = tarfile.open(path)
	tar.extractall()
	tar.close()