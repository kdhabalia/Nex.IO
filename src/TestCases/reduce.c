#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

int main() {

  FILE* fp;
  fp = fopen("result.txt", "r");

  char* buf;

  int result = 0;

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

    result += element;
  }

  fclose(fp);

  fp = fopen("finalResult.txt", "w");

  fprintf(fp, "%d\n", result);

  fclose(fp);

  return 0;

}

