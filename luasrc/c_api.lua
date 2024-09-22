require 'raylib_api'
local ffi = require("ffi")
ffi.cdef[[

typedef struct {
  Texture2D tex;
  Rectangle region;
} Sprite;


typedef struct {
  const char* help_name[50];
  const char* help_txt[50];
  Sprite help_sprites[50][20];
  struct {
    int ilevel;
    const char* name;
    const char* desc;
    Sprite icon;
    Sprite sprites[20];
  } options[60];
  char* startup_image_path;
} LevelOptions;


typedef struct {
  int ni;
  int no;
  bool dirty;
  int wires_in_x[128];
  int wires_in_y[128];
  int wires_out_x[128];
  int wires_out_y[128];
} ExtComp;

typedef enum {
  CONN_INPUT,
  CONN_OUTPUT,
} ConnType;

typedef struct {
  int num_conn;
  struct {
    ConnType type;
    int len;
    const char* name;
  } conn[50];
} PinDesc;

typedef struct {
  int ilevel;
  int num_components;
  PinDesc* pindesc;
  ExtComp* extcomps;
} LevelDesc;

typedef struct {
  int ic;
  int* prev_in;
  int* next_in;
  int* output;
  bool reset;
} ComponentUpdateCtx;

typedef struct {
  float elapsed;
  float clock_time;
  float dt;
  float cx;
  float cy;
  float cs;
  RenderTexture2D rt;
  int requested_level;
  int clock_update_value;
  int clock_count;
  bool por;
  LevelDesc level_desc;
  LevelOptions level_options;
  ComponentUpdateCtx update_ctx;
} SharedState;

SharedState* GetSharedState();

int printf(const char *fmt, ...);
void free( void * pointer );
void * malloc( size_t memorySize );
void CaDrawText(const char* txt, int x, int y, Color c);
int CaGetDrawTextSize(const char* txt);
void CaDrawTextBox(const char* txt, int x, int y, int w, Color c);
void CaSetPalFromImage(Image img);
void CaAddMessage(const char* txt, float duration);
void CaSetStartupImage(const char* path);

typedef struct RlDrawTextureProArgs{
 Texture2D texture;
 Rectangle source;
 Rectangle dest;
 Vector2 origin;
 float rotation;
 Color tint;
} RlDrawTextureProArgs;

void RlDrawTexturePro(RlDrawTextureProArgs* args);

]]

return ffi.C
