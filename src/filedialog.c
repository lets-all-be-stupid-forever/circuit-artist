#include "filedialog.h"

#include <stdlib.h>

#define NFD_NATIVE
#include <nfd.h>

static nfdresult_t ModalInitResult = NFD_ERROR;
// TODO: do something in case of error
void ModalLoad() { ModalInitResult = NFD_Init(); }

void ModalUnload() {
  if (ModalInitResult == NFD_OKAY) NFD_Quit();
}

ModalResult ModalSaveFile(const char* default_path, const char* default_name) {
  ModalResult mr = {0};
  nfdchar_t* outpath = NULL;
  nfdu8filteritem_t filter_list[] = {{"Image", "png"}};
  nfdresult_t result =
      NFD_SaveDialogU8(&outpath, filter_list, 1, default_path, default_name);
  if (result != NFD_CANCEL && result != NFD_OKAY) {
    mr.error_msg = NFD_GetError();
  }
  mr.path = outpath;
  mr.ok = result == NFD_OKAY;
  mr.cancel = result == NFD_CANCEL;
  return mr;
}

ModalResult ModalOpenFile(const char* default_path) {
  nfdchar_t* outpath = NULL;
  nfdu8filteritem_t filter_list[] = {{"Image", "png"}};
  const nfdnchar_t* n_default_path = default_path;
  nfdresult_t result =
      NFD_OpenDialogU8(&outpath, filter_list, 1, n_default_path);
  ModalResult mr = {0};
  mr.ok = result == NFD_OKAY;
  mr.path = outpath;
  mr.cancel = result == NFD_CANCEL;
  if (result != NFD_CANCEL && result != NFD_OKAY) {
    mr.error_msg = NFD_GetError();
  }
  return mr;
}

void UnloadModalResult(ModalResult mr) {
  if (mr.path) {
    free(mr.path);
  }
}
