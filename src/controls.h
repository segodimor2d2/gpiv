#ifndef CONTROLS_H
#define CONTROLS_H

#include <gtk/gtk.h>

#include "render.h"


/* ============================================================
 * CONTROLES
 *
 * Conecta teclado, mouse, scroll e drag aos widgets.
 *
 * window:
 *     janela principal
 *
 * gl_area:
 *     GtkGLArea usado pelo Render
 *
 * info_label:
 *     label usado para mensagens
 *
 * render:
 *     objeto responsável pela renderização e Player
 *
 * filename:
 *     arquivo atualmente reproduzido
 * ============================================================ */

void controls_setup(
    GtkWidget *window,
    GtkWidget *gl_area,
    GtkWidget *info_label,
    Render *render,
    const char *filename
);

#endif
