#include <stdio.h>
#include <stdlib.h>

typedef enum { false, true } bool;

char *Create_Process(char *command) {
  // Run shell command and return its output as a string pointer
  FILE *fp;
  char ch;
  // Remember to free buffer pointer
  char *buffer = malloc(128 * sizeof(char));
  int i = 0;

  fp = popen(command, "r");

  if (fp == NULL) {
    // Replace with exception handling
    printf("Operation failed");
  }

  else {
    while ((ch = fgetc(fp)) != EOF) {
      // Handle buffer length more than 256chars
      if (i > 128) {
        buffer = realloc(buffer, sizeof(256 * sizeof(char)));
      }
      buffer[i] = ch;
      i++;
    }
    buffer[i] = '\0';
  }
  pclose(fp);

  return buffer;
}

bool CheckNM() {
  // Call out nmcli -h to determine if machine has NetworkManager
  char *command = "nmcli -h";
  char *output = Create_Process(command);
  printf("output: %s", output);
  free(output);
  // Check output

  return true;
}

int main() {

  CheckNM();

  return 0;
}
