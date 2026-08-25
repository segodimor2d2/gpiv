#ifndef PLAYER_H
#define PLAYER_H

#include <mpv/client.h>

typedef struct _Player Player;


/* ============================================================
 * CRIAÇÃO / DESTRUIÇÃO
 * ============================================================ */

Player *player_new(void);

void player_free(
    Player *player
);


/* ============================================================
 * INICIALIZAÇÃO
 *
 * O player é responsável somente pelo mpv.
 *
 * O mpv_render_context pertence ao render.c.
 * ============================================================ */

int player_initialize(
    Player *player,
    const char *filename
);


/* ============================================================
 * MPV HANDLE
 * ============================================================ */

mpv_handle *player_get_mpv(
    Player *player
);


/* ============================================================
 * ARQUIVO
 * ============================================================ */

/*
 * Carrega ou troca o arquivo atualmente
 * reproduzido pelo mpv.
 */
int player_load_file(
    Player *player,
    const char *filename
);

/* ============================================================
 * INFORMAÇÕES DO VÍDEO
 * ============================================================ */

/*
 * Retorna a posição atual e a duração total em segundos.
 *
 * position:
 *     tempo atual do vídeo
 *
 * duration:
 *     duração total do vídeo
 *
 * Retorna:
 *     0  = sucesso
 *    -1  = erro
 */
int player_get_time(
    Player *player,
    double *position,
    double *duration
);


/*
 * Retorna o nome/path do arquivo atualmente reproduzido.
 *
 * A string pertence ao PlayerApp/FileList.
 * Não deve ser liberada pelo chamador.
 */
const char *player_get_filename(
    Player *player
);


/* ============================================================
 * EVENTOS
 * ============================================================ */

void player_check_events(
    Player *player
);


/* ============================================================
 * REPRODUÇÃO
 * ============================================================ */

void player_toggle_pause(
    Player *player
);

void player_frame_back(
    Player *player
);

void player_frame_forward(
    Player *player
);

/* ============================================================
 * SEEK
 * ============================================================ */

void player_seek_forward(
    Player *player,
    int seconds
);

void player_seek_backward(
    Player *player,
    int seconds
);

/* ============================================================
 * VOLUME
 * ============================================================ */

void player_change_volume(
    Player *player,
    int amount
);

/* ============================================================
 * ZOOM
 * ============================================================ */

void player_change_zoom(
    Player *player,
    double amount
);


/* ============================================================
 * PAN
 * ============================================================ */

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


/* ============================================================
 * RESET VIEW
 * ============================================================ */

void player_reset_view(
    Player *player
);


/* ============================================================
 * IMAGEM
 * ============================================================ */

void player_rotate(
    Player *player
);

void player_change_brightness(
    Player *player,
    int amount
);

void player_change_contrast(
    Player *player,
    int amount
);

void player_change_saturation(
    Player *player,
    int delta
);

void player_change_gamma(
    Player *player,
    int amount
);

/* ============================================================
 * SCREENSHOT
 * ============================================================ */

const char *player_save_frame(
    Player *player
);

#endif
