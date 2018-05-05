#include "csapp.h"
#include "sbuf.h"
#include "../PyInterface/pythonInterface.h"
#include <time.h>

#define NTHREADS 8
#define SBUFSIZE 100
#define SYSTEM_STATE 0
#define SEND_FILE 1
#define INT_SIZE 4
#define NEW_DEVICE 2



void doit(int fd);
void* thread(void* vargp);
void Device (void* threadArgs);
