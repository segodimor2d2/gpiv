#include "control_keys.h"
#include "player.h"
#include "control_tags.h"
#include "control_video.h"
#include "dirtags.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/stat.h>
#include <errno.h>

#include <sys/wait.h>
#include <unistd.h>


#define SEEK_SECONDS 1
#define SEEK_HARD_SECONDS 10

/* ============================================================
 * KEYBOARD
 * ============================================================ */

gboolean control_keys_handle(
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

        /*
         * Se o leader F já está ativo,
         * f é a segunda tecla:
         *
         * ff
         */
        if (controls->leadf) {

            controls->leadfvar = 'f';

            fprintf(
                stderr,
                "[LEADER] ff\n"
            );

            return TRUE;
        }

        /*
         * Se o leader T está ativo,
         * f é a segunda tecla:
         *
         * tf
         */
        if (controls->leadt) {

            const char *filename =
                player_get_filename(
                    player
                );

            if (filename) {

                tag_file(
                    controls,
                    filename,
                    "tf"
                );
            }

            controls->leadt = FALSE;

            fprintf(
                stderr,
                "[LEADER] tf\n"
            );

            return TRUE;
        }

        /*
         * Ativa leader F.
         */

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

        /*
         * Se o leader F está ativo,
         * t é a segunda tecla:
         *
         * ft
         */
        if (controls->leadf) {

            controls->leadfvar = 't';

            fprintf(
                stderr,
                "[LEADER] ft\n"
            );

            return TRUE;
        }

        /*
         * Se o leader T já está ativo,
         * t é a segunda tecla:
         *
         * tt
         */
        if (controls->leadt) {

            const char *filename =
                player_get_filename(
                    player
                );

            if (filename) {

                tag_file(
                    controls,
                    filename,
                    "tt"
                );
            }

            controls->leadt = FALSE;

            fprintf(
                stderr,
                "[LEADER] tt\n"
            );

            return TRUE;
        }

        /*
         * Ativa leader T.
         */

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

            if (!all_valid) {

                controls->leadf = FALSE;
                controls->leadfvar = '\0';

                return TRUE;
            }


            /*
             * Todas as pastas são válidas.
             *
             * Agora podemos executar a pré-validação
             * dos arquivos e destinos.
             */

            fprintf(
                stderr,
                "[LEADER] fR\n"
            );


            /*
             * ------------------------------------------------
             * PRÉ-VALIDAÇÃO
             * ------------------------------------------------
             *
             * Nada é movido se existir qualquer problema.
             */

            gboolean valid =
                dirtags_prevalidate(
                    controls
                );


            if (!valid) {

                controls->leadf = FALSE;
                controls->leadfvar = '\0';

                fprintf(
                    stderr,
                    "[fR] movimentação cancelada "
                    "pela pré-validação\n"
                );

                return TRUE;
            }


            /*
             * ------------------------------------------------
             * MOVE OS ARQUIVOS
             * ------------------------------------------------
             */

            dirtags_move_files(
                controls
            );


            /*
             * fR terminou.
             */

            controls->leadf = FALSE;
            controls->leadfvar = '\0';


            return TRUE;
        }


        /* ----------------------------------------------------
         * w -> DETECTAR FORMATO DO VIDEO
         *
         * fw
         *
         * Usa ffprobe para descobrir:
         *
         * container
         * codec de video
         * pixel format
         * resolucao
         * codec de audio
         * ---------------------------------------------------- */

        if (keyval == GDK_KEY_w) {

            fprintf(
                stderr,
                "[LEADER] fw\n"
            );

            const char *filename =
                player_app_get_filename(
                    controls->app
                );

            if (!filename ||
                !filename[0]) {

                show_message(
                    controls,
                    "Nenhum video carregado"
                );

                controls->leadf = FALSE;
                controls->leadfvar = '\0';

                return TRUE;
            }

            detect_video_format(
                controls,
                filename
            );

            convert_video_format(
                controls,
                filename
            );

            controls->leadf = FALSE;
            controls->leadfvar = '\0';

            fprintf(
                stderr,
                "[LEADER] fw OFF\n"
            );

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
     * , -> FRAME ANTERIOR
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_comma) {

        if (player) {

            player_frame_back(
                player
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * m -> PRÓXIMO FRAME
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_m) {

        if (player) {

            player_frame_forward(
                player
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * N -> SEEK +10
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_N) {

        if (player) {

            player_seek_forward(
                player,
                SEEK_HARD_SECONDS
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * > -> SEEK -10
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_greater) {

        if (player) {

            player_seek_backward(
                player,
                SEEK_HARD_SECONDS
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * n -> SEEK +1
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_n) {

        if (player) {

            player_seek_forward(
                player,
                SEEK_SECONDS
            );
        }

        return TRUE;
    }


    /* --------------------------------------------------------
     * . -> SEEK -1
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_period) {

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
