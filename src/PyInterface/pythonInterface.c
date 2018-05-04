#include "pythonInterface.h"

char* filename = "../Cache/packet";
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

int numLines (char* filename) {

  struct stat S;
  if (stat(filename, &S) == -1) {
    return 0;
  }

  FILE *fp;
  char c;

  fp = fopen(filename, "r");

  int count = 0;
  // Extract characters from file and store in character c
  for (c = getc(fp); c != EOF; c = getc(fp)) {
    if (c == '\n') {// Increment count if this character is newline
      count = count + 1;
    }
  }

  return count;
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
    while ((stat(fullname, &S) == -1) || (numLines(fullname) != 4));

    int jobID;
    int exeID;
    const char* tempEP = malloc(50);
    const char* tempDP = malloc(50);
    int load;
    int workloadType;

    char* buf;

    FILE* fp = fopen(fullname, "r");

    // Job ID
    buf = malloc(50);
    fgets(buf, 50, fp);
    l = strlen(buf);
    buf[l-1] = '\0';
    jobID = atoi(buf);
    free(buf);

    // EXE ID
    buf = malloc(50);
    fgets(buf, 50, fp);
    l = strlen(buf);
    buf[l-1] = '\0';
    jobID = atoi(buf);
    free(buf);

    // EXE pathname
    buf = malloc(50);
    fgets(buf, 50, fp);
    l = strlen(buf);
    buf[l-1] = '\0';
    strcpy(tempEP, buf);
    free(buf);

    // Textfiles pathname
    buf = malloc(50);
    fgets(buf, 50, fp);
    l = strlen(buf);
    buf[l-1] = '\0';
    strcpy(tempDP, buf);
    free(buf);

    // load weight
    buf = malloc(50);
    fgets(buf, 50, fp);
    l = strlen(buf);
    buf[l-1] = '\0';
    load = atoi(buf);
    free(buf);

    // load type - Fully Independant or Reduce
    buf = malloc(50);
    fgets(buf, 50, fp);
    l = strlen(buf);
    buf[l-1] = '\0';
    workloadType = atoi(buf);
    free(buf);

    WorkloadPacket WP = malloc(sizeof(struct Workload));
    WP->jobID = jobID;
    WP->exeID = exeID;
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

    fclose(fp);

  }

}





