#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include "../Queue/queue.h"

typedef struct Workload* WorkloadPacket;
struct Workload {

  int jobID;
  char* executablePath;
  char* dataPath;
  int load;
  int workloadType;

};

typedef struct Device* HardwareDevice;
struct Device {

  char* deviceIP;
  float capUtilization;
  float capMemoryUsage;
  float utilization;
  float memoryUsage;
  Queue Q;

  pthread_rwlock_t lock;

};

extern Queue inQ;

extern int registeredDevices;

extern HardwareDevice* devices;

void registerDevice (char* deviceIP, float capUtilization, float capMemoryUsage, float utilization, float memoryUsage);

void balanceLoads (void* threadArgs);

int numberOfDevices ();

HardwareDevice grabDevice (int index);



