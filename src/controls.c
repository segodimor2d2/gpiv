#include "controls.h"
#include "player.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/stat.h>
#include <errno.h>


/* ============================================================
 * CONFIGURAÇÃO
 * ============================================================ */

#define FILE_JUMP 20
#define SEEK_SECONDS 3
#define SEEK_HARD_SECONDS 10

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
     * Lista de tags.
     *
     * Cada arquivo pode ter somente uma tag.
     */

    FileTag leadtvar[MAX_TAGS];

    int leadt_count;

    /*
     * --------------------------------------------------------
     * DIRETÓRIOS DE TAGS
     * --------------------------------------------------------
     */

    char dirtags[MAX_DIR_TAGS][MAX_TAG_LENGTH];

    int dirtags_count;


    /*
     * Pasta atualmente carregada.
     *
     * Usada para saber qual tags.csv deve ser utilizado.
     */

    char tags_directory[PATH_MAX];

    char tags_file[PATH_MAX];

} Controls;


/* ============================================================
 * PROTÓTIPOS DIRTAGS
 * ============================================================ */

static void dirtags_clear(
    Controls *controls
);

static void dirtags_add_unique(
    Controls *controls,
    const char *tag
);

static void dirtags_build(
    Controls *controls
);

static void dirtags_create_directories(
    Controls *controls
);

static void show_message(
    Controls *controls,
    const char *message
);

/* ============================================================
 * RETORNA DIRETÓRIO DO ARQUIVO
 * ============================================================ */

static int get_file_directory(
    const char *filename,
    char *directory,
    size_t directory_size)
{
    if (!filename ||
        !directory ||
        directory_size == 0)
        return -1;


    const char *slash =
        strrchr(
            filename,
            '/'
        );


    if (!slash) {

        snprintf(
            directory,
            directory_size,
            "."
        );

        return 0;
    }


    size_t length =
        (size_t)(slash - filename);


    if (length == 0) {

        if (directory_size < 2)
            return -1;


        snprintf(
            directory,
            directory_size,
            "/"
        );

        return 0;
    }


    if (length >= directory_size)
        return -1;


    memcpy(
        directory,
        filename,
        length
    );


    directory[length] =
        '\0';


    return 0;
}


/* ============================================================
 * CRIA TAGS.CSV SE NÃO EXISTIR
 * ============================================================ */

static void create_tags_file(
    Controls *controls)
{
    if (!controls ||
        !controls->tags_file[0])
        return;


    FILE *file =
        fopen(
            controls->tags_file,
            "a"
        );


    if (!file) {

        fprintf(
            stderr,
            "[TAGS] não foi possível criar: %s\n",
            controls->tags_file
        );

        return;
    }


    fclose(file);


    fprintf(
        stderr,
        "[TAGS] arquivo disponível: %s\n",
        controls->tags_file
    );
}


/* ============================================================
 * LIMPA LISTA DE TAGS
 * ============================================================ */

static void tags_clear(
    Controls *controls)
{
    if (!controls)
        return;


    controls->leadt_count =
        0;


    memset(
        controls->leadtvar,
        0,
        sizeof(controls->leadtvar)
    );
}


/* ============================================================
 * CRIA DIRS
 * ============================================================ */

static void dirtags_create_directories(
    Controls *controls)
{
    if (!controls ||
        !controls->tags_directory[0])
        return;


    char message[4096];

    message[0] = '\0';


    for (int i = 0;
         i < controls->dirtags_count;
         i++) {

        char directory[PATH_MAX];


        int written =
            snprintf(
                directory,
                sizeof(directory),
                "%s/%s",
                controls->tags_directory,
                controls->dirtags[i]
            );


        if (written < 0 ||
            (size_t)written >= sizeof(directory)) {

            fprintf(
                stderr,
                "[DIRTAGS] ERRO: caminho muito longo: %s\n",
                controls->dirtags[i]
            );

            show_message(
                controls,
                "Erro: caminho da pasta muito longo"
            );

            return;
        }


        /*
         * 0 = erro
         * 1 = criado
         * 2 = já existia e é diretório
         */

        int directory_status = 0;


        /*
         * ----------------------------------------------------
         * TENTA CRIAR
         * ----------------------------------------------------
         */

        if (mkdir(directory, 0755) == 0) {

            directory_status = 1;


            fprintf(
                stderr,
                "[DIRTAGS] criado: %s\n",
                directory
            );
        }

        /*
         * ----------------------------------------------------
         * JÁ EXISTE
         * ----------------------------------------------------
         */

        else if (errno == EEXIST) {

            struct stat st;


            if (stat(directory, &st) != 0) {

                directory_status = 0;


                fprintf(
                    stderr,
                    "[DIRTAGS] ERRO: não foi possível verificar '%s': %s\n",
                    directory,
                    strerror(errno)
                );

            } else if (!S_ISDIR(st.st_mode)) {

                directory_status = 0;


                fprintf(
                    stderr,
                    "[DIRTAGS] ERRO: '%s' existe mas não é um diretório\n",
                    directory
                );

            } else {

                directory_status = 2;


                fprintf(
                    stderr,
                    "[DIRTAGS] já existe: %s\n",
                    directory
                );
            }
        }

        /*
         * ----------------------------------------------------
         * OUTRO ERRO
         * ----------------------------------------------------
         */

        else {

            directory_status = 0;


            fprintf(
                stderr,
                "[DIRTAGS] ERRO: mkdir('%s'): %s\n",
                directory,
                strerror(errno)
            );
        }


        /*
         * ----------------------------------------------------
         * ERRO
         * ----------------------------------------------------
         */

        if (directory_status == 0) {

            struct stat st;


            /*
             * Descobre novamente se o problema foi porque
             * existe um objeto que não é diretório.
             */

            if (stat(directory, &st) == 0 &&
                !S_ISDIR(st.st_mode)) {

                char error_message[4096];


                snprintf(
                    error_message,
                    sizeof(error_message),
                    "Erro: '%s' existe mas nao e diretorio",
                    controls->dirtags[i]
                );


                show_message(
                    controls,
                    error_message
                );

            } else {

                char error_message[4096];


                snprintf(
                    error_message,
                    sizeof(error_message),
                    "Erro ao criar pasta %s",
                    controls->dirtags[i]
                );


                show_message(
                    controls,
                    error_message
                );
            }


            /*
             * MUITO IMPORTANTE:
             *
             * Não continua.
             */

            return;
        }


        /*
         * ----------------------------------------------------
         * RESULTADO
         * ----------------------------------------------------
         */

        size_t used =
            strlen(message);


        if (used >= sizeof(message) - 1)
            break;


        if (directory_status == 1) {

            snprintf(
                message + used,
                sizeof(message) - used,
                "%sdiretorio criado: %s",
                used ? "\n" : "",
                controls->dirtags[i]
            );

        } else if (directory_status == 2) {

            snprintf(
                message + used,
                sizeof(message) - used,
                "%sdiretorio ja existe: %s",
                used ? "\n" : "",
                controls->dirtags[i]
            );
        }
    }


    /*
     * --------------------------------------------------------
     * MOSTRA RESULTADO
     * --------------------------------------------------------
     */

    if (message[0]) {

        show_message(
            controls,
            message
        );
    }
}

/* ============================================================
 * MONTA DIRTAGS COM LEADTVAR
 * ============================================================ */

static void dirtags_build(
    Controls *controls)
{
    if (!controls)
        return;


    /*
     * Começa sempre com lista vazia.
     */

    dirtags_clear(
        controls
    );


    /*
     * Percorre todas as tags carregadas.
     */

    for (int i = 0;
         i < controls->leadt_count;
         i++) {

        const char *tag =
            controls->leadtvar[i].tag;


        if (!tag ||
            !tag[0])
            continue;


        dirtags_add_unique(
            controls,
            tag
        );
    }


    fprintf(
        stderr,
        "[DIRTAGS] tags únicas: %d\n",
        controls->dirtags_count
    );


    /*
     * Cria os diretórios.
     */

    dirtags_create_directories(
        controls
    );
}

/* ============================================================
 * PROCURA TAG DE UM ARQUIVO
 * ============================================================ */


static int tags_find(
    Controls *controls,
    const char *path)
{
    if (!controls ||
        !path)
        return -1;


    for (int i = 0;
         i < controls->leadt_count;
         i++) {

        if (strcmp(
                controls->leadtvar[i].path,
                path
            ) == 0) {

            return i;
        }
    }


    return -1;
}


/* ============================================================
 * RETORNA TAG DO ARQUIVO
 * ============================================================ */

static const char *tags_get(
    Controls *controls,
    const char *path)
{
    if (!controls ||
        !path)
        return NULL;


    int index =
        tags_find(
            controls,
            path
        );


    if (index < 0)
        return NULL;


    return controls->leadtvar[index].tag;
}


/* ============================================================
 * REMOVE QUEBRA DE LINHA
 * ============================================================ */

static void remove_newline(
    char *text)
{
    if (!text)
        return;


    size_t length =
        strlen(text);


    while (length > 0 &&
           (text[length - 1] == '\n' ||
            text[length - 1] == '\r')) {

        text[length - 1] =
            '\0';

        length--;
    }
}


/* ============================================================
 * LIMPAR DIRTAGS
 * ============================================================ */

static void dirtags_clear(
    Controls *controls)
{
    if (!controls)
        return;

    controls->dirtags_count = 0;

    memset(
        controls->dirtags,
        0,
        sizeof(controls->dirtags)
    );
}

/* ============================================================
 * ADICIONAR TAGS UNICAS
 * ============================================================ */

static void dirtags_add_unique(
    Controls *controls,
    const char *tag)
{
    if (!controls ||
        !tag ||
        !tag[0])
        return;


    /*
     * Verifica se a tag já existe.
     */

    for (int i = 0;
         i < controls->dirtags_count;
         i++) {

        if (strcmp(
                controls->dirtags[i],
                tag
            ) == 0) {

            /*
             * Já existe.
             *
             * Não adiciona novamente.
             */

            return;
        }
    }


    /*
     * Verifica limite.
     */

    if (controls->dirtags_count >= MAX_DIR_TAGS) {

        fprintf(
            stderr,
            "[DIRTAGS] limite atingido\n"
        );

        return;
    }


    /*
     * Adiciona nova tag.
     */

    snprintf(
        controls->dirtags[
            controls->dirtags_count
        ],
        MAX_TAG_LENGTH,
        "%s",
        tag
    );


    controls->dirtags_count++;


    fprintf(
        stderr,
        "[DIRTAGS] adicionada: %s\n",
        tag
    );
}


/* ============================================================
 * CARREGA TAGS.CSV
 * ============================================================ */

static void tags_load(
    Controls *controls,
    const char *directory)
{
    if (!controls ||
        !directory)
        return;


    tags_clear(
        controls
    );


    snprintf(
        controls->tags_directory,
        sizeof(controls->tags_directory),
        "%s",
        directory
    );


    if (strlen(directory) + strlen("/tags.csv") >=
        sizeof(controls->tags_file)) {

        fprintf(
            stderr,
            "[TAGS] caminho para tags.csv muito longo\n"
        );

        return;
    }

    snprintf(
        controls->tags_file,
        sizeof(controls->tags_file),
        "%s/tags.csv",
        directory
    );


    /*
     * Se não existir, cria.
     */

    create_tags_file(
        controls
    );


    FILE *file =
        fopen(
            controls->tags_file,
            "r"
        );


    if (!file) {

        fprintf(
            stderr,
            "[TAGS] não foi possível abrir: %s\n",
            controls->tags_file
        );

        return;
    }


    char line[PATH_MAX + MAX_TAG_LENGTH + 64];


    while (fgets(
        line,
        sizeof(line),
        file)) {

        remove_newline(
            line
        );


        if (!line[0])
            continue;


        /*
         * Formato:
         *
         * /path/video.mp4,ta
         */

        char *comma =
            strrchr(
                line,
                ','
            );


        if (!comma)
            continue;


        *comma =
            '\0';


        char *path =
            line;


        char *tag =
            comma + 1;


        while (*tag == ' ')
            tag++;


        if (!path[0] ||
            !tag[0])
            continue;


        if (controls->leadt_count >= MAX_TAGS) {

            fprintf(
                stderr,
                "[TAGS] limite de tags atingido\n"
            );

            break;
        }

        strncpy(
            controls->leadtvar[controls->leadt_count].path,
            path,
            PATH_MAX - 1
        );

        controls->leadtvar[controls->leadt_count].path[PATH_MAX - 1] = '\0';

        snprintf(
            controls->leadtvar[
                controls->leadt_count
            ].tag,
            MAX_TAG_LENGTH,
            "%s",
            tag
        );


        controls->leadt_count++;
    }


    fclose(file);


    fprintf(
        stderr,
        "[TAGS] carregadas: %d\n",
        controls->leadt_count
    );
}


/* ============================================================
 * SALVA TAGS.CSV
 * ============================================================ */

static void tags_save(
    Controls *controls)
{
    if (!controls ||
        !controls->tags_file[0])
        return;


    FILE *file =
        fopen(
            controls->tags_file,
            "w"
        );


    if (!file) {

        fprintf(
            stderr,
            "[TAGS] erro salvando: %s\n",
            controls->tags_file
        );

        return;
    }


    for (int i = 0;
         i < controls->leadt_count;
         i++) {

        fprintf(
            file,
            "%s,%s\n",
            controls->leadtvar[i].path,
            controls->leadtvar[i].tag
        );
    }


    fclose(file);


    fprintf(
        stderr,
        "[TAGS] salvo: %s (%d tags)\n",
        controls->tags_file,
        controls->leadt_count
    );
}


/* ============================================================
 * TROCA tags.csv CONFORME O ARQUIVO ATUAL
 * ============================================================ */

static void tags_update_directory(
    Controls *controls,
    const char *filename)
{
    if (!controls ||
        !filename)
        return;


    char directory[PATH_MAX];


    if (get_file_directory(
            filename,
            directory,
            sizeof(directory)
        ) < 0)
        return;


    /*
     * Se continuamos na mesma pasta,
     * não precisamos recarregar.
     */

    if (strcmp(
            controls->tags_directory,
            directory
        ) == 0) {

        return;
    }


    fprintf(
        stderr,
        "[TAGS] mudando pasta:\n"
        "       %s\n",
        directory
    );


    tags_load(
        controls,
        directory
    );
}


/* ============================================================
 * ADICIONA / ATUALIZA TAG
 * ============================================================ */

static void tag_file(
    Controls *controls,
    const char *path,
    const char *tag)
{
    if (!controls ||
        !path ||
        !tag ||
        !tag[0])
        return;


    int index =
        tags_find(
            controls,
            path
        );


    /*
     * Arquivo já possui tag.
     *
     * Substituímos.
     */

    if (index >= 0) {

        snprintf(
            controls->leadtvar[index].tag,
            MAX_TAG_LENGTH,
            "%s",
            tag
        );


        fprintf(
            stderr,
            "[TAGS] atualizado:\n"
            "       %s -> %s\n",
            path,
            tag
        );

    } else {

        /*
         * Nova tag.
         */

        if (controls->leadt_count >= MAX_TAGS) {

            fprintf(
                stderr,
                "[TAGS] limite máximo atingido\n"
            );

            return;
        }


        snprintf(
            controls->leadtvar[
                controls->leadt_count
            ].path,
            PATH_MAX,
            "%s",
            path
        );


        snprintf(
            controls->leadtvar[
                controls->leadt_count
            ].tag,
            MAX_TAG_LENGTH,
            "%s",
            tag
        );


        controls->leadt_count++;


        fprintf(
            stderr,
            "[TAGS] nova:\n"
            "       %s -> %s\n",
            path,
            tag
        );
    }


    /*
     * Salva imediatamente.
     */

    tags_save(
        controls
    );

}


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


    const char *filename =
        player_get_filename(
            player
        );


    if (!filename)
        filename = "";


    /*
     * --------------------------------------------------------
     * VERIFICA TAGS.CSV DA PASTA ATUAL
     * --------------------------------------------------------
     */

    tags_update_directory(
        controls,
        filename
    );


    /*
     * --------------------------------------------------------
     * TEMPO
     * --------------------------------------------------------
     */

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


    /*
     * --------------------------------------------------------
     * LEADER F
     * --------------------------------------------------------
     */

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


        } else if (controls->leadfvar == 'g') {

            snprintf(
                leader,
                sizeof(leader),
                "fg"
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


    /*
     * --------------------------------------------------------
     * LEADER T
     * --------------------------------------------------------
     *
     * O leader T é apenas um estado temporário:
     *
     *     t -> ativa
     *     t + letra -> salva tag e desativa
     *
     * Não usamos leadtvar[0] aqui.
     *
     * leadtvar[] contém a lista de arquivos tagueados,
     * e a posição [0] não representa necessariamente
     * o arquivo atual.
     */

    char tag_leader[64];

    tag_leader[0] = '\0';


    if (controls->leadt) {

        snprintf(
            tag_leader,
            sizeof(tag_leader),
            "t"
        );
    }


    /*
     * --------------------------------------------------------
     * TAG DO ARQUIVO
     * --------------------------------------------------------
     */

    const char *tag =
        tags_get(
            controls,
            filename
        );


    /*
     * --------------------------------------------------------
     * NOME + TAG
     * --------------------------------------------------------
     */

    char filename_with_tag[PATH_MAX + 64];


    if (tag) {

        snprintf(
            filename_with_tag,
            sizeof(filename_with_tag),
            "%s %s",
            filename,
            tag
        );

    } else {

        snprintf(
            filename_with_tag,
            sizeof(filename_with_tag),
            "%s",
            filename
        );
    }


    /*
     * --------------------------------------------------------
     * LEADER FINAL
     * --------------------------------------------------------
     */

    char active_leader[128];

    active_leader[0] = '\0';


    if (leader[0] &&
        tag_leader[0]) {

        snprintf(
            active_leader,
            sizeof(active_leader),
            "%s / %s",
            leader,
            tag_leader
        );

    } else if (leader[0]) {

        snprintf(
            active_leader,
            sizeof(active_leader),
            "%s",
            leader
        );

    } else if (tag_leader[0]) {

        snprintf(
            active_leader,
            sizeof(active_leader),
            "%s",
            tag_leader
        );
    }


    /*
     * --------------------------------------------------------
     * LABEL
     * --------------------------------------------------------
     */

    char text[16384];


    if (controls->message[0]) {

        if (active_leader[0]) {

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
                filename_with_tag,
                controls->message,
                active_leader
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
                filename_with_tag,
                controls->message
            );
        }

    } else {

        if (active_leader[0]) {

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
                filename_with_tag,
                active_leader
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
                filename_with_tag
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


    /* ========================================================
     * ESC
     * ======================================================== */

    if (keyval == GDK_KEY_Escape) {

        controls->leadf = FALSE;
        controls->leadfvar = '\0';

        controls->leadt = FALSE;

        fprintf(
            stderr,
            "[LEADER] OFF\n"
        );

        return TRUE;
    }


    /* ========================================================
     * F -> LEADER F
     * ======================================================== */

    if (keyval == GDK_KEY_f) {

        controls->leadf = TRUE;
        controls->leadfvar = '\0';

        fprintf(
            stderr,
            "[LEADER] F ON\n"
        );

        return TRUE;
    }


    /* ========================================================
     * T -> LEADER T
     * ======================================================== */

    if (keyval == GDK_KEY_t) {

        controls->leadt = TRUE;

        fprintf(
            stderr,
            "[LEADER] T ON\n"
        );

        return TRUE;
    }


    /* ========================================================
     * LEADER T
     *
     * ta
     * tb
     * tc
     * ...
     *
     * Qualquer letra cria/substitui a tag.
     * ======================================================== */

    if (controls->leadt) {

        /*
         * Somente letras.
         */

        if ((keyval >= GDK_KEY_a &&
             keyval <= GDK_KEY_z) ||
            (keyval >= GDK_KEY_A &&
             keyval <= GDK_KEY_Z)) {

            char tag_letter;


            /*
             * Converte maiúscula para minúscula.
             */

            if (keyval >= GDK_KEY_A &&
                keyval <= GDK_KEY_Z) {

                tag_letter =
                    (char)('a' +
                    (keyval - GDK_KEY_A));

            } else {

                tag_letter =
                    (char)keyval;
            }


            const char *filename =
                player_get_filename(
                    player
                );


            if (filename) {

                char tag[4];


                /*
                 * Monta:
                 *
                 * ta
                 * tb
                 * tc
                 * ...
                 */

                snprintf(
                    tag,
                    sizeof(tag),
                    "t%c",
                    tag_letter
                );


                /*
                 * Salva a tag no tags.csv.
                 */

                tag_file(
                    controls,
                    filename,
                    tag
                );

            }


            /*
             * ------------------------------------------------
             * TERMINA O LEADER T
             * ------------------------------------------------
             *
             * Depois de:
             *
             * t + letra
             *
             * a operação terminou.
             *
             * Não é mais necessário pressionar ESC.
             */

            controls->leadt = FALSE;


            fprintf(
                stderr,
                "[LEADER] T OFF\n"
            );


            return TRUE;
        }
    }


    /* ========================================================
     * LEADER F
     * ======================================================== */

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
         * G -> GAMMA
         *
         * fg
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_g ||
            keyval == GDK_KEY_G) {

            controls->leadfvar = 'g';

            fprintf(
                stderr,
                "[LEADER] fg\n"
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
         * r -> CRIAR DIRETÓRIOS DAS TAGS
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_r) {

            fprintf(
                stderr,
                "[LEADER] fr\n"
            );


            dirtags_build(
                controls
            );


            controls->leadfvar = 'r';


            return TRUE;
        }

        /* ----------------------------------------------------
         * R -> TESTAR DIRETÓRIOS DAS TAGS
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_R) {

            fprintf(
                stderr,
                "[LEADER] fR\n"
            );


            /*
             * Primeiro monta a lista de tags únicas.
             */

            dirtags_clear(
                controls
            );


            for (int i = 0;
                 i < controls->leadt_count;
                 i++) {

                const char *tag =
                    controls->leadtvar[i].tag;


                if (!tag ||
                    !tag[0])
                    continue;


                dirtags_add_unique(
                    controls,
                    tag
                );
            }


            /*
             * Verifica todas as pastas.
             */

            gboolean all_valid = TRUE;


            for (int i = 0;
                 i < controls->dirtags_count;
                 i++) {

                char directory[PATH_MAX];


                int written =
                    snprintf(
                        directory,
                        sizeof(directory),
                        "%s/%s",
                        controls->tags_directory,
                        controls->dirtags[i]
                    );


                if (written < 0 ||
                    (size_t)written >= sizeof(directory)) {

                    char message[4096];


                    snprintf(
                        message,
                        sizeof(message),
                        "Erro: caminho muito longo para %s",
                        controls->dirtags[i]
                    );


                    show_message(
                        controls,
                        message
                    );


                    all_valid = FALSE;

                    break;
                }


                struct stat st;


                if (stat(directory, &st) != 0) {

                    fprintf(
                        stderr,
                        "[fR] ERRO: não foi possível verificar '%s': %s\n",
                        directory,
                        strerror(errno)
                    );


                    char message[4096];


                    snprintf(
                        message,
                        sizeof(message),
                        "Erro: pasta nao existe: %s",
                        controls->dirtags[i]
                    );


                    show_message(
                        controls,
                        message
                    );


                    all_valid = FALSE;

                    break;
                }


                if (!S_ISDIR(st.st_mode)) {

                    fprintf(
                        stderr,
                        "[fR] ERRO: '%s' existe mas não é um diretório\n",
                        directory
                    );


                    char message[4096];


                    snprintf(
                        message,
                        sizeof(message),
                        "Erro: '%s' existe mas nao e diretorio",
                        controls->dirtags[i]
                    );


                    show_message(
                        controls,
                        message
                    );


                    all_valid = FALSE;

                    break;
                }


                fprintf(
                    stderr,
                    "[fR] OK: %s é diretório\n",
                    directory
                );
            }


            /*
             * ----------------------------------------------------
             * RESULTADO DO TESTE
             * ----------------------------------------------------
             */

            if (all_valid) {

                show_message(
                    controls,
                    "fR: todas as pastas de tags sao validas"
                );


                fprintf(
                    stderr,
                    "[fR] todas as pastas são válidas\n"
                );
            }


            /*
             * fR terminou.
             */

            controls->leadf = FALSE;
            controls->leadfvar = '\0';


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
                        1
                    );

                } else if (controls->leadfvar == 'g') {

                    player_change_gamma(
                        player,
                        1
                    );

                } else if (controls->leadfvar == 'z') {

                    player_change_zoom(
                        player,
                        1
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

                } else if (controls->leadfvar == 'g') {

                    player_change_gamma(
                        player,
                        -1
                    );

                } else if (controls->leadfvar == 'z') {

                    player_change_zoom(
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
     * TECLADO NORMAL
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
     * l -> VOLTA 20 ARQUIVOS
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_l) {

        jump_backward(
            controls
        );

        return TRUE;
    }


    /* --------------------------------------------------------
     * h -> AVANÇA 20 ARQUIVOS
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_h) {

        jump_forward(
            controls
        );

        return TRUE;
    }


    /* --------------------------------------------------------
     * SPACE / ENTER
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

    if (keyval == GDK_KEY_n ||
        keyval == GDK_KEY_N) {

        if (player) {

            player_frame_forward(
                player
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * M -> SEEK +10
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
     * < -> SEEK -10
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
     * m -> SEEK +3
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
     * , -> SEEK -3
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
     * X -> VOLUME 0
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
        G_CALLBACK(on_key_pressed),
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
