#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>

#include "render.h"


typedef struct _PlayerApp {

    GtkApplication *app;

    GtkWidget *window;

    GtkWidget *info_label;

    Render *render;

    /*
     * Arquivo atualmente reproduzido.
     *
     * O PlayerApp possui sua própria cópia.
     */
    char *filename;

} PlayerApp;


/* ============================================================
 * CRIAÇÃO
 * ============================================================ */

PlayerApp *player_app_new(
    const char *filename
);


/* ============================================================
 * EXECUÇÃO
 * ============================================================ */

int player_app_run(
    PlayerApp *pa,
    int argc,
    char **argv
);


/* ============================================================
 * DESTRUIÇÃO
 * ============================================================ */

void player_app_free(
    PlayerApp *pa
);

#endif
