#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { false, true } bool;

#define INITIAL_SIZE 64
#define MAX_SIZE 4096
#define ADAPTER_STRING "GENERAL.DEVICE"
#define CHECKNM_STRING "nmcli tool"
#define INITIAL_COUNT 10

struct RenderList {
  char **items;
  int count;
  int capacity;
};

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
                 "max_size: %d \n",
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

  // current approach string handling function
  char *match = strstr(output, expected);
  if (match != NULL) {
    return true;
  } else {
    return false;
  }
}

bool CheckNM() {
  // Call out nmcli -h to determine if machine has NetworkManager
  bool result;
  char *command = "nmcli -v 2>&1";
  // nmcli sends o/p to stderr , redirect o/p to stdout
  char *output = Create_Process(command);

  result = Validate_OP(output, CHECKNM_STRING);
  // printf("Ouput: %s", output);
  free(output);
  // Check output

  return result;
}

bool CheckWifiAdapter() {
  // Check wifi adapter availability on device
  // command - nmcli device show
  char *output = Create_Process("nmcli device show 2>&1 | grep -B 1 -E '^.*: "
                                "+wifi$' | grep -v 'GENERAL.TYPE'");
  bool result = Validate_OP(output, ADAPTER_STRING);
  free(output);
  return result;
}

void PopulateNMOptions() {
  // Disable/Enable Wifi
  // Delete a Connection
  // Rescan Wifi
  // Show Password
  // Saved Connections
}

char *GetNetworks(char *mode) {}

void PopulateNetworks(char *mode) {
  if (strcmp(mode, "available")) {
    char *buffer = GetNetworks();
  }
  // Add available networks
  // Add scanned networks
}

struct RenderList *InitRenderList() {
  // Initialise RenderList
  struct RenderList *renderList;
  renderList->count = 0;
  renderList->capacity = INITIAL_COUNT;
  renderList->items = malloc(sizeof(char *) * renderList->capacity);

  if (renderList->items == NULL) {
    printf("Memory allocation failed to renderList \n");
    return NULL;
  }

  else {
    return renderList;
  }
}

int main() {

  bool NM_present = CheckNM();
  if (NM_present == true) {
    printf("NM present \n");
  } else {
    printf("NM not accessible \n");
  }

  bool AdapterPresent = CheckWifiAdapter();
  if (AdapterPresent == true) {
    printf("Adapter found \n");
  } else {
    printf("Adapter could not be found \n");
  }
  return 0;
}
