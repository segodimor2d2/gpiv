#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <sys/stat.h>

#include "app.h"
#include "filelist.h"


/* ============================================================
 * MAIN
 * ============================================================ */

int main(
    int argc,
    char **argv)
{
    setlocale(
        LC_NUMERIC,
        "C"
    );


    fprintf(
        stderr,
        "[MAIN] LC_NUMERIC = %s\n",
        setlocale(
            LC_NUMERIC,
            NULL
        )
    );


    /* --------------------------------------------------------
     * ORDEM
     * -------------------------------------------------------- */

    FileListOrder order =
        FILELIST_ORDER_DEFAULT;


    /* --------------------------------------------------------
     * PATH
     * -------------------------------------------------------- */

    const char *path = ".";

    int path_argument = 0;


    /*
     * Exemplos:
     *
     * ./gpiv
     * ./gpiv -n
     * ./gpiv -t
     * ./gpiv /home/segodimo/videos/
     * ./gpiv /home/segodimo/videos/video.mp4
     */

    for (int i = 1;
         i < argc;
         i++) {

        /* -----------------------------------------------
         * -n -> nome
         * ----------------------------------------------- */

        if (strcmp(
                argv[i],
                "-n") == 0) {

            order =
                FILELIST_ORDER_NAME;

            continue;
        }


        /* -----------------------------------------------
         * -t -> tempo
         * ----------------------------------------------- */

        if (strcmp(
                argv[i],
                "-t") == 0) {

            order =
                FILELIST_ORDER_TIME;

            continue;
        }


        /* -----------------------------------------------
         * PATH
         * ----------------------------------------------- */

        if (!path_argument) {

            path =
                argv[i];

            path_argument = 1;

            continue;
        }


        fprintf(
            stderr,
            "[MAIN] argumento ignorado: %s\n",
            argv[i]
        );
    }


    fprintf(
        stderr,
        "[MAIN] path = %s\n",
        path
    );


    /* --------------------------------------------------------
     * FILELIST
     * -------------------------------------------------------- */

    FileList *list =
        filelist_new();


    if (!list) {

        fprintf(
            stderr,
            "[MAIN] ERRO criando FileList\n"
        );

        return EXIT_FAILURE;
    }


    /* --------------------------------------------------------
     * VERIFICA PATH
     * -------------------------------------------------------- */

    struct stat st;


    if (stat(
            path,
            &st) < 0) {

        fprintf(
            stderr,
            "[MAIN] ERRO: não foi possível acessar '%s'\n",
            path
        );


        filelist_free(
            list
        );


        return EXIT_FAILURE;
    }


    /* --------------------------------------------------------
     * DIRETÓRIO
     * -------------------------------------------------------- */

    if (S_ISDIR(st.st_mode)) {

        fprintf(
            stderr,
            "[MAIN] carregando diretório: %s\n",
            path
        );


        if (filelist_load_directory(
                list,
                path
            ) < 0) {

            filelist_free(
                list
            );

            return EXIT_FAILURE;
        }
    }


    /* --------------------------------------------------------
     * ARQUIVO
     * -------------------------------------------------------- */

    else if (S_ISREG(st.st_mode)) {

        fprintf(
            stderr,
            "[MAIN] carregando arquivo: %s\n",
            path
        );


        if (filelist_add_path(
                list,
                path
            ) < 0) {

            filelist_free(
                list
            );

            return EXIT_FAILURE;
        }
    }


    /* --------------------------------------------------------
     * OUTRO
     * -------------------------------------------------------- */

    else {

        fprintf(
            stderr,
            "[MAIN] ERRO: path não é arquivo nem diretório\n"
        );


        filelist_free(
            list
        );


        return EXIT_FAILURE;
    }


    /* --------------------------------------------------------
     * ORDENAÇÃO
     * -------------------------------------------------------- */

    filelist_sort(
        list,
        order
    );


    /* --------------------------------------------------------
     * DEBUG
     * -------------------------------------------------------- */

    filelist_print(
        list
    );


    size_t count =
        filelist_count(
            list
        );


    fprintf(
        stderr,
        "[MAIN] %zu arquivo(s) encontrado(s)\n",
        count
    );


    /* --------------------------------------------------------
     * LISTA VAZIA
     * -------------------------------------------------------- */

    if (count == 0) {

        fprintf(
            stderr,
            "[MAIN] nenhum arquivo encontrado\n"
        );


        filelist_free(
            list
        );


        return EXIT_FAILURE;
    }


    /* --------------------------------------------------------
     * PRIMEIRO ITEM DA FILELIST
     * -------------------------------------------------------- */

    const char *first_file =
        filelist_get(
            list,
            0
        );


    if (!first_file) {

        fprintf(
            stderr,
            "[MAIN] ERRO: primeiro item da FileList é NULL\n"
        );


        filelist_free(
            list
        );


        return EXIT_FAILURE;
    }


    fprintf(
        stderr,
        "[MAIN] primeiro item da FileList:\n"
        "[MAIN]     %s\n",
        first_file
    );


    /* --------------------------------------------------------
     * PLAYER APP
     *
     * PlayerApp recebe o PRIMEIRO ITEM da FileList.
     *
     * player_app_new() faz uma cópia da string.
     * Portanto a FileList poderá ser liberada imediatamente.
     * -------------------------------------------------------- */

    PlayerApp *pa =
        player_app_new(
            first_file
        );


    if (!pa) {

        fprintf(
            stderr,
            "[MAIN] ERRO criando PlayerApp\n"
        );


        filelist_free(
            list
        );


        return EXIT_FAILURE;
    }


    /* --------------------------------------------------------
     * FILELIST NÃO É MAIS NECESSÁRIA NESTA ETAPA
     * -------------------------------------------------------- */

    filelist_free(
        list
    );


    list = NULL;


    /* --------------------------------------------------------
     * EXECUTA PLAYER APP
     * -------------------------------------------------------- */

    fprintf(
        stderr,
        "[MAIN] iniciando PlayerApp\n"
    );


    int status =
        player_app_run(
            pa,
            argc,
            argv
        );


    /* --------------------------------------------------------
     * FREE APP
     * -------------------------------------------------------- */

    player_app_free(
        pa
    );


    return status;
}
