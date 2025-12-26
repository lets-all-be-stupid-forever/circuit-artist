#ifndef CA_NANDRDR_H
#define CA_NANDRDR_H
#include "common.h"

typedef struct {
  int idx;
  int rot;
  Color c1; /* Socket 1 */
  Color c2; /* Socket 2 */
  Color c3; /* Driver */
} NandDesc;

typedef struct {
  int activation_counter;
  int id_nand;
  int next_value;
  int max_delay;
  float de;  // energy spent per tick
} NandState;

typedef struct {
  int* pos[4];     // each orientation is treated separately
  int* bugged[4];  // each orientation is treated separately
  u32 vbo[4];
  u32 vbo_vertices[4];
  int vao[4];
  Texture2D nand_tex;
  Vector4 roi[4];
  Color* colors[4];
  int error_mode;
} NandRenderer;

void nand_renderer_init(NandRenderer* r);
void nand_renderer_set_error_mode(NandRenderer* r, int error_mode);
void nand_renderer_add(NandRenderer* r, int rot, int x, int y, Color c1,
                       Color c2, Color c3, int bugged);
void nand_renderer_prepare(NandRenderer* r);
void nand_renderer_render(NandRenderer* r, Cam2D cam, float utime);
void nand_renderer_destroy(NandRenderer* r);

void render_nand_states(int w, NandDesc* nidx, int ns, NandState* states,
                        Cam2D cam, float slack, float utime);

#endif
