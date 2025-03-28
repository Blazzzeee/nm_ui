#include <libnm/nm-dbus-interface.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <libnm/NetworkManager.h>

typedef enum { false, true } bool;

#define INITIAL_SIZE 64
#define MAX_SIZE 4096

struct ConnList {
  char **ssidList;
  int *strengthList;
  int count;
  int capacity;
};

struct ConnList *InitConnList() {

  struct ConnList *ConnList;

  ConnList->count = 0;
  ConnList->capacity = 20;
  ConnList->ssidList = (char **)malloc(sizeof(char *) * ConnList->capacity);
  ConnList->strengthList = (int *)malloc(sizeof(int) * ConnList->capacity);

  if (ConnList->ssidList == NULL || ConnList->strengthList == NULL) {
    printf("Memory allocation failed \n");
    return NULL;
  } else {
    return ConnList;
  }
}

int AddConnList(struct ConnList *ConnList, char *ssid, int strength) {

  ConnList->ssidList[ConnList->count] = ssid;
  ConnList->strengthList[ConnList->count] = strength;
  ConnList->count++;

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
  client = nm_client_new(NULL, NULL);

  if (client != NULL) {
    const char *versionInfo = nm_client_get_version(client);
    printf("%s \n", versionInfo);
    return client;
  }

  else {
    printf("NetworkManager not running\n");
    return NULL;
  }
}

void GetNetworks(NMClient *client, struct ConnList *ConnList) {
  // Get currently active connection
  const GPtrArray *devices = nm_client_get_devices(client);

  for (guint i = 0; i < devices->len; i++) {
    const NMDevice *device = (NMDevice *)g_ptr_array_index(devices, i);
    NMDeviceType deviceType = nm_device_get_device_type(device);

    switch (deviceType) {
    case NM_DEVICE_TYPE_WIFI: {
      const GPtrArray *APList =
          nm_device_wifi_get_access_points((NMDeviceWifi *)device);
      for (guint i = 0; i < APList->len; i++) {
        NMAccessPoint *AP = (NMAccessPoint *)g_ptr_array_index(APList, i);
        GBytes *ssid_bytes = nm_access_point_get_ssid(AP);
        guint strength = nm_access_point_get_strength(AP);
        if (ssid_bytes) {
          gsize len;
          const char *raw_ssid = g_bytes_get_data(ssid_bytes, &len);

          char *ssid = g_strndup(raw_ssid, len);
          AddConnList(ConnList, ssid, strength);

        } else {
          printf("SSID not available\n");
        }
      }
    }
    default:
      true;
    }
  }
}

int main() {

  NMClient *client;
  client = CreateClient();

  struct ConnList *ConnList;
  ConnList = InitConnList();

  if (client != NULL) {
    // Rescan test
    GetNetworks(client, ConnList);
  }

  for (int i = 0; i < ConnList->count; i++) {
    printf("SSID: %s Strength: %d\n", ConnList->ssidList[i],
           ConnList->strengthList[i]);
  }

  return 0;
}
