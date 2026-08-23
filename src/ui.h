#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>

#include "render.h"


/* ============================================================
 * CRIAÇÃO DA INTERFACE
 * ============================================================ */

GtkWidget *ui_create_window(
    GtkApplication *app,
    Render *render,
    const char *filename,
    GtkWidget **info_label
);


/* ============================================================
 * CSS
 * ============================================================ */

void ui_setup_css(void);

#endif
