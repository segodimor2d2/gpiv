#include "dirtags.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>


/* ============================================================
 * LIMPAR DIRTAGS
 * ============================================================ */

void dirtags_clear(
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

void dirtags_add_unique(
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
 * MONTA DIRTAGS COM LEADTVAR
 * ============================================================ */

void dirtags_build(
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
 * CRIA DIRS
 * ============================================================ */

void dirtags_create_directories(
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
