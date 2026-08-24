#include "player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ============================================================
 * PLAYER
 * ============================================================ */

struct _Player {

    mpv_handle *mpv;

    /*
     * Ponteiro para o caminho armazenado pela FileList.
     *
     * Player NÃO é dono dessa string.
     */
    const char *filename;

    /*
     * Estado visual.
     */

    int video_rotation;

    int brightness;

    double video_zoom;

    double video_pan_x;
    double video_pan_y;

    /*
     * Estado do pan.
     */

    int panning;

    double pan_start_x;
    double pan_start_y;

    double pan_start_pan_x;
    double pan_start_pan_y;
};


/* ============================================================
 * COMANDOS MPV
 * ============================================================ */

static int player_set_property(
    Player *player,
    const char *property,
    const char *value)
{
    if (!player ||
        !player->mpv ||
        !property ||
        !value)
        return -1;


    const char *command[] = {

        "set",
        property,
        value,
        NULL
    };


    int status =
        mpv_command(
            player->mpv,
            command
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro set %s=%s: %s\n",
            property,
            value,
            mpv_error_string(status)
        );

        return status;
    }


    return 0;
}


/* ============================================================
 * CRIAÇÃO
 * ============================================================ */

Player *player_new(void)
{
    Player *player =
        calloc(
            1,
            sizeof(Player)
        );


    if (!player)
        return NULL;


    player->mpv = NULL;

    player->filename = NULL;

    player->video_rotation = 0;

    player->brightness = 0;

    player->video_zoom = 0.0;

    player->video_pan_x = 0.0;
    player->video_pan_y = 0.0;

    player->panning = 0;

    player->pan_start_x = 0.0;
    player->pan_start_y = 0.0;

    player->pan_start_pan_x = 0.0;
    player->pan_start_pan_y = 0.0;


    return player;
}


/* ============================================================
 * DESTRUIÇÃO
 * ============================================================ */

void player_free(
    Player *player)
{
    if (!player)
        return;


    if (player->mpv) {

        fprintf(
            stderr,
            "[MPV] encerrando mpv\n"
        );


        mpv_terminate_destroy(
            player->mpv
        );


        player->mpv = NULL;
    }


    /*
     * filename pertence à FileList.
     *
     * Portanto NÃO fazemos free(filename).
     */

    player->filename = NULL;


    free(player);
}


/* ============================================================
 * INICIALIZAÇÃO
 * ============================================================ */

int player_initialize(
    Player *player,
    const char *filename)
{
    if (!player ||
        !filename)
        return -1;


    fprintf(
        stderr,
        "[MPV] criando mpv\n"
    );


    player->mpv =
        mpv_create();


    if (!player->mpv) {

        fprintf(
            stderr,
            "[MPV] ERRO: mpv_create() retornou NULL\n"
        );

        return -1;
    }


    /* --------------------------------------------------------
     * OPÇÕES
     * -------------------------------------------------------- */

    int status;


    status =
        mpv_set_option_string(
            player->mpv,
            "vo",
            "libmpv"
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro configurando vo: %s\n",
            mpv_error_string(status)
        );

        goto error;
    }


    status =
        mpv_set_option_string(
            player->mpv,
            "hwdec",
            "auto"
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro configurando hwdec: %s\n",
            mpv_error_string(status)
        );

        goto error;
    }


    status =
        mpv_set_option_string(
            player->mpv,
            "loop-file",
            "yes"
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro configurando loop-file: %s\n",
            mpv_error_string(status)
        );

        goto error;
    }


    status =
        mpv_set_option_string(
            player->mpv,
            "msg-level",
            "all=warn"
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro configurando msg-level: %s\n",
            mpv_error_string(status)
        );

        goto error;
    }


    /* --------------------------------------------------------
     * INICIALIZA
     * -------------------------------------------------------- */

    status =
        mpv_initialize(
            player->mpv
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] ERRO mpv_initialize(): %s\n",
            mpv_error_string(status)
        );

        goto error;
    }


    fprintf(
        stderr,
        "[MPV] mpv_initialize() OK\n"
    );


    /* --------------------------------------------------------
     * PRIMEIRO ARQUIVO
     * -------------------------------------------------------- */

    status =
        player_load_file(
            player,
            filename
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] ERRO carregando arquivo inicial\n"
        );

        mpv_terminate_destroy(
            player->mpv
        );

        player->mpv = NULL;

        return -1;
    }


    return 0;


error:

    mpv_terminate_destroy(
        player->mpv
    );

    player->mpv = NULL;


    return -1;
}


/* ============================================================
 * GET MPV
 * ============================================================ */

mpv_handle *player_get_mpv(
    Player *player)
{
    if (!player)
        return NULL;


    return player->mpv;
}


/* ============================================================
 * LOAD FILE
 * ============================================================ */

int player_load_file(
    Player *player,
    const char *filename)
{
    if (!player ||
        !player->mpv ||
        !filename)
        return -1;


    fprintf(
        stderr,
        "[PLAYER] load file: %s\n",
        filename
    );


    const char *command[] = {

        "loadfile",
        filename,
        "replace",
        NULL
    };


    int status =
        mpv_command(
            player->mpv,
            command
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] ERRO loadfile: %s\n",
            mpv_error_string(status)
        );

        return status;
    }


    /*
     * Somente atualizamos o ponteiro depois que
     * o comando foi aceito pelo mpv.
     *
     * A string pertence à FileList.
     */
    player->filename =
        filename;


    /* --------------------------------------------------------
     * RESET ESTADO VISUAL
     * -------------------------------------------------------- */

    player->video_rotation = 0;

    player->brightness = 0;

    player->video_zoom = 0.0;

    player->video_pan_x = 0.0;
    player->video_pan_y = 0.0;

    player->panning = 0;


    player->pan_start_x = 0.0;
    player->pan_start_y = 0.0;

    player->pan_start_pan_x = 0.0;
    player->pan_start_pan_y = 0.0;


    /* --------------------------------------------------------
     * RESET MPV
     * -------------------------------------------------------- */

    player_set_property(
        player,
        "video-rotate",
        "0"
    );


    player_set_property(
        player,
        "brightness",
        "0"
    );


    player_set_property(
        player,
        "video-zoom",
        "0"
    );


    player_set_property(
        player,
        "video-pan-x",
        "0"
    );


    player_set_property(
        player,
        "video-pan-y",
        "0"
    );


    fprintf(
        stderr,
        "[MPV] arquivo carregado: %s\n",
        filename
    );


    return 0;
}


/* ============================================================
 * EVENTOS
 * ============================================================ */

void player_check_events(
    Player *player)
{
    if (!player ||
        !player->mpv)
        return;


    while (1) {

        mpv_event *event =
            mpv_wait_event(
                player->mpv,
                0
            );


        if (!event)
            break;


        if (event->event_id ==
            MPV_EVENT_NONE)
            break;


        fprintf(
            stderr,
            "[MPV-EVENT] id=%d name=%s\n",
            event->event_id,
            mpv_event_name(event->event_id)
        );
    }
}


/* ============================================================
 * PAUSE / PLAY
 * ============================================================ */

void player_toggle_pause(
    Player *player)
{
    if (!player ||
        !player->mpv)
        return;


    const char *command[] = {

        "cycle",
        "pause",
        NULL
    };


    int status =
        mpv_command(
            player->mpv,
            command
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro pause/play: %s\n",
            mpv_error_string(status)
        );
    }
}


/* ============================================================
 * FRAME BACK
 * ============================================================ */

void player_frame_back(
    Player *player)
{
    if (!player ||
        !player->mpv)
        return;


    const char *command[] = {

        "frame-back-step",
        NULL
    };


    mpv_command(
        player->mpv,
        command
    );
}


/* ============================================================
 * FRAME FORWARD
 * ============================================================ */

void player_frame_forward(
    Player *player)
{
    if (!player ||
        !player->mpv)
        return;


    const char *command[] = {

        "frame-step",
        NULL
    };


    mpv_command(
        player->mpv,
        command
    );
}


/* ============================================================
 * ZOOM
 * ============================================================ */

void player_change_zoom(
    Player *player,
    double amount)
{
    if (!player ||
        !player->mpv)
        return;


    /*
     * amount positivo:
     *
     * zoom in
     *
     * amount negativo:
     *
     * zoom out
     */

    player->video_zoom +=
        amount * 0.25;


    if (player->video_zoom > 5.0)
        player->video_zoom = 5.0;


    if (player->video_zoom < -5.0)
        player->video_zoom = -5.0;


    char value[64];


    snprintf(
        value,
        sizeof(value),
        "%.3f",
        player->video_zoom
    );


    player_set_property(
        player,
        "video-zoom",
        value
    );


    fprintf(
        stderr,
        "[PLAYER] zoom = %.3f\n",
        player->video_zoom
    );
}


/* ============================================================
 * PAN BEGIN
 * ============================================================ */

void player_pan_begin(
    Player *player,
    double x,
    double y)
{
    if (!player)
        return;


    /*
     * Pan somente quando há zoom.
     */

    if (player->video_zoom <= 0.0) {

        player->panning = 0;

        return;
    }


    player->panning = 1;


    player->pan_start_x =
        x;

    player->pan_start_y =
        y;


    player->pan_start_pan_x =
        player->video_pan_x;

    player->pan_start_pan_y =
        player->video_pan_y;


    fprintf(
        stderr,
        "[PAN] begin x=%.2f y=%.2f\n",
        x,
        y
    );
}


/* ============================================================
 * PAN UPDATE
 * ============================================================ */

void player_pan_update(
    Player *player,
    double offset_x,
    double offset_y,
    int width,
    int height)
{
    if (!player ||
        !player->panning)
        return;


    if (width <= 0 ||
        height <= 0)
        return;


    /*
     * Converte o movimento do mouse
     * para a escala do vídeo.
     *
     * Mantemos uma velocidade razoável
     * independente do tamanho da janela.
     */

    double dx =
        offset_x / (double)width;

    double dy =
        offset_y / (double)height;


    /*
     * mpv video-pan usa valores relativos
     * ao tamanho do vídeo.
     */

    player->video_pan_x =
        player->pan_start_pan_x +
        dx * 2.0;


    player->video_pan_y =
        player->pan_start_pan_y +
        dy * 2.0;


    if (player->video_pan_x > 2.0)
        player->video_pan_x = 2.0;

    if (player->video_pan_x < -2.0)
        player->video_pan_x = -2.0;


    if (player->video_pan_y > 2.0)
        player->video_pan_y = 2.0;

    if (player->video_pan_y < -2.0)
        player->video_pan_y = -2.0;


    char value_x[64];
    char value_y[64];


    snprintf(
        value_x,
        sizeof(value_x),
        "%.4f",
        player->video_pan_x
    );


    snprintf(
        value_y,
        sizeof(value_y),
        "%.4f",
        player->video_pan_y
    );


    player_set_property(
        player,
        "video-pan-x",
        value_x
    );


    player_set_property(
        player,
        "video-pan-y",
        value_y
    );
}


/* ============================================================
 * PAN END
 * ============================================================ */

void player_pan_end(
    Player *player)
{
    if (!player)
        return;


    player->panning = 0;


    fprintf(
        stderr,
        "[PAN] end\n"
    );
}


/* ============================================================
 * RESET VIEW
 * ============================================================ */

void player_reset_view(
    Player *player)
{
    if (!player ||
        !player->mpv)
        return;


    player->video_zoom = 0.0;

    player->video_pan_x = 0.0;
    player->video_pan_y = 0.0;

    player->panning = 0;


    player_set_property(
        player,
        "video-zoom",
        "0"
    );


    player_set_property(
        player,
        "video-pan-x",
        "0"
    );


    player_set_property(
        player,
        "video-pan-y",
        "0"
    );


    fprintf(
        stderr,
        "[PLAYER] view reset\n"
    );
}


/* ============================================================
 * ROTATE
 * ============================================================ */

void player_rotate(
    Player *player)
{
    if (!player ||
        !player->mpv)
        return;


    player->video_rotation += 90;


    if (player->video_rotation >= 360)
        player->video_rotation = 0;


    char value[32];


    snprintf(
        value,
        sizeof(value),
        "%d",
        player->video_rotation
    );


    player_set_property(
        player,
        "video-rotate",
        value
    );


    fprintf(
        stderr,
        "[PLAYER] rotation = %d\n",
        player->video_rotation
    );
}


/* ============================================================
 * BRIGHTNESS
 * ============================================================ */

void player_change_brightness(
    Player *player,
    int amount)
{
    if (!player ||
        !player->mpv)
        return;


    player->brightness +=
        amount;


    if (player->brightness > 100)
        player->brightness = 100;


    if (player->brightness < -100)
        player->brightness = -100;


    char value[32];


    snprintf(
        value,
        sizeof(value),
        "%d",
        player->brightness
    );


    player_set_property(
        player,
        "brightness",
        value
    );


    fprintf(
        stderr,
        "[PLAYER] brightness = %d\n",
        player->brightness
    );
}


/* ============================================================
 * SCREENSHOT
 * ============================================================ */

const char *player_save_frame(
    Player *player)
{
    if (!player ||
        !player->mpv)
        return NULL;


    static char screenshot_path[4096];


    const char *command[] = {

        "screenshot",
        "video",
        NULL
    };


    int status =
        mpv_command(
            player->mpv,
            command
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro screenshot: %s\n",
            mpv_error_string(status)
        );

        return NULL;
    }


    /*
     * O mpv escolhe o nome do screenshot.
     *
     * Neste momento retornamos uma mensagem
     * simples. O comportamento anterior do
     * programa pode ser refinado depois com
     * screenshot directory / screenshot template.
     */

    snprintf(
        screenshot_path,
        sizeof(screenshot_path),
        "Screenshot salvo"
    );


    fprintf(
        stderr,
        "[MPV] screenshot solicitado\n"
    );


    return screenshot_path;
}
