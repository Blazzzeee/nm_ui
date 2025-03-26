#include <stdio.h>
#include <stdlib.h>

typedef enum { false, true } bool;

#define INITIAL_SIZE 128
#define MAX_SIZE 512
#define COMPARE_LENGTH 20

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
bool Validate_OP(char *output, char *expected) {
  // Output is the output buffer returned by running subprocess
  // Expected is the Expected output of that command
  // The function returns whether the 'output' is similar to 'expected' or not

  // current approach char by char comparison , possible improvements shell
  // regex
  int i, matched = 0;
  while (output[i] != '\0' || expected[i] != '\0') {
    // printf("Comaprison output[i]: %c, expected[i]: %c \n", output[i],
    // expected[i]);
    if (output[i] == expected[i]) {
      matched++;
    }
    if (matched >= COMPARE_LENGTH) {
      break;
    }
    i++;
  }
  if (matched >= COMPARE_LENGTH) {
    return true;
  }

  else {
    return false;
  }
}

bool CheckNM() {
  // Call out nmcli -h to determine if machine has NetworkManager
  bool result;
  char *command = "nmcli -v 2>&1";
  char *expected = "nmcli tool, version 1.52.0-1";
  // nmcli sends o/p to stderr , redirect o/p to stdout
  char *output = Create_Process(command);

  result = Validate_OP(output, expected);
  // printf("Ouput: %s", output);
  free(output);
  // Check output

  return result;
}

void InitialiseAdapter() {
  // Check wifi adapter availability on device
  // command - nmcli device show

  // prompt the user about choosing adapter if there are more than one available
  // Initialise the adapter that will be used for rest of execution
}

int main() {

  bool NM_present = CheckNM();
  if (NM_present == true) {
    printf("NM present");
  } else {
    printf("NM not accessible");
  }
  return 0;
}
