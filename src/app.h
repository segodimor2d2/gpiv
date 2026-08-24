#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>

#include "render.h"
#include "filelist.h"


/* ============================================================
 * PLAYER APP
 * ============================================================ */

typedef struct _PlayerApp {

    GtkApplication *app;

    GtkWidget *window;

    GtkWidget *info_label;

    Render *render;

    /*
     * Lista completa de arquivos.
     *
     * A FileList pertence ao PlayerApp durante
     * toda a execução da aplicação.
     */
    FileList *filelist;

    /*
     * Índice do arquivo atualmente reproduzido.
     */
    size_t current_index;

} PlayerApp;


/* ============================================================
 * CRIAÇÃO
 * ============================================================ */

PlayerApp *player_app_new(
    FileList *filelist,
    size_t current_index
);


/* ============================================================
 * EXECUÇÃO
 * ============================================================ */

int player_app_run(
    PlayerApp *pa,
    int argc,
    char **argv
);


/* ============================================================
 * NAVEGAÇÃO
 * ============================================================ */

/*
 * Carrega o próximo arquivo da FileList.
 *
 * Retorna:
 *
 *     0  -> sucesso
 *    -1  -> não foi possível mudar
 */
int player_app_next(
    PlayerApp *pa
);


/*
 * Carrega o arquivo anterior da FileList.
 *
 * Retorna:
 *
 *     0  -> sucesso
 *    -1  -> não foi possível mudar
 */
int player_app_previous(
    PlayerApp *pa
);


/*
 * Retorna o arquivo atualmente selecionado.
 *
 * O ponteiro pertence à FileList.
 * Não deve ser liberado pelo chamador.
 */
const char *player_app_get_filename(
    PlayerApp *pa
);


/*
 * Retorna o índice atual.
 */
size_t player_app_get_current_index(
    PlayerApp *pa
);


/* ============================================================
 * DESTRUIÇÃO
 * ============================================================ */

void player_app_free(
    PlayerApp *pa
);

#endif
