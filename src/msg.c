#include "msg.h"

#include <math.h>
#include <stdlib.h>

#include "colors.h"
#include "common.h"
#include "font.h"
#include "utils.h"

typedef struct msg_s {
  char* txt;          // Text to be displayed
  float expire_at;    // Time when the message will expire.
  struct msg_s* nxt;  // Linked list handle.
} msg_t;

static struct {
  msg_t* msgStack;  // popup message system
  bool clear_permanent;
} C = {0};

void msg_init() { C.clear_permanent = false; }

// Adds a message that will expire within `duration` seconds.
// If msgDuration is negative, the message is permanent until msg_clear_permanent() is called.
void msg_add(const char* msg_txt, float msgDuration) {
  msg_t* m = malloc(sizeof(msg_t));
  m->txt = clone_string(msg_txt);
  if (msgDuration < 0) {
    m->expire_at = msgDuration;  // Store negative value to indicate permanent
  } else {
    m->expire_at = GetTime() + msgDuration;
  }
  m->nxt = C.msgStack;
  C.msgStack = m;
}

// Draws messages on screen.
void msg_draw() {
  msg_t* m = C.msgStack;
  int y = 50;
  int sw = GetScreenWidth();
  int s = 2;
  int dh = 24;
  float now = GetTime();
  Color r = get_lut_color(COLOR_BTN2);
  Color r2 = r;
  r2.a = 255 * 0.3;
  while (m) {
    rlPushMatrix();
    rlScalef(s, s, 1);
    float dt = m->expire_at - now;
    Vector2 size = get_rendered_text_size(m->txt);
    int x = sw / s / 2 - size.x / 2;
    int p = 6;
    DrawRectangle(x - p, y - p, size.x + 2 * p, size.y + 2 * p, BLACK);
    DrawRectangle(x - p, y - p, size.x + 2 * p, size.y + 2 * p, r2);
    DrawRectangle(x - p + 1, y - p + 1, size.x + 2 * p - 2, size.y + 2 * p - 2,
                  BLACK);
    float t = sinf(10 * dt);
    Color c = r;
    c.a = (t * 0.4 + 0.6) * 255;
    font_draw_texture(m->txt, x, y, c);
    rlPopMatrix();
    y += dh;
    m = m->nxt;
  }
}

bool msg_can_remove(msg_t* m, double now) {
  if (m->expire_at > 0) {
    return now > m->expire_at;
  } else {
    return C.clear_permanent;
  }
}

// Updates lifetime of messages.
void msg_update() {
  msg_t* prv = NULL;
  msg_t* m = C.msgStack;
  double now = GetTime();
  while (m) {
    msg_t* nxt = m->nxt;
    if (msg_can_remove(m, now)) {
      free(m->txt);
      free(m);
      m = prv;
      if (prv) {
        prv->nxt = nxt;
      } else {
        C.msgStack = nxt;
      }
    }
    prv = m;
    m = nxt;
  }
  C.clear_permanent = false;
}

void msg_clear_permanent() { C.clear_permanent = true; }
