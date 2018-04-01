#include "csapp.h"
#include "sbuf.h"
//#include "loadbalancer.h"
#define NTHREADS 8
#define SBUFSIZE 100
#define SYSTEM_STATE 0
#define SEND_FILE 1
#define INT_SIZE 4



void doit(int fd);
void* thread(void* vargp);
