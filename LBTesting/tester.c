#include "loadBalancer.h"

int main () {

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

  printf("Creating inQ\n");

  Queue inQ = queueInit();

  for (int i = 1; i < 11; i++) {

    int JID = i;
    char* Jep = "./";
    char* Jdp[1];
    Jdp[0] = "./";
    int Jdq = 1;
    int Jl = i%4;

    WorkloadPacket W = malloc(sizeof(struct Workload));
    W->jobID = JID;
    W->executablePath = Jep;
    W->dataPaths = Jdp;
    W->dataQuantity = Jdq;
    W->load = Jl;

    printf("Putting job %d into inQ\n", i);
    queueEnqueue(inQ, (void*)W);

  }

  printf("inQ has a total of %d jobs\n", queueLength(inQ));

  pthread_t worker;

  printf("Launching thread to execute load balancing\n");
  pthread_create(&worker, NULL, balanceLoads, (void*)inQ);

  printf("Waiting on thread to finish load balancing\n");
  pthread_join(worker, NULL);

  printf("Load balancing done\n");
  return 0;
}










