#include "capstoneClient.h"
int sockfd;
int main() {
  acceptUserInput();
  return 0;
}

void acceptUserInput() {
  char name[100];
  int num, nitems;
  printf("Great! Now you are done with the basic steps. Welcome to system!\n");
  printf("We hope to run all your tasks in a reasonable amount of time.\n");
  printf("Here you can submit tasks and we will run them on voluntary computing resources avaiable to us.\n");
  printf("What is your name?\n");
  fgets(name, 100, stdin);
  printf("Hi there, %s!\n", strtok(name, "\n"));
  afterSystemState:
    printf("You currently have two options, you can check the system state or submit a task to run.\n");
    printf("Type the number 1 to check the system state or type 2 to submit jobs\n");
    restartInput:
      nitems = scanf("%d", &num);
      if(nitems == EOF) err(1, "%s\n", "EOF error");
      if(num == 1) {
        sendData();
        goto afterSystemState;
      } else if(num == 2) {
        getSystemState();
        goto afterSystemState;
      } else {
        printf("You entered an incorrect input why don't you try again %s\n", name);
        goto restartInput;
      }
}


void sendData() {
  char path[100];
  int nitems, num;
  startData:
    printf("Please make sure to understand the requirements of the tar file by reading README.txt\n");
    printf("Enter the *jobname*.tar to submit:\n");
    scanf("%s", &path);
    printf("Make sure the jobs name is correct: %s\n", path);
    printf("If it is correct type 1 and if incorrect type 2:\n");
    restartInput:
      nitems = scanf("%d", &num);
      if(nitems == EOF) err(1, "%s\n", "EOF error");
      if(num == 1) {
        sendClientFile(path);
      } else if(num == 2) {
        goto startData;
      } else {
        printf("You entered an incorrect input why don't you try again\n");
        goto restartInput;
      }
}


void sendClientFile(char* path) {
  serverReattempt:
    printf("Attempting to connect to server\n");
    int initRet = init();
    if(initRet == -1) {
      printf("Connection to server failed\n");
      printf("Reattempting server connection in 5 seconds\n");
      for(int i = 0; i < 5; i++) {
        sleep(1);
        fprintf(stdout, "%d seconds have passed\n", i+1);
      }
      goto serverReattempt;
   } else {
      marshallAndSendData(1, path);
    }

}

void getSystemState() {
  serverReattempt:
    printf("Attempting to connect to server\n");
    int initRet = init();
    if(initRet == -1) {
      printf("Connection to server failed\n");
      printf("Reattempting server connection in 5 seconds\n");
      for(int i = 0; i < 5; i++) {
        sleep(1);
        fprintf(stdout, "%d seconds have passed\n", i+1);
      }
      goto serverReattempt;
   } else {
      marshallAndSendData(0, "");
    }
}

void marshallAndSendData(int option, char* path) {
  if(!option) {
    int retVal;
    float* infoStruct = malloc(4*INT_SIZE);
    float capU, capMU, util, mutil;
    send(sockfd, &option, INT_SIZE, 0);
    recv(sockfd, &retVal, INT_SIZE, MSG_WAITALL);
    printf("****************************************\n");
    printf("SYSTEM STATE\n");
    printf("****************************************\n\n");
    printf("Number of devices current avaiable are: %d\n\n", retVal);
    for(int i = 0; i < retVal; i++) {
      printf("Device number: %d\n", i);
      recv(sockfd, infoStruct, INT_SIZE*4, MSG_WAITALL);
      capU = infoStruct[0];
      capMU = infoStruct[1];
      util = infoStruct[2];
      mutil = infoStruct[3];
      printf("Cap utilization: %f\n", capU);
      printf("Cap memory usage: %f\n", capMU);
      printf("Current utilzation: %f\n", util);
      printf("Current memory usage: %f\n\n", mutil);
    }
    printf("*****************************************\n");
    printf("*****************************************\n\n");
    free(infoStruct);
  } else {
    char* prefix = "./clientJobs/";
    char* totalResult = malloc(strlen(prefix) + strlen(path));
    strcpy(totalResult, prefix);
    strcat(totalResult, path);
    int fd = open(totalResult, O_RDONLY);
    int sizeOfPath = strlen(path);
    int sizeOfFile = getFileSize(fd);
    char* fileBuf = malloc(sizeOfFile);
    read(fd, fileBuf, sizeOfFile);
    send(sockfd, &option, INT_SIZE,0); //Send the function to call
    send(sockfd, &sizeOfPath, INT_SIZE,0); //Send size of path
    send(sockfd, path, sizeOfPath, 0); //Send path
    send(sockfd, &sizeOfFile, INT_SIZE, 0); //Send file size
    send(sockfd, fileBuf, sizeOfFile, 0); //Send actually file
  }
}

int getFileSize(int fd) {
  struct stat S;
  fstat(fd, &S);
  int size = S.st_size;
  return size;
}


// This function is automatically called when program is started
int init(void) {

  char *serverip;
	char *serverport;
	unsigned short port;
	int rv;
	struct sockaddr_in srv;

 	// Get environment variable indicating the ip address of the server
	serverip = "128.2.13.145";
  serverport = "15744";

	port = (unsigned short)atoi(serverport);

	// Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);	// TCP/IP socket
	if (sockfd<0) err(1, 0);			// in case of error

	// setup address structure to point to server
	memset(&srv, 0, sizeof(srv));			// clear it first
	srv.sin_family = AF_INET;			// IP family
	srv.sin_addr.s_addr = inet_addr(serverip);	// IP address of server
	srv.sin_port = htons(port);			// server port

	// actually connect to the server
	rv = connect(sockfd, (struct sockaddr*)&srv, sizeof(struct sockaddr));
	if (rv<0) {
    return -1;
  } else {
    printf("Connected to server\n");
    return 0;
  }
}


