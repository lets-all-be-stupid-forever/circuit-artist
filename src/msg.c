#include "msg.h"

#include <math.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdlib.h>

#include "colors.h"
#include "font.h"
#include "utils.h"

typedef struct Msg {
  // Text to be displayed
  char* txt;
  // Time when the message will expire.
  float expire_at;
  // Linked list handle.
  struct Msg* nxt;
} Msg;

static struct {
  Msg* msg_queue;
} C = {0};

void MsgAdd(const char* msg_txt, float duration) {
  Msg* m = malloc(sizeof(Msg));
  m->txt = CloneString(msg_txt);
  m->expire_at = GetTime() + duration;
  m->nxt = C.msg_queue;
  C.msg_queue = m;
}

void MsgDraw(Ui* ui) {
  Msg* m = C.msg_queue;
  int y = 50;
  int sw = GetScreenWidth();
  int s = 2;
  int dh = 24;
  float now = GetTime();
  Color r = GetLutColor(COLOR_BTN2);
  Color r2 = r;
  r2.a = 255 * 0.3;
  while (m) {
    rlPushMatrix();
    rlScalef(s, s, 1);
    float dt = m->expire_at - now;
    Vector2 size = GetRenderedTextSize(m->txt);
    int x = sw / s / 2 - size.x / 2;
    int p = 6;
    DrawRectangle(x - p, y - p, size.x + 2 * p, size.y + 2 * p, BLACK);
    DrawRectangle(x - p, y - p, size.x + 2 * p, size.y + 2 * p, r2);
    DrawRectangle(x - p + 1, y - p + 1, size.x + 2 * p - 2, size.y + 2 * p - 2,
                  BLACK);
    float t = sinf(10 * dt);
    Color c = r;
    c.a = (t * 0.4 + 0.6) * 255;
    FontDrawTexture(m->txt, x, y, c);
    rlPopMatrix();
    y += dh;
    m = m->nxt;
  }
}

void MsgUpdate() {
  Msg* prv = NULL;
  Msg* m = C.msg_queue;
  double now = GetTime();
  while (m) {
    Msg* nxt = m->nxt;
    if (now > m->expire_at) {
      free(m->txt);
      free(m);
      m = prv;
      if (prv) {
        prv->nxt = nxt;
      } else {
        C.msg_queue = nxt;
      }
    }
    prv = m;
    m = nxt;
  }
}
