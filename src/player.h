#ifndef PLAYER_H
#define PLAYER_H

#include <mpv/client.h>
#include <mpv/render_gl.h>

typedef struct _Player Player;


/*
 * Criação / destruição
 */

Player *player_new(void);

void player_free(
    Player *player
);


/*
 * Inicialização
 *
 * get_proc_address:
 *     função usada pelo mpv para obter
 *     os ponteiros OpenGL.
 *
 * update_callback:
 *     chamado pelo mpv quando precisa
 *     que a área OpenGL seja redesenhada.
 */

int player_initialize(
    Player *player,
    const char *filename,
    mpv_render_update_fn update_callback,
    void *update_callback_ctx
);


/*
 * Arquivo
 */

int player_load_file(
    Player *player,
    const char *filename
);


/*
 * Renderização
 */

mpv_render_context *player_get_render_context(
    Player *player
);


/*
 * Eventos
 */

void player_check_events(
    Player *player
);


/*
 * Reprodução
 */

void player_toggle_pause(
    Player *player
);

void player_frame_back(
    Player *player
);

void player_frame_forward(
    Player *player
);


/*
 * Zoom
 */

void player_change_zoom(
    Player *player,
    double amount
);


/*
 * Pan
 */

void player_pan_begin(
    Player *player,
    double x,
    double y
);

void player_pan_update(
    Player *player,
    double offset_x,
    double offset_y,
    int width,
    int height
);

void player_pan_end(
    Player *player
);


/*
 * Reset
 */

void player_reset_view(
    Player *player
);


/*
 * Imagem
 */

void player_rotate(
    Player *player
);

void player_change_brightness(
    Player *player,
    int amount
);


/*
 * Screenshot
 */

void player_save_frame(
    Player *player
);

#endif
