#include "controls.h"
#include "player.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>


/* ============================================================
 * CONTEXTO INTERNO
 * ============================================================ */

typedef struct {

    GtkWidget *window;

    GtkWidget *gl_area;

    GtkWidget *info_label;

    PlayerApp *app;

} Controls;


/* ============================================================
 * RESTAURA INFO LABEL
 * ============================================================ */

static gboolean restore_info_label(
    gpointer data)
{
    Controls *controls = data;


    if (!controls ||
        !controls->info_label)
        return G_SOURCE_REMOVE;


    const char *filename =
        player_app_get_filename(
            controls->app
        );


    if (filename) {

        gtk_label_set_text(
            GTK_LABEL(controls->info_label),
            filename
        );

    } else {

        gtk_label_set_text(
            GTK_LABEL(controls->info_label),
            ""
        );
    }


    return G_SOURCE_REMOVE;
}


/* ============================================================
 * MENSAGEM NO LABEL
 * ============================================================ */

static void show_message(
    Controls *controls,
    const char *message)
{
    if (!controls ||
        !controls->info_label ||
        !message)
        return;


    gtk_label_set_text(
        GTK_LABEL(controls->info_label),
        message
    );


    g_timeout_add(
        2000,
        restore_info_label,
        controls
    );
}


/* ============================================================
 * CLIPBOARD
 * ============================================================ */

static void copy_video_path(
    Controls *controls)
{
    if (!controls ||
        !controls->app ||
        !controls->window)
        return;


    const char *filename =
        player_app_get_filename(
            controls->app
        );


    if (!filename)
        return;


    GdkDisplay *display =
        gtk_widget_get_display(
            controls->window
        );


    if (!display)
        return;


    GdkClipboard *clipboard =
        gdk_display_get_clipboard(
            display
        );


    if (!clipboard)
        return;


    gdk_clipboard_set_text(
        clipboard,
        filename
    );


    fprintf(
        stderr,
        "[CLIPBOARD] copiado: %s\n",
        filename
    );


    show_message(
        controls,
        "Path copiado"
    );
}


/* ============================================================
 * ARQUIVO ANTERIOR
 * ============================================================ */

static void previous_file(
    Controls *controls)
{
    if (!controls ||
        !controls->app)
        return;


    int status =
        player_app_previous(
            controls->app
        );


    if (status == 0) {

        const char *filename =
            player_app_get_filename(
                controls->app
            );


        if (filename) {

            gtk_label_set_text(
                GTK_LABEL(controls->info_label),
                filename
            );
        }


        fprintf(
            stderr,
            "[KEY] arquivo anterior carregado\n"
        );

    } else {

        show_message(
            controls,
            "Primeiro arquivo"
        );
    }
}


/* ============================================================
 * PRÓXIMO ARQUIVO
 * ============================================================ */

static void next_file(
    Controls *controls)
{
    if (!controls ||
        !controls->app)
        return;


    int status =
        player_app_next(
            controls->app
        );


    if (status == 0) {

        const char *filename =
            player_app_get_filename(
                controls->app
            );


        if (filename) {

            gtk_label_set_text(
                GTK_LABEL(controls->info_label),
                filename
            );
        }


        fprintf(
            stderr,
            "[KEY] próximo arquivo carregado\n"
        );

    } else {

        show_message(
            controls,
            "Último arquivo"
        );
    }
}


/* ============================================================
 * KEYBOARD
 * ============================================================ */

static gboolean on_key_pressed(
    GtkEventControllerKey *controller,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    Controls *controls)
{
    (void)controller;
    (void)keycode;
    (void)state;


    if (!controls)
        return FALSE;


    fprintf(
        stderr,
        "[KEY] keyval=0x%x\n",
        keyval
    );


    Player *player = NULL;


    if (controls->app &&
        controls->app->render) {

        player =
            render_get_player(
                controls->app->render
            );
    }


    /* --------------------------------------------------------
     * Q -> SAIR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_q ||
        keyval == GDK_KEY_Q) {

        fprintf(
            stderr,
            "[KEY] Q DETECTADO -> saindo\n"
        );


        if (controls->window) {

            gtk_window_destroy(
                GTK_WINDOW(controls->window)
            );
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * H -> ARQUIVO ANTERIOR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_h ||
        keyval == GDK_KEY_H) {

        previous_file(
            controls
        );


        return TRUE;
    }


    /* --------------------------------------------------------
     * L -> PRÓXIMO ARQUIVO
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_l ||
        keyval == GDK_KEY_L) {

        next_file(
            controls
        );


        return TRUE;
    }


    /* --------------------------------------------------------
     * SPACE -> PAUSE / PLAY
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_space) {

        if (player) {

            player_toggle_pause(
                player
            );
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * K -> FRAME ANTERIOR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_k ||
        keyval == GDK_KEY_K) {

        if (player) {

            player_frame_back(
                player
            );
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * J -> PRÓXIMO FRAME
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_j ||
        keyval == GDK_KEY_J) {

        if (player) {

            player_frame_forward(
                player
            );
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * S -> SCREENSHOT
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_s ||
        keyval == GDK_KEY_S) {

        if (player) {

            const char *screenshot =
                player_save_frame(
                    player
                );


            if (screenshot) {

                show_message(
                    controls,
                    screenshot
                );

            } else {

                show_message(
                    controls,
                    "Erro ao salvar screenshot"
                );
            }
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * Y -> COPIAR PATH
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_y ||
        keyval == GDK_KEY_Y) {

        copy_video_path(
            controls
        );


        return TRUE;
    }


    /* --------------------------------------------------------
     * R -> ROTATE
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_r ||
        keyval == GDK_KEY_R) {

        if (player) {

            player_rotate(
                player
            );
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * U -> BRIGHTNESS +
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_u ||
        keyval == GDK_KEY_U) {

        if (player) {

            player_change_brightness(
                player,
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

        if (player) {

            player_change_brightness(
                player,
                -5
            );
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * 0 -> RESET ZOOM / PAN
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_0) {

        if (player) {

            player_reset_view(
                player
            );
        }


        return TRUE;
    }


    return FALSE;
}


/* ============================================================
 * ZOOM
 * ============================================================ */

static gboolean on_scroll(
    GtkEventControllerScroll *controller,
    double dx,
    double dy,
    Controls *controls)
{
    (void)controller;
    (void)dx;


    if (!controls ||
        !controls->app ||
        !controls->app->render)
        return FALSE;


    Player *player =
        render_get_player(
            controls->app->render
        );


    if (!player)
        return FALSE;


    player_change_zoom(
        player,
        dy
    );


    return TRUE;
}


/* ============================================================
 * PAN BEGIN
 * ============================================================ */

static void on_drag_begin(
    GtkGestureDrag *gesture,
    double start_x,
    double start_y,
    Controls *controls)
{
    (void)gesture;


    if (!controls ||
        !controls->app ||
        !controls->app->render)
        return;


    Player *player =
        render_get_player(
            controls->app->render
        );


    if (!player)
        return;


    player_pan_begin(
        player,
        start_x,
        start_y
    );
}


/* ============================================================
 * PAN UPDATE
 * ============================================================ */

static void on_drag_update(
    GtkGestureDrag *gesture,
    double offset_x,
    double offset_y,
    Controls *controls)
{
    (void)gesture;


    if (!controls ||
        !controls->app ||
        !controls->app->render ||
        !controls->gl_area)
        return;


    Player *player =
        render_get_player(
            controls->app->render
        );


    if (!player)
        return;


    int width =
        gtk_widget_get_width(
            controls->gl_area
        );


    int height =
        gtk_widget_get_height(
            controls->gl_area
        );


    player_pan_update(
        player,
        offset_x,
        offset_y,
        width,
        height
    );
}


/* ============================================================
 * PAN END
 * ============================================================ */

static void on_drag_end(
    GtkGestureDrag *gesture,
    double offset_x,
    double offset_y,
    Controls *controls)
{
    (void)gesture;
    (void)offset_x;
    (void)offset_y;


    if (!controls ||
        !controls->app ||
        !controls->app->render)
        return;


    Player *player =
        render_get_player(
            controls->app->render
        );


    if (!player)
        return;


    player_pan_end(
        player
    );
}


/* ============================================================
 * FOCUS
 * ============================================================ */

static gboolean grab_gl_focus(
    gpointer data)
{
    Controls *controls = data;


    if (!controls ||
        !controls->gl_area)
        return G_SOURCE_REMOVE;


    gtk_widget_grab_focus(
        controls->gl_area
    );


    fprintf(
        stderr,
        "[gtk] foco GtkGLArea = %d\n",
        gtk_widget_has_focus(
            controls->gl_area
        )
    );


    return G_SOURCE_REMOVE;
}


/* ============================================================
 * WINDOW REALIZE
 * ============================================================ */

static void on_window_realize(
    GtkWindow *window,
    Controls *controls)
{
    (void)controls;


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
    Controls *controls)
{
    (void)controls;


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
}


/* ============================================================
 * SETUP
 * ============================================================ */

void controls_setup(
    GtkWidget *window,
    GtkWidget *gl_area,
    GtkWidget *info_label,
    PlayerApp *app)
{
    if (!window ||
        !gl_area ||
        !app ||
        !app->render)
        return;


    /*
     * O Controls precisa permanecer vivo porque
     * os callbacks GTK usam este ponteiro.
     */

    Controls *controls =
        calloc(
            1,
            sizeof(Controls)
        );


    if (!controls) {

        fprintf(
            stderr,
            "[CONTROLS] ERRO: calloc()\n"
        );

        return;
    }


    controls->window = window;
    controls->gl_area = gl_area;
    controls->info_label = info_label;
    controls->app = app;


    fprintf(
        stderr,
        "[CONTROLS] configurando controles\n"
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
        controls
    );


    g_signal_connect(
        drag,
        "drag-update",
        G_CALLBACK(on_drag_update),
        controls
    );


    g_signal_connect(
        drag,
        "drag-end",
        G_CALLBACK(on_drag_end),
        controls
    );


    gtk_widget_add_controller(
        gl_area,
        GTK_EVENT_CONTROLLER(drag)
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
        controls
    );


    gtk_widget_add_controller(
        gl_area,
        GTK_EVENT_CONTROLLER(scroll_controller)
    );


    /* --------------------------------------------------------
     * FOCUS
     * -------------------------------------------------------- */

    gtk_widget_set_focusable(
        gl_area,
        TRUE
    );


    /* --------------------------------------------------------
     * KEYBOARD
     * -------------------------------------------------------- */

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
        controls
    );


    gtk_widget_add_controller(
        window,
        GTK_EVENT_CONTROLLER(key_controller)
    );


    /* --------------------------------------------------------
     * WINDOW SIGNALS
     * -------------------------------------------------------- */

    g_signal_connect(
        window,
        "realize",
        G_CALLBACK(on_window_realize),
        controls
    );


    g_signal_connect(
        window,
        "map",
        G_CALLBACK(on_window_map),
        controls
    );


    /* --------------------------------------------------------
     * FOCO INICIAL
     * -------------------------------------------------------- */

    g_idle_add(
        grab_gl_focus,
        controls
    );


    fprintf(
        stderr,
        "[CONTROLS] controles configurados\n"
    );
}
