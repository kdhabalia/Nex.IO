#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

int main() {

  FILE* fp;
  fp = fopen("array.txt", "w");

  for (int i = 0; i < 10; i++) {
    fprintf(fp, "%d\n", i);
  }

  fclose(fp);

  return 0;

}

