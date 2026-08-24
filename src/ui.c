#include "ui.h"

#include <stdio.h>


/* ============================================================
 * CSS
 * ============================================================ */

void ui_setup_css(void)
{
    GtkCssProvider *provider =
        gtk_css_provider_new();


    gtk_css_provider_load_from_string(
        provider,
        ".video-info {"
        "  font-size: 12px;"
        "  color: white;"
        "  background-color: rgba(0, 0, 0, 0.3);"
        "  padding: 5px 8px;"
        "}"
    );


    GdkDisplay *display =
        gdk_display_get_default();


    if (display) {

        gtk_style_context_add_provider_for_display(
            display,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }


    g_object_unref(
        provider
    );
}


/* ============================================================
 * CREATE WINDOW
 * ============================================================ */

GtkWidget *ui_create_window(
    GtkApplication *app,
    Render *render,
    const char *filename,
    GtkWidget **info_label)
{
    if (!app ||
        !render)
        return NULL;


    fprintf(
        stderr,
        "[UI] criando janela\n"
    );


    /* --------------------------------------------------------
     * WINDOW
     * -------------------------------------------------------- */

    GtkWidget *window =
        gtk_application_window_new(
            app
        );


    gtk_window_set_title(
        GTK_WINDOW(window),
        "GtkGLArea TEST"
    );


    gtk_window_set_default_size(
        GTK_WINDOW(window),
        960,
        540
    );


    /* --------------------------------------------------------
     * GL AREA
     * -------------------------------------------------------- */

    GtkWidget *gl_area =
        render_get_widget(
            render
        );


    if (!gl_area) {

        fprintf(
            stderr,
            "[UI] ERRO: GtkGLArea NULL\n"
        );


        gtk_window_destroy(
            GTK_WINDOW(window)
        );


        return NULL;
    }


    /* --------------------------------------------------------
     * OVERLAY
     * -------------------------------------------------------- */

    GtkWidget *overlay =
        gtk_overlay_new();


    gtk_overlay_set_child(
        GTK_OVERLAY(overlay),
        gl_area
    );


    /* --------------------------------------------------------
     * CSS
     * -------------------------------------------------------- */

    ui_setup_css();


    /* --------------------------------------------------------
     * INFO LABEL
     * -------------------------------------------------------- */

    GtkWidget *label =
        gtk_label_new(NULL);

    gtk_label_set_xalign(
        GTK_LABEL(label),
        0.0
    );

    gtk_label_set_yalign(
        GTK_LABEL(label),
        0.0
    );

    gtk_label_set_wrap(
        GTK_LABEL(label),
        TRUE
    );


    gtk_widget_add_css_class(
        label,
        "video-info"
    );


    if (filename) {

        gtk_label_set_text(
            GTK_LABEL(label),
            filename
        );

    } else {

        gtk_label_set_text(
            GTK_LABEL(label),
            ""
        );
    }


    gtk_widget_set_halign(
        label,
        GTK_ALIGN_START
    );

    gtk_widget_set_valign(
        label,
        GTK_ALIGN_END
    );


    gtk_widget_set_margin_end(
        label,
        15
    );


    gtk_widget_set_margin_bottom(
        label,
        15
    );


    gtk_overlay_add_overlay(
        GTK_OVERLAY(overlay),
        label
    );


    /* --------------------------------------------------------
     * WINDOW CHILD
     * -------------------------------------------------------- */

    gtk_window_set_child(
        GTK_WINDOW(window),
        overlay
    );


    /* --------------------------------------------------------
     * RETORNA LABEL
     * -------------------------------------------------------- */

    if (info_label) {

        *info_label = label;
    }


    fprintf(
        stderr,
        "[UI] interface criada\n"
    );


    return window;
}
