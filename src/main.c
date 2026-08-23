#include <stdio.h>
#include <locale.h>

#include "app.h"

int main(int argc, char **argv)
{
    setlocale(LC_NUMERIC, "C");

    fprintf(
        stderr,
        "[MAIN] LC_NUMERIC = %s\n",
        setlocale(LC_NUMERIC, NULL)
    );

    const char *filename = NULL;

    if (argc > 1) {
        filename = argv[1];

        fprintf(
            stderr,
            "[MAIN] arquivo = %s\n",
            filename
        );
    }

    PlayerApp *pa =
        player_app_new(filename);

    if (!pa) {
        fprintf(
            stderr,
            "[MAIN] ERRO: não foi possível criar PlayerApp\n"
        );

        return 1;
    }

    int status =
        player_app_run(
            pa,
            argc,
            argv
        );

    player_app_free(pa);

    return status;
}
