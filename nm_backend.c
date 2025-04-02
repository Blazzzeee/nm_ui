#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libnm/NetworkManager.h>

typedef enum { false, true } bool;

#define INITIAL_SIZE 64
#define SIZE_MAX 4096

#define CONNLIST_SIZE 20;
#define NMLIST_SIZE 20;
#define RENDERLIST_SIZE 40;

struct _renderString {
  char *string;
  size_t size;
  int length;
};

typedef struct _renderString RenderString;

RenderString *InitRenderString() {
  RenderString *renderString = malloc(sizeof(RenderString));
  renderString->length = 0;
  renderString->size = sizeof(char) * 1024;
  renderString->string = (char *)malloc(renderString->size);

  return renderString;
}

void AddRenderString(char *ssid, int strength, RenderString *RenderString) {
  char strengthBuffer[4];

  if (RenderString->length >= RenderString->size) {
    printf("DEBUG: Buffer Overflow reallocating size\n");
    size_t newSize = RenderString->size * 2;
    if (newSize <= SIZE_MAX) {
      char *temp = realloc(RenderString->string, newSize);
      if (!temp) {
        RenderString->string = temp;
      }
      RenderString->size = newSize;
    }
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

char *Create_Process(char *Rawcommand, char *args) {
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

void ProcessWifiAP(NMDeviceWifi *device, RenderString *RenderString) {
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
          AddRenderString(ssid, strength, RenderString);
        }

      } else {
        printf("SSID not available\n");
      }
    }

  } else {
    printf("cannot request Access Points from device");
  }
}

int PopulateNetworks(NMClient *client, RenderString *RenderString) {
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
          ProcessWifiAP((NMDeviceWifi *)device, RenderString);
        }
      }
    }
    return true;
  }
}

int EnableDisableWifi(NMClient *client, RenderString *RenderString) {
  // Requests state of Wifi and adds appropriately to Network List
  bool WifiState = (bool)nm_client_wireless_get_enabled(client);
  if (WifiState == true) {
    char *option = "Disable Wifi";
    AddRenderString(option, -1, RenderString);
    return 0;
  }

  else {
    char *option = "Enable Wifi";
    AddRenderString(option, -1, RenderString);
    return 0;
  }
}

bool PopulateNMRelatedOptions(NMClient *client, RenderString *RenderString) {
  // Adds all Netwok related options in Network List
  // Enable/Disable Wifi
  EnableDisableWifi(client, RenderString);
  // Delete a Connection
  AddRenderString("Delete a Connection", -1, RenderString);
  // Rescan Wifi
  AddRenderString("Rescan Wifi", -1, RenderString);
  // Show Password
  AddRenderString("Show Password", -1, RenderString);
  // Saved Connections
  AddRenderString("Saved Connections", -1, RenderString);

  return true;
}

void Render(char *string) {
  int i = strlen(string);
  int j = strlen("echo ''");
  char *command;
  printf("LOG: Memory allocated for bytes %d \n", (int)(i + j + 1));
  command = malloc((i + j + 1) * sizeof(char));
  if (command == NULL) {
    printf("Memory allocation failed \n");
    return;
  }
  command[0] = '\0';
  strcpy(command, "echo '");
  strcat(command, string);
  strcat(command, "'");

  char *buffer = Create_Process(
      command, "| rofi -dmenu -theme ~/.config/rofi/wifi/config.rasi");
  printf("%s\n", buffer);
  free(buffer);
  free(command);
}

int main() {
  NMClient *client;
  client = CreateClient();

  RenderString *RenderString;
  RenderString = InitRenderString();

  bool Networks, Options = false;
  char *string;
  if (client != NULL) {
    // Rescan test
    bool Networks = PopulateNetworks(client, RenderString);

    bool Options = PopulateNMRelatedOptions(client, RenderString);
  }

  RenderString->string[RenderString->length++] = '\0';
  printf("%s\n", RenderString->string);
  Render(RenderString->string);

  free(RenderString);
  return 0;
}
