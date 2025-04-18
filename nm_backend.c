#include "glib.h"
#include "stdbool.h"
#include "utils.h"
#include <libnm/NetworkManager.h>
#include <libnm/nm-core-types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

SessionContext *global_ctx;

SessionContext *InitialiseSession() {
  // Initialise Session Context
  // The function is used to create Global Context that will be used for the
  // current render NOTE this must be followed by TerminateSession or it will
  // cause memory leak
  NMClient *client;
  client = CreateClient();

  RenderString *RenderString;
  RenderString = InitRenderString();

  RenderList *RenderList;
  RenderList = InitRenderList();

  SessionContext *SessionContext;
  SessionContext = malloc(sizeof(*SessionContext));

  SessionContext->RegisteredActions = false;
  SessionContext->client = client;
  SessionContext->RenderList = RenderList;
  SessionContext->RenderString = RenderString;
  SessionContext->ActiveSsid = NULL;
  SessionContext->Index = -1;

  if (!SessionContext) {
    printf("LOG: Error could not allocate memory to Session Context \n");
    return NULL;
  }

  return SessionContext;
}

// Section Helpers

char *CreateProcess(char *Rawcommand, char *args) {
  // Run shell command and accept arguments and return its output as a string
  // pointer
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
  // The function determines whether the RenderList entry matches the user input
  // On identifying user input , invoke its registered callback
  // This function also has responsibility to map the Index of RenderList and
  // update it in global context
  void *callbackArgs;
  int i = *(int *)args;
  char *match = strstr(global_ctx->Input, global_ctx->RenderList->List[i]);
  if (match != NULL) {
    // TODO Create a thread to suspend all the running threads on finding a
    // match
    global_ctx->Index = i;
    global_ctx->RenderList->callbackArray[i](callbackArgs);
    g_print("LOG: Identified User Input with index from RenderList\n");
  } else {
  }
}

void StartEventLoop();

// Section Callbacks

static void property_set_callback(GObject *object, GAsyncResult *result,
                                  gpointer user_data) {
  // Called by nm_client_dbus_set_property and
  // nm_client_deactivate_connection_async after the GMainLoop is run the
  // GMainLoop processes all the async callbacks that were registered
  GMainLoop *loop = (GMainLoop *)user_data;
  GError *error = NULL;
  nm_client_dbus_set_property_finish(NM_CLIENT(object), result, &error);
  if (error) {
    g_printerr("lOG: Error setting property: %s\n", error->message);
    g_error_free(error);
  } else {
    g_print("LOG: Successfully set property\n");
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
  global_ctx->RegisteredActions = true;
  printf("Enable Wifi Call to IPC\n");
  return NULL;
}

void *DisableWifi(void *args) {

  // Callback called by on Validating input for Disable Wifi
  GVariant *val = g_variant_new_boolean(FALSE);
  nm_client_dbus_set_property(global_ctx->client, NM_DBUS_PATH,
                              NM_DBUS_INTERFACE, "WirelessEnabled", val, -1,
                              NULL, property_set_callback, global_ctx->loop);
  printf("Disable Wifi Call to IPC\n");

  global_ctx->RegisteredActions = true;
  return NULL;
}

static void RescanCallback(GObject *source_object, GAsyncResult *res,
                           gpointer data) {
  // This function is used as the actual callback to perform Rescan event

  // TODO Add polling for rescan complete and add timeout
  g_main_loop_quit(global_ctx->loop);
  g_print("Scanning networks");
  sleep(2);

  g_print("Scan completed \n");
  g_print("Rerendering GUI \n");

  Terminate(global_ctx);
  StartEventLoop();
  // Scan failure
}

void *RescanWifi(void *args) {
  // This function follow OnValidate Generic and is called by identifying user
  // Input linked to Rescan wifi
  bool WifiEnabled = (bool)nm_client_wireless_get_enabled(global_ctx->client);

  if (WifiEnabled) {

    nm_device_wifi_request_scan_async((NMDeviceWifi *)global_ctx->device, NULL,
                                      RescanCallback, NULL);
    global_ctx->RegisteredActions = true;
    return NULL;
  }

  else {
    g_print("Wireless not Enabled on device, Cannot rescan , Enable Wifi "
            "first\n");
    return NULL;
  }
}

void *DeleteConn(void *args) {
  // TODO To be implemented
  printf("Prompt user about Saved connections \n");
  printf("Delete request to IPC \n");

  printf("Notify delete sucessfull \n");
  return NULL;
}

char *ConvertSsid(GBytes *ssid_bytes) {
  // Helper used to convert bytes to string
  // if the bytes are valid
  gsize len;
  // change bytes to data
  const char *raw_ssid = g_bytes_get_data(ssid_bytes, &len);
  // Safe copy the ssid into ssid;
  char *ssid = g_strndup(raw_ssid, len);

  return ssid;
}

void HandleDisconnect() {
  // This function is callback used on identifying user giving Disconnect
  // request

  const GPtrArray *activeList =
      nm_client_get_active_connections(global_ctx->client);

  if (!activeList) {
    g_print("LOG: Could not request active connection list\n");
  }

  for (guint i = 0; i < activeList->len; i++) {
    // Iterate over GPtrArray , request APs for wifi device
    NMActiveConnection *device = g_ptr_array_index(activeList, i);
    // Fetch all active connections to get NMActiveConnection object
    const char *ssid = nm_active_connection_get_id(device);

    if (strcmp(ssid, global_ctx->ActiveSsid) == 0) {
      // Register GMainLoop callback
      nm_client_deactivate_connection_async(global_ctx->client, device, NULL,
                                            property_set_callback,
                                            global_ctx->loop);
      global_ctx->RegisteredActions = true;
    }
  }
}

void HandleConnect() {
  // Get connections and check if known
  const char *ssid = global_ctx->RenderList->List[global_ctx->Index];
  const GPtrArray *conns = nm_client_get_connections(global_ctx->client);
  if (!conns) {
    g_print("Could not fetch active connections\n");
  }

  for (guint i = 0; i < conns->len; i++) {
    // Iterate over GPtrArray , request APs for wifi device
    NMConnection *conn = g_ptr_array_index(conns, i);
    const char *id = nm_connection_get_id(conn);
    if (conn != NULL) {
      if (strcmp(id, ssid) == 0) {
        g_print("Found matching connection %s\n", ssid);
        // TODO do a NMClient connect
        nm_client_activate_connection_async(
            global_ctx->client, conn, (NMDevice *)global_ctx->device, NULL,
            NULL, property_set_callback, global_ctx->loop);
        global_ctx->RegisteredActions = true;
      }
    }
  }
  g_print("No connection was found");
}

void *ProcessConnect(void *args) {
  // This is a generic callback that processes Wifi connection and
  // disconnection requests The function is registered while adding entries
  // to RenderList
  if (global_ctx->Index != -1) {

    if (global_ctx->ActiveSsid != NULL &&
        strstr(global_ctx->ActiveSsid,
               global_ctx->RenderList->List[global_ctx->Index])) {
      g_print("Handle Disconnect to %s", global_ctx->Input);
      HandleDisconnect();
    } else {
      HandleConnect();
      return NULL;
    }
  } else {
    g_print("LOG: User input index was not identified\n");
  }

  // Connect if known network
  // Prompt password if unknown
}

// Section runtime

int GetWifiState(SessionContext *SessionContext) {
  // Requests state of Wifi and adds appropriately to Network List
  bool WifiState = (bool)nm_client_wireless_get_enabled(SessionContext->client);
  if (WifiState == true) {
    char *option = "Disable Wifi";
    AddRenderEntry(option, -1, false, SessionContext, &DisableWifi);
    return 0;
  }

  else {
    char *option = "Enable Wifi";
    AddRenderEntry(option, -1, false, SessionContext, &EnableWifi);
    return 0;
  }
}

void ProcessWifiAP(NMDeviceWifi *device, SessionContext *SessionContext) {
  // Request Access Point(networks : connected + available) from the current
  // device
  NMAccessPoint *active_ap = nm_device_wifi_get_active_access_point(device);
  if (active_ap) {
    GBytes *active_ssid_bytes = nm_access_point_get_ssid(active_ap);
    char *ssid = ConvertSsid(active_ssid_bytes);
    if (ssid) {
      g_print("LOG: Active connection was identified\n");
      global_ctx->ActiveSsid = ssid;
    }
  } else {
    g_print("LOG: Could not determine active connection \n");
  }

  const GPtrArray *APList = nm_device_wifi_get_access_points(device);
  if (APList != NULL) {
    for (guint i = 0; i < APList->len; i++) {
      // Iterate over the Access Point List
      NMAccessPoint *ap = (NMAccessPoint *)g_ptr_array_index(APList, i);
      if (ap != NULL) {
        GBytes *ssid_bytes = nm_access_point_get_ssid(ap);
        // ssid is stored as raw_bytes
        guint strength = nm_access_point_get_strength(ap);
        // request signal strength
        if (ssid_bytes) {
          char *ssid = ConvertSsid(ssid_bytes);
          if (active_ap == ap) {
            // If active connection
            AddRenderEntry(ssid, strength, true, SessionContext,
                           &ProcessConnect);

          } else {
            // Non active connections
            AddRenderEntry(ssid, strength, false, SessionContext,
                           &ProcessConnect);
          }
        }

      } else {
        printf("SSID not available\n");
      }
    }

  } else {
    printf("cannot request Access Points from device");
  }
}

void *PopulateNetworks(SessionContext *SessionContext) {
  // Populate the Connection List with avalilable networks

  // Request all the available devices managed by the client
  const GPtrArray *devices = nm_client_get_devices(SessionContext->client);

  if (devices == NULL) {
    g_print("LOG: Erorr requesting devices from client \n");
    return NULL;
  } else {
    for (guint i = 0; i < devices->len; i++) {
      // Iterate over GPtrArray , request APs for wifi device
      const NMDevice *device = (NMDevice *)g_ptr_array_index(devices, i);
      if (device != NULL) {
        NMDeviceType deviceType = nm_device_get_device_type(device);

        if (deviceType == NM_DEVICE_TYPE_WIFI) {
          // ProcessWifiAP adds networks to Connection List
          global_ctx->device = (NMDeviceWifi *)device;
          ProcessWifiAP((NMDeviceWifi *)device, SessionContext);
        }
      }
    }
    return NULL;
  }
}

void *PopulateNMRelatedOptions(SessionContext *SessionContext) {
  // Adds all Netwok related options in Network List
  // Enable/Disable Wifi
  GetWifiState(SessionContext);
  // Delete a Connection
  AddRenderEntry("Delete a Connection", -1, false, SessionContext, &DeleteConn);
  // Rescan Wifi
  AddRenderEntry("Rescan Wifi", -1, false, SessionContext, &RescanWifi);
  // Show Password
  AddRenderEntry("Show Password", -1, false, SessionContext, NULL);
  // Saved Connections
  AddRenderEntry("Saved Connections", -1, false, SessionContext, NULL);

  return NULL;
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
  char *buffer = CreateProcess(
      command, "| rofi -dmenu -theme ~/.config/rofi/wifi/config.rasi");
  printf("%s\n", buffer);
  free(command);

  // Remember to free output buffer
  return buffer;
}

void CreateThreads(SessionContext *SessionContext) {
  // This function is ued to Create wokrer threads that perform
  // simultanteous string Validation to determine user input this function
  // must also suspend these worker threads

  // Initialise Thread array
  pthread_t *threadArray;
  int *args;

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
  // TODO replace with thread suspend
  for (int i = 0; i < SessionContext->RenderList->length; i++) {
    *args = i + 1;
    pthread_join(threadArray[i], NULL);
    printf("Joined Thread: %d\n", *args);
  }
}

void StartEventLoop() {
  // The actual event loop that performs all the actions
  global_ctx = InitialiseSession();

  char *string;
  void *args = NULL;

  if (global_ctx->client != NULL) {
    PopulateNetworks(global_ctx);
    PopulateNMRelatedOptions(global_ctx);
  }
  global_ctx->RenderString->string[global_ctx->RenderString->length++] = '\0';
  // printf("%s\n", RenderString->string);
  global_ctx->Input = Render(global_ctx->RenderString->string);

  global_ctx->loop = g_main_loop_new(NULL, TRUE);
  CreateThreads(global_ctx);

  if (global_ctx->RegisteredActions) {
    g_main_loop_run(global_ctx->loop);
    g_main_loop_quit(global_ctx->loop);
  }
}

int main() {
  StartEventLoop();
  Terminate(global_ctx);
  return 0;
}
