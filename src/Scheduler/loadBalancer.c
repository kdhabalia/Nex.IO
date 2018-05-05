#include "loadBalancer.h"

Queue inQ;

int registeredDevices = 0;
HardwareDevice* devices;
pthread_mutex_t mLock = PTHREAD_MUTEX_INITIALIZER;

int numberOfDevices () {

  pthread_mutex_lock(&mLock);
  int numDevices = registeredDevices;
  pthread_mutex_unlock(&mLock);

  return numDevices;

}

HardwareDevice grabDevice (int index) {

  pthread_mutex_lock(&mLock);
  if (index >= registeredDevices) {
    pthread_mutex_unlock(&mLock);
    return NULL;
  }
  HardwareDevice H = devices[index];
  pthread_mutex_unlock(&mLock);

  return H;

}

float* deviceStats (int index) {

  HardwareDevice H = grabDevice(index);
  if (H == NULL) {
    return NULL;
  }

  float* returnArray = malloc(4*sizeof(float));

  pthread_rwlock_rdlock(&(H->lock));
  returnArray[0] = H->capUtilization;
  returnArray[1] = H->capMemoryUsage;
  returnArray[2] = H->utilization;
  returnArray[3] = H->memoryUsage;
  pthread_rwlock_unlock(&(H->lock));

  return returnArray;

}

int eval (void* e) {

  WorkloadPacket element = (WorkloadPacket)e;
  int load = element->load;
  return load;

}

HardwareDevice registerDevice (float capUtilization, float capMemoryUsage, float utilization, float memoryUsage) {

  pthread_mutex_lock(&mLock);

  HardwareDevice* newDevices = malloc((registeredDevices+1)*sizeof(HardwareDevice));
  for (int i = 0; i < registeredDevices; i++) {
    newDevices[i] = devices[i];
  }

  HardwareDevice new = malloc(sizeof(struct Device));
  pthread_rwlock_init(&(new->lock), NULL);
  new->ID = registeredDevices;
  new->Q = queueInit(MAX_QUEUE_SIZE);
  new->numLaunched = 0;
  new->launchedPackets = NULL;
  new->memoryUsage = memoryUsage;
  new->utilization = utilization;
  new->capMemoryUsage = capMemoryUsage;
  new->capUtilization = capUtilization;

  registeredDevices++;
  newDevices[registeredDevices-1] = new;
  free(devices);
  devices = newDevices;

  pthread_mutex_unlock(&mLock);

  return new;

}

int* hardwareDevicesQueueLoads () {

  int* loads = malloc(registeredDevices*sizeof(int));
  for (int i = 0; i < registeredDevices; i++) {
    HardwareDevice H = devices[i];
    Queue Q = H->Q;
    int load = queueLoad(Q, eval);
    loads[i] = load;
  }

  return loads;

}

float* hardwareDevicesUtilizations () {

  float* utilizations = malloc(registeredDevices*sizeof(float));

  for (int i = 0; i < registeredDevices; i++) {
    HardwareDevice H = devices[i];
    pthread_rwlock_rdlock(&(H->lock));
    utilizations[i] = H->utilization;
    pthread_rwlock_unlock(&(H->lock));
  }

  return utilizations;

}

float* hardwareDevicesMemoryUsages () {

  float* memoryUsages = malloc(registeredDevices*sizeof(float));

  for (int i = 0; i < registeredDevices; i++) {
    HardwareDevice H = devices[i];
    pthread_rwlock_rdlock(&(H->lock));
    memoryUsages[i] = H->memoryUsage;
    pthread_rwlock_unlock(&(H->lock));
  }

  return memoryUsages;

}

float* hardwareDevicesScores (int* queueLoads, float* hardwareUtilizations, float* hardwareMemoryUsages) {

  float* scores = malloc(registeredDevices*sizeof(float));

  for (int i = 0; i < registeredDevices; i++) {
    float queueLoad = (float)queueLoads[i];
    float hardwareUtilization = hardwareUtilizations[i];
    float hardwareMemoryUsage = hardwareMemoryUsages[i];
    float score = queueLoad * hardwareUtilization * hardwareMemoryUsage;
    scores[i] = score;
  }

  return scores;

}

int minimumUsedDevice (float* hardwareScores) {

  int minimumIndex = 0;
  float minimumScore = 0.0;
  for (int i = 0; i < registeredDevices; i++) {
    if (i == 0) {
      minimumIndex = 0;
      minimumScore = hardwareScores[i];
    }
    else if (hardwareScores[i] < minimumScore) {
      minimumIndex = i;
      minimumScore = hardwareScores[i];
    }
  }

  return minimumIndex;

}

void balanceLoads (void* threadArgs) {

  Pthread_detach(pthread_self());

  while (1) {

    if (queueLength(inQ) > 0 && numberOfDevices() > 0) {
      pthread_mutex_lock(&mLock);

      printf("LB: %d devices available\n", registeredDevices);
      printf("LB: %d packets available\n", queueLength(inQ));

      void* e = queueDequeue(inQ);

      int* queueLoads = hardwareDevicesQueueLoads();
      float* hardwareUtilizations = hardwareDevicesUtilizations();
      float* hardwareMemoryUsages = hardwareDevicesMemoryUsages();

      float* hardwareScores = hardwareDevicesScores(queueLoads, hardwareUtilizations, hardwareMemoryUsages);
      int minimumUsedHardwareDevice = minimumUsedDevice(hardwareScores);

      Queue outQ = (devices[minimumUsedHardwareDevice])->Q;
      while (1) {
        int rv = queueEnqueue(outQ, e);
        if (rv == 0) {
          break;
        }
      }

      pthread_mutex_unlock(&mLock);
    }

  }

}

void addLaunchedData (HardwareDevice H, WorkloadPacket e) {

  WorkloadPacket* newPackets = malloc((H->numLaunched+1) * sizeof(WorkloadPacket));
  for (int i = 0; i < H->numLaunched; i++) {
    newPackets[i] = H->launchedPackets[i];
  }

  H->numLaunched++;
  newPackets[H->numLaunched-1] = e;

  free(H->launchedPackets);

  H->launchedPackets = newPackets;

}

void* emptyHardwareQueue (HardwareDevice H) {

  pthread_rwlock_wrlock(&(H->lock));
  float capU = H->capUtilization;
  float capMU = H->capMemoryUsage;
  float U = H->utilization;
  float MU = H->memoryUsage;
  Queue Q = H->Q;

  if (U < capU && MU < capMU) {
    void* e = queueDequeue(Q);
    if (e != NULL) {
      addLaunchedData(H, e);
    }
    pthread_rwlock_unlock(&(H->lock));
    return e;
  }
  else {
    pthread_rwlock_unlock(&(H->lock));
    return NULL;
  }

}

int sizeOfFile (int fd) {

  struct stat S;
  fstat(fd, &S);
  int size = S.st_size;
  return size;

}

void sendPacket (int sessfd, int jobID, int  exeID, char* executablePath, char* dataPath, int workloadType) {

  int fd;

  // Read executable data
  fd = open(executablePath, O_RDONLY);
  int exeNameSize = strlen(executablePath);
  int exeDataSize = sizeOfFile(fd);
  char* exeData = malloc(exeDataSize);
  read(fd, exeData, exeDataSize);
  close(fd);

  // Read text file data
  fd = open(dataPath, O_RDONLY);
  int dataNameSize = strlen(dataPath);
  int textDataSize = sizeOfFile(fd);
  char* textData = malloc(textDataSize);
  read(fd, textData, textDataSize);
  close(fd);

  int numTextFiles = 1;

  // Create send buffer
  int sendBufferSize = 8*sizeof(int) + exeNameSize + dataNameSize + exeDataSize + textDataSize;
  char* sendBuffer = malloc(sendBufferSize);
  int sizeOfArray = sendBufferSize - sizeof(int);

  // Put all data into send buffer (marshal)
  memcpy(sendBuffer, &sizeOfArray, sizeof(int));
  memcpy(sendBuffer+sizeof(int), &jobID, sizeof(int));
  memcpy(sendBuffer+2*sizeof(int), &exeID, sizeof(int));
  memcpy(sendBuffer+3*sizeof(int), &exeNameSize, sizeof(int));
  memcpy(sendBuffer+4*sizeof(int), executablePath, exeNameSize);
  memcpy(sendBuffer+4*sizeof(int)+exeNameSize, &exeDataSize, sizeof(int));
  memcpy(sendBuffer+5*sizeof(int)+exeNameSize, exeData, exeDataSize);
  memcpy(sendBuffer+5*sizeof(int)+exeNameSize+exeDataSize, &numTextFiles, sizeof(int));
  memcpy(sendBuffer+6*sizeof(int)+exeNameSize+exeDataSize, &dataNameSize, sizeof(int));
  memcpy(sendBuffer+7*sizeof(int)+exeNameSize+exeDataSize, dataPath, dataNameSize);
  memcpy(sendBuffer+7*sizeof(int)+exeNameSize+exeDataSize+dataNameSize, &textDataSize, sizeof(int));
  memcpy(sendBuffer+8*sizeof(int)+exeNameSize+exeDataSize+dataNameSize, textData, textDataSize);

  // Send data
  send(sessfd, sendBuffer, sendBufferSize, 0);

  printf("SN: Sent packet for job: %d and exe: %d\n", jobID, exeID);

  free(exeData);
  free(textData);
  free(sendBuffer);

}

void sendToHardwareDevice (void* threadArgs) {

  Pthread_detach(pthread_self());
  int oldState[1];
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, oldState);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, oldState);

  struct senderArgs* args = (struct senderArgs*)threadArgs;
  HardwareDevice H = args->H;
  int sessfd = args->sessfd;

  while(1) {

    char* buf;
    char* sendBuffer;

    WorkloadPacket e = (WorkloadPacket)emptyHardwareQueue(H);

    if (e != NULL) {
      printf("SN: Sending packet for job: %d and exe: %d\n", e->jobID, e->exeID);
      sendPacket(sessfd, e->jobID, e->exeID, e->executablePath, e->dataPath, e->workloadType);
    }

  }

}

void updateDeviceStats (HardwareDevice H, float capUtilization, float capMemoryUsage, float utilization, float memoryUsage) {

  pthread_rwlock_wrlock(&(H->lock));
  H->capUtilization = capUtilization;
  H->capMemoryUsage = capMemoryUsage;
  H->utilization = utilization;
  H->memoryUsage = memoryUsage;
  pthread_rwlock_unlock(&(H->lock));

}

void removeLaunchedData (HardwareDevice H, int jobID, int exeID) {

  pthread_rwlock_wrlock(&(H->lock));

  WorkloadPacket* newPackets = malloc((H->numLaunched-1) * sizeof(WorkloadPacket));

  for (int i = 0; i < H->numLaunched; i++) {
    WorkloadPacket e = H->launchedPackets[i];

    if (e->jobID == jobID && e->exeID == exeID) {
      free(e->dataPath);
      free(e->executablePath);
      free(e);
    }
    else {
      newPackets[i] = e;
    }
  }

  H->numLaunched--;

  free(H->launchedPackets);

  H->launchedPackets = newPackets;

  pthread_rwlock_unlock(&(H->lock));

}

void unregisterDevice (HardwareDevice H, pthread_t sendNodeWorker) {

  pthread_mutex_lock(&mLock);
  pthread_rwlock_wrlock(&(H->lock));

  pthread_cancel(sendNodeWorker);

  while (1) {
    WorkloadPacket e = queueDequeue(H->Q);
    if (e == NULL) {
      break;
    }
    else {
      while (1) {
        int rv = queueEnqueue(inQ, (void*)e);
        if (rv == 0) {
          break;
        }
      }
    }
  }

  queueFree(H->Q);

  for (int i = 0; i < H->numLaunched; i++) {
    WorkloadPacket e = H->launchedPackets[i];
    while (1) {
      int rv = queueEnqueue(inQ, (void*)e);
      if (rv == 0) {
        break;
      }
    }
  }

  free(H->launchedPackets);

  HardwareDevice* newDevices = malloc((registeredDevices-1)*sizeof(HardwareDevice));
  for (int i = 0; i < registeredDevices; i++) {
    HardwareDevice current = devices[i];
    if (current->ID == H->ID) {
      continue;
    }
    else {
      newDevices[i] = current;
    }
  }

  registeredDevices--;
  free(devices);
  devices = newDevices;

  pthread_rwlock_unlock(&(H->lock));
  pthread_mutex_unlock(&mLock);

  free(H);

}

void receiveFromHardwareDevice (void* threadArgs) {

  Pthread_detach(pthread_self());

  struct receiverArgs* args = (struct receiverArgs*)threadArgs;

  HardwareDevice H = args->H;
  int sockfd = args->sockfd;
  pthread_t sendNodeWorker = args->sendNodeWorker;

  while (1) {

    int bufSize;
    char* buf;
    char initialReceive[2*sizeof(int)];

    int functionality;

    // Text data
    int jobID;
    int exeID;
    int textNameSize;
    char* textName;
    int textDataSize;
    char* textData;

    // Hardware Utilization stats
    int utilization;
    int memoryUsage;
    int capMemoryUsage;
    int capUtilization;

    // First receive the intent from the client
    recv(sockfd, initialReceive, 2*sizeof(int), MSG_WAITALL);

    // Unmarshal the first two ints
    memcpy(&functionality, initialReceive, sizeof(int));
    memcpy(&bufSize, initialReceive+sizeof(int), sizeof(int));

    printf("RN: Received data from device with functionality: %d\n", functionality);

    switch (functionality) {
      // Receive a result file
      case (RESULT_FILE):
        printf("RN: Receiving result file\n");
        buf = malloc(bufSize);
        recv(sockfd, buf, bufSize, MSG_WAITALL);

        memcpy(&jobID, buf, sizeof(int));
        memcpy(&exeID, buf+sizeof(int), sizeof(int));
        memcpy(&textNameSize, buf+2*sizeof(int), sizeof(int));
        textName = malloc(textNameSize);
        memcpy(textName, buf+3*sizeof(int), textNameSize);
        memcpy(&textDataSize, buf+3*sizeof(int)+textNameSize, sizeof(int));
        textData = malloc(textDataSize);
        memcpy(textData, buf+4*sizeof(int)+textNameSize, textDataSize);

        removeLaunchedData(H, jobID, exeID);

        printf("RN: Received result file for job: %d and exe: %d\n", jobID, exeID);

        // Write buf to an output file in filesystem

        free(buf);
        free(textName);
        free(textData);
        break;

      // Update statistical data about device
      case (HARDWARE_STATS):
        printf("RN: Receiving hardware stats\n");
        buf = malloc(4*sizeof(int));
        recv(sockfd, buf, 4*sizeof(int), MSG_WAITALL);

        memcpy(&utilization, buf, sizeof(int));
        memcpy(&memoryUsage, buf+sizeof(int), sizeof(int));
        memcpy(&capUtilization, buf+2*sizeof(int), sizeof(int));
        memcpy(&capMemoryUsage, buf+3*sizeof(int), sizeof(int));

        printf("RN: Got these hardware stats: %d, %d, %d, %d\n", utilization, memoryUsage, capUtilization, capMemoryUsage);

        updateDeviceStats(H, (float)capUtilization, (float)capMemoryUsage, (float)utilization, (float)memoryUsage);

        printf("RN: Received hardware stats\n");

        free(buf);
        break;

      case (UNREGISTER):
        printf("RN: Unregistering device\n");
        unregisterDevice(H, sendNodeWorker);
        printf("RN: Unregistered device, exiting\n");
        pthread_exit(NULL);
        break;
    }

  }

}













