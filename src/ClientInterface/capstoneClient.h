#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <netinet/in.h>
#define SYSTEM_STATE 0
#define SEND_FILE 1
#define INT_SIZE 4

void acceptUserInput();
void delay();
void getSystemState();
void sendData();
void marshallAndSendData(int option, char* path);
int init(void);
