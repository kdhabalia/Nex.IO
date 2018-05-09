#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

int main() {

  FILE* fp;
  fp = fopen("array.txt", "r");

  char* buf;

  int* array;
  int size = 0;

  while (1) {
    buf = malloc(50);
    char* rv = fgets(buf, 50, fp);
    if (rv == NULL) {
      break;
    }
    int l = strlen(buf);
    buf[l-1] = '\0';
    int element = atoi(buf);
    free(buf);

    int* newArray = malloc((size+1)*sizeof(int));
    for (int i = 0; i < size; i++) {
      newArray[i] = array[i];
    }
    size++;
    newArray[size-1] = element + 1;
    free(array);
    array = newArray;
  }

  fclose(fp);

  fp = fopen("/sdcard/result.txt", "w");

  for (int i = 0; i < size; i++) {
    fprintf(fp, "%d\n", array[i]);
  }

  fflush(fp);
  fclose(fp);

  return 0;

}

