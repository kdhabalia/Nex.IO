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
          printf("LB: Enqueued packet into Hardware Device: %d\n", (devices[minimumUsedHardwareDevice])->ID);
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

char* stripFilename (char* filename, int textFile) {

  int l;
  int i;

  l = strlen(filename);
  i = 0;
  int lastOccurence = 0;
  while (i < l) {
    if (filename[i] == '/') {
      lastOccurence = i;
    }
    i++;
  }

  char* stripped;

  if (lastOccurence > 0) {
    stripped = malloc(l-lastOccurence);
    memcpy(stripped, filename+lastOccurence+1, l-lastOccurence);
    free(filename);
  }
  else {
    stripped = filename;
  }

  if (!textFile) {
    return stripped;
  }

  int digits = 0;
  for (int j = 0; j < strlen(stripped); j++) {
    if (isdigit(stripped[j])) {
      digits++;
    }
  }

  l = strlen(stripped)-digits+1;
  char* noDigits = malloc(l);
  i = 0;
  for (int j = 0; j < strlen(stripped); j++) {
    if (isdigit(stripped[j])) {
      continue;
    }
    else {
      noDigits[i] = stripped[j];
      i++;
    }
  }

  noDigits[l-1] = '\0';
  free(stripped);

  return noDigits;

}

int numberRecombineFiles (char* dataPath) {

  struct dirent *de;

  DIR *dr = opendir(dataPath);
  if (dr == NULL) {
    printf("SN: Unable to open data path directory for recombine step\n");
  }

  int num = 0;
  while ((de = readdir(dr)) != NULL) {
    char* ret = strstr(de->d_name, ".txt");
    if (ret != NULL) {
      num++;
    }
  }

  closedir(dr);

  return num;

}

void sendPacket (int sessfd, int jobID, int  exeID, char* executablePath, char* dataPath, int workloadType) {

  int fd;
  int* textDataSizes;
  int* textNameSizes;
  char** textNameArray;
  char** textDataArray;
  int numTextFiles;

  char* exePath = malloc(strlen(executablePath)+1);
  strcpy(exePath, executablePath);
  char* dPath = malloc(strlen(dataPath)+1);
  strcpy(dPath, dataPath);

  // Read executable data
  fd = open(exePath, O_RDONLY);
  int exeDataSize = sizeOfFile(fd);
  char* exeData = malloc(exeDataSize);
  read(fd, exeData, exeDataSize);
  close(fd);
  exePath = stripFilename(exePath, 0);
  int exeNameSize = strlen(exePath);

  if (workloadType == MAP) {
    printf("SN: Sending a map job\n");
    // Read text file data
    fd = open(dPath, O_RDONLY);
    int textDataSize = sizeOfFile(fd);
    char* textData = malloc(textDataSize);
    read(fd, textData, textDataSize);
    close(fd);

    dPath = stripFilename(dPath, 1);
    int textNameSize = strlen(dPath);

    textDataSizes = malloc(sizeof(int));
    textNameSizes = malloc(sizeof(int));
    textNameArray = malloc(sizeof(char*));
    textDataArray = malloc(sizeof(char*));

    textDataSizes[0] = textDataSize;
    textNameSizes[0] = textNameSize;
    textNameArray[0] = dPath;
    textDataArray[0] = textData;

    numTextFiles = 1;
  }
  else {
    printf("SN: Sending a recombine job\n");
    numTextFiles = numberRecombineFiles(dPath);
    textDataSizes = malloc(numTextFiles*sizeof(int));
    textNameSizes = malloc(numTextFiles*sizeof(int));
    textNameArray = malloc(numTextFiles*sizeof(char*));
    textDataArray = malloc(numTextFiles*sizeof(char*));

    struct dirent *de;

    DIR *dr = opendir(dPath);
    if (dr == NULL) {
      printf("SN: Unable to open data path directory for recombine step\n");
    }

    int i = 0;
    while ((de = readdir(dr)) != NULL) {
      char* ret = strstr(de->d_name, ".txt");
      if (ret != NULL) {
        char* textName = malloc(strlen(de->d_name)+1);
        char* fullName = malloc(strlen(de->d_name)+strlen(dPath)+1);
        strcpy(textName, de->d_name);
        strcpy(fullName, dPath);
        strcat(fullName, textName);

        fd = open(fullName, O_RDONLY);
        int textDataSize = sizeOfFile(fd);
        char* textData = malloc(textDataSize);
        read(fd, textData, textDataSize);
        close(fd);

        textDataSizes[i] = textDataSize;
        textNameSizes[i] = strlen(textName);
        textNameArray[i] = textName;
        textDataArray[i] = textData;

        free(fullName);
        i++;
      }
    }

    free(dPath);
    closedir(dr);
  }

  // Create send buffer
  int sendBufferSize = 6*sizeof(int) + exeNameSize + exeDataSize;
  for (int i = 0; i < numTextFiles; i++) {
    sendBufferSize += 2*sizeof(int);
    sendBufferSize += textDataSizes[i];
    sendBufferSize += textNameSizes[i];
  }
  char* sendBuffer = malloc(sendBufferSize);
  int sizeOfArray = sendBufferSize - sizeof(int);

  // Put all data into send buffer (marshal)
  memcpy(sendBuffer, &sizeOfArray, sizeof(int));
  memcpy(sendBuffer+sizeof(int), &jobID, sizeof(int));
  memcpy(sendBuffer+2*sizeof(int), &exeID, sizeof(int));
  memcpy(sendBuffer+3*sizeof(int), &exeNameSize, sizeof(int));
  memcpy(sendBuffer+4*sizeof(int), exePath, exeNameSize);
  memcpy(sendBuffer+4*sizeof(int)+exeNameSize, &exeDataSize, sizeof(int));
  memcpy(sendBuffer+5*sizeof(int)+exeNameSize, exeData, exeDataSize);
  memcpy(sendBuffer+5*sizeof(int)+exeNameSize+exeDataSize, &numTextFiles, sizeof(int));

  int byteOffset = 0;
  for (int i = 0; i < numTextFiles; i++) {
    memcpy(sendBuffer+6*sizeof(int)+exeNameSize+exeDataSize+byteOffset, &(textNameSizes[i]), sizeof(int));
    memcpy(sendBuffer+7*sizeof(int)+exeNameSize+exeDataSize+byteOffset, textNameArray[i], textNameSizes[i]);
    memcpy(sendBuffer+7*sizeof(int)+exeNameSize+exeDataSize+textNameSizes[i]+byteOffset, &(textDataSizes[i]), sizeof(int));
    memcpy(sendBuffer+8*sizeof(int)+exeNameSize+exeDataSize+textNameSizes[i]+byteOffset, textDataArray[i], textDataSizes[i]);
    byteOffset += 2*sizeof(int) + textNameSizes[i] + textDataSizes[i];
  }

  // Send data
  int rv = send(sessfd, sendBuffer, sendBufferSize, 0);
  if (rv == -1) {
    printf("SN: Unable to send workload to device, errno: %d\n", errno);
    return;
  }

  printf("SN: Sent packet for job: %d and exe: %d\n", jobID, exeID);

  for (int i = 0; i < numTextFiles; i++) {
    free(textNameArray[i]);
    free(textDataArray[i]);
  }

  free(textNameArray);
  free(textDataArray);
  free(textDataSizes);
  free(textNameSizes);
  free(exePath);
  free(exeData);
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

  if (H->numLaunched == 0) {
    pthread_rwlock_unlock(&(H->lock));
    return;
  }

  WorkloadPacket* newPackets = malloc((H->numLaunched-1) * sizeof(WorkloadPacket));

  int p = 0;
  for (int i = 0; i < H->numLaunched; i++) {
    WorkloadPacket e = H->launchedPackets[i];

    if (e->jobID == jobID && e->exeID == exeID) {
      free(e->dataPath);
      free(e->executablePath);
      free(e);
    }
    else {
      newPackets[p] = e;
      p++;
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
  int p = 0;
  for (int i = 0; i < registeredDevices; i++) {
    HardwareDevice current = devices[i];
    if (current->ID == H->ID) {
      continue;
    }
    else {
      newDevices[p] = current;
      p++;
    }
  }

  registeredDevices--;
  free(devices);
  devices = newDevices;

  pthread_rwlock_unlock(&(H->lock));
  pthread_mutex_unlock(&mLock);

  free(H);

}

void writeResultFile (char* textName, char* textData, int textDataSize, int jobID, int exeID) {

  char* prefix = "../Database/";
  char* backslash = "/";

  char* jobNum = malloc(BUFSIZE);
  itoa(jobID, jobNum, 10);
  strcat(jobNum, backslash);

  char* exeNum = malloc(BUFSIZE);
  itoa((exeID+1), exeNum, 10);
  strcat(exeNum, backslash);

  char* fullname = malloc(strlen(textName)+strlen(jobNum)+strlen(exeNum)+strlen(prefix)+1);
  strcpy(fullname, prefix);
  strcat(fullname, jobNum);
  strcat(fullname, exeNum);
  strcat(fullname, textName);

  int fd = open(fullname, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  write(fd, textData, textDataSize);
  close(fd);

  printf("RN: Wrote result file to Database/%d/%d\n", jobID, exeID+1);

  free(jobNum);
  free(exeNum);
  free(fullname);

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

    int functionality = -1;

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
    int rv;
    rv = recv(sockfd, initialReceive, 2*sizeof(int), MSG_WAITALL);
    if (rv == -1) {
      unregisterDevice(H, sendNodeWorker);
      printf("RN: Unable to receive intent from device, errno: %d\n", errno);
      return;
    }

    // Unmarshal the first two ints
    memcpy(&functionality, initialReceive, sizeof(int));
    memcpy(&bufSize, initialReceive+sizeof(int), sizeof(int));
    if (bufSize <= 0) {
      printf("RN: Received bad buf size of %d\n", bufSize);
    }

    printf("RN: Received data from device with functionality: %d\n", functionality);

    switch (functionality) {
      // Receive a result file
      case (RESULT_FILE):
        printf("RN: Receiving result file\n", bufSize);
        buf = malloc(bufSize);
        rv = recv(sockfd, buf, bufSize, MSG_WAITALL);
        if (rv == -1) {
          unregisterDevice(H, sendNodeWorker);
          free(buf);
          printf("RN: Unable to receive result file from device, errno: %d\n", errno);
          return;
        }

        memcpy(&jobID, buf, sizeof(int));
        memcpy(&exeID, buf+sizeof(int), sizeof(int));
        memcpy(&textNameSize, buf+2*sizeof(int), sizeof(int));
        textName = malloc(textNameSize + 1);
        memcpy(textName, buf+3*sizeof(int), textNameSize);
        textName[textNameSize] = '\0';
        memcpy(&textDataSize, buf+3*sizeof(int)+textNameSize, sizeof(int));
        textData = malloc(textDataSize);
        memcpy(textData, buf+4*sizeof(int)+textNameSize, textDataSize);

        removeLaunchedData(H, jobID, exeID);

        printf("RN: Received result file for job: %d and exe: %d\n", jobID, exeID);

        // Write buf to an output file in filesystem
        writeResultFile(textName, textData, textDataSize, jobID, exeID);

        free(buf);
        free(textName);
        free(textData);
        break;

      // Update statistical data about device
      case (HARDWARE_STATS):
        printf("RN: Receiving hardware stats\n");
        buf = malloc(4*sizeof(int));
        rv = recv(sockfd, buf, 4*sizeof(int), MSG_WAITALL);
        if (rv == -1) {
          unregisterDevice(H, sendNodeWorker);
          free(buf);
          printf("RN: Unable to receive hardware stats from device, errno: %d\n", errno);
          return;
        }

        memcpy(&utilization, buf, sizeof(int));
        memcpy(&memoryUsage, buf+sizeof(int), sizeof(int));
        memcpy(&capUtilization, buf+2*sizeof(int), sizeof(int));
        memcpy(&capMemoryUsage, buf+3*sizeof(int), sizeof(int));

        if (utilization < 0 || memoryUsage < 0 || capUtilization < 0 || capMemoryUsage < 0 ||
            utilization > 100 || memoryUsage > 100 || capUtilization > 100 || capMemoryUsage > 100) {
          unregisterDevice(H, sendNodeWorker);
          free(buf);
          printf("RN: Receiving erroneous hardware stats, terminating device\n");
          return;
        }

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













