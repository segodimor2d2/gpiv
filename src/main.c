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
     * ./gpiv /home/segodimo/videos/tst2.mp4
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
     * IMPRIME LISTA
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
     * PRIMEIRO ITEM
     * -------------------------------------------------------- */

    const char *filename =
        filelist_get(
            list,
            0
        );


    if (!filename) {

        fprintf(
            stderr,
            "[MAIN] ERRO: primeiro arquivo NULL\n"
        );


        filelist_free(
            list
        );


        return EXIT_FAILURE;
    }


    fprintf(
        stderr,
        "[MAIN] primeiro item da FileList:\n"
    );


    fprintf(
        stderr,
        "[MAIN]     %s\n",
        filename
    );


    /* --------------------------------------------------------
     * PLAYER APP
     * -------------------------------------------------------- */

    PlayerApp *pa =
        player_app_new(
            list,
            0
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
     * EXECUTA GTK
     * -------------------------------------------------------- */

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


    /*
     * PlayerApp usou a FileList durante toda a
     * execução do GTK.
     *
     * Agora o GTK terminou e podemos liberar.
     */

    filelist_free(
        list
    );


    return status;
}
