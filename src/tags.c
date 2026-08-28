#include "tags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


/* ============================================================
 * CRIAÇÃO
 * ============================================================ */

Tags *tags_new(void)
{
    Tags *tags =
        calloc(
            1,
            sizeof(Tags)
        );

    if (!tags)
        return NULL;

    tags->count = 0;

    tags->directory[0] = '\0';

    return tags;
}


/* ============================================================
 * DESTRUIÇÃO
 * ============================================================ */

void tags_free(
    Tags *tags)
{
    if (!tags)
        return;


    for (size_t i = 0;
         i < tags->count;
         i++) {

        free(
            tags->entries[i].filename
        );
    }


    free(tags);
}


/* ============================================================
 * LIMPA LISTA
 * ============================================================ */

static void tags_clear(
    Tags *tags)
{
    if (!tags)
        return;


    for (size_t i = 0;
         i < tags->count;
         i++) {

        free(
            tags->entries[i].filename
        );
    }


    tags->count = 0;
}


/* ============================================================
 * CARREGAR
 * ============================================================ */

int tags_load(
    Tags *tags,
    const char *directory)
{
    if (!tags ||
        !directory)
        return -1;

    if (strlen(directory) >=
        sizeof(tags->directory)) {

        fprintf(
            stderr,
            "[TAGS] erro: diretório muito longo\n"
        );

        return -1;
    }

    /*
     * Agora podemos limpar as tags antigas.
     */

    tags_clear(
        tags
    );


    strcpy(
        tags->directory,
        directory
    );


    /*
     * Monta:
     *
     * directory/tags.csv
     */

    char path[PATH_MAX];

    size_t directory_len =
        strlen(directory);

    const char suffix[] =
        "/tags.csv";


    if (directory_len +
        sizeof(suffix) >
        sizeof(path)) {

        fprintf(
            stderr,
            "[TAGS] erro: caminho para tags.csv muito longo\n"
        );

        return -1;
    }


    memcpy(
        path,
        directory,
        directory_len
    );


    memcpy(
        path + directory_len,
        suffix,
        sizeof(suffix)
    );


    /*
     * Abre tags.csv.
     */

    FILE *file =
        fopen(
            path,
            "r"
        );


    /*
     * tags.csv ainda não existe.
     *
     * Cria o arquivo vazio.
     */

    if (!file) {

        file =
            fopen(
                path,
                "w"
            );


        if (!file) {

            fprintf(
                stderr,
                "[TAGS] erro criando %s\n",
                path
            );

            return -1;
        }


        fclose(file);


        fprintf(
            stderr,
            "[TAGS] criado: %s\n",
            path
        );


        return 0;
    }


    /*
     * Cada linha pode conter:
     *
     * /caminho/video.mp4,a
     */

    char line[PATH_MAX + 32];


    while (fgets(
        line,
        sizeof(line),
        file)) {

        /*
         * Remove newline.
         */

        line[
            strcspn(
                line,
                "\r\n"
            )
        ] = '\0';


        if (!line[0])
            continue;


        /*
         * Procura a última vírgula.
         *
         * Isso permite que o path
         * contenha outras vírgulas.
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


        const char *filename =
            line;


        char tag = comma[1];

        if (comma[2] != '\0')
            continue;

        if (tag < 'a' || tag > 'z')
            continue;


        /*
         * Verifica filename.
         */

        if (!filename[0])
            continue;


        /*
         * Atualmente as tags válidas
         * são a-z.
         */

        if (tag < 'a' ||
            tag > 'z')
            continue;


        /*
         * Limite máximo.
         */

        if (tags->count >= TAGS_MAX)
            break;


        /*
         * Copia filename.
         */

        tags->entries[
            tags->count
        ].filename =
            strdup(
                filename
            );


        if (!tags->entries[
                tags->count
            ].filename) {

            continue;
        }


        /*
         * Guarda tag.
         */

        tags->entries[
            tags->count
        ].tag =
            tag;


        tags->count++;
    }


    fclose(file);


    fprintf(
        stderr,
        "[TAGS] carregadas: %zu\n",
        tags->count
    );


    return 0;
}


/* ============================================================
 * ENCONTRA ARQUIVO
 * ============================================================ */

static ssize_t tags_find(
    Tags *tags,
    const char *filename)
{
    if (!tags ||
        !filename)
        return -1;


    for (size_t i = 0;
         i < tags->count;
         i++) {

        if (strcmp(
                tags->entries[i].filename,
                filename
            ) == 0) {

            return (ssize_t)i;
        }
    }


    return -1;
}


/* ============================================================
 * GET TAG
 * ============================================================ */

char tags_get(
    Tags *tags,
    const char *filename)
{
    ssize_t index =
        tags_find(
            tags,
            filename
        );


    if (index < 0)
        return '\0';


    return tags->entries[index].tag;
}


/* ============================================================
 * SET TAG
 * ============================================================ */

int tags_set(
    Tags *tags,
    const char *filename,
    char tag)
{
    if (!tags ||
        !filename)
        return -1;


    /*
     * Somente a-z.
     */

    if (tag < 'a' ||
        tag > 'z')
        return -1;


    ssize_t index =
        tags_find(
            tags,
            filename
        );


    /*
     * Arquivo já possui tag.
     *
     * Apenas substituímos.
     */

    if (index >= 0) {

        tags->entries[index].tag =
            tag;


        return tags_save(
            tags
        );
    }


    /*
     * Novo arquivo.
     */

    if (tags->count >= TAGS_MAX)
        return -1;


    tags->entries[
        tags->count
    ].filename =
        strdup(
            filename
        );


    if (!tags->entries[
            tags->count
        ].filename) {

        return -1;
    }


    tags->entries[
        tags->count
    ].tag =
        tag;


    tags->count++;


    return tags_save(
        tags
    );
}


/* ============================================================
 * SALVAR
 * ============================================================ */

int tags_save(
    Tags *tags)
{
    if (!tags ||
        !tags->directory[0])
        return -1;


    /*
     * Monta:
     *
     * directory/tags.csv
     */

    char path[PATH_MAX];

    size_t directory_len =
        strlen(
            tags->directory
        );

    const char suffix[] =
        "/tags.csv";


    if (directory_len +
        sizeof(suffix) >
        sizeof(path)) {

        fprintf(
            stderr,
            "[TAGS] erro: caminho para tags.csv muito longo\n"
        );

        return -1;
    }


    memcpy(
        path,
        tags->directory,
        directory_len
    );


    memcpy(
        path + directory_len,
        suffix,
        sizeof(suffix)
    );


    /*
     * Abre para escrita.
     */

    FILE *file =
        fopen(
            path,
            "w"
        );


    if (!file) {

        fprintf(
            stderr,
            "[TAGS] erro salvando %s\n",
            path
        );

        return -1;
    }


    /*
     * Salva todas as tags.
     *
     * Formato:
     *
     * /caminho/video.mp4,a
     */

    for (size_t i = 0;
         i < tags->count;
         i++) {

        fprintf(
            file,
            "%s,%c\n",
            tags->entries[i].filename,
            tags->entries[i].tag
        );
    }


    fclose(file);


    fprintf(
        stderr,
        "[TAGS] salvo: %s\n",
        path
    );


    return 0;
}
