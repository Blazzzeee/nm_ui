#include "glib.h"
#include <libnm/NetworkManager.h>
#include <libnm/nm-dbus-interface.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { false, true } bool;
#define INITIAL_SIZE 64
#define SIZE_MAX 4096

#define RENDER_LIST_SIZE 512
#define MAX_ENTRIES 50

typedef void *(*onValidate)(void *);

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

struct _SessionContext {
  NMClient *client;
  RenderString *RenderString;
  RenderList *RenderList;
  GMainLoop *loop;
};

typedef struct _SessionContext SessionContext;

SessionContext *global_ctx;

char *Input;

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

void AddRenderEntry(char *ssid, int strength, SessionContext *SessionContext,
                    onValidate callback) {
  char strengthBuffer[4];

  if (SessionContext->RenderString->length >=
      SessionContext->RenderString->size) {
    printf("LOG: Buffer Overflow reallocating size\n");
    int newSize = SessionContext->RenderString->size * 2;
    if (newSize <= SIZE_MAX) {
      char *temp = realloc(SessionContext->RenderString->string, newSize);
      printf("LOG: Reallocated sucessfully size to RenderString %zu",
             SessionContext->RenderString->size);
      if (!temp) {
        SessionContext->RenderString->string = temp;
      }
      SessionContext->RenderString->size = newSize;
    }
  }

  if (SessionContext->RenderList && callback) {

    if (SessionContext->RenderList->length >=
        SessionContext->RenderList->size) {

      printf("LOG: Buffer Overflow reallocating size\n");
      int newSize = SessionContext->RenderList->size * 2;
      if (newSize <= SIZE_MAX) {
        char **temp =
            realloc(SessionContext->RenderList->List, newSize * sizeof(char *));
        printf("LOG: Reallocated sucessfully size to RenderString %d",
               SessionContext->RenderList->size);
        if (temp) {
          SessionContext->RenderList->List = temp;
        }
        SessionContext->RenderList->size = newSize;
      }

      // Handle function list overflow
    }

    SessionContext->RenderList->List[SessionContext->RenderList->length] = ssid;
    SessionContext->RenderList
        ->callbackArray[SessionContext->RenderList->length++] = callback;
  }

  for (int i = 0; i < strlen(ssid); i++) {
    SessionContext->RenderString
        ->string[SessionContext->RenderString->length++] = ssid[i];
  }

  SessionContext->RenderString->string[SessionContext->RenderString->length++] =
      ' ';
  if (strength != -1) {

    snprintf(strengthBuffer, 4, "%d", strength);
    for (int i = 0; i < 3; i++) {
      if (strengthBuffer[i] != '\0') {

        SessionContext->RenderString
            ->string[SessionContext->RenderString->length++] =
            strengthBuffer[i];
      }
    }
  }

  SessionContext->RenderString->string[SessionContext->RenderString->length++] =
      '\n';
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

void *ValidateOP(void *args) {
  // Output is the output buffer returned by running subprocess
  // Expected is the Expected output of that command
  // The function returns whether the 'output' is similar to 'expected' or not

  // current approach string handling function
  void *callbackArgs;
  int i = *(int *)args;
  char *match = strstr(Input, global_ctx->RenderList->List[i]);
  if (match != NULL) {
    // TODO Create a thread to suspend all the running threads on finding a
    // match
    global_ctx->RenderList->callbackArray[i](callbackArgs);
  } else {
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

// Section Callbacks

static void property_set_callback(GObject *object, GAsyncResult *result,
                                  gpointer user_data) {
  // Called by nm_client_dbus_set_property after the GMainLoop is run
  // the GMainLoop processes all the async callbacks that were registered
  GMainLoop *loop = (GMainLoop *)user_data;
  GError *error = NULL;
  nm_client_dbus_set_property_finish(NM_CLIENT(object), result, &error);
  if (error) {
    g_printerr("Error setting property: %s\n", error->message);
    g_error_free(error);
  } else {
    g_print("Successfully set WirelessEnabled property.\n");
  }
  g_main_loop_quit(loop);
}

void *EnableWifi(void *args) {
  // Callback called by on Validating input for Enable Wifi

  // The GMainLoop must be started to register your async callbacks
  // In our case we use property_set_callback to complete the async callback
  // operation that was registered

  GVariant *val = g_variant_new_boolean(TRUE);
  nm_client_dbus_set_property(global_ctx->client, NM_DBUS_PATH,
                              NM_DBUS_INTERFACE, "WirelessEnabled", val, -1,
                              NULL, property_set_callback, global_ctx->loop);
  printf("Enable Wifi Call to IPC\n");
  return NULL;
}

void *DisableWifi(void *args) {

  GVariant *val = g_variant_new_boolean(FALSE);
  nm_client_dbus_set_property(global_ctx->client, NM_DBUS_PATH,
                              NM_DBUS_INTERFACE, "WirelessEnabled", val, -1,
                              NULL, property_set_callback, global_ctx->loop);
  printf("Disable Wifi Call to IPC\n");

  return NULL;
}

void *RescanWifi(void *args) {
  printf("Rescan async call to IPC \n");

  printf("Rerender on callback \n");
  return NULL;
}

void *DeleteConn(void *args) {
  printf("Prompt user about Saved connections \n");
  printf("Delete request to IPC \n");

  printf("Notify delete sucessfull \n");
  return NULL;
}

int GetWifiState(SessionContext *SessionContext) {
  // Requests state of Wifi and adds appropriately to Network List
  bool WifiState = (bool)nm_client_wireless_get_enabled(SessionContext->client);
  if (WifiState == true) {
    char *option = "Disable Wifi";
    AddRenderEntry(option, -1, SessionContext, &DisableWifi);
    return 0;
  }

  else {
    char *option = "Enable Wifi";
    AddRenderEntry(option, -1, SessionContext, &EnableWifi);
    return 0;
  }
}

void *ProcessConnect(void *args) {
  printf("Connect call to IPC \n");
  return 0;
}

void ProcessWifiAP(NMDeviceWifi *device, SessionContext *SessionContext) {
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
          // TODO Replace by nm_utils_ssid_to_utf-8
          // if the bytes are valid
          gsize len;
          // change bytes to data
          const char *raw_ssid = g_bytes_get_data(ssid_bytes, &len);
          // Safe copy the ssid into ssid;
          char *ssid = g_strndup(raw_ssid, len);
          // Add the ssid and signal strength to Connection List
          // AddConnList(ConnList, ssid, strength);
          // printf("DEBUG: ssid %s\n", ssid);
          AddRenderEntry(ssid, strength, SessionContext, &ProcessConnect);
        }

      } else {
        printf("SSID not available\n");
      }
    }

  } else {
    printf("cannot request Access Points from device");
  }
}

int PopulateNetworks(SessionContext *SessionContext) {
  // Populate the Connection List with avalilable networks

  // Request all the available devices managed by the client
  const GPtrArray *devices = nm_client_get_devices(SessionContext->client);

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
          ProcessWifiAP((NMDeviceWifi *)device, SessionContext);
        }
      }
    }
    return true;
  }
}

bool PopulateNMRelatedOptions(SessionContext *SessionContext) {
  // Adds all Netwok related options in Network List
  // Enable/Disable Wifi
  GetWifiState(SessionContext);
  // Delete a Connection
  AddRenderEntry("Delete a Connection", -1, SessionContext, &DeleteConn);
  // Rescan Wifi
  AddRenderEntry("Rescan Wifi", -1, SessionContext, &RescanWifi);
  // Show Password
  AddRenderEntry("Show Password", -1, SessionContext, NULL);
  // Saved Connections
  AddRenderEntry("Saved Connections", -1, SessionContext, NULL);

  return true;
}

char *Render(char *string) {
  // Construct the render command to render GUI
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
  // Capture output of the process by allocating buffer using CreateBuffer
  // TODO Trim symbols from user input
  char *buffer = CreateProcess(
      command, "| rofi -dmenu -theme ~/.config/rofi/wifi/config.rasi");
  printf("%s\n", buffer);
  free(command);

  // Remember to free output buffer
  return buffer;
}

void CreateThreads(SessionContext *SessionContext) {

  // Initialise Thread array
  pthread_t *threadArray;
  int *args;

  global_ctx->loop = g_main_loop_new(NULL, TRUE);
  // Assign memory dynamically depending on nummber of Entries rendered
  threadArray = malloc(sizeof(pthread_t) * SessionContext->RenderList->length);

  if (!threadArray) {
    printf("LOG: Level: Critical Error could not allocate pthread Array \n");
    return;
  }

  args = malloc(sizeof(int) * SessionContext->RenderList->length);
  if (!args) {
    printf("LOG: Critical Could not allocate memory to args Array \n");
    return;
  }

  // Create Threads
  for (int i = 0; i < SessionContext->RenderList->length; i++) {
    args[i] = i;
    pthread_create(&threadArray[i], NULL, ValidateOP,
                   (void *)args + (i * sizeof(int)));
  }
  // Wait for threads to finish

  for (int i = 0; i < SessionContext->RenderList->length; i++) {
    *args = i + 1;
    pthread_join(threadArray[i], NULL);
    printf("Joined Thread: %d\n", *args);
  }
}

SessionContext *InitialiseSession() {
  // Initialise Session Context
  NMClient *client;
  client = CreateClient();

  RenderString *RenderString;
  RenderString = InitRenderString();

  RenderList *RenderList;
  RenderList = InitRenderList();

  SessionContext *SessionContext;
  SessionContext = malloc(sizeof(SessionContext));

  SessionContext->client = client;
  SessionContext->RenderList = RenderList;
  SessionContext->RenderString = RenderString;

  if (!SessionContext) {
    printf("LOG: Error could not allocate memory to Session Context \n");
    return NULL;
  }

  return SessionContext;
}

int main() {

  global_ctx = InitialiseSession();

  bool Networks, Options = false;
  char *string;
  void *args = NULL;

  if (global_ctx->client != NULL) {
    bool Networks = PopulateNetworks(global_ctx);

    bool Options = PopulateNMRelatedOptions(global_ctx);
  }
  if (!(Networks == true && Options == true)) {
    global_ctx->RenderString->string[global_ctx->RenderString->length++] = '\0';
    // printf("%s\n", RenderString->string);
    Input = Render(global_ctx->RenderString->string);
    CreateThreads(global_ctx);
  }

  g_main_loop_run(global_ctx->loop);

  free(global_ctx->RenderString->string);
  free(global_ctx->RenderString);
  free(global_ctx->RenderList->List);
  free(global_ctx->RenderList->callbackArray);
  free(global_ctx->RenderList);
  free(Input);
  return 0;
}
