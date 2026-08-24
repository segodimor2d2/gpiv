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
     * ARQUIVO ATUAL
     * -------------------------------------------------------- */

    const char *filename =
        player_app_get_filename(
            pa
        );


    if (!filename) {

        fprintf(
            stderr,
            "[APP] ERRO: filename atual NULL\n"
        );

        return;
    }

    /* --------------------------------------------------------
     * RENDER
     * -------------------------------------------------------- */

    pa->render =
        render_new(
            filename
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
            filename,
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
        pa
    );


    /* --------------------------------------------------------
     * MOSTRA WINDOW
     * -------------------------------------------------------- */

    gtk_window_present(
        GTK_WINDOW(pa->window)
    );
}


/* ============================================================
 * GET FILENAME
 * ============================================================ */

const char *player_app_get_filename(
    PlayerApp *pa)
{
    if (!pa ||
        !pa->filelist)
        return NULL;


    return filelist_get(
        pa->filelist,
        pa->current_index
    );
}


/* ============================================================
 * GET CURRENT INDEX
 * ============================================================ */

size_t player_app_get_current_index(
    PlayerApp *pa)
{
    if (!pa)
        return 0;


    return pa->current_index;
}


/* ============================================================
 * JUMP
 * ============================================================ */

int player_app_jump(
    PlayerApp *pa,
    int delta)
{
    if (!pa ||
        !pa->filelist ||
        !pa->render)
        return -1;


    size_t count =
        filelist_count(
            pa->filelist
        );


    if (count == 0)
        return -1;


    /*
     * Calcula o novo índice usando
     * aritmética assinada para evitar
     * problemas com size_t.
     */

    long new_index =
        (long)pa->current_index +
        delta;


    /*
     * Não permite passar do primeiro
     * ou do último arquivo.
     */

    if (new_index < 0)
        return -1;


    if ((size_t)new_index >= count)
        return -1;


    const char *filename =
        filelist_get(
            pa->filelist,
            (size_t)new_index
        );


    if (!filename)
        return -1;


    /* --------------------------------------------------------
     * PLAYER
     * -------------------------------------------------------- */

    Player *player =
        render_get_player(
            pa->render
        );


    if (!player) {

        fprintf(
            stderr,
            "[APP] Player NULL\n"
        );

        return -1;
    }


    fprintf(
        stderr,
        "[APP] salto: %zu -> %zu\n",
        pa->current_index,
        (size_t)new_index
    );


    fprintf(
        stderr,
        "[APP] arquivo: %s\n",
        filename
    );


    int status =
        player_load_file(
            player,
            filename
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[APP] ERRO carregando arquivo\n"
        );

        return -1;
    }


    pa->current_index =
        (size_t)new_index;


    return 0;
}


/* ============================================================
 * NEXT
 * ============================================================ */

int player_app_next(
    PlayerApp *pa)
{
    if (!pa ||
        !pa->filelist ||
        !pa->render)
        return -1;


    size_t count =
        filelist_count(
            pa->filelist
        );


    if (count == 0)
        return -1;


    /* --------------------------------------------------------
     * JÁ ESTÁ NO ÚLTIMO
     * -------------------------------------------------------- */

    if (pa->current_index + 1 >= count) {

        return -1;
    }


    size_t next_index =
        pa->current_index + 1;


    const char *filename =
        filelist_get(
            pa->filelist,
            next_index
        );


    if (!filename)
        return -1;


    /* --------------------------------------------------------
     * PLAYER
     * -------------------------------------------------------- */

    Player *player =
        render_get_player(
            pa->render
        );


    if (!player) {

        fprintf(
            stderr,
            "[APP] Player NULL\n"
        );

        return -1;
    }


    // fprintf(
    //     stderr,
    //     "[APP] índice: %zu -> %zu\n",
    //     pa->current_index,
    //     next_index
    // );


    fprintf(
        stderr,
        "[APP] arquivo: %s\n",
        filename
    );


    int status =
        player_load_file(
            player,
            filename
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[APP] ERRO carregando próximo arquivo\n"
        );

        return -1;
    }


    pa->current_index =
        next_index;


    // fprintf(
    //     stderr,
    //     "[APP] arquivo atual = %zu\n",
    //     pa->current_index
    // );


    return 0;
}


/* ============================================================
 * PREVIOUS
 * ============================================================ */

int player_app_previous(
    PlayerApp *pa)
{
    if (!pa ||
        !pa->filelist ||
        !pa->render)
        return -1;


    size_t count =
        filelist_count(
            pa->filelist
        );


    if (count == 0)
        return -1;


    /* --------------------------------------------------------
     * JÁ ESTÁ NO PRIMEIRO
     * -------------------------------------------------------- */

    if (pa->current_index == 0) {

        return -1;
    }


    size_t previous_index =
        pa->current_index - 1;


    const char *filename =
        filelist_get(
            pa->filelist,
            previous_index
        );


    if (!filename)
        return -1;


    /* --------------------------------------------------------
     * PLAYER
     * -------------------------------------------------------- */

    Player *player =
        render_get_player(
            pa->render
        );


    if (!player) {

        fprintf(
            stderr,
            "[APP] Player NULL\n"
        );

        return -1;
    }


    // fprintf(
    //     stderr,
    //     "[APP] índice: %zu -> %zu\n",
    //     pa->current_index,
    //     previous_index
    // );


    fprintf(
        stderr,
        "[APP] arquivo: %s\n",
        filename
    );


    int status =
        player_load_file(
            player,
            filename
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[APP] ERRO carregando arquivo anterior\n"
        );

        return -1;
    }


    pa->current_index =
        previous_index;


    // fprintf(
    //     stderr,
    //     "[APP] arquivo atual = %zu\n",
    //     pa->current_index
    // );


    return 0;
}


/* ============================================================
 * CRIAÇÃO
 * ============================================================ */

PlayerApp *player_app_new(
    FileList *filelist,
    size_t current_index)
{
    if (!filelist)
        return NULL;


    size_t count =
        filelist_count(
            filelist
        );


    if (count == 0)
        return NULL;


    if (current_index >= count)
        return NULL;


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

    pa->filelist =
        filelist;

    pa->current_index =
        current_index;


    fprintf(
        stderr,
        "[APP] PlayerApp criado\n"
    );


    // fprintf(
    //     stderr,
    //     "[APP] current_index = %zu\n",
    //     pa->current_index
    // );


    fprintf(
        stderr,
        "[APP] filename = %s\n",
        player_app_get_filename(pa)
    );


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


    fprintf(
        stderr,
        "[MAIN] iniciando PlayerApp\n"
    );


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


    /*
     * A FileList NÃO é liberada aqui.
     *
     * Ela pertence ao main(), que criou a lista.
     */

    pa->filelist = NULL;


    free(pa);
}
