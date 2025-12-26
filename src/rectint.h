#ifndef CA_RECTINT_H
#define CA_RECTINT_H
#include "common.h"

bool rect_int_is_empty(RectangleInt r);
bool rect_int_check_collision(RectangleInt a, RectangleInt b);
RectangleInt rect_int_get_collision(RectangleInt a, RectangleInt b);

#endif
