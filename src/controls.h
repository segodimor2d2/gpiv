#ifndef CONTROLS_H
#define CONTROLS_H

#include <gtk/gtk.h>
#include <limits.h>

#include "app.h"


/* ============================================================
 * CONFIGURAÇÃO
 * ============================================================ */

#define MAX_TAGS 4096
#define MAX_TAG_LENGTH 32
#define MAX_DIR_TAGS MAX_TAGS

/* ============================================================
 * TAG
 * ============================================================ */

typedef struct {
    char path[PATH_MAX];
    char tag[MAX_TAG_LENGTH];
} FileTag;


/* ============================================================
 * CONTEXTO DE CONTROLES
 * ============================================================ */

typedef struct Controls {

    GtkWidget *window;

    GtkWidget *gl_area;

    GtkWidget *info_label;

    PlayerApp *app;

    guint info_timer_id;

    char message[4096];


    /*
     * --------------------------------------------------------
     * LEADER F
     * --------------------------------------------------------
     */

    gboolean leadf;

    char leadfvar;


    /*
     * --------------------------------------------------------
     * LEADER T
     * --------------------------------------------------------
     *
     * t -> ativa leader
     *
     * ta
     * tb
     * tc
     * ...
     */

    gboolean leadt;


    /*
     * --------------------------------------------------------
     * LISTA DE TAGS
     * --------------------------------------------------------
     */

    FileTag leadtvar[MAX_TAGS];

    int leadt_count;


    /*
     * --------------------------------------------------------
     * DIRTAGS
     * --------------------------------------------------------
     *
     * Lista de tags únicas usadas para criar
     * os diretórios.
     */

    char dirtags[MAX_DIR_TAGS][MAX_TAG_LENGTH];

    int dirtags_count;


    /*
     * --------------------------------------------------------
     * DIRETÓRIO / ARQUIVO DE TAGS
     * --------------------------------------------------------
     */

    char tags_directory[PATH_MAX];

    char tags_file[PATH_MAX];

} Controls;

typedef struct Controls {

    GtkWidget *window;

    GtkWidget *gl_area;

    GtkWidget *info_label;

    PlayerApp *app;

    guint info_timer_id;

    char message[4096];

    /*
     * --------------------------------------------------------
     * LEADER F
     * --------------------------------------------------------
     */

    gboolean leadf;

    char leadfvar;

    /*
     * --------------------------------------------------------
     * LEADER T
     * --------------------------------------------------------
     */

    gboolean leadt;

    /*
     * --------------------------------------------------------
     * LISTA DE TAGS
     * --------------------------------------------------------
     */

    FileTag leadtvar[MAX_TAGS];

    int leadt_count;

    /*
     * --------------------------------------------------------
     * DIRTAGS
     * --------------------------------------------------------
     */

    char dirtags[MAX_DIR_TAGS][MAX_TAG_LENGTH];

    int dirtags_count;

    /*
     * --------------------------------------------------------
     * DIRETÓRIO / ARQUIVO DE TAGS
     * --------------------------------------------------------
     */

    char tags_directory[PATH_MAX];

    char tags_file[PATH_MAX];

} Controls;


/* ============================================================
 * FUNÇÕES USADAS PELO TECLADO
 * ============================================================ */

gboolean dirtags_prevalidate(
    Controls *controls
);

gboolean dirtags_move_files(
    Controls *controls
);

void copy_video_path(
    Controls *controls
);

void jump_forward(
    Controls *controls
);

void jump_backward(
    Controls *controls
);

void previous_file(
    Controls *controls
);

void next_file(
    Controls *controls
);


/* ============================================================
 * MENSAGEM NO LABEL
 * ============================================================ */

void show_message(
    Controls *controls,
    const char *message
);


/* ============================================================
 * CONTROLES
 *
 * Conecta teclado, mouse, scroll e drag aos widgets.
 *
 * window:
 *     janela principal
 *
 * gl_area:
 *     GtkGLArea usado pelo Render
 *
 * info_label:
 *     label usado para mensagens
 *
 * app:
 *     PlayerApp responsável pela aplicação,
 *     FileList e navegação entre arquivos.
 * ============================================================ */

void controls_setup(
    GtkWidget *window,
    GtkWidget *gl_area,
    GtkWidget *info_label,
    PlayerApp *app
);

#endif
