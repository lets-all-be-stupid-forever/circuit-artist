#ifndef CA_PIN_SPEC_H
#define CA_PIN_SPEC_H

#define PIN_IMG2LUA 0
#define PIN_LUA2IMG 1

typedef struct {
  int x;
  int y;
} Coord2D;

typedef struct {
  int type;      /* input/output */
  char* id;      /* Pin ID */
  Coord2D* pins; /* stb array with pins */
} PinGroup;

void pg_add_pin(PinGroup* pg, int x, int y);
void pg_destroy(PinGroup* pg);

#endif
