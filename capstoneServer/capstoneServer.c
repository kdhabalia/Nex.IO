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
  printf("The function is: %d\n", whichFunction);
  if(whichFunction == SYSTEM_STATE) {
    int registeredDevices = 3;
    send(fd, &registeredDevices, INT_SIZE, 0);
    float* infoArray = malloc(4*INT_SIZE);
    //MARIOS INFO ARRAY FUNCTION CALL IS HERE
    for(int i = 0; i < registeredDevices; i++) {
      infoArray[0] = 98.3;
      infoArray[1] = 45.5;
      infoArray[2] = 22.4;
      infoArray[3] = 24.5;
      send(fd, infoArray, 4*INT_SIZE, 0);
    }
    free(infoArray);
  } else if(whichFunction == SEND_FILE) {
    int pathSize;
    recv(fd, &pathSize, INT_SIZE, MSG_WAITALL);
    char* fileName = malloc(pathSize);
    char* parentName = "../clientTempStore/";
    char* result = malloc(strlen(fileName) + strlen(parentName) +1);
    recv(fd, fileName, pathSize, MSG_WAITALL);
    printf("Right before the strcopy\n");
    strcpy(result, parentName);
    printf("Right before the strcat\n");
    strcat(result, fileName);
    printf("Tthe file to make is: %s\n", result);
    int copyFd = open(result, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    int fileSize;
    recv(fd, &fileSize, INT_SIZE, MSG_WAITALL);
    char* bufferToRecv = malloc(fileSize);
    recv(fd, bufferToRecv, fileSize, MSG_WAITALL);
    write(copyFd, bufferToRecv, fileSize);

    printf("Right before freeing\n");
    free(fileName);
    free(result);
    free(bufferToRecv);
    close(copyFd);
  }

}

