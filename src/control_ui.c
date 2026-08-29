#include "control_ui.h"
#include "player.h"
#include "control_tags.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


static gboolean update_info_label_timer(
    gpointer data)
{
    Controls *controls = data;

    if (!controls)
        return G_SOURCE_REMOVE;

    return update_info_label(
        controls
    );
}


guint control_ui_start_info_timer(
    Controls *controls)
{
    if (!controls)
        return 0;

    return g_timeout_add(
        250,
        update_info_label_timer,
        controls
    );
}


/* ============================================================
 * ATUALIZA INFO LABEL
 * ============================================================ */

gboolean update_info_label(
    Controls *controls)
{
    if (!controls)
        return G_SOURCE_REMOVE;


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
     * LABEL
     * --------------------------------------------------------
     *
     * Linha 1:
     *   tempo
     *
     * Linha 2:
     *   arquivo + tag permanente
     *
     * Linha 3:
     *   mensagem temporária
     *
     * O leader T NÃO aparece.
     */

    char text[16384];


    if (controls->message[0]) {

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


    gtk_label_set_text(
        GTK_LABEL(controls->info_label),
        text
    );

    return G_SOURCE_CONTINUE;
}


/* ============================================================
 * MENSAGEM NO LABEL
 * ============================================================ */

void show_message(
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

