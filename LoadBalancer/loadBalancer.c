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

int eval (void* e) {

  WorkloadPacket element = (WorkloadPacket)e;
  int load = element->load;
  return load;

}

void registerDevice (char* deviceIP, float capUtilization, float capMemoryUsage, float utilization, float memoryUsage) {

  int numDevices = numberOfDevices();

  HardwareDevice* newDevices = malloc((numDevices+1)*sizeof(HardwareDevice));
  for (int i = 0; i < numDevices; i++) {
    newDevices[i] = grabDevice(i);
  }

  HardwareDevice new = malloc(sizeof(struct Device));
  pthread_rwlock_init(&(new->lock), NULL);
  new->Q = queueInit();
  new->memoryUsage = memoryUsage;
  new->utilization = utilization;
  new->capMemoryUsage = capMemoryUsage;
  new->capUtilization = capUtilization;
  new->deviceIP = deviceIP;

  pthread_mutex_lock(&mLock);
  registeredDevices++;
  newDevices[registeredDevices-1] = new;
  free(devices);
  devices = newDevices;
  pthread_mutex_unlock(&mLock);

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
      void* e = queueDequeue(inQ);
      printf("Removed job ID is %d\n", ((WorkloadPacket)e)->jobID);

      int* queueLoads = hardwareDevicesQueueLoads();
      printf("Queue loads for the devices is %d, %d, %d\n", queueLoads[0], queueLoads[1], queueLoads[2]);
      float* hardwareUtilizations = hardwareDevicesUtilizations();
      printf("Hardware Utilizations for the devices is %f, %f, %f\n", hardwareUtilizations[0], hardwareUtilizations[1], hardwareUtilizations[2]);
      float* hardwareMemoryUsages = hardwareDevicesMemoryUsages();
      printf("Hardware Memory Utilizations for the devices is %f, %f, %f\n", hardwareMemoryUsages[0], hardwareMemoryUsages[1], hardwareMemoryUsages[2]);

      float* hardwareScores = hardwareDevicesScores(queueLoads, hardwareUtilizations, hardwareMemoryUsages);
      printf("Hardware load scores for the devices is %f, %f, %f\n", hardwareScores[0], hardwareScores[1], hardwareScores[2]);
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

char* hardwareDeviceIP (HardwareDevice H) {

  pthread_rwlock_rdlock(&(H->lock));
  char* deviceIP = H->deviceIP;
  pthread_rwlock_unlock(&(H->lock));
  return deviceIP;

}

int sizeOfFile (int fd) {

  struct stat S;
  fstat(fd, &S);
  int size = S.st_size;
  return size;

}

void sendToHardwareDevice (HardwareDevice H) {

  // Connection variables
  char *serverport;
  unsigned short port;
  int sockfd;
  int sessfd;
  int rv;
  struct sockaddr_in srv;
  struct sockaddr_in cli;
  socklen_t sa_size;

  char* buf;
  char* sendBuffer;
  int fd;
  int size;

  int serverPort = 80;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&srv, 0, sizeof(srv));     // clear it first
  srv.sin_family = AF_INET;     // IP family
  srv.sin_addr.s_addr = htonl(INADDR_ANY);  // don't care IP address
  srv.sin_port = htons(port);     // server port

  // bind to our port
  rv = bind(sockfd, (struct sockaddr*)&srv, sizeof(struct sockaddr));

  // start listening for connections
  rv = listen(sockfd, 0);

  // Wait for client, get session socket
  sa_size = sizeof(struct sockaddr_in);
  sessfd = accept(sockfd, (struct sockaddr *)&cli, &sa_size);

  while(1) {

    WorkloadPacket e = (WorkloadPacket)emptyHardwareQueue(H);

    char* executablePath = e->executablePath;
    char* dataPath = e->dataPath;

    fd = open(executablePath, O_RDONLY);

    size = sizeOfFile(fd);
    buf = malloc(size);

    read(fd, buf, size);

    sendBuffer = malloc(sizeof(int)+size);
    memcpy(sendBuffer, &size, sizeof(int));
    memcpy(sendBuffer+sizeof(int), buf, size);

    // Send back to client
    send(sessfd, sendBuffer, (sizeof(int)+size), 0);

    close(fd);
    free(buf);
    free(sendBuffer);

    fd = open(dataPath, O_RDONLY);

    size = sizeOfFile(fd);
    buf = malloc(size);

    read(fd, buf, size);

    sendBuffer = malloc(sizeof(int)+size);
    memcpy(sendBuffer, &size, sizeof(int));
    memcpy(sendBuffer+sizeof(int), buf, size);

    send(sessfd, sendBuffer, (sizeof(int)+size), 0);

    close(fd);
    free(buf);
    free(sendBuffer);

    free(dataPath);
    free(executablePath);
    free(e);

  }

}

void receiveFromHardwareDevice (HardwareDevice H) {

  char *serverip;
  char *serverport;
  unsigned short port;
  int rv;
  struct sockaddr_in srv;
  int sockfd;

  // In this case the server is the Hardware Device
  pthread_rwlock_rdlock(&(H->lock));
  serverip = H->deviceIP;
  pthread_rwlock_unlock(&(H->lock));

  serverport = 80;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&srv, 0, sizeof(srv));
  srv.sin_family = AF_INET;
  srv.sin_addr.s_addr = inet_addr(serverip);
  srv.sin_port = htons(port);

  rv = connect(sockfd, (struct sockaddr*)&srv, sizeof(struct sockaddr));

  int size;
  int functionality;
  float capUtilization;
  float capMemoryUsage;
  float utilization;
  float memoryUsage;
  float data[4];
  while (1) {

    // First receive the intent from the client
    recv(sockfd, &functionality, sizeof(int), MSG_WAITALL);

    switch (functionality) {
      // Receive a result file
      case (0):
        recv(sockfd, &size, sizeof(int), MSG_WAITALL);
        char* buf = malloc(size);
        recv(sockfd, buf, size, MSG_WAITALL);
        // Write buf to an output file in filesystem
        break;

      // Update statistical data about device
      case (1):
        // Receive the four values all at once
        recv(sockfd, &data, 4*sizeof(float), MSG_WAITALL);

        memcpy(&capUtilization, &(data[0]), sizeof(float));
        memcpy(&capMemoryUsage, &(data[1]), sizeof(float));
        memcpy(&utilization, &(data[2]), sizeof(float));
        memcpy(&memoryUsage, &(data[3]), sizeof(float));

        pthread_rwlock_wrlock(&(H->lock));
        H->capUtilization = capUtilization;
        H->capMemoryUsage = capMemoryUsage;
        H->utilization = utilization;
        H->memoryUsage = memoryUsage;
        pthread_rwlock_unlock(&(H->lock));
        break;
    }

  }

}













