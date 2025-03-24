#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { false, true } bool;
#define INITIAL_SIZE 128
#define MAX_SIZE 512

char *Create_Process(char *command) {
  // Run shell command and return its output as a string pointer
  FILE *fp;
  char ch;
  int size = INITIAL_SIZE;
  // Remember to free buffer pointer
  char *buffer = malloc(INITIAL_SIZE);
  int i = 0;

  fp = popen(command, "r");

  if (fp == NULL) {
    // Replace with exception handling
    printf("DEBUG: Operation failed \n");
  }

  else {
    while ((ch = fgetc(fp)) != EOF) {
      if (i < size - 1) {
        buffer[i++] = ch;
      }

      else {
        size = size * 2;
        if (size <= MAX_SIZE) {
          char *temp = realloc(buffer, size);
          if (temp != NULL) {
            // safely exteneded size
            buffer = temp;
            buffer[i++] = ch;
          } else {
            // Handle realloc error
            printf("DEBUG: Error extending size\n");
            return buffer;
          }
        } else {
          // Loop termination to avoid stackoverflow
          printf("DEBUG: MAX Buffer limit reached , current size: %d , "
                 "max_size: %d\n",
                 size, MAX_SIZE);
          break;
        }
      }
    }
    buffer[i] = '\0';
  }
  pclose(fp);

  return buffer;
}

bool CheckNM() {
  // Call out nmcli -h to determine if machine has NetworkManager
  char *command = "nmcli -h 2>&1";
  // nmcli sends o/p to stderr , redirect o/p to stdout
  char *output = Create_Process(command);
  printf("Output: %s \n", output);
  free(output);
  // Check output

  return true;
}

int main() {

  CheckNM();

  return 0;
}
