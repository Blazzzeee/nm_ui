#include <libnm/NetworkManager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { false, true } bool;
#define INITIAL_SIZE 64
#define SIZE_MAX 4096

#define RENDER_LIST_SIZE 512
#define MAX_ENTRIES 50

typedef int (*onValidate)(void *);

struct _RenderString {
  char *string;
  size_t size;
  int length;
};

typedef struct _RenderString RenderString;

struct _RenderList {
  char **List;
  onValidate *callbackArray;
  int length;
  int size;
};

typedef struct _RenderList RenderList;

RenderString *InitRenderString() {
  RenderString *renderString = malloc(sizeof(RenderString));
  if (!renderString) {
    printf("LOG: Error: Could not allocate RenderString struct \n");
    return NULL;
  }
  renderString->length = 0;
  renderString->size = sizeof(char) * RENDER_LIST_SIZE;
  renderString->string = (char *)malloc(renderString->size);
  if (!renderString->string) {
    printf("LOG: Could not allocate memory to RenderString string \n");
    free(renderString->string);
    free(renderString);
    return NULL;
  }

  return renderString;
}

RenderList *InitRenderList() {
  RenderList *renderList = malloc(sizeof(RenderList));

  if (!renderList) {
    printf("LOG: Error: Could not allocate RenderList struct\n");
    return NULL;
  }

  renderList->size = SIZE_MAX;
  renderList->length = 0;

  renderList->List = malloc(sizeof(char) * SIZE_MAX);
  renderList->callbackArray = malloc(sizeof(onValidate) * MAX_ENTRIES);

  if (!renderList->List || !renderList->callbackArray) {
    printf("LOG: Error: Could not allocate memory for List or callbackArray\n");
    free(renderList->List);
    free(renderList->callbackArray);
    free(renderList);
    return NULL;
  }

  printf("LOG: Created RenderList \n");
  return renderList;
}

void AddRenderEntry(char *ssid, int strength, RenderString *RenderString,
                    RenderList *RenderList, onValidate callback) {
  char strengthBuffer[4];

  if (RenderString->length >= RenderString->size) {
    printf("LOG: Buffer Overflow reallocating size\n");
    int newSize = RenderString->size * 2;
    if (newSize <= SIZE_MAX) {
      char *temp = realloc(RenderString->string, newSize);
      printf("LOG: Reallocated sucessfully size to RenderString %zu",
             RenderString->size);
      if (!temp) {
        RenderString->string = temp;
      }
      RenderString->size = newSize;
    }
  }

  if (RenderList && callback) {

    if (RenderList->length >= RenderList->size) {

      printf("LOG: Buffer Overflow reallocating size\n");
      int newSize = RenderList->size * 2;
      if (newSize <= SIZE_MAX) {
        char **temp = realloc(RenderList->List, newSize * sizeof(char *));
        printf("LOG: Reallocated sucessfully size to RenderString %d",
               RenderList->size);
        if (temp) {
          RenderList->List = temp;
        }
        RenderList->size = newSize;
      }

      // Handle function list overflow
    }

    RenderList->List[RenderList->length] = ssid;
    RenderList->callbackArray[RenderList->length++] = callback;
  }

  for (int i = 0; i < strlen(ssid); i++) {
    RenderString->string[RenderString->length++] = ssid[i];
  }

  RenderString->string[RenderString->length++] = ' ';
  if (strength != -1) {

    snprintf(strengthBuffer, 4, "%d", strength);
    for (int i = 0; i < 3; i++) {
      if (strengthBuffer[i] != '\0') {

        RenderString->string[RenderString->length++] = strengthBuffer[i];
      }
    }
  }

  RenderString->string[RenderString->length++] = '\n';
}

char *CreateProcess(char *Rawcommand, char *args) {
  // Run shell command and return its output as a string pointer
  FILE *fp;
  char ch;
  int size = INITIAL_SIZE;
  // Remember to free buffer pointer
  char *buffer = malloc(INITIAL_SIZE);
  int i = 0;

  // Create command along with adding args
  size_t j = strlen(Rawcommand);
  size_t k = strlen(args);
  char *command = malloc(j + k + 2);
  if (command == NULL) {
    printf("Command string memory allocation failed \n");
    free(buffer);
    return NULL;
  }
  strcpy(command, Rawcommand);
  strcat(command, args);

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
        if (size <= SIZE_MAX) {
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
                 size, SIZE_MAX);
          break;
        }
      }
    }
    buffer[i] = '\0';
  }
  pclose(fp);
  free(command);
  return buffer;
}

int ValidateOP(char *output, char *expected, onValidate callback) {
  // Output is the output buffer returned by running subprocess
  // Expected is the Expected output of that command
  // The function returns whether the 'output' is similar to 'expected' or not

  // current approach string handling function

  char *match = strstr(output, expected);
  if (match != NULL) {
    int result = callback((void *)true);
    return result;
  } else {
    return false;
  }
}

NMClient *CreateClient() {
  NMClient *client;
  // request a synchronous call to NetworkManager over ipc socket
  client = nm_client_new(NULL, NULL);

  if (client != NULL) {
    // version info check
    const char *versionInfo = nm_client_get_version(client);
    printf("%s \n", versionInfo);
    return client;
  }

  else {
    printf("NetworkManager not running\n");
    return NULL;
  }
}

int ProcessConnect(void *args) {
  printf("Connect call to IPC %s", (char *)args);
  return 0;
}

void ProcessWifiAP(NMDeviceWifi *device, RenderString *RenderString,
                   RenderList *RenderList) {
  // Request Access Point(networks : connected + available) from the current
  // device
  const GPtrArray *APList = nm_device_wifi_get_access_points(device);
  if (APList != NULL) {
    for (guint i = 0; i < APList->len; i++) {
      // Iterate over the Access Point List
      NMAccessPoint *AP = (NMAccessPoint *)g_ptr_array_index(APList, i);
      if (AP != NULL) {
        GBytes *ssid_bytes = nm_access_point_get_ssid(AP);
        // ssid is stored as raw_bytes
        guint strength = nm_access_point_get_strength(AP);
        // request signal strength
        if (ssid_bytes) {
          // if the bytes are valid
          gsize len;
          // change bytes to data
          const char *raw_ssid = g_bytes_get_data(ssid_bytes, &len);
          // Safe copy the ssid into ssid;
          char *ssid = g_strndup(raw_ssid, len);
          // Add the ssid and signal strength to Connection List
          // AddConnList(ConnList, ssid, strength);
          // printf("DEBUG: ssid %s\n", ssid);
          AddRenderEntry(ssid, strength, RenderString, RenderList,
                         &ProcessConnect);
        }

      } else {
        printf("SSID not available\n");
      }
    }

  } else {
    printf("cannot request Access Points from device");
  }
}

int PopulateNetworks(NMClient *client, RenderString *RenderString,
                     RenderList *RenderList) {
  // Populate the Connection List with avalilable networks

  // Request all the available devices managed by the client
  const GPtrArray *devices = nm_client_get_devices(client);

  if (devices == NULL) {
    printf("Erorr requesting devices from client \n");
    return false;
  } else {
    for (guint i = 0; i < devices->len; i++) {
      // Iterate over GPtrArray , request APs for wifi device
      const NMDevice *device = (NMDevice *)g_ptr_array_index(devices, i);
      if (device != NULL) {
        NMDeviceType deviceType = nm_device_get_device_type(device);

        if (deviceType == NM_DEVICE_TYPE_WIFI) {
          // ProcessWifiAP adds networks to Connection List
          ProcessWifiAP((NMDeviceWifi *)device, RenderString, RenderList);
        }
      }
    }
    return true;
  }
}

int GetWifiState(NMClient *client, RenderString *RenderString) {
  // Requests state of Wifi and adds appropriately to Network List
  bool WifiState = (bool)nm_client_wireless_get_enabled(client);
  if (WifiState == true) {
    char *option = "Disable Wifi";
    AddRenderEntry(option, -1, RenderString, NULL, NULL);
    return 0;
  }

  else {
    char *option = "Enable Wifi";
    AddRenderEntry(option, -1, RenderString, NULL, NULL);
    return 0;
  }
}

int RescanWifi(void *args) {
  printf("Rescan async call to IPC \n");

  printf("Rerender on callback \n");
  return 0;
}

int DeleteConn(void *args) {
  printf("Prompt user about Saved connections \n");
  printf("Delete request to IPC \n");

  printf("Notify delete sucessfull \n");
  return 0;
}

int EnableDisableWifi(void *args) {
  // Callback called by on Validating input for Enable or Disable Wifi
  bool Enable;
  if (!Enable) {

    Enable = (bool)args;

    if (Enable == true) {
      printf("Enable Wifi Call to IPC\n");
      return 0;
    }
  }
  printf("Disable Wifi Call to IPC\n");
  return 0;
}

bool PopulateNMRelatedOptions(NMClient *client, RenderString *RenderString,
                              RenderList *RenderList) {
  // Adds all Netwok related options in Network List
  // Enable/Disable Wifi
  GetWifiState(client, RenderString);
  // Delete a Connection
  AddRenderEntry("Delete a Connection", -1, RenderString, RenderList,
                 &DeleteConn);
  // Rescan Wifi
  AddRenderEntry("Rescan Wifi", -1, RenderString, RenderList, &RescanWifi);
  // Show Password
  AddRenderEntry("Show Password", -1, RenderString, RenderList, NULL);
  // Saved Connections
  AddRenderEntry("Saved Connections", -1, RenderString, RenderList, NULL);

  return true;
}

char *Render(char *string) {
  int i = strlen(string);
  int j = strlen("echo ''");
  char *command;
  printf("LOG: Memory allocated for bytes %d \n", (int)(i + j + 1));
  command = malloc((i + j + 1) * sizeof(char));
  if (command == NULL) {
    printf("Memory allocation failed \n");
    return NULL;
  }
  command[0] = '\0';
  strcpy(command, "echo '");
  strcat(command, string);
  strcat(command, "'");

  char *buffer = CreateProcess(
      command, "| rofi -dmenu -theme ~/.config/rofi/wifi/config.rasi");
  printf("%s\n", buffer);
  free(command);

  return buffer;
}

void ProcessInput(char *Input) {

  ValidateOP(Input, "Enable Wifi", &EnableDisableWifi);
  ValidateOP(Input, "Disable Wifi", &EnableDisableWifi);
  ValidateOP(Input, "Rescan Wifi", &RescanWifi);
  ValidateOP(Input, "Delete a Connection", &DeleteConn);
}

int main() {
  NMClient *client;
  client = CreateClient();

  RenderString *RenderString;
  RenderString = InitRenderString();

  RenderList *RenderList;
  RenderList = InitRenderList();

  bool Networks, Options = false;
  char *string, *Input;
  if (client != NULL) {
    // Rescan test
    bool Networks = PopulateNetworks(client, RenderString, RenderList);

    bool Options = PopulateNMRelatedOptions(client, RenderString, RenderList);
  }
  if (!(Networks == true && Options == true)) {
    RenderString->string[RenderString->length++] = '\0';
    // printf("%s\n", RenderString->string);
    Input = Render(RenderString->string);
    ProcessInput(Input);
  }

  for (int i = 0; i < RenderList->length; i++) {
    printf("%s  entry registered \n", RenderList->List[i]);
  }

  free(RenderString->string);
  free(RenderString);
  free(RenderList->List);
  free(RenderList->callbackArray);
  free(RenderList);
  free(Input);
  return 0;
}
