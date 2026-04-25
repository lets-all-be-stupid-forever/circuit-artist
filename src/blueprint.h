#ifndef CA_BLUEPRINT_H
#define CA_BLUEPRINT_H
#include "common.h"
#include "raylib.h"
#include "widgets.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define S_COLS 15
#define S_ROWS 7
#define F_COLS 2
#define NUM_PAGES S_COLS
#define NUM_FIXED (F_COLS * S_ROWS)
#define PAGESIZE (S_COLS * S_ROWS)
#define TOTAL_BLUEPRINTS (NUM_FIXED + NUM_PAGES * PAGESIZE + NUM_PAGES)

/* Blueprints */
typedef struct {
  char* name;
  char* desc;
  char* lvl;
  int rot;
  Texture thumbnail;

  /* This id lives only in runtime and on local saves
   * 2 options:
   *   local:<folder_name>
   *   steam:<steam_id>
   * */
  char* id;
  char* folder; /* Absolute path folder location*/
  int width;
  int height;
  int layers;
  u64 steam_id;
  char* steam_author_name;
  u64 steam_author_id;
} Blueprint;

typedef struct {
  Blueprint** blueprints;
  int nbp;
} BlueprintStore;

Blueprint* get_blueprint(BlueprintStore* store, int idx);
int find_available_fixed_slot(BlueprintStore* store);
int find_available_page_slot(BlueprintStore* store, int page);
int get_fixed_blueprint_idx(BlueprintStore* store, int i);
int get_page_blueprint_idx(BlueprintStore* store, int i);
int get_page_slot_blueprint_idx(BlueprintStore* store, int page, int i);

const char* blueprint_fname_full(Blueprint* s);
const char* blueprint_fname_thumbnail(Blueprint* s);
void blueprint_paste(Blueprint* bp);
void blueprint_rename(Blueprint* bp, const char* new_name);
void blueprint_rot(Blueprint* bp);
void blueprint_save_meta(Blueprint* s);
void blueprint_set_desc(Blueprint* bp, const char* new_desc);

void inject_blueprint_from_folder(BlueprintStore* store, const char* id,
                                  const char* folder);
int blueprint_create(BlueprintStore* store, int nl, Image* imgs, Image full);
void blueprint_store_swap(BlueprintStore* store, int i0, int i1);
void blueprint_store_init(BlueprintStore* store);
void blueprint_store_load(BlueprintStore* store);
void blueprint_store_rm(BlueprintStore* store, int idx);
void blueprint_store_save(BlueprintStore* store);
void blueprint_draw(Btn* b, Blueprint* s, int scale);
void blueprint_draw_leg(Btn* b, Blueprint* s, int scale);
void blueprint_update_thumbnail(Blueprint* s, int nl, Image* imgs);
bool blueprint_can_delete(Blueprint* s);
void blueprint_copy_to_clipboard(Blueprint* s);

#if defined(__cplusplus)
}
#endif

#endif
