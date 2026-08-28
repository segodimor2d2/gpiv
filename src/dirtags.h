#ifndef DIRTAGS_H
#define DIRTAGS_H

#include "controls.h"


void dirtags_clear(
    Controls *controls
);


void dirtags_add_unique(
    Controls *controls,
    const char *tag
);


void dirtags_build(
    Controls *controls
);


void dirtags_create_directories(
    Controls *controls
);

#endif
