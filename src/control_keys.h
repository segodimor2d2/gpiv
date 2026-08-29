#ifndef CONTROL_KEYS_H
#define CONTROL_KEYS_H

#include "controls.h"

gboolean control_keys_handle(
    GtkEventControllerKey *controller,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    Controls *controls
);

#endif
