#ifndef CA_FIND_NANDS_H
#define CA_FIND_NANDS_H

#include "common.h"

/* Describes a NAND location in the image */
typedef struct {
  int s1; /* socket 1 index */
  int s2; /* socket 2 index */
  int d;  /* driver index */
} NandLoc;

void find_nands(int w, int h, u8* img, NandLoc** out_nands);
void remove_nand_pixels(int w, u8* img, NandLoc* nands);

#endif
