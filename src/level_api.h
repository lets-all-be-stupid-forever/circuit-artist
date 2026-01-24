#ifndef CA_LEVEL_API
#define CA_LEVEL_API
#include "buffer.h"
#include "pin_spec.h"
#include "status.h"

struct Sim;

typedef struct {
  void* u;
  PinGroup* pg;
  PinGroup* pg_left;
  PinGroup* pg_right;
  void (*destroy)(void* u);
  Status (*start)(void* u, struct Sim* sim);
  Status (*update)(void* u, Buffer* buffer);
  Status (*fw)(void* u, Buffer buf);
  Status (*bw)(void* u, Buffer buf);
  Status (*draw)(void* u);
} LevelAPI;

void level_api_add_port(LevelAPI* api, int width, const char* id, int type,
                        bool right);
void level_api_draw_pin_sockets(LevelAPI* api, Cam2D cam, int w, int h,
                                RenderTexture target);
void level_api_draw_board(LevelAPI* api, Cam2D cam, int w, int h,
                          RenderTexture rt);
void level_api_destroy(LevelAPI* api);

#endif

