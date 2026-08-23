#include "app.h"
#include "player.h"
#include "render.h"
#include "ui.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>


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

static gboolean restore_info_label(
    gpointer data)
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


    show_clipboard_message(
        pa
    );


    g_timeout_add(
        2000,
        restore_info_label,
        pa
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


    if (!pa)
        return FALSE;


    fprintf(
        stderr,
        "[KEY] keyval=0x%x\n",
        keyval
    );


    Player *player = NULL;


    if (pa->render) {

        player =
            render_get_player(
                pa->render
            );
    }


    /* --------------------------------------------------------
     * Q
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
     * SPACE
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_space) {

        if (player)
            player_toggle_pause(player);


        return TRUE;
    }


    /* --------------------------------------------------------
     * K
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_k ||
        keyval == GDK_KEY_K) {

        if (player)
            player_frame_back(player);


        return TRUE;
    }


    /* --------------------------------------------------------
     * J
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_j ||
        keyval == GDK_KEY_J) {

        if (player)
            player_frame_forward(player);


        return TRUE;
    }


    /* --------------------------------------------------------
     * S
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_s ||
        keyval == GDK_KEY_S) {

        if (player)
            player_save_frame(player);


        return TRUE;
    }


    /* --------------------------------------------------------
     * Y
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_y ||
        keyval == GDK_KEY_Y) {

        copy_video_path(pa);

        return TRUE;
    }


    /* --------------------------------------------------------
     * R
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_r ||
        keyval == GDK_KEY_R) {

        if (player)
            player_rotate(player);


        return TRUE;
    }


    /* --------------------------------------------------------
     * U
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_u ||
        keyval == GDK_KEY_U) {

        if (player)
            player_change_brightness(
                player,
                5
            );


        return TRUE;
    }


    /* --------------------------------------------------------
     * I
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_i ||
        keyval == GDK_KEY_I) {

        if (player)
            player_change_brightness(
                player,
                -5
            );


        return TRUE;
    }


    /* --------------------------------------------------------
     * 0
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_0) {

        if (player)
            player_reset_view(player);


        return TRUE;
    }


    return FALSE;
}


/* ============================================================
 * FOCUS
 * ============================================================ */

static gboolean grab_gl_focus(
    gpointer data)
{
    PlayerApp *pa = data;


    if (!pa ||
        !pa->render)
        return G_SOURCE_REMOVE;


    GtkWidget *gl_area =
        render_get_widget(
            pa->render
        );


    if (!gl_area)
        return G_SOURCE_REMOVE;


    gtk_widget_grab_focus(
        gl_area
    );


    fprintf(
        stderr,
        "[gtk] foco GtkGLArea = %d\n",
        gtk_widget_has_focus(gl_area)
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
        !pa->render)
        return FALSE;


    Player *player =
        render_get_player(
            pa->render
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
        !pa->render)
        return;


    Player *player =
        render_get_player(
            pa->render
        );


    if (!player)
        return;


    player_pan_begin(
        player,
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
        !pa->render)
        return;


    Player *player =
        render_get_player(
            pa->render
        );


    if (!player)
        return;


    GtkWidget *gl_area =
        render_get_widget(
            pa->render
        );


    if (!gl_area)
        return;


    int width =
        gtk_widget_get_width(
            gl_area
        );


    int height =
        gtk_widget_get_height(
            gl_area
        );


    player_pan_update(
        player,
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
        !pa->render)
        return;


    Player *player =
        render_get_player(
            pa->render
        );


    if (!player)
        return;


    player_pan_end(
        player
    );
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
     * RENDER
     * -------------------------------------------------------- */

    pa->render =
        render_new(
            pa->filename
        );


    if (!pa->render) {

        fprintf(
            stderr,
            "[RENDER] ERRO criando Render\n"
        );

        return;
    }


    /* --------------------------------------------------------
     * UI
     * -------------------------------------------------------- */

    pa->window =
        ui_create_window(
            app,
            pa->render,
            pa->filename,
            &pa->info_label
        );


    if (!pa->window) {

        fprintf(
            stderr,
            "[UI] ERRO criando interface\n"
        );


        render_free(
            pa->render
        );


        pa->render = NULL;


        return;
    }


    /* --------------------------------------------------------
     * GL AREA
     * -------------------------------------------------------- */

    GtkWidget *gl_area =
        render_get_widget(
            pa->render
        );


    if (!gl_area) {

        fprintf(
            stderr,
            "[RENDER] GtkGLArea NULL\n"
        );


        return;
    }


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
        pa
    );


    gtk_widget_add_controller(
        gl_area,
        GTK_EVENT_CONTROLLER(scroll_controller)
    );


    /* --------------------------------------------------------
     * KEYBOARD
     * -------------------------------------------------------- */

    gtk_widget_set_focusable(
        gl_area,
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
     * WINDOW SIGNALS
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


    /* --------------------------------------------------------
     * MOSTRA
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
    pa->info_label = NULL;
    pa->render = NULL;
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


    if (pa->render) {

        render_free(
            pa->render
        );

        pa->render = NULL;
    }


    free(pa);
}
