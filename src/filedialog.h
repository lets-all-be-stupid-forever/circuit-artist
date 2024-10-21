#ifndef FILEDIALOG_H
#define FILEDIALOG_H
#include <stdbool.h>

// Result of the call to a file modal (open or save).
// There's a need to call UnloadModalResult once the results have been treated.
typedef struct {
  // Chosen file path.
  char* path;
  // OK flag. If false, it means an error has occurred.
  bool ok;
  // Cancel flag.
  bool cancel;
  // If not ok, contains a message describing why it didnt work.
  const char* error_msg;
} ModalResult;

// Initialize the file system modal library
void ModalLoad();

// De-initialize the file system modal library
void ModalUnload();

// Opens a save file system modal.
ModalResult ModalSaveFile(const char* default_path, const char* default_name);

// Opens an open file system modal.
ModalResult ModalOpenFile(const char* default_path);

// Destructor for the moddal result.
void UnloadModalResult(ModalResult mr);

#endif
