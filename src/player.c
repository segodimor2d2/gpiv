#include "player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>

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

    int contrast;

    int saturation;

    double video_zoom;

    double video_pan_x;
    double video_pan_y;

    /*
     * Estado do pan.
     */

    int panning;

    /*
     * Estado do áudio.
     */

    int volume;

    double pan_start_x;
    double pan_start_y;

    double pan_start_pan_x;
    double pan_start_pan_y;
};

/* ============================================================
 * API DE PROPRIEDADES
 * ============================================================ */
static int player_set_int_property(
    Player *player,
    const char *property,
    int value)
{
    if (!player ||
        !player->mpv ||
        !property)
        return -1;


    int status =
        mpv_set_property(
            player->mpv,
            property,
            MPV_FORMAT_INT64,
            &(int64_t){ value }
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro set %s=%d: %s\n",
            property,
            value,
            mpv_error_string(status)
        );

        return status;
    }


    return 0;
}

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

    player->contrast = 0;

    player->saturation = 0;

    player->volume = 100;

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

    player->contrast = 0;

    player->saturation = 0;

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
        "contrast",
        "0"
    );

    player_set_property(
        player,
        "saturation",
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

    return 0;
}


/* ============================================================
 * GET TIME
 * ============================================================ */

int player_get_time(
    Player *player,
    double *position,
    double *duration)
{
    if (!player ||
        !player->mpv ||
        !position ||
        !duration)
        return -1;


    *position = 0.0;
    *duration = 0.0;


    int status =
        mpv_get_property(
            player->mpv,
            "time-pos",
            MPV_FORMAT_DOUBLE,
            position
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro obtendo time-pos: %s\n",
            mpv_error_string(status)
        );

        return -1;
    }


    status =
        mpv_get_property(
            player->mpv,
            "duration",
            MPV_FORMAT_DOUBLE,
            duration
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] erro obtendo duration: %s\n",
            mpv_error_string(status)
        );

        return -1;
    }


    return 0;
}


/* ============================================================
 * GET FILENAME
 * ============================================================ */

const char *player_get_filename(
    Player *player)
{
    if (!player)
        return NULL;


    return player->filename;
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
 * SEEK FORWARD
 * ============================================================ */

void player_seek_forward(
    Player *player,
    int seconds)
{
    if (!player ||
        !player->mpv)
        return;


    char seconds_str[32];

    snprintf(
        seconds_str,
        sizeof(seconds_str),
        "%d",
        seconds
    );


    const char *command[] = {

        "seek",
        seconds_str,
        "relative",
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
            "[MPV] erro seek forward: %s\n",
            mpv_error_string(status)
        );
    }
}


/* ============================================================
 * SEEK BACKWARD
 * ============================================================ */

void player_seek_backward(
    Player *player,
    int seconds)
{
    if (!player ||
        !player->mpv)
        return;


    char seconds_str[32];

    snprintf(
        seconds_str,
        sizeof(seconds_str),
        "-%d",
        seconds
    );


    const char *command[] = {

        "seek",
        seconds_str,
        "relative",
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
            "[MPV] erro seek backward: %s\n",
            mpv_error_string(status)
        );
    }
}

/* ============================================================
 * VOLUME
 * ============================================================ */

void player_change_volume(
    Player *player,
    int amount)
{
    if (!player ||
        !player->mpv)
        return;


    player->volume +=
        amount;


    if (player->volume > 300)
        player->volume = 300;


    if (player->volume < 0)
        player->volume = 0;


    char value[32];


    snprintf(
        value,
        sizeof(value),
        "%d",
        player->volume
    );


    player_set_property(
        player,
        "volume",
        value
    );


    fprintf(
        stderr,
        "[MPV] volume = %d\n",
        player->volume
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


    player_set_int_property(
        player,
        "brightness",
        player->brightness
    );
}

/* ============================================================
 * CONTRAST
 * ============================================================ */

void player_change_contrast(
    Player *player,
    int amount)
{
    if (!player ||
        !player->mpv)
        return;


    player->contrast +=
        amount;


    if (player->contrast > 100)
        player->contrast = 100;


    if (player->contrast < -100)
        player->contrast = -100;


    player_set_int_property(
        player,
        "contrast",
        player->contrast
    );
}

/* ============================================================
 * SATURATION
 * ============================================================ */

void player_change_saturation(
    Player *player,
    int amount)
{
    if (!player ||
        !player->mpv)
        return;

    player->saturation +=
        amount;


    if (player->saturation > 100)
        player->saturation = 100;


    if (player->saturation < -100)
        player->saturation = -100;


    player_set_int_property(
        player,
        "saturation",
        player->saturation
    );


    int64_t actual = 0;

    int status =
        mpv_get_property(
            player->mpv,
            "saturation",
            MPV_FORMAT_INT64,
            &actual
        );


    if (status >= 0) {

        fprintf(
            stderr,
            "[SATURATION] interno=%d mpv=%ld\n",
            player->saturation,
            (long)actual
        );
    }
}


/* ============================================================
 * SCREENSHOT
 * ============================================================ */

const char *player_save_frame(
    Player *player)
{
    if (!player ||
        !player->mpv ||
        !player->filename)
        return NULL;


    /*
     * Buffer estático para retornar o caminho
     * para controls.c.
     */
    static char screenshot_path[PATH_MAX];


    /*
     * --------------------------------------------------------
     * SEPARA DIRETÓRIO E NOME DO VÍDEO
     * --------------------------------------------------------
     */

    const char *filename =
        player->filename;


    const char *slash =
        strrchr(
            filename,
            '/'
        );


    char directory[PATH_MAX];
    char basename[PATH_MAX];


    if (slash) {

        size_t dir_len =
            (size_t)(slash - filename);


        if (dir_len >= sizeof(directory))
            return NULL;


        memcpy(
            directory,
            filename,
            dir_len
        );


        directory[dir_len] =
            '\0';


        snprintf(
            basename,
            sizeof(basename),
            "%s",
            slash + 1
        );

    } else {

        /*
         * Arquivo sem '/'.
         * Nesse caso usamos o diretório atual.
         */

        snprintf(
            directory,
            sizeof(directory),
            "."
        );


        snprintf(
            basename,
            sizeof(basename),
            "%s",
            filename
        );
    }


    /*
     * --------------------------------------------------------
     * REMOVE EXTENSÃO DO VÍDEO
     * --------------------------------------------------------
     */

    char *dot =
        strrchr(
            basename,
            '.'
        );


    if (dot &&
        dot != basename) {

        *dot =
            '\0';
    }


    /*
     * --------------------------------------------------------
     * PROCURA UM NOME LIVRE
     * --------------------------------------------------------
     *
     * Exemplo:
     *
     * video-0001.png
     * video-0002.png
     * video-0003.png
     *
     */

    int number;


    for (number = 1;
         number <= 9999;
         number++) {

        int written =
            snprintf(
                screenshot_path,
                sizeof(screenshot_path),
                "%s/%s-%04d.png",
                directory,
                basename,
                number
            );


        /*
         * O caminho não coube no buffer.
         */
        if (written < 0 ||
            (size_t)written >= sizeof(screenshot_path)) {

            fprintf(
                stderr,
                "[MPV] ERRO: caminho do screenshot muito longo\n"
            );

            return NULL;
        }


        /*
         * Encontramos um nome que ainda não existe.
         */
        if (access(
                screenshot_path,
                F_OK
            ) != 0) {

            break;
        }
    }


    if (number > 9999) {

        fprintf(
            stderr,
            "[MPV] ERRO: não foi possível encontrar nome para screenshot\n"
        );


        return NULL;
    }


    /*
     * --------------------------------------------------------
     * SCREENSHOT-TO-FILE
     * --------------------------------------------------------
     *
     * Diferentemente de "screenshot", aqui nós
     * passamos explicitamente o arquivo de destino.
     */

    const char *command[] = {

        "screenshot-to-file",
        screenshot_path,
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


    fprintf(
        stderr,
        "[MPV] screenshot salvo: %s\n",
        screenshot_path
    );


    return screenshot_path;
}
