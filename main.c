#include <stdio.h>
#include <stdlib.h>

char *Create_Process(char *command) {
  // Run shell command and return its output as a string pointer
  FILE *fp;
  char ch;
  char *buffer = malloc(128 * sizeof(char));
  int temp, i = 0;

  fp = popen(command, "r");

  if (fp == NULL) {
    // Replace with exception handling
    printf("Operation failed");
  }

  else {
    while ((ch = fgetc(fp)) != EOF) {
      if (i < 256) {
        buffer[i] = ch;
        i++;
      }
    }
    buffer[i] = '\0';
  }
  pclose(fp);

  return buffer;
}

int main() {

  char *command = "ls";
  char *output = Create_Process(command);
  printf("Output: %s", output);
  free(output);

  return 0;
}
