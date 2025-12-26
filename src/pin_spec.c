#include "pin_spec.h"

#include "stb_ds.h"

void pg_add_pin(PinGroup* pg, int x, int y) {
  arrput(pg->pins, ((Coord2D){x, y}));
}

void pg_destroy(PinGroup* pg) {
  if (pg->id) free(pg->id);
  if (pg->pins) arrfree(pg->pins);
  *pg = (PinGroup){0};
}
