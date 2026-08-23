#include "app.h"

#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <libgen.h>


/* ============================================================
 * PLAYER APP
 * ============================================================ */

struct _PlayerApp {

    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *gl_area;
    GtkWidget *info_label;

    mpv_handle *mpv;
    mpv_render_context *mpv_render;

    guint mpv_event_source;

    const char *filename;

    int video_rotation;
    int brightness;

    double video_zoom;
    double video_pan_x;
    double video_pan_y;

    gboolean panning;

    double pan_start_x;
    double pan_start_y;

    double pan_start_pan_x;
    double pan_start_pan_y;

    gboolean mouse_moved;
    double mouse_press_x;
    double mouse_press_y;
};


/* ============================================================
 * PROTÓTIPOS INTERNOS
 * ============================================================ */

static gboolean restore_info_label(gpointer data);

static void on_activate(
    GtkApplication *app,
    PlayerApp *pa
);


/* ============================================================
 * CLIPBOARD
 * ============================================================ */

static void show_clipboard_message(PlayerApp *pa)
{
    if (!pa->info_label || !pa->filename)
        return;

    char message[8192];

    snprintf(
        message,
        sizeof(message),
        "%s\nPath copiado",
        pa->filename
    );

    gtk_label_set_text(
        GTK_LABEL(pa->info_label),
        message
    );
}


static void copy_video_path(PlayerApp *pa)
{
    if (!pa->filename || !pa->window)
        return;

    GdkDisplay *display =
        gtk_widget_get_display(pa->window);

    GdkClipboard *clipboard =
        gdk_display_get_clipboard(display);

    gdk_clipboard_set_text(
        clipboard,
        pa->filename
    );

    fprintf(
        stderr,
        "[CLIPBOARD] copiado: %s\n",
        pa->filename
    );

    show_clipboard_message(pa);

    g_timeout_add(
        2000,
        restore_info_label,
        pa
    );
}


/* ============================================================
 * MPV ZOOM
 * ============================================================ */

static gboolean on_scroll(
    GtkEventControllerScroll *controller,
    double dx,
    double dy,
    PlayerApp *pa)
{
    (void)controller;
    (void)dx;

    if (!pa->mpv || !pa->gl_area)
        return FALSE;

    if (dy < 0)
        pa->video_zoom += 0.25;
    else if (dy > 0)
        pa->video_zoom -= 0.25;

    if (pa->video_zoom > 5.0)
        pa->video_zoom = 5.0;

    if (pa->video_zoom < 0.0)
        pa->video_zoom = 0.0;

    char zoom[64];

    snprintf(
        zoom,
        sizeof(zoom),
        "%.3f",
        pa->video_zoom
    );

    const char *zoom_cmd[] = {
        "set",
        "video-zoom",
        zoom,
        NULL
    };

    int status = mpv_command(
        pa->mpv,
        zoom_cmd
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[ZOOM] ERRO: %s\n",
            mpv_error_string(status)
        );
    }

    fprintf(
        stderr,
        "[ZOOM] zoom=%.3f pan=%.3f,%.3f\n",
        pa->video_zoom,
        pa->video_pan_x,
        pa->video_pan_y
    );

    return TRUE;
}


/* ============================================================
 * PAN
 * ============================================================ */

static void mpv_set_pan(PlayerApp *pa)
{
    if (!pa->mpv)
        return;

    char pan_x[64];
    char pan_y[64];

    snprintf(
        pan_x,
        sizeof(pan_x),
        "%.6f",
        pa->video_pan_x
    );

    snprintf(
        pan_y,
        sizeof(pan_y),
        "%.6f",
        pa->video_pan_y
    );

    const char *cmd_x[] = {
        "set",
        "video-pan-x",
        pan_x,
        NULL
    };

    const char *cmd_y[] = {
        "set",
        "video-pan-y",
        pan_y,
        NULL
    };

    mpv_command(pa->mpv, cmd_x);
    mpv_command(pa->mpv, cmd_y);
}


static void on_drag_begin(
    GtkGestureDrag *gesture,
    double start_x,
    double start_y,
    PlayerApp *pa)
{
    (void)gesture;

    if (pa->video_zoom <= 0.0)
        return;

    pa->panning = TRUE;

    pa->pan_start_x = start_x;
    pa->pan_start_y = start_y;

    pa->pan_start_pan_x =
        pa->video_pan_x;

    pa->pan_start_pan_y =
        pa->video_pan_y;

    fprintf(
        stderr,
        "[PAN] begin mouse=%.1f,%.1f pan=%.3f,%.3f\n",
        start_x,
        start_y,
        pa->video_pan_x,
        pa->video_pan_y
    );
}


static void on_drag_update(
    GtkGestureDrag *gesture,
    double offset_x,
    double offset_y,
    PlayerApp *pa)
{
    (void)gesture;

    if (!pa->panning)
        return;

    int width =
        gtk_widget_get_width(pa->gl_area);

    int height =
        gtk_widget_get_height(pa->gl_area);

    if (width <= 0 || height <= 0)
        return;

    double scale =
        pow(2.0, pa->video_zoom);

    double pan_x_delta =
        offset_x / width * 2.0 / scale;

    double pan_y_delta =
        offset_y / height * 2.0 / scale;

    pa->video_pan_x =
        pa->pan_start_pan_x + pan_x_delta;

    pa->video_pan_y =
        pa->pan_start_pan_y + pan_y_delta;

    if (pa->video_pan_x > 1.0)
        pa->video_pan_x = 1.0;

    if (pa->video_pan_x < -1.0)
        pa->video_pan_x = -1.0;

    if (pa->video_pan_y > 1.0)
        pa->video_pan_y = 1.0;

    if (pa->video_pan_y < -1.0)
        pa->video_pan_y = -1.0;

    mpv_set_pan(pa);
}


static void on_drag_end(
    GtkGestureDrag *gesture,
    double offset_x,
    double offset_y,
    PlayerApp *pa)
{
    (void)gesture;
    (void)offset_x;
    (void)offset_y;

    if (!pa->panning)
        return;

    pa->panning = FALSE;

    fprintf(
        stderr,
        "[PAN] end pan=%.3f,%.3f\n",
        pa->video_pan_x,
        pa->video_pan_y
    );
}


/* ============================================================
 * BRIGHTNESS
 * ============================================================ */

static void mpv_set_brightness(
    PlayerApp *pa,
    int value)
{
    if (!pa->mpv)
        return;

    if (value > 100)
        value = 100;

    if (value < -100)
        value = -100;

    pa->brightness = value;

    char brightness[16];

    snprintf(
        brightness,
        sizeof(brightness),
        "%d",
        pa->brightness
    );

    const char *command[] = {
        "set",
        "brightness",
        brightness,
        NULL
    };

    fprintf(
        stderr,
        "[BRIGHTNESS] %d\n",
        pa->brightness
    );

    int status = mpv_command(
        pa->mpv,
        command
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[BRIGHTNESS] ERRO: %s\n",
            mpv_error_string(status)
        );

    } else {

        fprintf(
            stderr,
            "[BRIGHTNESS] OK -> %d\n",
            pa->brightness
        );
    }
}


static void mpv_change_brightness(
    PlayerApp *pa,
    int amount)
{
    mpv_set_brightness(
        pa,
        pa->brightness + amount
    );
}


/* ============================================================
 * CSS
 * ============================================================ */

static void setup_info_label_css(void)
{
    GtkCssProvider *provider =
        gtk_css_provider_new();

    gtk_css_provider_load_from_string(
        provider,
        ".video-info {"
        "  font-size: 12px;"
        "  color: white;"
        "  background-color: rgba(0, 0, 0, 0.3);"
        "  padding: 5px 8px;"
        "}"
    );

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);
}


/* ============================================================
 * INFO LABEL
 * ============================================================ */

static gboolean restore_info_label(gpointer data)
{
    PlayerApp *pa = data;

    if (!pa->info_label || !pa->filename)
        return G_SOURCE_REMOVE;

    gtk_label_set_text(
        GTK_LABEL(pa->info_label),
        pa->filename
    );

    return G_SOURCE_REMOVE;
}


/* ============================================================
 * MPV EVENTS
 * ============================================================ */

static void mpv_check_events(PlayerApp *pa)
{
    if (!pa->mpv)
        return;

    while (1) {

        mpv_event *event =
            mpv_wait_event(pa->mpv, 0);

        if (!event)
            break;

        if (event->event_id == MPV_EVENT_NONE)
            break;

        fprintf(
            stderr,
            "[MPV-EVENT] id=%d name=%s\n",
            event->event_id,
            mpv_event_name(event->event_id)
        );

        switch (event->event_id) {

        case MPV_EVENT_FILE_LOADED:

            fprintf(
                stderr,
                "[MPV-EVENT] FILE_LOADED\n"
            );

            break;


        case MPV_EVENT_VIDEO_RECONFIG:

            fprintf(
                stderr,
                "[MPV-EVENT] VIDEO_RECONFIG\n"
            );

            break;


        case MPV_EVENT_PLAYBACK_RESTART:

            fprintf(
                stderr,
                "[MPV-EVENT] PLAYBACK_RESTART\n"
            );

            break;


        case MPV_EVENT_END_FILE: {

            mpv_event_end_file *end =
                event->data;

            fprintf(
                stderr,
                "[MPV-EVENT] END_FILE reason=%d error=%s\n",
                end ? (int)end->reason : -1,
                end ? mpv_error_string(end->error) : "NULL"
            );

            break;
        }


        case MPV_EVENT_LOG_MESSAGE: {

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

            break;
        }


        default:
            break;
        }
    }
}


static gboolean mpv_event_timer(gpointer data)
{
    PlayerApp *pa = data;

    if (!pa->mpv)
        return G_SOURCE_REMOVE;

    mpv_check_events(pa);

    return G_SOURCE_CONTINUE;
}


/* ============================================================
 * MPV COMMAND
 * ============================================================ */

static int mpv_send_command(
    PlayerApp *pa,
    const char *cmd,
    const char *arg)
{
    if (!pa->mpv) {

        fprintf(
            stderr,
            "[MPV-CMD] ERRO: mpv não está disponível\n"
        );

        return -1;
    }

    const char *command[3];

    command[0] = cmd;
    command[1] = arg;
    command[2] = NULL;

    fprintf(
        stderr,
        "[MPV-CMD] %s%s%s\n",
        cmd,
        arg ? " " : "",
        arg ? arg : ""
    );

    int status = mpv_command(
        pa->mpv,
        command
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[MPV-CMD] ERRO: %s\n",
            mpv_error_string(status)
        );

    } else {

        fprintf(
            stderr,
            "[MPV-CMD] OK\n"
        );
    }

    return status;
}


/* ============================================================
 * SAVE FRAME
 * ============================================================ */

static void show_saved_message(PlayerApp *pa)
{
    if (!pa->info_label || !pa->filename)
        return;

    char path[4096];

    snprintf(
        path,
        sizeof(path),
        "%s",
        pa->filename
    );

    char *filename = basename(path);

    char name[4096];

    snprintf(
        name,
        sizeof(name),
        "%s",
        filename
    );

    char *dot = strrchr(name, '.');

    if (dot)
        *dot = '\0';

    char message[8192];

    snprintf(
        message,
        sizeof(message),
        "%s\nFrame %.*s salvo",
        pa->filename,
        (int)(sizeof(message) - strlen(pa->filename) - 20),
        name
    );

    gtk_label_set_text(
        GTK_LABEL(pa->info_label),
        message
    );
}


static void mpv_save_frame(PlayerApp *pa)
{
    if (!pa->mpv || !pa->filename)
        return;

    char path[4096];

    snprintf(
        path,
        sizeof(path),
        "%s",
        pa->filename
    );

    char *dir = dirname(path);

    fprintf(
        stderr,
        "[SCREENSHOT] diretório = %s\n",
        dir
    );

    int status = mpv_set_option_string(
        pa->mpv,
        "screenshot-dir",
        dir
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[SCREENSHOT] ERRO screenshot-dir: %s\n",
            mpv_error_string(status)
        );

        return;
    }

    status = mpv_set_option_string(
        pa->mpv,
        "screenshot-template",
        "%F_%n"
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[SCREENSHOT] ERRO screenshot-template: %s\n",
            mpv_error_string(status)
        );

        return;
    }

    const char *command[] = {
        "screenshot",
        "video",
        NULL
    };

    fprintf(
        stderr,
        "[SCREENSHOT] salvando frame\n"
    );

    status = mpv_command(
        pa->mpv,
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

        show_saved_message(pa);

        g_timeout_add(
            2000,
            restore_info_label,
            pa
        );
    }
}


/* ============================================================
 * ROTATE
 * ============================================================ */

static void mpv_rotate_video(PlayerApp *pa)
{
    if (!pa->mpv)
        return;

    pa->video_rotation += 90;

    if (pa->video_rotation >= 360)
        pa->video_rotation = 0;

    char rotation[16];

    snprintf(
        rotation,
        sizeof(rotation),
        "%d",
        pa->video_rotation
    );

    fprintf(
        stderr,
        "[ROTATE] video-rotate=%s\n",
        rotation
    );

    const char *command[] = {
        "set",
        "video-rotate",
        rotation,
        NULL
    };

    int status = mpv_command(
        pa->mpv,
        command
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[ROTATE] ERRO: %s\n",
            mpv_error_string(status)
        );

    } else {

        fprintf(
            stderr,
            "[ROTATE] OK -> %d graus\n",
            pa->video_rotation
        );
    }
}


/* ============================================================
 * PAUSE / PLAY
 * ============================================================ */

static void mpv_toggle_pause(PlayerApp *pa)
{
    fprintf(
        stderr,
        "[MPV] PAUSE/PLAY\n"
    );

    mpv_send_command(
        pa,
        "cycle",
        "pause"
    );
}


/* ============================================================
 * KEYBOARD
 * ============================================================ */

static gboolean on_key_pressed(
    GtkEventControllerKey *controller,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    PlayerApp *pa)
{
    (void)controller;
    (void)keycode;
    (void)state;

    fprintf(
        stderr,
        "[KEY] keyval=0x%x\n",
        keyval
    );


    if (keyval == GDK_KEY_q ||
        keyval == GDK_KEY_Q) {

        fprintf(
            stderr,
            "[KEY] Q DETECTADO -> saindo\n"
        );

        gtk_window_destroy(
            GTK_WINDOW(pa->window)
        );

        return TRUE;
    }


    if (keyval == GDK_KEY_space) {

        mpv_toggle_pause(pa);

        return TRUE;
    }


    if (keyval == GDK_KEY_k ||
        keyval == GDK_KEY_K) {

        mpv_send_command(
            pa,
            "frame-back-step",
            NULL
        );

        return TRUE;
    }


    if (keyval == GDK_KEY_j ||
        keyval == GDK_KEY_J) {

        mpv_send_command(
            pa,
            "frame-step",
            NULL
        );

        return TRUE;
    }


    if (keyval == GDK_KEY_s ||
        keyval == GDK_KEY_S) {

        mpv_save_frame(pa);

        return TRUE;
    }


    if (keyval == GDK_KEY_y ||
        keyval == GDK_KEY_Y) {

        copy_video_path(pa);

        return TRUE;
    }


    if (keyval == GDK_KEY_r ||
        keyval == GDK_KEY_R) {

        mpv_rotate_video(pa);

        return TRUE;
    }


    if (keyval == GDK_KEY_u ||
        keyval == GDK_KEY_U) {

        mpv_change_brightness(pa, 5);

        return TRUE;
    }


    if (keyval == GDK_KEY_i ||
        keyval == GDK_KEY_I) {

        mpv_change_brightness(pa, -5);

        return TRUE;
    }


    if (keyval == GDK_KEY_0) {

        fprintf(
            stderr,
            "[KEY] 0 -> reset zoom\n"
        );

        pa->video_zoom = 0.0;
        pa->video_pan_x = 0.0;
        pa->video_pan_y = 0.0;

        const char *cmd_zoom[] = {
            "set",
            "video-zoom",
            "0",
            NULL
        };

        const char *cmd_pan_x[] = {
            "set",
            "video-pan-x",
            "0",
            NULL
        };

        const char *cmd_pan_y[] = {
            "set",
            "video-pan-y",
            "0",
            NULL
        };

        mpv_command(pa->mpv, cmd_zoom);
        mpv_command(pa->mpv, cmd_pan_x);
        mpv_command(pa->mpv, cmd_pan_y);

        return TRUE;
    }

    return FALSE;
}


/* ============================================================
 * MPV -> GTK RENDER
 * ============================================================ */

static gboolean mpv_queue_render_idle(gpointer data)
{
    PlayerApp *pa = data;

    if (!pa->gl_area)
        return G_SOURCE_REMOVE;

    gtk_gl_area_queue_render(
        GTK_GL_AREA(pa->gl_area)
    );

    return G_SOURCE_REMOVE;
}


static void on_mpv_update(void *ctx)
{
    PlayerApp *pa = ctx;

    g_main_context_invoke(
        NULL,
        mpv_queue_render_idle,
        pa
    );
}


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
 * GtkGLArea REALIZE
 * ============================================================ */

static void on_gl_realize(
    GtkGLArea *area,
    PlayerApp *pa)
{
    fprintf(
        stderr,
        "[GL] REALIZE GtkGLArea\n"
    );

    gtk_gl_area_make_current(area);

    GError *error =
        gtk_gl_area_get_error(area);

    if (error != NULL) {

        fprintf(
            stderr,
            "[GL] ERRO ao criar contexto OpenGL: %s\n",
            error->message
        );

        return;
    }

    GdkGLContext *ctx =
        gtk_gl_area_get_context(area);

    if (!ctx) {

        fprintf(
            stderr,
            "[GL] contexto NULL\n"
        );

        return;
    }

    fprintf(
        stderr,
        "[GL] contexto OpenGL criado\n"
    );

    fprintf(
        stderr,
        "[GL] vendor   = %s\n",
        glGetString(GL_VENDOR)
    );

    fprintf(
        stderr,
        "[GL] renderer = %s\n",
        glGetString(GL_RENDERER)
    );

    fprintf(
        stderr,
        "[GL] version  = %s\n",
        glGetString(GL_VERSION)
    );


    /* --------------------------------------------------------
     * LC_NUMERIC
     * -------------------------------------------------------- */

    fprintf(
        stderr,
        "[MPV] LC_NUMERIC antes = %s\n",
        setlocale(LC_NUMERIC, NULL)
    );

    if (setlocale(LC_NUMERIC, "C") == NULL) {

        fprintf(
            stderr,
            "[MPV] ERRO: não foi possível definir LC_NUMERIC=C\n"
        );

        return;
    }

    fprintf(
        stderr,
        "[MPV] LC_NUMERIC depois = %s\n",
        setlocale(LC_NUMERIC, NULL)
    );


    /* --------------------------------------------------------
     * MPV CREATE
     * -------------------------------------------------------- */

    fprintf(
        stderr,
        "[MPV] chamando mpv_create()\n"
    );

    pa->mpv = mpv_create();

    if (!pa->mpv) {

        fprintf(
            stderr,
            "[MPV] ERRO: mpv_create() retornou NULL\n"
        );

        return;
    }

    fprintf(
        stderr,
        "[MPV] mpv_create() OK\n"
    );


    mpv_set_option_string(
        pa->mpv,
        "terminal",
        "yes"
    );

    mpv_set_option_string(
        pa->mpv,
        "msg-level",
        "all=warn"
    );


    /* --------------------------------------------------------
     * VO
     * -------------------------------------------------------- */

    int status = mpv_set_option_string(
        pa->mpv,
        "vo",
        "libmpv"
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] ERRO: vo=libmpv: %s\n",
            mpv_error_string(status)
        );

        mpv_terminate_destroy(pa->mpv);
        pa->mpv = NULL;

        return;
    }


    /* --------------------------------------------------------
     * LOOP
     * -------------------------------------------------------- */

    status = mpv_set_option_string(
        pa->mpv,
        "loop-file",
        "yes"
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] ERRO: loop-file=yes: %s\n",
            mpv_error_string(status)
        );
    }


    /* --------------------------------------------------------
     * HWDEC
     * -------------------------------------------------------- */

    status = mpv_set_option_string(
        pa->mpv,
        "hwdec",
        "auto"
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] ERRO: hwdec=auto: %s\n",
            mpv_error_string(status)
        );

        mpv_terminate_destroy(pa->mpv);
        pa->mpv = NULL;

        return;
    }


    /* --------------------------------------------------------
     * MPV INITIALIZE
     * -------------------------------------------------------- */

    status = mpv_initialize(pa->mpv);

    if (status < 0) {

        fprintf(
            stderr,
            "[MPV] ERRO em mpv_initialize(): %s\n",
            mpv_error_string(status)
        );

        mpv_terminate_destroy(pa->mpv);
        pa->mpv = NULL;

        return;
    }

    fprintf(
        stderr,
        "[MPV] mpv_initialize() OK\n"
    );


    /* --------------------------------------------------------
     * RENDER CONTEXT
     * -------------------------------------------------------- */

    mpv_opengl_init_params gl_init = {
        .get_proc_address = mpv_get_proc_address,
        .get_proc_address_ctx = NULL
    };

    mpv_render_param params[] = {

        {
            MPV_RENDER_PARAM_API_TYPE,
            (void *)MPV_RENDER_API_TYPE_OPENGL
        },

        {
            MPV_RENDER_PARAM_OPENGL_INIT_PARAMS,
            &gl_init
        },

        {
            MPV_RENDER_PARAM_INVALID,
            NULL
        }
    };


    int render_status =
        mpv_render_context_create(
            &pa->mpv_render,
            pa->mpv,
            params
        );

    if (render_status < 0) {

        fprintf(
            stderr,
            "[MPV-RENDER] ERRO: %s\n",
            mpv_error_string(render_status)
        );

        mpv_terminate_destroy(pa->mpv);
        pa->mpv = NULL;

        return;
    }


    fprintf(
        stderr,
        "[MPV-RENDER] mpv_render_context criado: %p\n",
        (void *)pa->mpv_render
    );


    mpv_render_context_set_update_callback(
        pa->mpv_render,
        on_mpv_update,
        pa
    );


    /* --------------------------------------------------------
     * LOAD FILE
     * -------------------------------------------------------- */

    if (pa->filename) {

        fprintf(
            stderr,
            "[MPV] carregando arquivo: %s\n",
            pa->filename
        );

        const char *cmd[] = {
            "loadfile",
            pa->filename,
            NULL
        };

        status = mpv_command(
            pa->mpv,
            cmd
        );

        if (status < 0) {

            fprintf(
                stderr,
                "[MPV] ERRO loadfile: %s\n",
                mpv_error_string(status)
            );
        }
    }
}


/* ============================================================
 * GtkGLArea UNREALIZE
 * ============================================================ */

static void on_gl_unrealize(
    GtkGLArea *area,
    PlayerApp *pa)
{
    fprintf(
        stderr,
        "[GL] UNREALIZE GtkGLArea\n"
    );

    (void)area;

    if (pa->mpv_event_source) {

        g_source_remove(
            pa->mpv_event_source
        );

        pa->mpv_event_source = 0;
    }


    if (pa->mpv_render) {

        fprintf(
            stderr,
            "[MPV-RENDER] destruindo render context\n"
        );

        mpv_render_context_free(
            pa->mpv_render
        );

        pa->mpv_render = NULL;
    }


    if (pa->mpv) {

        fprintf(
            stderr,
            "[MPV] destruindo mpv\n"
        );

        mpv_terminate_destroy(
            pa->mpv
        );

        pa->mpv = NULL;
    }
}


/* ============================================================
 * RESIZE
 * ============================================================ */

static void on_gl_resize(
    GtkGLArea *area,
    int width,
    int height,
    PlayerApp *pa)
{
    (void)area;
    (void)pa;

    fprintf(
        stderr,
        "[gtk-resize] size=%dx%d\n",
        width,
        height
    );
}


/* ============================================================
 * RENDER
 * ============================================================ */

static gboolean on_gl_render(
    GtkGLArea *area,
    GdkGLContext *ctx,
    PlayerApp *pa)
{
    (void)ctx;

    int width =
        gtk_widget_get_width(
            GTK_WIDGET(area)
        );

    int height =
        gtk_widget_get_height(
            GTK_WIDGET(area)
        );


    if (width <= 0 || height <= 0)
        return TRUE;


    if (!pa->mpv_render) {

        glClearColor(
            0.2f,
            0.2f,
            0.2f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT
        );

        return TRUE;
    }


    glViewport(
        0,
        0,
        width,
        height
    );


    GLint current_fbo = 0;

    glGetIntegerv(
        GL_DRAW_FRAMEBUFFER_BINDING,
        &current_fbo
    );


    mpv_opengl_fbo fbo = {
        .fbo = current_fbo,
        .w = width,
        .h = height,
        .internal_format = 0
    };


    int flip_y = 1;


    mpv_render_param params[] = {

        {
            MPV_RENDER_PARAM_OPENGL_FBO,
            &fbo
        },

        {
            MPV_RENDER_PARAM_FLIP_Y,
            &flip_y
        },

        {
            MPV_RENDER_PARAM_INVALID,
            NULL
        }
    };


    int status =
        mpv_render_context_render(
            pa->mpv_render,
            params
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV-RENDER] ERRO: %s\n",
            mpv_error_string(status)
        );
    }


    mpv_render_context_report_swap(
        pa->mpv_render
    );


    return TRUE;
}


/* ============================================================
 * WINDOW REALIZE
 * ============================================================ */

static void on_window_realize(
    GtkWindow *window,
    PlayerApp *pa)
{
    (void)pa;

    fprintf(
        stderr,
        "[window] REALIZE\n"
    );

    fprintf(
        stderr,
        "[window] visible=%d mapped=%d\n",
        gtk_widget_get_visible(
            GTK_WIDGET(window)
        ),
        gtk_widget_get_mapped(
            GTK_WIDGET(window)
        )
    );
}


/* ============================================================
 * WINDOW MAP
 * ============================================================ */

static void on_window_map(
    GtkWindow *window,
    PlayerApp *pa)
{
    fprintf(
        stderr,
        "[window] MAP\n"
    );

    fprintf(
        stderr,
        "[window] visible=%d mapped=%d\n",
        gtk_widget_get_visible(
            GTK_WIDGET(window)
        ),
        gtk_widget_get_mapped(
            GTK_WIDGET(window)
        )
    );

    (void)pa;
}


/* ============================================================
 * FOCUS
 * ============================================================ */

static gboolean grab_gl_focus(gpointer data)
{
    PlayerApp *pa = data;

    gtk_widget_grab_focus(
        pa->gl_area
    );

    fprintf(
        stderr,
        "[gtk] foco GtkGLArea = %d\n",
        gtk_widget_has_focus(pa->gl_area)
    );

    return G_SOURCE_REMOVE;
}


/* ============================================================
 * ACTIVATE
 * ============================================================ */

static void on_activate(
    GtkApplication *app,
    PlayerApp *pa)
{
    fprintf(
        stderr,
        "[gtk] ACTIVATE\n"
    );


    pa->window =
        gtk_application_window_new(app);


    gtk_window_set_title(
        GTK_WINDOW(pa->window),
        "GtkGLArea TEST"
    );


    gtk_window_set_default_size(
        GTK_WINDOW(pa->window),
        960,
        540
    );


    /* --------------------------------------------------------
     * GL AREA
     * -------------------------------------------------------- */

    pa->gl_area =
        gtk_gl_area_new();


    gtk_gl_area_set_allowed_apis(
        GTK_GL_AREA(pa->gl_area),
        GDK_GL_API_GL
    );


    gtk_gl_area_set_required_version(
        GTK_GL_AREA(pa->gl_area),
        3,
        3
    );


    gtk_widget_set_hexpand(
        pa->gl_area,
        TRUE
    );

    gtk_widget_set_vexpand(
        pa->gl_area,
        TRUE
    );


    /* --------------------------------------------------------
     * OVERLAY
     * -------------------------------------------------------- */

    GtkWidget *overlay =
        gtk_overlay_new();


    gtk_overlay_set_child(
        GTK_OVERLAY(overlay),
        pa->gl_area
    );


    setup_info_label_css();


    pa->info_label =
        gtk_label_new(NULL);


    gtk_widget_add_css_class(
        pa->info_label,
        "video-info"
    );


    gtk_label_set_text(
        GTK_LABEL(pa->info_label),
        pa->filename
    );


    gtk_widget_set_halign(
        pa->info_label,
        GTK_ALIGN_END
    );


    gtk_widget_set_valign(
        pa->info_label,
        GTK_ALIGN_END
    );


    gtk_widget_set_margin_end(
        pa->info_label,
        15
    );


    gtk_widget_set_margin_bottom(
        pa->info_label,
        15
    );


    gtk_overlay_add_overlay(
        GTK_OVERLAY(overlay),
        pa->info_label
    );


    /* --------------------------------------------------------
     * DRAG
     * -------------------------------------------------------- */

    GtkGestureDrag *drag =
        GTK_GESTURE_DRAG(
            gtk_gesture_drag_new()
        );


    g_signal_connect(
        drag,
        "drag-begin",
        G_CALLBACK(on_drag_begin),
        pa
    );


    g_signal_connect(
        drag,
        "drag-update",
        G_CALLBACK(on_drag_update),
        pa
    );


    g_signal_connect(
        drag,
        "drag-end",
        G_CALLBACK(on_drag_end),
        pa
    );


    gtk_widget_add_controller(
        pa->gl_area,
        GTK_EVENT_CONTROLLER(drag)
    );


    /* --------------------------------------------------------
     * WINDOW CHILD
     * -------------------------------------------------------- */

    gtk_window_set_child(
        GTK_WINDOW(pa->window),
        overlay
    );


    /* --------------------------------------------------------
     * KEYBOARD
     * -------------------------------------------------------- */

    gtk_widget_set_focusable(
        pa->gl_area,
        TRUE
    );


    GtkEventControllerKey *key_controller =
        GTK_EVENT_CONTROLLER_KEY(
            gtk_event_controller_key_new()
        );


    gtk_event_controller_set_propagation_phase(
        GTK_EVENT_CONTROLLER(key_controller),
        GTK_PHASE_CAPTURE
    );


    g_signal_connect(
        key_controller,
        "key-pressed",
        G_CALLBACK(on_key_pressed),
        pa
    );


    gtk_widget_add_controller(
        pa->window,
        GTK_EVENT_CONTROLLER(key_controller)
    );


    /* --------------------------------------------------------
     * SCROLL
     * -------------------------------------------------------- */

    GtkEventControllerScroll *scroll_controller =
        GTK_EVENT_CONTROLLER_SCROLL(
            gtk_event_controller_scroll_new(
                GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES
            )
        );


    g_signal_connect(
        scroll_controller,
        "scroll",
        G_CALLBACK(on_scroll),
        pa
    );


    gtk_widget_add_controller(
        pa->gl_area,
        GTK_EVENT_CONTROLLER(scroll_controller)
    );


    /* --------------------------------------------------------
     * SIGNALS
     * -------------------------------------------------------- */

    g_signal_connect(
        pa->window,
        "realize",
        G_CALLBACK(on_window_realize),
        pa
    );


    g_signal_connect(
        pa->window,
        "map",
        G_CALLBACK(on_window_map),
        pa
    );


    g_signal_connect(
        pa->gl_area,
        "realize",
        G_CALLBACK(on_gl_realize),
        pa
    );


    g_signal_connect(
        pa->gl_area,
        "unrealize",
        G_CALLBACK(on_gl_unrealize),
        pa
    );


    g_signal_connect(
        pa->gl_area,
        "resize",
        G_CALLBACK(on_gl_resize),
        pa
    );


    g_signal_connect(
        pa->gl_area,
        "render",
        G_CALLBACK(on_gl_render),
        pa
    );


    gtk_gl_area_set_auto_render(
        GTK_GL_AREA(pa->gl_area),
        FALSE
    );


    gtk_window_present(
        GTK_WINDOW(pa->window)
    );


    pa->mpv_event_source =
        g_timeout_add(
            10,
            mpv_event_timer,
            pa
        );


    g_idle_add(
        grab_gl_focus,
        pa
    );
}


/* ============================================================
 * PUBLIC API
 * ============================================================ */

PlayerApp *player_app_new(const char *filename)
{
    PlayerApp *pa =
        calloc(1, sizeof(PlayerApp));

    if (!pa)
        return NULL;

    pa->filename = filename;

    pa->video_zoom = 0.0;
    pa->video_pan_x = 0.0;
    pa->video_pan_y = 0.0;

    pa->brightness = 0;
    pa->video_rotation = 0;

    return pa;
}


int player_app_run(
    PlayerApp *pa,
    int argc,
    char **argv)
{
    (void)argc;

    if (!pa)
        return EXIT_FAILURE;

    pa->app =
        gtk_application_new(
            "dev.local.gpiv-test",
            G_APPLICATION_NON_UNIQUE
        );

    g_signal_connect(
        pa->app,
        "activate",
        G_CALLBACK(on_activate),
        pa
    );


    /*
     * Não entregar os argumentos do vídeo
     * para o GApplication.
     */
    int gtk_argc = 1;

    char *gtk_argv[] = {
        argv[0],
        NULL
    };


    int status =
        g_application_run(
            G_APPLICATION(pa->app),
            gtk_argc,
            gtk_argv
        );


    g_object_unref(pa->app);

    pa->app = NULL;

    return status;
}


void player_app_free(PlayerApp *pa)
{
    if (!pa)
        return;

    /*
     * Em condições normais o GtkGLArea
     * já terá chamado unrealize e destruído mpv.
     */

    free(pa);
}
