#ifndef CONTROL_VIDEO_H
#define CONTROL_VIDEO_H

#include "controls.h"

void detect_video_format(
    Controls *controls,
    const char *filename
);

void convert_video_format(
    Controls *controls,
    const char *filename
);

#endif
