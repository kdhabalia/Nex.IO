#include <Python.h>
#include <>

static PyObject* enqueuePacket (PyObject *self, PyObject *args) {

  int jobID;
  int exeID;
  const char* tempEP;
  const char* tempDP;
  int load;
  int workloadType;

  PyArg_ParseTuple(args, "issii", &jobID, &exeID, executablePath, dataPath, &load, &workloadType);

  WP = malloc(sizeof(struct Workload));
  WP->jobID = jobID;
  WP->executablePath = malloc(strlen(tempEP));
  memcpy(WP->executablePath, tempEP, strlen(tempEP));
  WP->dataPath = malloc(strlen(tempDP));
  memcpy(WP->dataPath, tempDP, strlen(tempDP));
  WP->load = load;
  WP->workloadType = workloadType;

  queueEnqueue(inQ, (void*)WP);

  return Py_BuildValue("i", 0);

}

static PyMethodDef enqueuePacketMethods[] = {
  {"enqueueWorkloadPacket",  enqueuePacket, METH_VARARGS, "Enqueue a workload packet into the main queue"},
  {NULL, NULL, 0, NULL}
};

void initEWP () {

  (void) Py_InitModule("EWP", enqueuePacketMethods);

}




