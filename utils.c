#include "utils.h"
#include "glib.h"
#include <libnm/NetworkManager.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Section Data Structures

RenderString *InitRenderString() {
  RenderString *renderString = malloc(sizeof(RenderString));
  if (!renderString) {
    printf("LOG: Error: Could not allocate RenderString struct\n");
    return NULL;
  }

  renderString->length = 0;
  renderString->size = sizeof(char) * RENDER_LIST_SIZE;
  renderString->string = (char *)malloc(renderString->size);
  if (!renderString->string) {
    printf("LOG: Could not allocate memory for RenderString->string\n");
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

  renderList->List = malloc(sizeof(char *) * SIZE_MAX);
  renderList->callbackArray = malloc(sizeof(onValidate) * MAX_ENTRIES);

  if (!renderList->List || !renderList->callbackArray) {
    printf("LOG: Error: Could not allocate memory for List or callbackArray\n");
    free(renderList->List);
    free(renderList->callbackArray);
    free(renderList);
    return NULL;
  }

  printf("LOG: Created RenderList\n");
  return renderList;
}

void AddRenderEntry(char *ssid, int strength, bool active,
                    SessionContext *SessionContext, onValidate callback) {
  char strengthBuffer[4];

  if (SessionContext->RenderString->length >=
      SessionContext->RenderString->size) {
    printf("LOG: Buffer Overflow reallocating size\n");
    int newSize = SessionContext->RenderString->size * 2;
    if (newSize <= SIZE_MAX) {
      char *temp = realloc(SessionContext->RenderString->string, newSize);
      if (temp) {
        SessionContext->RenderString->string = temp;
        SessionContext->RenderString->size = newSize;
        printf("LOG: Reallocated successfully size to RenderString %zu\n",
               SessionContext->RenderString->size);
      } else {
        printf("LOG: Failed to reallocate RenderString\n");
        return;
      }
    }
  }

  if (SessionContext->RenderList && callback) {
    if (SessionContext->RenderList->length >=
        SessionContext->RenderList->size) {
      printf("LOG: Buffer Overflow reallocating size for RenderList\n");
      int newSize = SessionContext->RenderList->size * 2;
      if (newSize <= SIZE_MAX) {
        char **temp =
            realloc(SessionContext->RenderList->List, newSize * sizeof(char *));
        if (temp) {
          SessionContext->RenderList->List = temp;
          SessionContext->RenderList->size = newSize;
          printf("LOG: Reallocated successfully size to RenderList %d\n",
                 newSize);
        } else {
          printf("LOG: Failed to reallocate RenderList\n");
          return;
        }
      }
    }

    SessionContext->RenderList->List[SessionContext->RenderList->length] = ssid;
    SessionContext->RenderList
        ->callbackArray[SessionContext->RenderList->length++] = callback;
  }

  if (active) {
    SessionContext->RenderString
        ->string[SessionContext->RenderString->length++] = '=';
    SessionContext->RenderString
        ->string[SessionContext->RenderString->length++] = '=';
    SessionContext->RenderString
        ->string[SessionContext->RenderString->length++] = ' ';

  } else {

    SessionContext->RenderString
        ->string[SessionContext->RenderString->length++] = ' ';
    SessionContext->RenderString
        ->string[SessionContext->RenderString->length++] = ' ';
    SessionContext->RenderString
        ->string[SessionContext->RenderString->length++] = ' ';
  }

  for (int i = 0; i < strlen(ssid); i++) {
    SessionContext->RenderString
        ->string[SessionContext->RenderString->length++] = ssid[i];
  }

  SessionContext->RenderString->string[SessionContext->RenderString->length++] =
      ' ';

  if (strength != -1) {
    // clamp 0–100
    if (strength < 0)
      strength = 0;
    if (strength > 100)
      strength = 100;

    // pick one of 5 glyphs by strength %
    // 0–24 → 0 bars, 25–49 → 1 bar, 50–74 → 2 bars, 75–99 → 3 bars, 100 → 4
    // bars
    const char *glyph = (strength < 25)    ? "󰤯"
                        : (strength < 50)  ? "󰤟"
                        : (strength < 75)  ? "󰤢"
                        : (strength < 100) ? "󰤥"
                                           : "󰤨";

    // append the UTF-8 bytes of that glyph into your render‐string
    for (const char *p = glyph; *p != '\0'; ++p) {
      SessionContext->RenderString
          ->string[SessionContext->RenderString->length++] = *p;
    }
  }

  SessionContext->RenderString->string[SessionContext->RenderString->length++] =
      '\n';
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

void Terminate(SessionContext *global_ctx) {

  g_print("LOG: Closing IPC Connection \n");
  g_print("LOG: Deallocating memory...\n");
  free(global_ctx->RenderString->string);
  free(global_ctx->RenderString);
  free(global_ctx->RenderList->List);
  free(global_ctx->RenderList->callbackArray);
  free(global_ctx->RenderList);
  free(global_ctx->Input);
  g_print("LOG: Memory was Deallocated Successfully\n");
  g_print("LOG: Closing GUI\n");
}
