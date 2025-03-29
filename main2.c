#include <libnm/nm-dbus-interface.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <libnm/NetworkManager.h>

typedef enum { false, true } bool;

#define INITIAL_SIZE 64
#define MAX_SIZE 4096

#define CONNLIST_SIZE 20;
#define NMLIST_SIZE 20;
#define RENDERLIST_SIZE 40;

struct _Connlist {
  char **ssidList;
  int *strengthList;
  int count;
  int capacity;
};

struct _Nmlist {
  char **List;
  int count;
  int capacity;
};

struct _RenderList {
  char **List;
  int count;
  int capacity;
};

typedef struct _Connlist ConnList;
typedef struct _Nmlist NMList;
typedef struct _RenderList RenderList;

ConnList *InitConnList() {

  ConnList *ConnList;

  ConnList->count = 0;
  ConnList->capacity = CONNLIST_SIZE;
  ConnList->ssidList = (char **)malloc(sizeof(char *) * ConnList->capacity);
  ConnList->strengthList = (int *)malloc(sizeof(int) * ConnList->capacity);

  if (ConnList->ssidList == NULL || ConnList->strengthList == NULL) {
    printf("Memory allocation failed \n");
    return NULL;
  } else {
    return ConnList;
  }
}

int AddConnList(ConnList *ConnList, char *ssid, int strength) {

  if (ConnList->count <= ConnList->capacity) {
    ConnList->ssidList[ConnList->count] = ssid;
    ConnList->strengthList[ConnList->count] = strength;
    ConnList->count++;
    return 0;
  }

  else {
    printf("Overflow Connlist\n");
    return 1;
  }
}

NMList *InitNMList() {
  NMList *NMlist = (NMList *)malloc(sizeof(NMList));
  if (NMlist == NULL) {
    printf("Memory allocation failed for NMList structure\n");
    return NULL;
  }

  NMlist->count = 0;
  NMlist->capacity = NMLIST_SIZE;
  NMlist->List = (char **)malloc(sizeof(char *) * NMlist->capacity);

  if (NMlist->List == NULL) {
    printf("Memory allocation failed for List array\n");
    free(NMlist);
    return NULL;
  }

  return NMlist;
}

int AddNMList(NMList *NMList, char *option) {
  if (NMList->count <= NMList->capacity) {

    NMList->List[NMList->count] = option;
    NMList->count++;
    return 0;
  } else {
    printf("NMList Overflow");
    return 1;
  }
}

RenderList *InitRenderList() {
  RenderList *Renderlist = (RenderList *)malloc(sizeof(RenderList));
  if (Renderlist == NULL) {
    printf("Memory allocation failed for RenderList structure\n");
    return NULL;
  }

  Renderlist->count = 0;
  Renderlist->capacity = RENDERLIST_SIZE;
  Renderlist->List = (char **)malloc(sizeof(char *) * Renderlist->capacity);

  if (Renderlist->List == NULL) {
    printf("Memory allocation failed for List array\n");
    free(Renderlist);
    return NULL;
  }

  return Renderlist;
}

int PopulateRenderList(RenderList *RenderList, ConnList *ConnList,
                       NMList *NMList) {
  char *entry;
  for (int i = 0; i < ConnList->count; i++) {
    char StrengthBuffer[5];

    snprintf(StrengthBuffer, sizeof(StrengthBuffer), " %d",
             ConnList->strengthList[i]);

    size_t ssid_len = strlen(ConnList->ssidList[i]);
    size_t strength_len = strlen(StrengthBuffer);
    size_t total_len = ssid_len + strength_len + 1;

    entry = malloc(total_len);
    if (entry == NULL) {
      return -1;
    }

    entry[0] = '\0';

    strcat(entry, ConnList->ssidList[i]);
    strcat(entry, StrengthBuffer);

    if (RenderList->count <= RenderList->capacity) {
      RenderList->List[RenderList->count] = entry;
      RenderList->count++;
    } else {
      printf("RenderList Overflow \n");
      return 1;
    }
  }

  for (int i = 0; i < NMList->count; i++) {
    if (RenderList->count < RenderList->capacity) {
      RenderList->List[RenderList->count] = NMList->List[i];
      RenderList->count++;
    }
  }

  return 0;
}

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

void ProcessWifiAP(NMDeviceWifi *device, ConnList *ConnList) {
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
          AddConnList(ConnList, ssid, strength);
        }

      } else {
        printf("SSID not available\n");
      }
    }

  } else {
    printf("cannot request Access Points from device");
  }
}

int GetNetworks(NMClient *client, ConnList *ConnList) {
  // Populate the Connection List with avalilable networks

  // Request all the available devices managed by the client
  const GPtrArray *devices = nm_client_get_devices(client);

  if (devices == NULL) {
    printf("Erorr requesting devices from client \n");
    return 1;
  } else {
    for (guint i = 0; i < devices->len; i++) {
      // Iterate over GPtrArray , request APs for wifi device
      const NMDevice *device = (NMDevice *)g_ptr_array_index(devices, i);
      if (device != NULL) {
        NMDeviceType deviceType = nm_device_get_device_type(device);

        if (deviceType == NM_DEVICE_TYPE_WIFI) {
          // ProcessWifiAP adds networks to Connection List
          ProcessWifiAP((NMDeviceWifi *)device, ConnList);
        }
      }
    }
    return 0;
  }
}

int EnableDisableWifi(NMClient *client, NMList *NMList) {
  // Requests state of Wifi and adds appropriately to Network List
  bool WifiState = (bool)nm_client_wireless_get_enabled(client);
  if (WifiState == true) {
    char *option = "Disable Wifi";
    AddNMList(NMList, option);
    return 0;
  }

  else {
    char *option = "Enable Wifi";
    AddNMList(NMList, option);
    return 0;
  }
}

bool PopulateNMRelatedOptions(NMClient *client, NMList *NMList) {
  // Adds all Netwok related options in Network List
  // Enable/Disable Wifi
  EnableDisableWifi(client, NMList);
  // Delete a Connection
  AddNMList(NMList, "Delete a Connection");
  // Rescan Wifi
  AddNMList(NMList, "Rescan Wifi");
  // Show Password
  AddNMList(NMList, "Show Password");
  // Saved Connections
  AddNMList(NMList, "Saved Connections");

  return true;
}

char **CreateRenderArray(RenderList *RenderList) {
  char **Array =
      malloc((sizeof(char **) * RenderList->count) + RenderList->count);
  if (Array != NULL) {

    for (int i = 0; i < RenderList->count; i++) {

      size_t entrySize = sizeof(RenderList->List[i]);
      Array[i] = RenderList->List[i];
    }
    return Array;
  } else {
    return NULL;
  }
}

int main() {

  NMClient *client;
  client = CreateClient();

  ConnList *ConnList;
  ConnList = InitConnList();

  NMList *NMList;
  NMList = InitNMList();

  RenderList *RenderList;
  RenderList = InitRenderList();

  if (client != NULL) {
    // Rescan test
    GetNetworks(client, ConnList);

    bool PopulatedList = PopulateNMRelatedOptions(client, NMList);
  }

  PopulateRenderList(RenderList, ConnList, NMList);

  char **Array = CreateRenderArray(RenderList);
  // TODO RenderArrray change to string , remove \0 for each element

  free(ConnList);
  free(NMList);
  free(RenderList);
  free(Array);

  return 0;
}
