import os
from threading import Thread
from multiprocessing import Process, Manager

def populateDictionary(databaseDictionary):
  directory = "../clientFileDatabase/"
  while(1):
    for folders in os.listdir(directory):
      if folders not in databaseDictionary:
        databaseDictionary[folders] = {}
        for execs in os.listdir(directory +"/" + folders):
          for txtFiles in os.listdir(directory + "/" + folders + "/" + execs):
            dataDictionary[folders][txtFiles] = (execs, 0)


def putTasksInQueue(databaseDictionary, granularity):
  while(1):
    lenOfDictionary = len(databaseDictionary)
    threadArray = []
    if(lenOfDictionary > granularity):
      chunk = lenOfDictionary / granularity
      for i in xrange(granularity):
        T = Thread(target=setToRun, args=(i*chunk,(i+1)*chunk, databaseDictionary,))
        threadArray.append(T)
    else:
      setToRun(0, lenOfDictionary, databaseDictionary)

    for i in range(granularity):
      threadArray[i].join()


def setToRun(startVal, tillVal, databaseDictionary):
  i = startVal
  while(i < tillVal):
    jobDict = databaseDictionary[str(i)]
    for keys, vals in jobsDict.iteritems():
      if(vals[1] == 0):
        vals[1] = 1
        #ENQUEU SHIT HERE
    i+=1



if __name__ = '__main__':
  manager = Manager()
  databaseDictionary = manager.dict()
  p1 = Process(target=populateDictionary, args=(databaseDictionary,))
  p2 = Process(targer=populateDictionary, args=(databaseDictionary,))
  p1.start()
  p2.start()
  p1.join()
  p2.join()

