#ifndef CA_UTILS_H
#define CA_UTILS_H

#include "common.h"
#include "lua.h"
#include "raylib.h"
#include "stdbool.h"

void draw_win(Rectangle r, const char* title);
void draw_tiled_screen(int s, Texture2D tex, Rectangle src);
void draw_tiled_rect(int s, Texture2D tex, Rectangle src, Rectangle dst);
void draw_default_tiled_screen();
void draw_default_tiled_frame(Rectangle inner_content);

char* clone_string(const char* str);
bool is_control_down();
bool next_token(const char* str, int* ci, char sep, char* out);
void print_matrix(const char* name, Matrix m);

typedef struct {
  char* data;
  int size;
  int cap;
} str_builder_t;

void str_builder_init(str_builder_t* sb);
void str_builder_add_raw(str_builder_t* sb, const char* txt);
void str_builder_add(str_builder_t* sb, const char* fmt, ...);
void str_builder_destroy(str_builder_t* sb);

sprite_t load_sprite_asset(const char* fname);
void load_text_sprites(const char* root, const char* txt,
                       sprite_t** out_sprites);
void delete_file(const char* path);

/* Hack for layout parsing */
Rectangle roff(Vector2 off, Rectangle r);

Rectangle* parse_layout_asset(const char* asset);
Vector2 find_modal_off(Rectangle layout);

typedef struct {
  Rectangle* rects;
  Color* colors;
} ParsedLayout;

ParsedLayout parse_layout2(const char* fname);

// Fuzzy matching functions
bool fuzzy_match_simple(const char* target, const char* query);
int fuzzy_score(const char* target, const char* query);

// File utilities
char** find_png_files(const char* folder_path);
char* extract_filename_no_ext(const char* filepath);

sprite_t* read_text_sprites(const char* desc, const char* root);
int dofile_with_traceback(lua_State* L, const char* filename);

struct TexPool;

typedef struct TexItem {
  RenderTexture rt;
  int refc; /* uses = 0 --> Free, uses>0 --> being used */
  int uses;
  int total_uses;
  int w;
  int h;
  struct TexItem* next;
  struct TexPool* pool;
} TexItem;

typedef struct TexPool {
  TexItem* items;
} TexPool;

TexPool* texpool_create();
TexItem* texpool_get(TexPool* pool, int w, int h);
TexItem* texitem_newlike(TexItem* t);
// TexItem* texitem_ref(TexItem* t);
void texpool_pre_render(TexPool* pool);
void texpool_post_render(TexPool* pool);
// void texitem_decref(TexItem* t);
void texpool_destroy(TexPool* pool);

void draw_bg(Rectangle r);
TexItem* incref(TexItem* v);
void decref(TexItem* v);
const char* randid();

const char* get_roman_number(int i);

#endif
