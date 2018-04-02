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

    // This code is only for testing
    char* D1IP = "1";
    float D1capU = 0.823;
    float D1capMU = 0.916;
    float D1U = 0.20987;
    float D1MU = 0.1023;

    char* D2IP = "2";
    float D2capU = 0.323;
    float D2capMU = 0.216;
    float D2U = 0.10987;
    float D2MU = 0.1023;

    char* D3IP = "3";
    float D3capU = 0.423;
    float D3capMU = 0.816;
    float D3U = 0.20987;
    float D3MU = 0.1023;

    printf("Registering Device 1\n");
    registerDevice(D1IP, D1capU, D1capMU, D1U, D1MU);

    printf("Registering Device 2\n");
    registerDevice(D2IP, D2capU, D2capMU, D2U, D2MU);

    printf("Registering Device 3\n");
    registerDevice(D3IP, D3capU, D3capMU, D3U, D3MU);

    // Launches threads for separate components
    inQ = queueInit();

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
  char funcNumber[4];
  int whichFunction;
  recv(fd, funcNumber, 4, MSG_WAITALL);
  memcpy(&whichFunction, funcNumber, 4);
  if(whichFunction == SYSTEM_STATE) {
    int registeredDevices = numberOfDevices();
    send(fd, &registeredDevices, INT_SIZE, 0);
    HardwareDevice H;
    float* infoArray = malloc(4*INT_SIZE);
    //MARIOS INFO ARRAY FUNCTION CALL IS HERE
    for(int i = 0; i < registeredDevices; i++) {
      H = grabDevice(i);
      infoArray[0] = H->capUtilization;
      infoArray[1] = H->capMemoryUsage;
      infoArray[2] = H->utilization;
      infoArray[3] = H->memoryUsage;
      send(fd, infoArray, 4*INT_SIZE, 0);
    }
    free(infoArray);
  } else if(whichFunction == SEND_FILE) {
    int pathSize;
    recv(fd, &pathSize, INT_SIZE, MSG_WAITALL);
    char* fileName = malloc(pathSize+1);
    char* parentName = "../clientTempStore/";
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
  }

}

