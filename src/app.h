#ifndef APP_H
#define APP_H

typedef struct _PlayerApp PlayerApp;

PlayerApp *player_app_new(const char *filename);

int player_app_run(
    PlayerApp *pa,
    int argc,
    char **argv
);

void player_app_free(PlayerApp *pa);

#endif
