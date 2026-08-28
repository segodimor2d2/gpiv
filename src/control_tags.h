#ifndef CONTROL_TAGS_H
#define CONTROL_TAGS_H

#include <stddef.h>
#include "controls.h"

int get_file_directory(
    const char *filename,
    char *directory,
    size_t directory_size
);

void tags_load(
    Controls *controls,
    const char *directory
);

void tags_save(
    Controls *controls
);

int tags_find(
    Controls *controls,
    const char *path
);

const char *tags_get(
    Controls *controls,
    const char *path
);

void tags_clear(
    Controls *controls
);

void tag_file(
    Controls *controls,
    const char *path,
    const char *tag
);

void tags_update_directory(
    Controls *controls,
    const char *filename
);

gboolean tags_archive(
    Controls *controls
);


#endif
