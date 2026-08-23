#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>

#include "player.h"

typedef struct _PlayerApp {

    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *gl_area;
    GtkWidget *info_label;

    Player *player;

    const char *filename;

} PlayerApp;


PlayerApp *player_app_new(
    const char *filename
);


int player_app_run(
    PlayerApp *pa,
    int argc,
    char **argv
);


void player_app_free(
    PlayerApp *pa
);

#endif
