#ifndef CA_MODAL_H
#define CA_MODAL_H
#include "stdbool.h"

// Result of the call to a file modal (open or save).
// There's a need to call UnloadModalResult once the results have been treated.
typedef struct {
  char* fPath;         // Chosen file path.
  bool ok;             // OK flag. If false, it means an error has occurred.
  bool cancel;         // Cancel flag.
  const char* errMsg;  // If not ok, message describing why it didnt work.
} ModalResult;

void modal_init();
void modal_destroy();
ModalResult modal_save_file(const char* default_path, const char* default_name);
ModalResult modal_open_file(const char* default_path);
ModalResult modal_open_file_lua(const char* default_path);
void modal_destroy_result(ModalResult mr);

#endif
