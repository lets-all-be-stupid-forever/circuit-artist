#include "modal.h"

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#define NFD_NATIVE
#include <nfd.h>

static nfdresult_t ModalInitResult = NFD_ERROR;

// Initialize the file system modal library
void modal_init() {
  ModalInitResult = NFD_Init();
  if (ModalInitResult == NFD_ERROR) {
    printf("ERROR: Couldnt initialize modal.\n");
    abort();
  }
}

// De-initialize the file system modal library
void modal_destroy() {
  if (ModalInitResult == NFD_OKAY) NFD_Quit();
}

// Opens a save file system modal.
ModalResult modal_save_file(const char* default_path,
                            const char* default_name) {
  ModalResult mr = {0};
  nfdchar_t* outpath = NULL;
  nfdu8filteritem_t filter_list[] = {{"Image", "png"}};
  nfdresult_t result =
      NFD_SaveDialogU8(&outpath, filter_list, 1, default_path, default_name);
  if (result != NFD_CANCEL && result != NFD_OKAY) {
    mr.errMsg = NFD_GetError();
  }
  mr.fPath = outpath;
  mr.ok = result == NFD_OKAY;
  mr.cancel = result == NFD_CANCEL;
  return mr;
}

// Opens an open file system modal.
ModalResult modal_open_file(const char* default_path) {
  nfdchar_t* outpath = NULL;
  nfdu8filteritem_t filter_list[] = {{"Image", "png"}};
  const nfdnchar_t* n_default_path = default_path;
  nfdresult_t result =
      NFD_OpenDialogU8(&outpath, filter_list, 1, n_default_path);
  ModalResult mr = {0};
  mr.ok = result == NFD_OKAY;
  mr.fPath = outpath;
  mr.cancel = result == NFD_CANCEL;
  if (result != NFD_CANCEL && result != NFD_OKAY) {
    mr.errMsg = NFD_GetError();
  }
  return mr;
}

ModalResult modal_open_file_lua(const char* default_path) {
  nfdchar_t* outpath = NULL;
  nfdu8filteritem_t filter_list[] = {{"Lua Script", "lua"}};
  const nfdnchar_t* n_default_path = default_path;
  nfdresult_t result =
      NFD_OpenDialogU8(&outpath, filter_list, 1, n_default_path);
  ModalResult mr = {0};
  mr.ok = result == NFD_OKAY;
  mr.fPath = outpath;
  mr.cancel = result == NFD_CANCEL;
  if (result != NFD_CANCEL && result != NFD_OKAY) {
    mr.errMsg = NFD_GetError();
  }
  return mr;
}

// Destructor for the moddal result.
void modal_destroy_result(ModalResult mr) {
  if (mr.fPath) {
    free(mr.fPath);
  }
}
