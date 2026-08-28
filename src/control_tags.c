
#include "control_tags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include <sys/stat.h>
#include <errno.h>

#include <unistd.h>


/* ============================================================
 * AUXILIAR: RETORNA DIRETÓRIO DO ARQUIVO
 * ============================================================ */

int get_file_directory(
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

void tags_clear(
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
 * PROCURA TAG DE UM ARQUIVO
 * ============================================================ */

int tags_find(
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

const char *tags_get(
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
 * CARREGA TAGS.CSV
 * ============================================================ */

void tags_load(
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


    char line[
        PATH_MAX +
        MAX_TAG_LENGTH +
        64
    ];


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
            controls->leadtvar[
                controls->leadt_count
            ].path,
            path,
            PATH_MAX - 1
        );


        controls->leadtvar[
            controls->leadt_count
        ].path[
            PATH_MAX - 1
        ] = '\0';


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

void tags_save(
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

void tags_update_directory(
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

void tag_file(
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
 * ARQUIVA tags.csv
 *
 * tags.csv
 *
 *     ->
 *
 * tagsYYYYMMDDHHMM.md
 *
 * ============================================================ */

gboolean tags_archive(
    Controls *controls)
{
    if (!controls ||
        !controls->tags_file[0])
        return FALSE;


    /*
     * --------------------------------------------------------
     * PEGA DATA/HORA ATUAL
     * --------------------------------------------------------
     */

    time_t now =
        time(NULL);


    if (now == (time_t)-1) {

        fprintf(
            stderr,
            "[TAGS] ERRO: time()\n"
        );

        show_message(
            controls,
            "Erro obtendo data e hora"
        );

        return FALSE;
    }


    struct tm local_time;


    if (localtime_r(
            &now,
            &local_time
        ) == NULL) {

        fprintf(
            stderr,
            "[TAGS] ERRO: localtime_r()\n"
        );

        show_message(
            controls,
            "Erro obtendo data e hora"
        );

        return FALSE;
    }


    /*
     * --------------------------------------------------------
     * MONTA NOME
     * --------------------------------------------------------
     *
     * tagsYYYYMMDDHHMM.md
     */

    char archive_name[64];


    int written =
        snprintf(
            archive_name,
            sizeof(archive_name),
            "tags%04d%02d%02d%02d%02d.md",
            local_time.tm_year + 1900,
            local_time.tm_mon + 1,
            local_time.tm_mday,
            local_time.tm_hour,
            local_time.tm_min
        );


    if (written < 0 ||
        (size_t)written >= sizeof(archive_name)) {

        fprintf(
            stderr,
            "[TAGS] ERRO: nome do arquivo muito longo\n"
        );

        show_message(
            controls,
            "Erro criando nome do arquivo tags"
        );

        return FALSE;
    }


    /*
     * --------------------------------------------------------
     * MONTA CAMINHO COMPLETO
     * --------------------------------------------------------
     */

    char archive_path[PATH_MAX];


    written =
        snprintf(
            archive_path,
            sizeof(archive_path),
            "%s/%s",
            controls->tags_directory,
            archive_name
        );


    if (written < 0 ||
        (size_t)written >= sizeof(archive_path)) {

        fprintf(
            stderr,
            "[TAGS] ERRO: caminho do arquivo muito longo\n"
        );

        show_message(
            controls,
            "Erro: caminho do arquivo tags muito longo"
        );

        return FALSE;
    }


    /*
     * --------------------------------------------------------
     * RENOMEIA
     * --------------------------------------------------------
     */

    fprintf(
        stderr,
        "[TAGS] arquivando:\n"
        "       %s\n"
        "       -> %s\n",
        controls->tags_file,
        archive_path
    );


    if (rename(
            controls->tags_file,
            archive_path
        ) != 0) {

        fprintf(
            stderr,
            "[TAGS] ERRO ao renomear:\n"
            "       %s\n"
            "       -> %s\n"
            "       %s\n",
            controls->tags_file,
            archive_path,
            strerror(errno)
        );

        show_message(
            controls,
            "Erro ao arquivar tags.csv"
        );

        return FALSE;
    }


    fprintf(
        stderr,
        "[TAGS] arquivo arquivado: %s\n",
        archive_path
    );


    /*
     * O tags.csv não existe mais.
     */

    controls->tags_file[0] =
        '\0';


    /*
     * Mensagem.
     */

    char message[4096];


    snprintf(
        message,
        sizeof(message),
        "fR concluido: %s",
        archive_name
    );


    show_message(
        controls,
        message
    );


    return TRUE;
}
