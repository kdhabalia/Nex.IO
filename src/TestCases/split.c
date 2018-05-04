#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

int main() {

  FILE* fp;
  fp = fopen("array.txt", "r");

  char* buf;

  for (int i = 0; i < 100000000; i++) {
    buf = malloc(50);
    char* rv = fgets(buf, 50, fp);
    if (rv == NULL) {
      break;
    }
    int l = strlen(buf);
    buf[l-1] = '\0';
    int element = atoi(buf);
    free(buf);


  }

  fclose(fp);

  fp = fopen("result.txt", "w");

  for (int i = 0; i < size; i++) {
    fprintf(fp, "%d\n", array[i]);
  }

  fclose(fp);

  return 0;

}

