#include "filedialog.h"

#include "stdlib.h"

#define NFD_NATIVE
#include "nfd.h"

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
