#include "controls.h"
#include "player.h"
#include "dirtags.h"
#include "control_tags.h"
#include "control_video.h"
#include "control_keys.h"
#include "control_ui.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/stat.h>
#include <errno.h>

#include <sys/wait.h>
#include <unistd.h>

/* ============================================================
 * CONFIGURAÇÃO
 * ============================================================ */

#define FILE_JUMP 20

/* ============================================================
 * PROTÓTIPOS DIRTAGS
 * ============================================================ */

gboolean dirtags_prevalidate(
    Controls *controls
);

gboolean dirtags_move_files(
    Controls *controls
);

/* ============================================================
 * PRÉ-VALIDAÇÃO PARA fR
 *
 * NÃO move nenhum arquivo.
 *
 * Verifica antecipadamente:
 *
 *   - origem existe
 *   - origem é arquivo regular
 *   - tag existe
 *   - diretório da tag existe
 *   - diretório da tag é realmente diretório
 *   - destino pode ser construído
 *   - nomes _bis_N podem ser calculados
 *   - caminhos não ultrapassam PATH_MAX
 *
 * Retorno:
 *
 *   TRUE  = tudo válido
 *   FALSE = existe algum problema
 * ============================================================ */

gboolean dirtags_prevalidate(
    Controls *controls)
{
    if (!controls)
        return FALSE;


    /*
     * --------------------------------------------------------
     * PRECISAMOS TER TAGS
     * --------------------------------------------------------
     */

    if (controls->leadt_count <= 0) {

        show_message(
            controls,
            "Erro: nenhuma tag encontrada"
        );

        fprintf(
            stderr,
            "[fR] ERRO: nenhuma tag encontrada\n"
        );

        return FALSE;
    }


    /*
     * --------------------------------------------------------
     * VERIFICA CADA ARQUIVO
     * --------------------------------------------------------
     */

    for (int i = 0;
         i < controls->leadt_count;
         i++) {

        const char *path =
            controls->leadtvar[i].path;

        const char *tag =
            controls->leadtvar[i].tag;


        /*
         * ----------------------------------------------------
         * PATH
         * ----------------------------------------------------
         */

        if (!path ||
            !path[0]) {

            fprintf(
                stderr,
                "[fR] ERRO: entrada %d possui path vazio\n",
                i
            );


            show_message(
                controls,
                "Erro: arquivo possui path vazio"
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * TAG
         * ----------------------------------------------------
         */

        if (!tag ||
            !tag[0]) {

            fprintf(
                stderr,
                "[fR] ERRO: '%s' possui tag vazia\n",
                path
            );


            show_message(
                controls,
                "Erro: arquivo possui tag vazia"
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * VERIFICA ORIGEM
         * ----------------------------------------------------
         */

        struct stat source_st;


        if (stat(path, &source_st) != 0) {

            fprintf(
                stderr,
                "[fR] ERRO: origem não existe: '%s': %s\n",
                path,
                strerror(errno)
            );


            char message[4096];


            snprintf(
                message,
                sizeof(message),
                "Erro: arquivo nao existe: %.*s",
                (int)(sizeof(message) - 27),
                path
            );


            show_message(
                controls,
                message
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * NÃO PODE SER DIRETÓRIO
         * ----------------------------------------------------
         */

        if (!S_ISREG(source_st.st_mode)) {

            fprintf(
                stderr,
                "[fR] ERRO: origem não é arquivo regular: '%s'\n",
                path
            );


            char message[4096];


            snprintf(
                message,
                sizeof(message),
                "Erro: origem nao e arquivo regular: %.*s",
                (int)(sizeof(message) - 37),
                path
            );


            show_message(
                controls,
                message
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * MONTA DIRETÓRIO DA TAG
         * ----------------------------------------------------
         */

        char directory[PATH_MAX];


        int written =
            snprintf(
                directory,
                sizeof(directory),
                "%s/%s",
                controls->tags_directory,
                tag
            );


        if (written < 0 ||
            (size_t)written >= sizeof(directory)) {

            fprintf(
                stderr,
                "[fR] ERRO: caminho do diretório muito longo: %s\n",
                tag
            );


            show_message(
                controls,
                "Erro: caminho do destino muito longo"
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * VERIFICA DIRETÓRIO
         * ----------------------------------------------------
         */

        struct stat directory_st;


        if (stat(directory, &directory_st) != 0) {

            fprintf(
                stderr,
                "[fR] ERRO: diretório da tag não existe: '%s': %s\n",
                directory,
                strerror(errno)
            );


            char message[4096];


            snprintf(
                message,
                sizeof(message),
                "Erro: pasta da tag nao existe: %s",
                tag
            );


            show_message(
                controls,
                message
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * GARANTE S_ISDIR
         * ----------------------------------------------------
         */

        if (!S_ISDIR(directory_st.st_mode)) {

            fprintf(
                stderr,
                "[fR] ERRO: '%s' existe mas não é diretório\n",
                directory
            );


            char message[4096];


            snprintf(
                message,
                sizeof(message),
                "Erro: '%s' existe mas nao e diretorio",
                tag
            );


            show_message(
                controls,
                message
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * PEGA NOME DO ARQUIVO
         * ----------------------------------------------------
         */

        const char *filename =
            strrchr(path, '/');


        if (filename) {

            filename++;

        } else {

            filename = path;
        }


        if (!filename[0]) {

            fprintf(
                stderr,
                "[fR] ERRO: nome de arquivo vazio: '%s'\n",
                path
            );


            show_message(
                controls,
                "Erro: nome de arquivo invalido"
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * MONTA DESTINO ORIGINAL
         * ----------------------------------------------------
         */

        char destination[PATH_MAX];


        written =
            snprintf(
                destination,
                sizeof(destination),
                "%s/%s",
                directory,
                filename
            );


        if (written < 0 ||
            (size_t)written >= sizeof(destination)) {

            fprintf(
                stderr,
                "[fR] ERRO: destino muito longo:\n"
                "      %s/%s\n",
                directory,
                filename
            );


            show_message(
                controls,
                "Erro: caminho do destino muito longo"
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * DESTINO JÁ EXISTE?
         * ----------------------------------------------------
         *
         * Se existir, ainda não é erro.
         *
         * Vamos calcular:
         *
         * arquivo.mp4
         * arquivo_bis_1.mp4
         * arquivo_bis_2.mp4
         * ...
         */

        struct stat destination_st;


        if (stat(destination, &destination_st) == 0) {

            fprintf(
                stderr,
                "[fR] destino já existe:\n"
                "      %s\n",
                destination
            );


            /*
             * ------------------------------------------------
             * SEPARA NOME E EXTENSÃO
             * ------------------------------------------------
             */

            char base[PATH_MAX];


            written =
                snprintf(
                    base,
                    sizeof(base),
                    "%s",
                    filename
                );


            if (written < 0 ||
                (size_t)written >= sizeof(base)) {

                show_message(
                    controls,
                    "Erro: nome de arquivo muito longo"
                );


                return FALSE;
            }


            char *dot =
                strrchr(
                    base,
                    '.'
                );


            char extension[PATH_MAX];

            extension[0] = '\0';


            if (dot &&
                dot != base) {

                snprintf(
                    extension,
                    sizeof(extension),
                    "%s",
                    dot
                );


                *dot = '\0';
            }


            /*
             * ------------------------------------------------
             * PROCURA _bis_N
             * ------------------------------------------------
             */

            gboolean found_free_name =
                FALSE;


            for (int n = 1;
                 n < INT_MAX;
                 n++) {

                char candidate[PATH_MAX];


                written =
                    snprintf(
                        candidate,
                        sizeof(candidate),
                        "%s/%s_bis_%d%s",
                        directory,
                        base,
                        n,
                        extension
                    );


                if (written < 0 ||
                    (size_t)written >= sizeof(candidate)) {

                    fprintf(
                        stderr,
                        "[fR] ERRO: candidato muito longo\n"
                    );


                    show_message(
                        controls,
                        "Erro: nome alternativo muito longo"
                    );


                    return FALSE;
                }


                if (stat(candidate, &destination_st) != 0) {

                    if (errno == ENOENT) {

                        fprintf(
                            stderr,
                            "[fR] destino disponível:\n"
                            "      %s\n",
                            candidate
                        );


                        found_free_name =
                            TRUE;


                        break;
                    }


                    /*
                     * Outro erro ao verificar o destino.
                     */

                    fprintf(
                        stderr,
                        "[fR] ERRO verificando '%s': %s\n",
                        candidate,
                        strerror(errno)
                    );


                    show_message(
                        controls,
                        "Erro verificando destino"
                    );


                    return FALSE;
                }
            }


            if (!found_free_name) {

                fprintf(
                    stderr,
                    "[fR] ERRO: não foi possível encontrar "
                    "nome disponível para '%s'\n",
                    filename
                );


                show_message(
                    controls,
                    "Erro: nao foi possivel criar nome _bis_N"
                );


                return FALSE;
            }
        }


        /*
         * ----------------------------------------------------
         * ARQUIVO VALIDADO
         * ----------------------------------------------------
         */

        fprintf(
            stderr,
            "[fR] OK:\n"
            "      origem: %s\n"
            "      tag:    %s\n"
            "      pasta:  %s\n",
            path,
            tag,
            directory
        );
    }


    /*
     * --------------------------------------------------------
     * TUDO OK
     * --------------------------------------------------------
     */

    fprintf(
        stderr,
        "[fR] PRÉ-VALIDAÇÃO OK\n"
    );


    show_message(
        controls,
        "fR: pre-validacao OK"
    );


    return TRUE;
}


/* ============================================================
 * MOVE ARQUIVOS PARA OS DIRETÓRIOS DAS TAGS
 *
 * A pré-validação deve ter sido executada antes.
 *
 * Exemplo:
 *
 *   /videos/a.mp4,ta
 *
 * vira:
 *
 *   /videos/ta/a.mp4
 *
 * Se o destino já existir:
 *
 *   a.mp4
 *   a_bis_1.mp4
 *   a_bis_2.mp4
 *   ...
 *
 * Retorno:
 *
 *   TRUE  = todos os arquivos foram movidos
 *   FALSE = ocorreu algum erro
 * ============================================================ */

gboolean dirtags_move_files(
    Controls *controls)
{
    if (!controls)
        return FALSE;


    if (controls->leadt_count <= 0) {

        show_message(
            controls,
            "Erro: nenhuma tag encontrada"
        );

        return FALSE;
    }


    fprintf(
        stderr,
        "[fR] iniciando movimentação dos arquivos\n"
    );


    int moved_count = 0;


    /*
     * --------------------------------------------------------
     * MOVE CADA ARQUIVO
     * --------------------------------------------------------
     */

    for (int i = 0;
         i < controls->leadt_count;
         i++) {

        const char *source =
            controls->leadtvar[i].path;

        const char *tag =
            controls->leadtvar[i].tag;


        if (!source ||
            !source[0] ||
            !tag ||
            !tag[0]) {

            fprintf(
                stderr,
                "[fR] ERRO: entrada inválida %d\n",
                i
            );

            show_message(
                controls,
                "Erro: entrada de tag invalida"
            );

            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * DIRETÓRIO DA TAG
         * ----------------------------------------------------
         */

        char directory[PATH_MAX];


        int written =
            snprintf(
                directory,
                sizeof(directory),
                "%s/%s",
                controls->tags_directory,
                tag
            );


        if (written < 0 ||
            (size_t)written >= sizeof(directory)) {

            fprintf(
                stderr,
                "[fR] ERRO: diretório muito longo: %s\n",
                tag
            );


            show_message(
                controls,
                "Erro: caminho do destino muito longo"
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * NOME DO ARQUIVO
         * ----------------------------------------------------
         */

        const char *filename =
            strrchr(
                source,
                '/'
            );


        if (filename)
            filename++;
        else
            filename = source;


        if (!filename[0]) {

            fprintf(
                stderr,
                "[fR] ERRO: nome de arquivo vazio\n"
            );


            show_message(
                controls,
                "Erro: nome de arquivo invalido"
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * DESTINO NORMAL
         * ----------------------------------------------------
         */

        char destination[PATH_MAX];


        written =
            snprintf(
                destination,
                sizeof(destination),
                "%s/%s",
                directory,
                filename
            );


        if (written < 0 ||
            (size_t)written >= sizeof(destination)) {

            fprintf(
                stderr,
                "[fR] ERRO: destino muito longo:\n"
                "      %s/%s\n",
                directory,
                filename
            );


            show_message(
                controls,
                "Erro: caminho do destino muito longo"
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * SE DESTINO EXISTIR, PROCURA _bis_N
         * ----------------------------------------------------
         */

        struct stat st;


        if (stat(destination, &st) == 0) {

            char base[PATH_MAX];


            written =
                snprintf(
                    base,
                    sizeof(base),
                    "%s",
                    filename
                );


            if (written < 0 ||
                (size_t)written >= sizeof(base)) {

                show_message(
                    controls,
                    "Erro: nome de arquivo muito longo"
                );

                return FALSE;
            }


            /*
             * Separa extensão.
             *
             * exemplo:
             *
             * video.mp4
             *
             * base      = video
             * extension = .mp4
             */

            char extension[PATH_MAX];

            extension[0] = '\0';


            char *dot =
                strrchr(
                    base,
                    '.'
                );


            if (dot &&
                dot != base) {

                snprintf(
                    extension,
                    sizeof(extension),
                    "%s",
                    dot
                );

                *dot = '\0';
            }


            /*
             * Procura:
             *
             * video_bis_1.mp4
             * video_bis_2.mp4
             * ...
             */

            gboolean found =
                FALSE;


            for (int n = 1;
                 n < INT_MAX;
                 n++) {

                written =
                    snprintf(
                        destination,
                        sizeof(destination),
                        "%s/%s_bis_%d%s",
                        directory,
                        base,
                        n,
                        extension
                    );


                if (written < 0 ||
                    (size_t)written >= sizeof(destination)) {

                    fprintf(
                        stderr,
                        "[fR] ERRO: nome alternativo muito longo\n"
                    );


                    show_message(
                        controls,
                        "Erro: nome alternativo muito longo"
                    );


                    return FALSE;
                }


                if (stat(destination, &st) != 0) {

                    if (errno == ENOENT) {

                        found =
                            TRUE;

                        break;
                    }


                    fprintf(
                        stderr,
                        "[fR] ERRO verificando destino '%s': %s\n",
                        destination,
                        strerror(errno)
                    );


                    show_message(
                        controls,
                        "Erro verificando destino"
                    );


                    return FALSE;
                }
            }


            if (!found) {

                fprintf(
                    stderr,
                    "[fR] ERRO: não encontrou nome disponível\n"
                );


                show_message(
                    controls,
                    "Erro: nao foi possivel encontrar nome livre"
                );


                return FALSE;
            }
        }


        /*
         * ----------------------------------------------------
         * MOVE
         * ----------------------------------------------------
         */

        fprintf(
            stderr,
            "[fR] movendo:\n"
            "      origem:  %s\n"
            "      destino: %s\n",
            source,
            destination
        );


        if (rename(
                source,
                destination
            ) != 0) {

            fprintf(
                stderr,
                "[fR] ERRO ao mover:\n"
                "      %s\n"
                "      -> %s\n"
                "      %s\n",
                source,
                destination,
                strerror(errno)
            );


            char message[4096];

            snprintf(
                message,
                sizeof(message),
                "Erro ao mover: %.*s",
                (int)(sizeof(message) - 16),
                filename
            );

            show_message(
                controls,
                message
            );


            return FALSE;
        }


        /*
         * ----------------------------------------------------
         * ATUALIZA PATH NA MEMÓRIA
         * ----------------------------------------------------
         *
         * Isso é importante.
         *
         * O tags.csv ainda precisa representar o novo
         * caminho do arquivo.
         */

        snprintf(
            controls->leadtvar[i].path,
            PATH_MAX,
            "%s",
            destination
        );


        moved_count++;


        fprintf(
            stderr,
            "[fR] OK: %s\n",
            destination
        );
    }


    /*
     * --------------------------------------------------------
     * SALVA tags.csv COM OS NOVOS PATHS
     * --------------------------------------------------------
     */

    tags_save(
        controls
    );


    /*
     * --------------------------------------------------------
     * ARQUIVA tags.csv
     * --------------------------------------------------------
     *
     * Só chegamos aqui se TODOS os arquivos foram
     * movidos com sucesso.
     */

    if (!tags_archive(
            controls
        )) {

        fprintf(
            stderr,
            "[fR] arquivos foram movidos, "
            "mas não foi possível arquivar tags.csv\n"
        );

        return FALSE;
    }


    /*
     * --------------------------------------------------------
     * RESULTADO
     * --------------------------------------------------------
     */

    char message[4096];


    snprintf(
        message,
        sizeof(message),
        "fR concluido: %d arquivo%s movido%s",
        moved_count,
        moved_count == 1 ? "" : "s",
        moved_count == 1 ? "" : "s"
    );


    show_message(
        controls,
        message
    );


    fprintf(
        stderr,
        "[fR] movimentação concluída: %d arquivo(s)\n",
        moved_count
    );

    return TRUE;

}





/* ============================================================
 * CLIPBOARD
 * ============================================================ */

void copy_video_path(
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

void jump_forward(
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

void jump_backward(
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

void previous_file(
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

void next_file(
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

        player_change_saturation(
            player,
            (dy < 0.0) ? 1 : -1
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * FG -> GAMMA
     * -------------------------------------------------------- */

    if (controls->leadf &&
        controls->leadfvar == 'g') {

        /*
         * Scroll para cima:
         * gamma +
         *
         * Scroll para baixo:
         * gamma -
         */

        player_change_gamma(
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

        player_change_volume(
            player,
            (dy < 0.0) ? 5 : -5
        );

        return TRUE;
    }


    /*
     * Leader T não usa scroll.
     *
     * t + letra = tag
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


    controls->window =
        window;

    controls->gl_area =
        gl_area;

    controls->info_label =
        info_label;

    controls->app =
        app;


    /*
     * Leader F.
     */

    controls->leadf =
        FALSE;

    controls->leadfvar =
        '\0';


    /*
     * Leader T.
     */

    controls->leadt = FALSE;

    controls->leadt_count = 0;

    controls->dirtags_count = 0;

    controls->tags_directory[0] = '\0';

    controls->tags_file[0] = '\0';


    /*
     * --------------------------------------------------------
     * CARREGA TAGS DA PASTA DO PRIMEIRO ARQUIVO
     * --------------------------------------------------------
     */

    Player *player =
        render_get_player(
            app->render
        );


    if (player) {

        const char *filename =
            player_get_filename(
                player
            );


        if (filename) {

            char directory[PATH_MAX];


            if (get_file_directory(
                    filename,
                    directory,
                    sizeof(directory)
                ) == 0) {

                tags_load(
                    controls,
                    directory
                );
            }
        }
    }


    fprintf(
        stderr,
        "[CONTROLS] configurando controles\n"
    );


    /* ========================================================
     * DRAG
     * ======================================================== */

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


    /* ========================================================
     * SCROLL
     * ======================================================== */

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


    /* ========================================================
     * FOCUS
     * ======================================================== */

    gtk_widget_set_focusable(
        gl_area,
        TRUE
    );


    /* ========================================================
     * KEYBOARD
     * ======================================================== */

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
        G_CALLBACK(control_keys_handle),
        controls
    );


    gtk_widget_add_controller(
        window,
        GTK_EVENT_CONTROLLER(key_controller)
    );


    /* ========================================================
     * FOCO INICIAL
     * ======================================================== */

    g_idle_add(
        grab_gl_focus,
        controls
    );


    /* ========================================================
     * TIMER DO INFO LABEL
     * ======================================================== */

    controls->info_timer_id =
        control_ui_start_info_timer(
            controls
        );


    fprintf(
        stderr,
        "[CONTROLS] controles configurados\n"
    );
}
