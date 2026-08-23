#include "app.h"
#include "player.h"

#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>


/* ============================================================
 * PROTÓTIPOS INTERNOS
 * ============================================================ */

static void on_activate(
    GtkApplication *app,
    PlayerApp *pa
);

static gboolean restore_info_label(
    gpointer data
);


/* ============================================================
 * INFO LABEL
 * ============================================================ */

static gboolean restore_info_label(gpointer data)
{
    PlayerApp *pa = data;

    if (!pa ||
        !pa->info_label)
        return G_SOURCE_REMOVE;

    if (pa->filename) {

        gtk_label_set_text(
            GTK_LABEL(pa->info_label),
            pa->filename
        );

    } else {

        gtk_label_set_text(
            GTK_LABEL(pa->info_label),
            ""
        );
    }

    return G_SOURCE_REMOVE;
}


/* ============================================================
 * CLIPBOARD
 * ============================================================ */

static void show_clipboard_message(
    PlayerApp *pa)
{
    if (!pa ||
        !pa->info_label ||
        !pa->filename)
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


static void copy_video_path(
    PlayerApp *pa)
{
    if (!pa ||
        !pa->filename ||
        !pa->window)
        return;

    GdkDisplay *display =
        gtk_widget_get_display(
            pa->window
        );

    GdkClipboard *clipboard =
        gdk_display_get_clipboard(
            display
        );

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

    GdkDisplay *display =
        gdk_display_get_default();

    if (display) {

        gtk_style_context_add_provider_for_display(
            display,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }

    g_object_unref(provider);
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

    if (!pa)
        return FALSE;

    fprintf(
        stderr,
        "[KEY] keyval=0x%x\n",
        keyval
    );


    /* --------------------------------------------------------
     * Q -> SAIR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_q ||
        keyval == GDK_KEY_Q) {

        fprintf(
            stderr,
            "[KEY] Q DETECTADO -> saindo\n"
        );

        if (pa->window) {

            gtk_window_destroy(
                GTK_WINDOW(pa->window)
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * SPACE -> PAUSE / PLAY
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_space) {

        if (pa->player) {

            player_toggle_pause(
                pa->player
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * K -> FRAME ANTERIOR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_k ||
        keyval == GDK_KEY_K) {

        if (pa->player) {

            player_frame_back(
                pa->player
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * J -> PRÓXIMO FRAME
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_j ||
        keyval == GDK_KEY_J) {

        if (pa->player) {

            player_frame_forward(
                pa->player
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * S -> SCREENSHOT
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_s ||
        keyval == GDK_KEY_S) {

        if (pa->player) {

            player_save_frame(
                pa->player
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * Y -> COPIAR PATH
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_y ||
        keyval == GDK_KEY_Y) {

        copy_video_path(pa);

        return TRUE;
    }


    /* --------------------------------------------------------
     * R -> ROTATE
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_r ||
        keyval == GDK_KEY_R) {

        if (pa->player) {

            player_rotate(
                pa->player
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * U -> BRIGHTNESS +
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_u ||
        keyval == GDK_KEY_U) {

        if (pa->player) {

            player_change_brightness(
                pa->player,
                5
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * I -> BRIGHTNESS -
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_i ||
        keyval == GDK_KEY_I) {

        if (pa->player) {

            player_change_brightness(
                pa->player,
                -5
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * 0 -> RESET ZOOM / PAN
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_0) {

        if (pa->player) {

            player_reset_view(
                pa->player
            );
        }

        return TRUE;
    }


    return FALSE;
}


/* ============================================================
 * MPV -> GTK RENDER
 * ============================================================ */

static gboolean mpv_queue_render_idle(
    gpointer data)
{
    PlayerApp *pa = data;

    if (!pa ||
        !pa->gl_area)
        return G_SOURCE_REMOVE;

    gtk_gl_area_queue_render(
        GTK_GL_AREA(pa->gl_area)
    );

    return G_SOURCE_REMOVE;
}


static void on_mpv_update(
    void *ctx)
{
    PlayerApp *pa = ctx;

    if (!pa)
        return;

    g_main_context_invoke(
        NULL,
        mpv_queue_render_idle,
        pa
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
        setlocale(
            LC_NUMERIC,
            NULL
        )
    );

    if (setlocale(
            LC_NUMERIC,
            "C") == NULL) {

        fprintf(
            stderr,
            "[MPV] ERRO: não foi possível definir "
            "LC_NUMERIC=C\n"
        );

        return;
    }

    fprintf(
        stderr,
        "[MPV] LC_NUMERIC depois = %s\n",
        setlocale(
            LC_NUMERIC,
            NULL
        )
    );


    /* --------------------------------------------------------
     * CRIA PLAYER
     * -------------------------------------------------------- */

    pa->player =
        player_new();

    if (!pa->player) {

        fprintf(
            stderr,
            "[PLAYER] ERRO: não foi possível criar Player\n"
        );

        return;
    }


    /* --------------------------------------------------------
     * CONFIGURA MPV
     * -------------------------------------------------------- */

    int status = player_initialize(
        pa->player,
        pa->filename,
        on_mpv_update,
        pa
    );

    if (status < 0) {

        fprintf(
            stderr,
            "[PLAYER] ERRO ao inicializar player: %s\n",
            mpv_error_string(status)
        );

        player_free(
            pa->player
        );

        pa->player = NULL;

        return;
    }

    fprintf(
        stderr,
        "[PLAYER] inicializado com sucesso\n"
    );

    gtk_gl_area_queue_render(
        GTK_GL_AREA(pa->gl_area)
    );

}


/* ============================================================
 * GtkGLArea UNREALIZE
 * ============================================================ */

static void on_gl_unrealize(
    GtkGLArea *area,
    PlayerApp *pa)
{
    (void)area;

    fprintf(
        stderr,
        "[GL] UNREALIZE GtkGLArea\n"
    );

    if (pa &&
        pa->player) {

        fprintf(
            stderr,
            "[PLAYER] destruindo player\n"
        );

        player_free(
            pa->player
        );

        pa->player = NULL;
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

    fprintf(
        stderr,
        "[GL] RENDER\n"
    );

    int width =
        gtk_widget_get_width(
            GTK_WIDGET(area)
        );

    int height =
        gtk_widget_get_height(
            GTK_WIDGET(area)
        );

    if (width <= 0 ||
        height <= 0)
        return TRUE;


    /* --------------------------------------------------------
     * PLAYER AINDA NÃO DISPONÍVEL
     * -------------------------------------------------------- */

    if (!pa ||
        !pa->player ||
        !player_get_render_context(pa->player)) {

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


    /* --------------------------------------------------------
     * VIEWPORT
     * -------------------------------------------------------- */

    glViewport(
        0,
        0,
        width,
        height
    );


    /* --------------------------------------------------------
     * FBO DO GTK
     * -------------------------------------------------------- */

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


    /* --------------------------------------------------------
     * FLIP Y
     * -------------------------------------------------------- */

    int flip_y = 1;


    /* --------------------------------------------------------
     * PARÂMETROS MPV
     * -------------------------------------------------------- */

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


    /* --------------------------------------------------------
     * RENDER
     * -------------------------------------------------------- */

    int status =
        mpv_render_context_render(
            player_get_render_context(
                pa->player
            ),
            params
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV-RENDER] ERRO: %s\n",
            mpv_error_string(status)
        );
    }


    /* --------------------------------------------------------
     * REPORT SWAP
     * -------------------------------------------------------- */

    mpv_render_context_report_swap(
        player_get_render_context(
            pa->player
        )
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

static gboolean grab_gl_focus(
    gpointer data)
{
    PlayerApp *pa = data;

    if (!pa ||
        !pa->gl_area)
        return G_SOURCE_REMOVE;

    gtk_widget_grab_focus(
        pa->gl_area
    );

    fprintf(
        stderr,
        "[gtk] foco GtkGLArea = %d\n",
        gtk_widget_has_focus(
            pa->gl_area
        )
    );

    return G_SOURCE_REMOVE;
}


/* ============================================================
 * ZOOM
 * ============================================================ */

static gboolean on_scroll(
    GtkEventControllerScroll *controller,
    double dx,
    double dy,
    PlayerApp *pa)
{
    (void)controller;
    (void)dx;

    if (!pa ||
        !pa->player)
        return FALSE;

    player_change_zoom(
        pa->player,
        dy
    );

    return TRUE;
}


/* ============================================================
 * PAN
 * ============================================================ */

static void on_drag_begin(
    GtkGestureDrag *gesture,
    double start_x,
    double start_y,
    PlayerApp *pa)
{
    (void)gesture;

    if (!pa ||
        !pa->player)
        return;

    /*
     * A API do Player agora recebe somente
     * player + posição inicial.
     */

    player_pan_begin(
        pa->player,
        start_x,
        start_y
    );
}


static void on_drag_update(
    GtkGestureDrag *gesture,
    double offset_x,
    double offset_y,
    PlayerApp *pa)
{
    (void)gesture;

    if (!pa ||
        !pa->player)
        return;

    int width =
        gtk_widget_get_width(
            pa->gl_area
        );

    int height =
        gtk_widget_get_height(
            pa->gl_area
        );

    player_pan_update(
        pa->player,
        offset_x,
        offset_y,
        width,
        height
    );
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

    if (!pa ||
        !pa->player)
        return;

    player_pan_end(
        pa->player
    );
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


    /* --------------------------------------------------------
     * WINDOW
     * -------------------------------------------------------- */

    pa->window =
        gtk_application_window_new(
            app
        );


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


    if (pa->filename) {

        gtk_label_set_text(
            GTK_LABEL(pa->info_label),
            pa->filename
        );

    } else {

        gtk_label_set_text(
            GTK_LABEL(pa->info_label),
            ""
        );
    }


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


    /* --------------------------------------------------------
     * RENDER MANUAL
     * -------------------------------------------------------- */

    gtk_gl_area_set_auto_render(
        GTK_GL_AREA(pa->gl_area),
        FALSE
    );


    /* --------------------------------------------------------
     * MOSTRA WINDOW
     * -------------------------------------------------------- */

    gtk_window_present(
        GTK_WINDOW(pa->window)
    );


    /* --------------------------------------------------------
     * FOCO
     * -------------------------------------------------------- */

    g_idle_add(
        grab_gl_focus,
        pa
    );
}


/* ============================================================
 * PUBLIC API
 * ============================================================ */

PlayerApp *player_app_new(
    const char *filename)
{
    PlayerApp *pa =
        calloc(
            1,
            sizeof(PlayerApp)
        );

    if (!pa)
        return NULL;


    pa->app = NULL;
    pa->window = NULL;
    pa->gl_area = NULL;
    pa->info_label = NULL;

    pa->player = NULL;

    pa->filename = filename;


    return pa;
}


/* ============================================================
 * RUN
 * ============================================================ */

int player_app_run(
    PlayerApp *pa,
    int argc,
    char **argv)
{
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


    (void)argc;


    int status =
        g_application_run(
            G_APPLICATION(pa->app),
            gtk_argc,
            gtk_argv
        );


    g_object_unref(
        pa->app
    );

    pa->app = NULL;


    return status;
}


/* ============================================================
 * FREE
 * ============================================================ */

void player_app_free(
    PlayerApp *pa)
{
    if (!pa)
        return;


    /*
     * Normalmente GtkGLArea já chamou
     * on_gl_unrealize().
     */

    if (pa->player) {

        player_free(
            pa->player
        );

        pa->player = NULL;
    }


    free(pa);
}
