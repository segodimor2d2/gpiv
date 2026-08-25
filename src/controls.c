#include "controls.h"
#include "player.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>


/* ============================================================
 * CONFIGURAÇÃO
 * ============================================================ */

#define FILE_JUMP 20
#define SEEK_SECONDS 3
#define SEEK_HARD_SECONDS 10

/* ============================================================
 * CONTEXTO INTERNO
 * ============================================================ */

typedef struct {

    GtkWidget *window;

    GtkWidget *gl_area;

    GtkWidget *info_label;

    PlayerApp *app;

    guint info_timer_id;

    char message[4096];

    /*
     * Leader
     */

    gboolean leadf;

    char leadfvar;

} Controls;

/* ============================================================
 * ATUALIZA INFO LABEL
 * ============================================================ */

static gboolean update_info_label(
    gpointer data)
{
    Controls *controls = data;


    if (!controls ||
        !controls->info_label ||
        !controls->app ||
        !controls->app->render)
        return G_SOURCE_CONTINUE;


    Player *player =
        render_get_player(
            controls->app->render
        );


    if (!player)
        return G_SOURCE_CONTINUE;


    double position = 0.0;
    double duration = 0.0;


    if (player_get_time(
            player,
            &position,
            &duration) < 0)
        return G_SOURCE_CONTINUE;


    int pos_sec =
        (int)position;

    int dur_sec =
        (int)duration;


    int pos_min =
        pos_sec / 60;

    int pos_seconds =
        pos_sec % 60;


    int dur_min =
        dur_sec / 60;

    int dur_seconds =
        dur_sec % 60;


    int percent = 0;


    if (duration > 0.0) {

        percent =
            (int)((position / duration) * 100.0);


        if (percent < 0)
            percent = 0;

        if (percent > 100)
            percent = 100;
    }


    const char *filename =
        player_get_filename(
            player
        );


    if (!filename)
        filename = "";


    char text[8192];

    char leader[16];

    leader[0] = '\0';


    if (controls->leadf) {

        if (controls->leadfvar == 'b') {

            snprintf(
                leader,
                sizeof(leader),
                "fb"
            );

        } else if (controls->leadfvar == 'c') {

            snprintf(
                leader,
                sizeof(leader),
                "fc"
            );

        } else if (controls->leadfvar == 's') {

            snprintf(
                leader,
                sizeof(leader),
                "fs"
            );

        } else if (controls->leadfvar == 'p') {

            snprintf(
                leader,
                sizeof(leader),
                "fp"
            );

        } else if (controls->leadfvar == 'z') {

            snprintf(
                leader,
                sizeof(leader),
                "fz"
            );

        } else if (controls->leadfvar == 'v') {

            snprintf(
                leader,
                sizeof(leader),
                "fv"
            );

        } else {

            snprintf(
                leader,
                sizeof(leader),
                "f"
            );
        }
    }


    if (controls->message[0]) {

        if (leader[0]) {

            snprintf(
                text,
                sizeof(text),
                "%02d:%02d / %02d:%02d / %d%%\n"
                "%s\n"
                "%s\n"
                "%s",
                pos_min,
                pos_seconds,
                dur_min,
                dur_seconds,
                percent,
                filename,
                controls->message,
                leader
            );

        } else {

            snprintf(
                text,
                sizeof(text),
                "%02d:%02d / %02d:%02d / %d%%\n"
                "%s\n"
                "%s",
                pos_min,
                pos_seconds,
                dur_min,
                dur_seconds,
                percent,
                filename,
                controls->message
            );
        }

    } else {

        if (leader[0]) {

            snprintf(
                text,
                sizeof(text),
                "%02d:%02d / %02d:%02d / %d%%\n"
                "%s\n"
                "%s",
                pos_min,
                pos_seconds,
                dur_min,
                dur_seconds,
                percent,
                filename,
                leader
            );

        } else {

            snprintf(
                text,
                sizeof(text),
                "%02d:%02d / %02d:%02d / %d%%\n"
                "%s",
                pos_min,
                pos_seconds,
                dur_min,
                dur_seconds,
                percent,
                filename
            );
        }
    }


    gtk_label_set_text(
        GTK_LABEL(controls->info_label),
        text
    );


    return G_SOURCE_CONTINUE;
}

/* ============================================================
 * MENSAGEM NO LABEL
 * ============================================================ */

static void show_message(
    Controls *controls,
    const char *message)
{
    if (!controls ||
        !message)
        return;


    snprintf(
        controls->message,
        sizeof(controls->message),
        "%s",
        message
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


    show_message(
        controls,
        "Path copiado"
    );
}


/* ============================================================
 * SALTO PARA FRENTE
 * ============================================================ */

static void jump_forward(
    Controls *controls)
{
    if (!controls ||
        !controls->app)
        return;


    int status =
        player_app_jump(
            controls->app,
            FILE_JUMP
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

    } else {

        show_message(
            controls,
            "Não há arquivos suficientes"
        );
    }
}


/* ============================================================
 * SALTO PARA TRÁS
 * ============================================================ */

static void jump_backward(
    Controls *controls)
{
    if (!controls ||
        !controls->app)
        return;


    int status =
        player_app_jump(
            controls->app,
            -(int)FILE_JUMP
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

    } else {

        show_message(
            controls,
            "Não há arquivos suficientes"
        );
    }
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

    Player *player = NULL;


    if (controls->app &&
        controls->app->render) {

        player =
            render_get_player(
                controls->app->render
            );
    }


    /* --------------------------------------------------------
     * ESC -> SAI DO LEADER
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_Escape) {

        controls->leadf = FALSE;
        controls->leadfvar = '\0';

        fprintf(
            stderr,
            "[LEADER] OFF\n"
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * F -> LEADER
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_f) {

        controls->leadf = TRUE;
        controls->leadfvar = '\0';

        fprintf(
            stderr,
            "[LEADER] ON\n"
        );

        return TRUE;
    }


    /* --------------------------------------------------------
     * LEADER
     *
     * f  -> ativa leader
     * fb -> brightness
     * fc -> contrast
     * fp -> screenshot
     *
     * O leader NÃO bloqueia outras teclas.
     * -------------------------------------------------------- */

    if (controls->leadf) {

        /* ----------------------------------------------------
         * B -> BRIGHTNESS
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_b ||
            keyval == GDK_KEY_B) {

            controls->leadfvar = 'b';

            fprintf(
                stderr,
                "[LEADER] fb\n"
            );

            return TRUE;
        }


        /* ----------------------------------------------------
         * C -> CONTRAST
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_c ||
            keyval == GDK_KEY_C) {

            controls->leadfvar = 'c';

            fprintf(
                stderr,
                "[LEADER] fc\n"
            );

            return TRUE;
        }

        /* ----------------------------------------------------
         * S -> SATURATION
         *
         * fs
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_s ||
            keyval == GDK_KEY_S) {

            controls->leadfvar = 's';

            fprintf(
                stderr,
                "[LEADER] fs\n"
            );

            return TRUE;
        }

        /* ----------------------------------------------------
         * V -> VOLUME
         *
         * fv
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_v ||
            keyval == GDK_KEY_V) {

            controls->leadfvar = 'v';

            fprintf(
                stderr,
                "[LEADER] fv\n"
            );

            return TRUE;
        }

        /* ----------------------------------------------------
         * P -> SCREENSHOT
         *
         * fp
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_p ||
            keyval == GDK_KEY_P) {

            controls->leadfvar = 'p';

            fprintf(
                stderr,
                "[LEADER] fp\n"
            );


            if (player) {

                const char *screenshot =
                    player_save_frame(
                        player
                    );


                if (screenshot) {

                    char message[4096];


                    snprintf(
                        message,
                        sizeof(message),
                        "screenshot salvo: %s",
                        screenshot
                    );


                    show_message(
                        controls,
                        message
                    );

                } else {

                    show_message(
                        controls,
                        "Erro ao salvar screenshot"
                    );
                }
            }


            /*
             * Continua dentro do leader.
             *
             * Portanto:
             *
             * f -> fp
             *
             * e depois ainda pode:
             *
             * fb
             * fc
             */

            return TRUE;
        }

        /* ----------------------------------------------------
         * Z -> ZOOM
         *
         * fz
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_z ||
            keyval == GDK_KEY_Z) {

            controls->leadfvar = 'z';

            fprintf(
                stderr,
                "[LEADER] fz\n"
            );

            return TRUE;
        }

        /* ----------------------------------------------------
         * U -> FILTRO +
         *
         * fb + u -> brightness +
         * fc + u -> contrast +
         *
         * fp + u -> não faz nada
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_u ||
            keyval == GDK_KEY_U) {

            if (player) {

                if (controls->leadfvar == 'b') {

                    player_change_brightness(
                        player,
                        1
                    );

                } else if (controls->leadfvar == 'c') {

                    player_change_contrast(
                        player,
                        1
                    );

                } else if (controls->leadfvar == 's') {

                    player_change_saturation(
                        player,
                        -1
                    );
                }
            }

            return TRUE;
        }


        /* ----------------------------------------------------
         * I -> FILTRO -
         *
         * fb + i -> brightness -
         * fc + i -> contrast -
         *
         * fp + i -> não faz nada
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_i ||
            keyval == GDK_KEY_I) {

            if (player) {

                if (controls->leadfvar == 'b') {

                    player_change_brightness(
                        player,
                        -1
                    );

                } else if (controls->leadfvar == 'c') {

                    player_change_contrast(
                        player,
                        -1
                    );

                } else if (controls->leadfvar == 's') {

                    player_change_saturation(
                        player,
                        -1
                    );
                }
            }

            return TRUE;
        }
    }

    /* ========================================================
     * A PARTIR DAQUI CONTINUA O TECLADO NORMAL
     * ======================================================== */

    /* --------------------------------------------------------
     * Q -> SAIR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_q ||
        keyval == GDK_KEY_Q) {


        if (controls->window) {

            gtk_window_destroy(
                GTK_WINDOW(controls->window)
            );
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * K -> ARQUIVO ANTERIOR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_k) {

        previous_file(
            controls
        );

        return TRUE;
    }


    /* --------------------------------------------------------
     * J -> PRÓXIMO ARQUIVO
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_j) {

        next_file(
            controls
        );

        return TRUE;
    }


    /* --------------------------------------------------------
     * l MAIÚSCULO -> RETROCEDE FILE_JUMP ARQUIVOS
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_l) {

        jump_backward(
            controls
        );

        return TRUE;
    }


    /* --------------------------------------------------------
     * h MAIÚSCULO -> AVANÇA FILE_JUMP ARQUIVOS
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_h) {

        jump_forward(
            controls
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * SPACE / ENTER -> PAUSE / PLAY
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_space ||
        keyval == GDK_KEY_Return) {

        if (player) {

            player_toggle_pause(
                player
            );
        }

        return TRUE;
    }

    /* --------------------------------------------------------
     * . -> FRAME ANTERIOR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_period) {

        if (player) {

            player_frame_back(
                player
            );
        }


        return TRUE;
    }


    /* --------------------------------------------------------
     * n -> PRÓXIMO FRAME
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_n || keyval == GDK_KEY_N) {

        if (player) {

            player_frame_forward(
                player
            );
        }


        return TRUE;
    }

    /* --------------------------------------------------------
     * M -> SEEK_HARD_SECONDS SEGUNDOS
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_M) {

        if (player) {

            player_seek_forward(
                player,
                SEEK_HARD_SECONDS
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * < -> SEEK_HARD_SECONDS SEGUNDOS
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_less) {

        if (player) {

            player_seek_backward(
                player,
                SEEK_HARD_SECONDS
            );
        }


        return TRUE;
    }

    /* --------------------------------------------------------
     * m -> AVANÇA SEGUNDOS
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_m) {

        if (player) {

            player_seek_forward(
                player,
                SEEK_SECONDS
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * , -> RETROCEDE SEGUNDOS
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_comma) {

        if (player) {

            player_seek_backward(
                player,
                SEEK_SECONDS
            );
        }


        return TRUE;
    }

    /* --------------------------------------------------------
     * V -> VOLUME +
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_V) {

        if (player) {

            player_change_volume(
                player,
                5
            );
        }


        return TRUE;
    }

    /* --------------------------------------------------------
     * C -> VOLUME -
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_C) {

        if (player) {

            player_change_volume(
                player,
                -5
            );
        }


        return TRUE;
    }

    /* --------------------------------------------------------
     * x -> VOLUME 0
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_X) {

        if (player) {

            player_change_volume(
                player,
                -200
            );
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
     * z -> RESET ZOOM / PAN
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_z) {

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
 * SCROLL
 * ============================================================ */

static gboolean on_scroll(
    GtkEventControllerScroll *controller,
    double dx,
    double dy,
    Controls *controls)
{
    (void)controller;
    (void)dx;


    if (!controls)
        return FALSE;


    if (!controls->app ||
        !controls->app->render)
        return FALSE;


    Player *player =
        render_get_player(
            controls->app->render
        );


    if (!player)
        return FALSE;


    /* --------------------------------------------------------
     * FZ -> ZOOM
     * -------------------------------------------------------- */

    if (controls->leadf &&
        controls->leadfvar == 'z') {

        player_change_zoom(
            player,
            -dy
        );

        return TRUE;
    }


    /* --------------------------------------------------------
     * FB -> BRIGHTNESS
     * -------------------------------------------------------- */

    if (controls->leadf &&
        controls->leadfvar == 'b') {

        /*
         * Scroll para cima:
         * brightness +
         *
         * Scroll para baixo:
         * brightness -
         */

        player_change_brightness(
            player,
            (dy < 0.0) ? 1 : -1
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * FC -> CONTRAST
     * -------------------------------------------------------- */

    if (controls->leadf &&
        controls->leadfvar == 'c') {

        /*
         * Scroll para cima:
         * contrast +
         *
         * Scroll para baixo:
         * contrast -
         */

        player_change_contrast(
            player,
            (dy < 0.0) ? 1 : -1
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * FS -> SATURATION
     * -------------------------------------------------------- */

    if (controls->leadf &&
        controls->leadfvar == 's') {

        /*
         * Scroll para cima:
         * saturation +
         *
         * Scroll para baixo:
         * saturation -
         */

        player_change_saturation(
            player,
            (dy < 0.0) ? 1 : -1
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * FV -> VOLUME
     * -------------------------------------------------------- */

    if (controls->leadf &&
        controls->leadfvar == 'v') {

        /*
         * Scroll para cima:
         * volume +
         *
         * Scroll para baixo:
         * volume -
         */

        player_change_volume(
            player,
            (dy < 0.0) ? 5 : -5
        );

        return TRUE;
    }

    /*
     * Sem leader de scroll:
     *
     * fz -> zoom
     * fb -> brightness
     * fc -> contrast
     * fv -> volume
     *
     * Fora desses modos o scroll não faz nada.
     */

    return FALSE;
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

    controls->leadf = FALSE;
    controls->leadfvar = '\0';

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
     * FOCO INICIAL
     * -------------------------------------------------------- */

    g_idle_add(
        grab_gl_focus,
        controls
    );


    /* --------------------------------------------------------
     * TIMER DO INFO LABEL
     *
     * Atualiza posição/duração aproximadamente 4 vezes
     * por segundo.
     * -------------------------------------------------------- */

    controls->info_timer_id =
        g_timeout_add(
            250,
            update_info_label,
            controls
        );


    fprintf(
        stderr,
        "[CONTROLS] controles configurados\n"
    );
}
