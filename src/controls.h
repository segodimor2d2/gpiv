#ifndef CONTROLS_H
#define CONTROLS_H

#include <gtk/gtk.h>

#include "app.h"


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
 * app:
 *     PlayerApp responsável pela aplicação,
 *     FileList e navegação entre arquivos.
 * ============================================================ */

void controls_setup(
    GtkWidget *window,
    GtkWidget *gl_area,
    GtkWidget *info_label,
    PlayerApp *app
);

#endif
