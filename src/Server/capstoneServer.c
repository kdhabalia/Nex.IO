#include "capstoneServer.h"
sbuf_t sbuf;

int main(int argc, char **argv)
{
    int i, listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);

    // Launches threads for separate components
    srand(time(NULL));

    inQ = queueInit(MAX_QUEUE_SIZE);

    pthread_t EPworker;
    pthread_create(&EPworker, NULL, enqueueNewPacket, (void*)NULL);

    pthread_t LBworker;
    pthread_create(&LBworker, NULL, balanceLoads, (void*)NULL);

    sbuf_init(&sbuf, SBUFSIZE);
    for(i = 0; i < NTHREADS; i++) {
      Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
		  clientlen = sizeof(clientaddr);
		  connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
			sbuf_insert(&sbuf, connfd);
    }
}

void* thread(void* vargp)
{
  Pthread_detach(pthread_self());
  while(1) {
    int connfd = sbuf_remove(&sbuf);
    doit(connfd);
    Close(connfd);
  }
}


void doit(int fd)
{
  printf("Server: Someone connected\n");
  char funcNumber[4];
  int whichFunction;
  recv(fd, funcNumber, 4, MSG_WAITALL);
  memcpy(&whichFunction, funcNumber, 4);
  printf("Server: Got connection identifier %d\n", whichFunction);
  if(whichFunction == SYSTEM_STATE) {
    int numDevices = numberOfDevices();
    send(fd, &numDevices, INT_SIZE, 0);
    float* infoArray;
    //MARIOS INFO ARRAY FUNCTION CALL IS HERE
    for(int i = 0; i < numDevices; i++) {
      infoArray = deviceStats(i);
      send(fd, infoArray, 4*INT_SIZE, 0);
      free(infoArray);
    }
  } else if(whichFunction == SEND_FILE) {
    int pathSize;
    recv(fd, &pathSize, INT_SIZE, MSG_WAITALL);
    char* fileName = malloc(pathSize+1);
    char* parentName = "../Storage/";
    char* result = malloc(pathSize + strlen(parentName) +1);
    recv(fd, fileName, pathSize, MSG_WAITALL);
    fileName[pathSize] = '\0';
    strcpy(result, parentName);
    strcat(result, fileName);
    int copyFd = open(result, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    int fileSize;
    recv(fd, &fileSize, INT_SIZE, MSG_WAITALL);
    char* bufferToRecv = malloc(fileSize);
    recv(fd, bufferToRecv, fileSize, MSG_WAITALL);
    write(copyFd, bufferToRecv, fileSize);

    free(fileName);
    free(result);
    free(bufferToRecv);
    close(copyFd);
  } else if(whichFunction == NEW_DEVICE) {
    char hardwareStats[4*sizeof(int)];
    int capUtilization;
    int capMemoryUsage;
    int utilization;
    int memoryUsage;

    printf("Server: Device connected with fd: %d\n", fd);

    recv(fd, hardwareStats, 4*sizeof(int), MSG_WAITALL);
    memcpy(&utilization, hardwareStats, sizeof(int));
    memcpy(&memoryUsage, hardwareStats+sizeof(int), sizeof(int));
    memcpy(&capUtilization, hardwareStats+2*sizeof(int), sizeof(int));
    memcpy(&capMemoryUsage, hardwareStats+3*sizeof(int), sizeof(int));

    printf("Server: Received hardware stats %d, %d, %d, %d\n", utilization, memoryUsage, capUtilization, capMemoryUsage);

    HardwareDevice H = registerDevice((float)capUtilization, (float)capMemoryUsage, (float)utilization, (float)memoryUsage);

    printf("Server: Registered device\n");

    struct senderArgs* sArgs = malloc(sizeof(struct senderArgs));
    sArgs->H = H;
    sArgs->sessfd = fd;

    pthread_t sendNodeWorker;
    pthread_create(&sendNodeWorker, NULL, sendToHardwareDevice, (void*)sArgs);

    printf("Server: Launched send node\n");

    struct receiverArgs* rArgs = malloc(sizeof(struct receiverArgs));
    rArgs->H = H;
    rArgs->sockfd = fd;
    rArgs->sendNodeWorker = sendNodeWorker;

    pthread_t receiveNodeWorker;
    pthread_create(&receiveNodeWorker, NULL, receiveFromHardwareDevice, (void*)rArgs);

    printf("Server: Launched receive node\n");

    pthread_join(sendNodeWorker, NULL);
    pthread_join(receiveNodeWorker, NULL);
  }
}

