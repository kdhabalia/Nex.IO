#include "pythonInterface.h"

char* filename = "packet";
char* suffix = ".txt";

// inline function to swap two numbers
void swap(char *x, char *y) {
  char t = *x; *x = *y; *y = t;
}

// function to reverse buffer[i..j]
char* reverse(char *buffer, int i, int j)
{
  while (i < j)
  swap(&buffer[i++], &buffer[j--]);

  return buffer;
}

char* itoa(int num, char* str, int base)
{
  int i = 0;
  int isNegative = 0;

  /* Handle 0 explicitely, otherwise empty string is printed for 0 */
  if (num == 0)
  {
    str[i++] = '0';
    str[i] = '\0';
    return str;
  }

  // In standard itoa(), negative numbers are handled only with
  // base 10. Otherwise numbers are considered unsigned.
  if (num < 0 && base == 10)
  {
    isNegative = 1;
    num = -num;
  }

  // Process individual digits
  while (num != 0)
  {
    int rem = num % base;
    str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
    num = num/base;
  }

  // If number is negative, append '-'
  if (isNegative)
  str[i++] = '-';

  str[i] = '\0'; // Append string terminator

  // Reverse the string
  reverse(str, 0, i-1);

  return str;
}

void enqueueNewPacket () {

  int currentPacket = 0;
  int l;
  while (1) {

    char* num = malloc(30);

    itoa(currentPacket, num, 10);
    char* fullname = malloc(strlen(filename)+strlen(num)+strlen(suffix)+1);

    strcpy(fullname, filename);
    strcat(fullname, num);
    strcat(fullname, suffix);

    free(num);

    struct stat S;
    while (stat(fullname, &S) == -1);

    int jobID;
    const char* tempEP = malloc(50);
    const char* tempDP = malloc(50);
    int load;
    int workloadType;

    FILE* fp = fopen(fullname, "r");

    char* buf0 = malloc(50);
    fgets(buf0, 50, fp);
    l = strlen(buf0);
    buf0[l-1] = '\0';
    jobID = atoi(buf0);
    free(buf0);

    char* buf1 = malloc(50);
    fgets(buf1, 50, fp);
    l = strlen(buf1);
    buf1[l-1] = '\0';
    strcpy(tempEP, buf1);
    free(buf1);

    char* buf2 = malloc(50);
    fgets(buf2, 50, fp);
    l = strlen(buf2);
    buf2[l-1] = '\0';
    strcpy(tempDP, buf2);
    free(buf2);

    char* buf3 = malloc(50);
    fgets(buf3, 50, fp);
    l = strlen(buf3);
    buf3[l-1] = '\0';
    load = atoi(buf3);
    free(buf3);

    char* buf4 = malloc(50);
    fgets(buf4, 50, fp);
    l = strlen(buf4);
    buf4[l-1] = '\0';
    workloadType = atoi(buf4);
    free(buf4);

    WorkloadPacket WP = malloc(sizeof(struct Workload));
    WP->jobID = jobID;
    WP->executablePath = malloc(strlen(tempEP)+1);
    memcpy(WP->executablePath, tempEP, strlen(tempEP)+1);
    WP->dataPath = malloc(strlen(tempDP)+1);
    memcpy(WP->dataPath, tempDP, strlen(tempDP)+1);
    WP->load = load;
    WP->workloadType = workloadType;

    free(tempEP);
    free(tempDP);

    queueEnqueue(inQ, (void*)WP);

    currentPacket++;

  }

}





