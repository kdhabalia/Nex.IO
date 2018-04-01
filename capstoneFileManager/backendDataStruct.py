import os
from threading import Thread
from multiprocessing import Process, Manager
from pythonInterface import *

databaseDictionary = {}

def populateDictionary():
  global databaseDictionary
  directory = "../clientFileDatabase/"
  while(1):
    for folders in os.listdir(directory):
      if folders not in databaseDictionary:
        databaseDictionary[folders] = {}
        for execs in os.listdir(directory +"/" + folders):
          print(execs)
          for txtFiles in os.listdir(directory + "/" + folders + "/" + execs):
            print(txtFiles)
            if(txtFiles.lower().endswith(".txt")):
              print("HELLLHERE HOW ARE TOU")
              (databaseDictionary[folders])[txtFiles] = (execs, 0)
              print(databaseDictionary[folders])


def putTasksInQueue(granularity):
  global databaseDictionary
  while(1):
    lenOfDictionary = len(databaseDictionary)
    threadArray = []
    flag = 0
    if(lenOfDictionary > granularity):
      chunk = lenOfDictionary / granularity
      for i in xrange(granularity):
        T = Thread(target=setToRun, args=(i*chunk,(i+1)*chunk, databaseDictionary,))
        threadArray.append(T)
    else:
      flag = 1
      setToRun(0, lenOfDictionary, databaseDictionary)
    if(not flag):
      for i in range(granularity):
        threadArray[i].join()


def setToRun(startVal, tillVal, databaseDictionary):
  i = startVal
  while(i < tillVal):
    jobDict = databaseDictionary[str(i)]
    for keys, vals in jobDict.iteritems():
      if(vals[1] == 0):
        vals[1] = 1
        tempPath = "../clientFileDatabase/"+str(i)+"/"
        enqueWorkloadPacket(i, tempPath+str(vals[0]),tempPath+str(vals[0])+"/"+keys, 1,0)
    i+=1



def main():
  p1 = Thread(target=populateDictionary, args=())
  p2 = Thread(target=putTasksInQueue, args=())
  p1.start()
  p2.start()
  p1.join()
  p2.join()

main()

