import os
import shutil
import json

def makeDataArray(path):
  dataArray = []
  try:
    for files in os.listdir(path):
      dataArray.append(path+"/"+str(files))
  except:
    return -1
  return dataArray

def clientRun():
  print("Welcome to Nex.IO! We are working very hard to save idle resources\n")
  print("Before we get into the system, let us start by covering some basic steps")
  print("First you will have to give us a comma demilited string consisting of the paths to your executables")
  print("Make sure you give this to us in the order you want to run the executables (Do not put spaces)\n")
  exeStr = raw_input("Enter exec array: ")
  print("\n")
  exeArray = exeStr.split(",")
  print("Now you will need to give us the path to the folder consisting of all data-text files your 1st exec needs")
  textFilePath = raw_input("Enter path to data: ")
  print("\n")
  dataArray = makeDataArray(textFilePath)
  if(dataArray == -1):
    print("ERROR: Wrong path was entered, run the program again")
    return
  print("Finally give us a job name")
  jobName = raw_input("Enter job name: ")
  print("\n")
  try:
    os.makedirs("./clientJobs/"+jobName)
  except:
    print("ERROR: Job already exists")
    return

  for exeFile in exeArray:
    shutil.copy(exeFile, "./clientJobs/"+jobName+"/")

  for textFile in dataArray:
    shutil.copy(textFile, "./clientJobs/"+jobName+"/")

  textFileArray = []
  executableArray = []
  for files in os.listdir("./clientJobs/"+jobName+"/"):
    if(str(files).lower().endswith(".txt")):
      textFileArray.append(str(files))
    else:
      executableArray.append(str(files))

  jsonDict = dict()
  jsonDict["executableArray"] = executableArray
  jsonDict["textFileArray"] = textFileArray
  with open("./clientJobs/"+jobName+"/"+"metaData.txt", "w+") as myfile:
    json.dump(jsonDict, myfile)

  tarCommandToRun = "cd ./clientJobs && tar -cvf " + jobName + ".tar " +jobName
  os.system(tarCommandToRun)
  print("Compression complete, file: " + jobName + ".tar created\n")
  os.system("./capstoneClient")

clientRun()
