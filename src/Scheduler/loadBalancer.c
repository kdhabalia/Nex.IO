#include "loadBalancer.h"

Queue inQ;

int registeredDevices = 0;
pthread_mutex_t mLock = PTHREAD_MUTEX_INITIALIZER;

HardwareDevice* devices;

int numberOfDevices () {

  pthread_mutex_lock(&mLock);
  int numDevices = registeredDevices;
  pthread_mutex_unlock(&mLock);

  return numDevices;

}

HardwareDevice grabDevice (int index) {

  pthread_mutex_lock(&mLock);
  HardwareDevice H = devices[index];
  pthread_mutex_unlock(&mLock);

  return H;

}

float* deviceStats (int index) {

  HardwareDevice H = grabDevice(index);

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
  new->Q = queueInit();
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

  int numDevices = numberOfDevices();

  int* loads = malloc(numDevices*sizeof(int));
  for (int i = 0; i < numDevices; i++) {
    HardwareDevice H = grabDevice(i);
    Queue Q = H->Q;
    int load = queueLoad(Q, eval);
    loads[i] = load;
  }

  return loads;

}

float* hardwareDevicesUtilizations () {

  int numDevices = numberOfDevices();

  float* utilizations = malloc(numDevices*sizeof(float));

  for (int i = 0; i < numDevices; i++) {
    HardwareDevice H = grabDevice(i);
    pthread_rwlock_rdlock(&(H->lock));
    utilizations[i] = H->utilization;
    pthread_rwlock_unlock(&(H->lock));
  }

  return utilizations;

}

float* hardwareDevicesMemoryUsages () {

  int numDevices = numberOfDevices();

  float* memoryUsages = malloc(numDevices*sizeof(float));

  for (int i = 0; i < numDevices; i++) {
    HardwareDevice H = grabDevice(i);
    pthread_rwlock_rdlock(&(H->lock));
    memoryUsages[i] = H->memoryUsage;
    pthread_rwlock_unlock(&(H->lock));
  }

  return memoryUsages;

}

float* hardwareDevicesScores (int* queueLoads, float* hardwareUtilizations, float* hardwareMemoryUsages) {

  int numDevices = numberOfDevices();

  float* scores = malloc(numDevices*sizeof(float));

  for (int i = 0; i < numDevices; i++) {
    float queueLoad = (float)queueLoads[i];
    float hardwareUtilization = hardwareUtilizations[i];
    float hardwareMemoryUsage = hardwareMemoryUsages[i];
    float score = queueLoad * hardwareUtilization * hardwareMemoryUsage;
    scores[i] = score;
  }

  return scores;

}

int minimumUsedDevice (float* hardwareScores) {

  int numDevices = numberOfDevices();

  int minimumIndex = 0;
  float minimumScore = 0.0;
  for (int i = 0; i < numDevices; i++) {
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

  printf("In load balancer\n");
  printf("Number of available devices is %d\n", registeredDevices);
  printf("Number of items in inQ is %d\n", queueLength(inQ));
  printf("\n");

  while (1) {

    if (queueLength(inQ) != 0 && numberOfDevices() != 0) {
      printf("Transferring job\n");
      void* e = queueDequeue(inQ);
      printf("Removed job ID is %d\n", ((WorkloadPacket)e)->jobID);
      printf("Packet load is %d\n", ((WorkloadPacket)e)->load);
      printf("Packet exe path is %s\n", ((WorkloadPacket)e)->executablePath);
      printf("Packet data path is %s\n", ((WorkloadPacket)e)->dataPath);
      printf("Packet type is %d\n", ((WorkloadPacket)e)->workloadType);


      int* queueLoads = hardwareDevicesQueueLoads();
      printf("Queue loads for the devices is %d, %d\n", queueLoads[0], queueLoads[1]);
      float* hardwareUtilizations = hardwareDevicesUtilizations();
      printf("Hardware Utilizations for the devices is %f, %f\n", hardwareUtilizations[0], hardwareUtilizations[1]);
      float* hardwareMemoryUsages = hardwareDevicesMemoryUsages();
      printf("Hardware Memory Utilizations for the devices is %f, %f\n", hardwareMemoryUsages[0], hardwareMemoryUsages[1]);

      float* hardwareScores = hardwareDevicesScores(queueLoads, hardwareUtilizations, hardwareMemoryUsages);
      printf("Hardware load scores for the devices is %f, %f\n", hardwareScores[0], hardwareScores[1]);
      int minimumUsedHardwareDevice = minimumUsedDevice(hardwareScores);

      printf("Putting job into device %d\n", minimumUsedHardwareDevice);
      Queue outQ = (devices[minimumUsedHardwareDevice])->Q;
      queueEnqueue(outQ, e);
      printf("\n");
    }

  }

}

void* emptyHardwareQueue (HardwareDevice H) {

  pthread_rwlock_rdlock(&(H->lock));
  float capU = H->capUtilization;
  float capMU = H->capMemoryUsage;
  float U = H->utilization;
  float MU = H->memoryUsage;
  Queue Q = H->Q;
  pthread_rwlock_unlock(&(H->lock));

  if (U < capU && MU < capMU) {
    void* e = queueDequeue(Q);
    return e;
  }
  else {
    return NULL;
  }

}

int sizeOfFile (int fd) {

  struct stat S;
  fstat(fd, &S);
  int size = S.st_size;
  return size;

}

void addLaunchedData (HardwareDevice H, WorkloadPacket e) {

  pthread_rwlock_wrlock(&(H->lock));

  WorkloadPacket* newPackets = malloc((H->numLaunched+1) * sizeof(WorkloadPacket));
  for (int i = 0; i < H->numLaunched; i++) {
    newPackets[i] = H->launchedPackets[i];
  }

  H->numLaunched++;
  newPackets[H->numLaunched-1] = e;

  free(H->launchedPackets);

  H->launchedPackets = newPackets;

  pthread_rwlock_unlock(&(H->lock));

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

  free(exeData);
  free(textData);
  free(sendBuffer);

}

void sendToHardwareDevice (void* threadArgs) {

  struct senderArgs* args = (struct senderArgs*)threadArgs;
  HardwareDevice H = args->H;
  int sessfd = args->sessfd;

  while(1) {

    char* buf;
    char* sendBuffer;

    WorkloadPacket e = (WorkloadPacket)emptyHardwareQueue(H);

    if (e == NULL) {
      continue;
    }

    else {
      char* executablePath = e->executablePath;
      char* dataPath = e->dataPath;

      sendPacket(sessfd, e->jobID, e->exeID, e->executablePath, e->dataPath, e->workloadType);

      addLaunchedData(H, e);
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

  int* newPackets = malloc((H->numLaunched-1) * sizeof(WorkloadPacket));

  for (int i = 0; i < H->numLaunched; i++) {
    WorkloadPacket e = H->launchedPackets[i];

    if (e->jobID == jobID && e->exeID == exeID) {
      free(e->dataPath);
      free(e->executablePath);
      free(e);
      continue;
    }
    else {
      newPackets = e;
    }
  }

  H->numLaunched--;

  free(H->launchedPackets);

  H->launchedPackets = newPackets;

  pthread_rwlock_unlock(&(H->lock));

}

void receiveFromHardwareDevice (void* threadArgs) {

  struct receiverArgs* args = (struct receiverArgs*)threadArgs;

  HardwareDevice H = args->H;
  int sockfd = args->sockfd;

  while (1) {

    int bufSize;
    char* buf;
    int initialReceive[2];

    int functionality;

    // Text data
    int jobID;
    int exeID;
    int textNameSize;
    char* textName;
    int textDataSize;
    char* textData;

    // Hardware Utilization stats
    float utilization;
    float memoryUsage;
    float capMemoryUsage;
    float capUtilization;

    // First receive the intent from the client
    recv(sockfd, initialReceive, 2*sizeof(int), MSG_WAITALL);

    // Unmarshal the first two ints
    memcpy(&bufSize, &(initialReceive[0]), sizeof(int));
    memcpy(&functionality, &(initialReceive[1]), sizeof(int));

    switch (functionality) {
      // Receive a result file
      case (RESULT_FILE):
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

        // Write buf to an output file in filesystem

        free(buf);
        free(textName);
        free(textData);
        break;

      // Update statistical data about device
      case (HARDWARE_STATS):
        buf = malloc(4*sizeof(float));
        recv(sockfd, buf, 4*sizeof(float), MSG_WAITALL);

        memcpy(&utilization, buf, sizeof(float));
        memcpy(&memoryUsage, buf+sizeof(float), sizeof(float));
        memcpy(&capUtilization, buf+2*sizeof(float), sizeof(float));
        memcpy(&capMemoryUsage, buf+3*sizeof(float), sizeof(float));

        updateDeviceStats(H, capUtilization, capMemoryUsage, utilization, memoryUsage);

        free(buf);
        break;


    }

  }

}













