#ifndef CONTROL_UI_H
#define CONTROL_UI_H

#include "controls.h"

void show_message(
    Controls *controls,
    const char *message
);

gboolean update_info_label(
    Controls *controls
);

guint control_ui_start_info_timer(
    Controls *controls
);

#endif
