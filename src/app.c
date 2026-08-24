#include "app.h"
#include "render.h"
#include "ui.h"
#include "controls.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>


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
     * CONTROLS
     * -------------------------------------------------------- */

    controls_setup(
        pa->window,
        gl_area,
        pa->info_label,
        pa->render,
        pa->filename
    );


    /* --------------------------------------------------------
     * MOSTRA WINDOW
     * -------------------------------------------------------- */

    gtk_window_present(
        GTK_WINDOW(pa->window)
    );
}


/* ============================================================
 * CRIAÇÃO
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


    if (pa->render) {

        render_free(
            pa->render
        );


        pa->render = NULL;
    }


    free(pa);
}
