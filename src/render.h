#ifndef RENDER_H
#define RENDER_H

#include <gtk/gtk.h>

#include "player.h"


typedef struct _Render Render;


/* ============================================================
 * CRIAÇÃO / DESTRUIÇÃO
 * ============================================================ */

Render *render_new(
    const char *filename
);

void render_free(
    Render *render
);


/* ============================================================
 * GTK WIDGET
 * ============================================================ */

GtkWidget *render_get_widget(
    Render *render
);


/* ============================================================
 * PLAYER
 * ============================================================ */

Player *render_get_player(
    Render *render
);

#endif
