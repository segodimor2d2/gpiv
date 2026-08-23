#include "player.h"

#include <epoxy/gl.h>
#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <math.h>
#include <libgen.h>


/* ============================================================
 * PLAYER
 * ============================================================ */

struct _Player {

    mpv_handle *mpv;

    mpv_render_context *mpv_render;

    const char *filename;

    int video_rotation;

    int brightness;

    double video_zoom;

    double video_pan_x;
    double video_pan_y;

    int panning;

    double pan_start_pan_x;
    double pan_start_pan_y;
};


/* ============================================================
 * OPENGL
 * ============================================================ */

static void *mpv_get_proc_address(
    void *ctx,
    const char *name)
{
    (void)ctx;

    return (void *)glXGetProcAddressARB(
        (const GLubyte *)name
    );
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
    player->mpv_render = NULL;

    player->filename = NULL;

    player->video_rotation = 0;
    player->brightness = 0;

    player->video_zoom = 0.0;

    player->video_pan_x = 0.0;
    player->video_pan_y = 0.0;

    player->panning = 0;

    player->pan_start_pan_x = 0.0;
    player->pan_start_pan_y = 0.0;

    return player;
}


/* ============================================================
 * INICIALIZAÇÃO
 * ============================================================ */

int player_initialize(
    Player *player,
    const char *filename,
    mpv_render_update_fn update_callback,
    void *update_callback_ctx)
{
    if (!player)
        return -1;


    fprintf(
        stderr,
        "[MPV] criando mpv\n"
    );


    /*
     * O mpv trabalha melhor com LC_NUMERIC=C.
     */

    if (setlocale(LC_NUMERIC, "C") == NULL) {

        fprintf(
            stderr,
            "[MPV] ERRO: LC_NUMERIC=C\n"
        );

        return -1;
    }


    /* --------------------------------------------------------
     * MPV
     * -------------------------------------------------------- */

    player->mpv =
        mpv_create();

    if (!player->mpv) {

        fprintf(
            stderr,
            "[MPV] ERRO: mpv_create()\n"
        );

        return -1;
    }


    mpv_set_option_string(
        player->mpv,
        "terminal",
        "yes"
    );


    mpv_set_option_string(
        player->mpv,
        "msg-level",
        "all=warn"
    );


    int status;


    status = mpv_set_option_string(
        player->mpv,
        "vo",
        "libmpv"
    );

    if (status < 0)
        goto error;


    status = mpv_set_option_string(
        player->mpv,
        "loop-file",
        "yes"
    );

    if (status < 0)
        goto error;


    status = mpv_set_option_string(
        player->mpv,
        "hwdec",
        "auto"
    );

    if (status < 0)
        goto error;


    /*
     * mpv_initialize()
     */

    status =
        mpv_initialize(
            player->mpv
        );

    if (status < 0)
        goto error;


    fprintf(
        stderr,
        "[MPV] mpv_initialize() OK\n"
    );


    /* --------------------------------------------------------
     * RENDER CONTEXT
     * -------------------------------------------------------- */

    mpv_opengl_init_params gl_init_params = {

        .get_proc_address =
            mpv_get_proc_address,

        .get_proc_address_ctx =
            NULL
    };


    mpv_render_param params[] = {

        {
            MPV_RENDER_PARAM_API_TYPE,
            MPV_RENDER_API_TYPE_OPENGL
        },

        {
            MPV_RENDER_PARAM_OPENGL_INIT_PARAMS,
            &gl_init_params
        },

        {
            MPV_RENDER_PARAM_INVALID,
            NULL
        }
    };


    status =
        mpv_render_context_create(
            &player->mpv_render,
            player->mpv,
            params
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV-RENDER] ERRO criando contexto: %s\n",
            mpv_error_string(status)
        );

        player->mpv_render = NULL;

        goto error;
    }


    fprintf(
        stderr,
        "[MPV-RENDER] contexto criado: %p\n",
        (void *)player->mpv_render
    );


    /* --------------------------------------------------------
     * CALLBACK DE UPDATE
     * -------------------------------------------------------- */

    if (update_callback) {

        mpv_render_context_set_update_callback(
            player->mpv_render,
            update_callback,
            update_callback_ctx
        );
    }


    /* --------------------------------------------------------
     * ARQUIVO
     * -------------------------------------------------------- */

    if (filename) {

        if (player_load_file(
                player,
                filename) < 0) {

            fprintf(
                stderr,
                "[MPV] ERRO carregando arquivo\n"
            );

            goto error;
        }
    }


    return 0;


/* ------------------------------------------------------------
 * ERRO
 * ------------------------------------------------------------ */

error:

    if (player->mpv_render) {

        mpv_render_context_free(
            player->mpv_render
        );

        player->mpv_render = NULL;
    }


    if (player->mpv) {

        mpv_terminate_destroy(
            player->mpv
        );

        player->mpv = NULL;
    }


    return -1;
}


/* ============================================================
 * RENDER CONTEXT
 * ============================================================ */

mpv_render_context *
player_get_render_context(
    Player *player)
{
    if (!player)
        return NULL;

    return player->mpv_render;
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


    player->filename = filename;


    const char *command[] = {

        "loadfile",
        filename,
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


    fprintf(
        stderr,
        "[MPV] arquivo carregado: %s\n",
        filename
    );


    return 0;
}


/* ============================================================
 * PAUSE
 * ============================================================ */

void player_toggle_pause(
    Player *player)
{
    if (!player || !player->mpv)
        return;


    const char *command[] = {

        "cycle",
        "pause",
        NULL
    };


    mpv_command(
        player->mpv,
        command
    );
}


/* ============================================================
 * FRAME BACK
 * ============================================================ */

void player_frame_back(
    Player *player)
{
    if (!player || !player->mpv)
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
    if (!player || !player->mpv)
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
    if (!player || !player->mpv)
        return;


    player->video_zoom += amount;


    if (player->video_zoom > 5.0)
        player->video_zoom = 5.0;


    if (player->video_zoom < 0.0)
        player->video_zoom = 0.0;


    char zoom[64];


    snprintf(
        zoom,
        sizeof(zoom),
        "%.3f",
        player->video_zoom
    );


    const char *command[] = {

        "set",
        "video-zoom",
        zoom,
        NULL
    };


    mpv_command(
        player->mpv,
        command
    );


    fprintf(
        stderr,
        "[ZOOM] %.3f\n",
        player->video_zoom
    );
}


/* ============================================================
 * PAN
 * ============================================================ */

static void player_set_pan(
    Player *player)
{
    if (!player || !player->mpv)
        return;


    char x[64];
    char y[64];


    snprintf(
        x,
        sizeof(x),
        "%.6f",
        player->video_pan_x
    );


    snprintf(
        y,
        sizeof(y),
        "%.6f",
        player->video_pan_y
    );


    const char *cmd_x[] = {

        "set",
        "video-pan-x",
        x,
        NULL
    };


    const char *cmd_y[] = {

        "set",
        "video-pan-y",
        y,
        NULL
    };


    mpv_command(
        player->mpv,
        cmd_x
    );


    mpv_command(
        player->mpv,
        cmd_y
    );
}


void player_pan_begin(
    Player *player,
    double x,
    double y)
{
    (void)x;
    (void)y;

    if (!player)
        return;


    if (player->video_zoom <= 0.0)
        return;


    player->panning = 1;


    player->pan_start_pan_x =
        player->video_pan_x;

    player->pan_start_pan_y =
        player->video_pan_y;
}


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


    double scale =
        pow(
            2.0,
            player->video_zoom
        );


    double pan_x_delta =
        offset_x /
        width *
        2.0 /
        scale;


    double pan_y_delta =
        offset_y /
        height *
        2.0 /
        scale;


    player->video_pan_x =
        player->pan_start_pan_x +
        pan_x_delta;


    player->video_pan_y =
        player->pan_start_pan_y +
        pan_y_delta;


    if (player->video_pan_x > 1.0)
        player->video_pan_x = 1.0;

    if (player->video_pan_x < -1.0)
        player->video_pan_x = -1.0;


    if (player->video_pan_y > 1.0)
        player->video_pan_y = 1.0;

    if (player->video_pan_y < -1.0)
        player->video_pan_y = -1.0;


    player_set_pan(player);
}


void player_pan_end(
    Player *player)
{
    if (!player)
        return;

    player->panning = 0;
}


/* ============================================================
 * RESET VIEW
 * ============================================================ */

void player_reset_view(
    Player *player)
{
    if (!player || !player->mpv)
        return;


    player->video_zoom = 0.0;

    player->video_pan_x = 0.0;
    player->video_pan_y = 0.0;


    const char *zoom[] = {

        "set",
        "video-zoom",
        "0",
        NULL
    };


    const char *pan_x[] = {

        "set",
        "video-pan-x",
        "0",
        NULL
    };


    const char *pan_y[] = {

        "set",
        "video-pan-y",
        "0",
        NULL
    };


    mpv_command(
        player->mpv,
        zoom
    );


    mpv_command(
        player->mpv,
        pan_x
    );


    mpv_command(
        player->mpv,
        pan_y
    );


    fprintf(
        stderr,
        "[VIEW] reset\n"
    );
}


/* ============================================================
 * ROTATE
 * ============================================================ */

void player_rotate(
    Player *player)
{
    if (!player || !player->mpv)
        return;


    player->video_rotation += 90;


    if (player->video_rotation >= 360)
        player->video_rotation = 0;


    char rotation[16];


    snprintf(
        rotation,
        sizeof(rotation),
        "%d",
        player->video_rotation
    );


    const char *command[] = {

        "set",
        "video-rotate",
        rotation,
        NULL
    };


    mpv_command(
        player->mpv,
        command
    );


    fprintf(
        stderr,
        "[ROTATE] %d graus\n",
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
    if (!player || !player->mpv)
        return;


    player->brightness += amount;


    if (player->brightness > 100)
        player->brightness = 100;


    if (player->brightness < -100)
        player->brightness = -100;


    char brightness[16];


    snprintf(
        brightness,
        sizeof(brightness),
        "%d",
        player->brightness
    );


    const char *command[] = {

        "set",
        "brightness",
        brightness,
        NULL
    };


    mpv_command(
        player->mpv,
        command
    );


    fprintf(
        stderr,
        "[BRIGHTNESS] %d\n",
        player->brightness
    );
}


/* ============================================================
 * SCREENSHOT
 * ============================================================ */

void player_save_frame(
    Player *player)
{
    if (!player ||
        !player->mpv ||
        !player->filename)
        return;


    char path[4096];


    snprintf(
        path,
        sizeof(path),
        "%s",
        player->filename
    );


    char *dir =
        dirname(path);


    mpv_set_option_string(
        player->mpv,
        "screenshot-dir",
        dir
    );


    mpv_set_option_string(
        player->mpv,
        "screenshot-template",
        "%F_%n"
    );


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
            "[SCREENSHOT] ERRO: %s\n",
            mpv_error_string(status)
        );

    } else {

        fprintf(
            stderr,
            "[SCREENSHOT] OK\n"
        );
    }
}


/* ============================================================
 * EVENTOS
 * ============================================================ */

void player_check_events(
    Player *player)
{
    if (!player || !player->mpv)
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
            mpv_event_name(
                event->event_id
            )
        );


        if (event->event_id ==
            MPV_EVENT_LOG_MESSAGE) {

            mpv_event_log_message *msg =
                event->data;


            if (msg) {

                fprintf(
                    stderr,
                    "[MPV-LOG] [%s] %s",
                    msg->prefix,
                    msg->text
                );
            }
        }
    }
}


/* ============================================================
 * FREE
 * ============================================================ */

void player_free(
    Player *player)
{
    if (!player)
        return;


    if (player->mpv_render) {

        mpv_render_context_free(
            player->mpv_render
        );

        player->mpv_render = NULL;
    }


    if (player->mpv) {

        mpv_terminate_destroy(
            player->mpv
        );

        player->mpv = NULL;
    }


    free(player);
}
