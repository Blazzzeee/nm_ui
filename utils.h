#ifndef UTILS_H
#define UTILS_H

#include "stdbool.h"
#include <libnm/NetworkManager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_SIZE 64
#define SIZE_MAX 4096

#define RENDER_LIST_SIZE 512
#define MAX_ENTRIES 100

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
  NMDeviceWifi *device;
  RenderString *RenderString;
  RenderList *RenderList;
  GMainLoop *loop;
  bool RegisteredActions;
  char *Input;
  char *ActiveSsid;
  int Index;
};

typedef struct _SessionContext SessionContext;

RenderString *InitRenderString(void);
RenderList *InitRenderList(void);
void AddRenderEntry(char *ssid, int strength, bool active,
                    SessionContext *SessionContext, onValidate callback);

NMClient *CreateClient();

void Terminate(SessionContext *global_ctx);

#endif
