#include "level_api.h"

#include "font.h"
#include "stb_ds.h"
#include "ui.h"

void level_api_add_port(LevelAPI* api, int width, const char* id, int type,
                        bool right) {
  PinGroup pg = {0};
  pg.id = clone_string(id), pg.type = type;
  PinGroup* dst = right ? api->pg_right : api->pg_left;
  int y = 0;
  int n = arrlen(dst);
  if (n > 0) {
    int wg = arrlen(dst[n - 1].pins);
    // position of last pin of previous port
    y = dst[n - 1].pins[wg - 1].y;
    y += 4;
  }
  y += 4;
  for (int i = 0; i < width; i++) {
    int px = right ? -1 : 0;
    int py = y + 2 * i;
    pg_add_pin(&pg, px, py);
  }
  if (right) {
    arrput(api->pg_right, pg);
  } else {
    arrput(api->pg_left, pg);
  }
  arrput(api->pg, pg);
}

void level_api_destroy(LevelAPI* api) {
  for (int i = 0; i < arrlen(api->pg); i++) {
    pg_destroy(&api->pg[i]);
  }
  arrfree(api->pg_left);
  arrfree(api->pg_right);
  arrfree(api->pg);
  if (api->u) {
    api->destroy(api->u);
  }
  *api = (LevelAPI){0};
}

static void draw_pin_sockets(PinGroup* l_pg, Cam2D cam, int w, int h,
                             RenderTexture target) {
  BeginTextureMode(target);
  rlPushMatrix();
  rlTranslatef(cam.off.x, cam.off.y, 0);
  rlScalef(cam.sp, cam.sp, 1);
  for (int ig = 0; ig < arrlen(l_pg); ig++) {
    PinGroup* pg = &l_pg[ig];
    int npin = arrlen(pg->pins);
    for (int ipin = 0; ipin < npin; ipin++) {
      int x = pg->pins[ipin].x;
      int y = pg->pins[ipin].y;
      if (x < 0) x = w + x;
      if (y < 0) y = h + y;
      Rectangle source;
      Texture sprite = ui_get_sprites();
      Rectangle target = {x, y, 1, 1};
      int type = pg->type;
      Color clr;
      if (type == PIN_LUA2IMG) {
        clr = GREEN;
        source = (Rectangle){576, 160, 16, 16};
      } else {
        clr = RED;
        source = (Rectangle){576, 160 + 16, 16, 16};
      }
      if (cam.sp < 16) {
        source = (Rectangle){576 + 16, 160, 16, 16};
      }
      DrawTexturePro(sprite, source, target, (Vector2){0, 0}, 0, clr);
    }
  }
  rlPopMatrix();
  EndTextureMode();
}

void level_api_draw_pin_sockets(LevelAPI* api, Cam2D cam, int w, int h,
                                RenderTexture target) {
  draw_pin_sockets(api->pg_left, cam, w, h, target);
  draw_pin_sockets(api->pg_right, cam, w, h, target);
}

void level_api_draw_board(LevelAPI* api, Cam2D cam, int w, int h,
                          RenderTexture rt) {
  PinGroup* pg = api->pg;
  int ng = arrlen(pg);
  BeginTextureMode(rt);
  ClearBackground(BLANK);
  rlPushMatrix();
  rlTranslatef(cam.off.x, cam.off.y, 0);
  rlScalef(cam.sp, cam.sp, 1);
  for (int ig = 0; ig < ng; ig++) {
    int np = arrlen(pg[ig].pins);
    int th = 2 * np;
    int lh = 8;
    int y = pg[ig].pins[0].y;
    y = y + (th - lh) / 2;
    int x = pg[ig].pins[0].x;
    bool input = pg[ig].type == PIN_LUA2IMG;
    const char* name;
    if (x == 0) {
      if (input) {
        name = TextFormat("%s ->", pg[ig].id);
      } else {
        name = TextFormat("%s <-", pg[ig].id);
      }
      int tw = get_rendered_text_size(name).x;
      font_draw_texture(name, -tw - 2, y, WHITE);
    } else {
      if (input) {
        name = TextFormat("<- %s", pg[ig].id);
      } else {
        name = TextFormat("-> %s", pg[ig].id);
      }
      font_draw_texture(name, w + 2, y, WHITE);
    }
  }
  rlPopMatrix();
  EndTextureMode();
}
