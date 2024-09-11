#include "msg.h"

#include "font.h"
#include "raylib.h"
#include "rlgl.h"
#include "stdlib.h"
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

void MsgAdd(const char* msg_txt, float duration)
{
  Msg* m = malloc(sizeof(Msg));
  m->txt = CloneString(msg_txt);
  m->expire_at = GetTime() + duration;
  m->nxt = C.msg_queue;
  C.msg_queue = m;
}

void MsgDraw(Ui* ui)
{
  Msg* m = C.msg_queue;
  int y = 100;
  int x = 100;
  int dh = 20;
  while (m) {
    rlPushMatrix();
    rlScalef(3, 3, 1);
    FontDrawTexture(m->txt, x + 1, y + 1, BLACK);
    FontDrawTexture(m->txt, x, y, WHITE);
    rlPopMatrix();
    y += dh;
    m = m->nxt;
  }
}

void MsgUpdate()
{
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
      }
      else {
        C.msg_queue = nxt;
      }
    }
    prv = m;
    m = nxt;
  }
}

