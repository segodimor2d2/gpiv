#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <locale.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *gl_area;
    mpv_handle *mpv;
    mpv_render_context *mpv_render;
    const char *filename;
} PlayerApp;


/* ============================================================
 * mpv → preciso redesenhar transformamos isso em gtk_gl_area_queue_render()
 * ============================================================ */

static void on_mpv_update(void *ctx)
{
    PlayerApp *pa = ctx;

    fprintf(stderr,
            "[MPV] UPDATE -> queue_render()\n");

    gtk_gl_area_queue_render(
        GTK_GL_AREA(pa->gl_area)
    );
}

/* ============================================================
 * MPV -> resolução de funções OpenGL
 * ============================================================ */

static void *mpv_get_proc_address(
    void *ctx,
    const char *name)
{
    (void)ctx;

    void *ptr = (void *)glXGetProcAddressARB(
        (const GLubyte *)name
    );

    // fprintf(stderr, "[GL-GETPROC] %s -> %p\n", name, ptr);

    return ptr;
}


/* ============================================================
 * GtkGLArea REALIZE
 * ============================================================ */

static void on_gl_realize(GtkGLArea *area, PlayerApp *pa)
{
    fprintf(stderr, "[GL] REALIZE GtkGLArea\n");

    gtk_gl_area_make_current(area);

    GError *error = gtk_gl_area_get_error(area);

    if (error != NULL) {
        fprintf(stderr,
                "[GL] ERRO ao criar contexto OpenGL: %s\n",
                error->message);
        return;
    }

    GdkGLContext *ctx = gtk_gl_area_get_context(area);

    if (!ctx) {
        fprintf(stderr,
                "[GL] contexto NULL\n");
        return;
    }

    fprintf(stderr,
            "[GL] contexto OpenGL criado\n");

    fprintf(stderr,
            "[GL] vendor   = %s\n",
            glGetString(GL_VENDOR));

    fprintf(stderr,
            "[GL] renderer = %s\n",
            glGetString(GL_RENDERER));

    fprintf(stderr,
            "[GL] version  = %s\n",
            glGetString(GL_VERSION));


    /* ========================================================
     * MPV
     * ======================================================== */

    fprintf(stderr,
            "[MPV] LC_NUMERIC antes = %s\n",
            setlocale(LC_NUMERIC, NULL));

    /*
     * GTK/GLib pode ter alterado LC_NUMERIC
     * durante a inicialização da aplicação.
     *
     * libmpv exige LC_NUMERIC=C.
     */
    if (setlocale(LC_NUMERIC, "C") == NULL) {
        fprintf(stderr,
                "[MPV] ERRO: não foi possível definir LC_NUMERIC=C\n");
        return;
    }

    fprintf(stderr,
            "[MPV] LC_NUMERIC depois = %s\n",
            setlocale(LC_NUMERIC, NULL));

    fprintf(stderr,
            "[MPV] chamando mpv_create()\n");

    pa->mpv = mpv_create();

    if (!pa->mpv) {
        fprintf(stderr,
                "[MPV] ERRO: mpv_create() retornou NULL\n");
        return;
    }

    fprintf(stderr,
            "[MPV] mpv_create() OK\n");


    /*
     * Neste teste ainda NÃO criamos
     * mpv_render_context.
     */

    int status = mpv_initialize(pa->mpv);

    if (status < 0) {
        fprintf(stderr,
                "[MPV] ERRO em mpv_initialize(): %s\n",
                mpv_error_string(status));

        mpv_terminate_destroy(pa->mpv);
        pa->mpv = NULL;

        return;
    }

    fprintf(stderr,
            "[MPV] mpv_initialize() OK\n");





    /* ========================================================
     * MPV RENDER CONTEXT
     * ======================================================== */

    fprintf(stderr,
            "[MPV-RENDER] criando mpv_render_context\n");

    int advanced_control = 1;

    mpv_opengl_init_params gl_init = {
        .get_proc_address = mpv_get_proc_address,
        .get_proc_address_ctx = NULL
    };


    mpv_render_param params[] = {
        {
            MPV_RENDER_PARAM_API_TYPE,
            (void *)MPV_RENDER_API_TYPE_OPENGL
        },
        {
            MPV_RENDER_PARAM_OPENGL_INIT_PARAMS,
            &gl_init
        },
        {
            MPV_RENDER_PARAM_ADVANCED_CONTROL,
            &advanced_control
        },
        {
            MPV_RENDER_PARAM_INVALID,
            NULL
        }
    };

    int render_status =
        mpv_render_context_create(
            &pa->mpv_render,
            pa->mpv,
            params
        );

    if (render_status < 0) {
        fprintf(stderr,
                "[MPV-RENDER] ERRO: %s\n",
                mpv_error_string(render_status));

        mpv_terminate_destroy(pa->mpv);
        pa->mpv = NULL;

        return;
    }

    fprintf(stderr,
            "[MPV-RENDER] mpv_render_context criado: %p\n",
            (void *)pa->mpv_render);

    /* --------------------------------------------------------
     * CALLBACK DE ATUALIZAÇÃO
     * -------------------------------------------------------- */

    mpv_render_context_set_update_callback(
        pa->mpv_render,
        on_mpv_update,
        pa
    );

    fprintf(stderr,
            "[MPV-RENDER] update callback registrado\n");



    /* ========================================================
     * CARREGAR VÍDEO
     * ======================================================== */

    if (pa->filename) {

        fprintf(stderr,
                "[MPV] carregando arquivo: %s\n",
                pa->filename);

        const char *cmd[] = {
            "loadfile",
            pa->filename,
            NULL
        };

        int status = mpv_command(
            pa->mpv,
            cmd
        );

        if (status < 0) {
            fprintf(stderr,
                    "[MPV] ERRO loadfile: %s\n",
                    mpv_error_string(status));
        } else {
            fprintf(stderr,
                    "[MPV] loadfile enviado com sucesso\n");
        }
    }

}


/* ============================================================
 * GtkGLArea UNREALIZE
 * ============================================================ */

static void on_gl_unrealize(
    GtkGLArea *area,
    PlayerApp *pa)
{
    fprintf(stderr,
            "[GL] UNREALIZE GtkGLArea\n");

    /*
     * O render context deve ser destruído antes
     * do mpv_handle.
     */
    if (pa->mpv_render) {
        fprintf(stderr,
                "[MPV-RENDER] destruindo render context\n");

        mpv_render_context_free(pa->mpv_render);

        pa->mpv_render = NULL;

        fprintf(stderr,
                "[MPV-RENDER] render context destruído\n");
    }

    if (pa->mpv) {
        fprintf(stderr,
                "[MPV] destruindo mpv\n");

        mpv_terminate_destroy(pa->mpv);

        pa->mpv = NULL;

        fprintf(stderr,
                "[MPV] mpv destruído\n");
    }

    (void)area;
}


/* ============================================================
 * GtkGLArea RESIZE
 * ============================================================ */

static void on_gl_resize(
    GtkGLArea *area,
    int width,
    int height,
    PlayerApp *pa)
{
    (void)area;
    (void)pa;

    fprintf(stderr,
            "[gtk-resize] size=%dx%d\n",
            width,
            height);
}


/* ============================================================
 * GtkGLArea RENDER
 * ============================================================ */

static gboolean on_gl_render(
    GtkGLArea *area,
    GdkGLContext *ctx,
    PlayerApp *pa)
{
    (void)ctx;

    int width =
        gtk_widget_get_width(
            GTK_WIDGET(area));

    int height =
        gtk_widget_get_height(
            GTK_WIDGET(area));

    fprintf(stderr,
            "[gtk-render] RENDER "
            "mpv=%p render=%p\n",
            (void *)pa->mpv,
            (void *)pa->mpv_render);

    fprintf(stderr,
            "[gtk-render] viewport=%dx%d\n",
            width,
            height);


    glViewport(
        0,
        0,
        width,
        height
    );


    if (!pa->mpv_render) {

        glClearColor(
            0.2f,
            0.2f,
            0.2f,
            1.0f
        );

        glClear(GL_COLOR_BUFFER_BIT);

        return TRUE;
    }


    mpv_render_param params[] = {

        {
            MPV_RENDER_PARAM_OPENGL_FBO,
            &(mpv_opengl_fbo){
                .fbo = 0,
                .w = width,
                .h = height,
                .internal_format = 0
            }
        },

        {
            MPV_RENDER_PARAM_FLIP_Y,
            &(int){1}
        },

        {
            MPV_RENDER_PARAM_INVALID,
            NULL
        }
    };


    int status =
        mpv_render_context_render(
            pa->mpv_render,
            params
        );


    if (status < 0) {

        fprintf(stderr,
                "[MPV-RENDER] ERRO render: %s\n",
                mpv_error_string(status));

    } else {

        fprintf(stderr,
                "[MPV-RENDER] render OK\n");
    }


    return TRUE;
}


/* ============================================================
 * WINDOW REALIZE
 * ============================================================ */

static void on_window_realize(
    GtkWindow *window,
    PlayerApp *pa)
{
    fprintf(stderr,
            "[window] REALIZE\n");

    fprintf(stderr,
            "[window] visible=%d mapped=%d\n",
            gtk_widget_get_visible(
                GTK_WIDGET(window)),
            gtk_widget_get_mapped(
                GTK_WIDGET(window)));

    fprintf(stderr,
            "[window] size=%dx%d\n",
            gtk_widget_get_width(
                GTK_WIDGET(window)),
            gtk_widget_get_height(
                GTK_WIDGET(window)));

    fprintf(stderr,
            "[gl-area] realized=%d mapped=%d visible=%d size=%dx%d\n",
            gtk_widget_get_realized(pa->gl_area),
            gtk_widget_get_mapped(pa->gl_area),
            gtk_widget_get_visible(pa->gl_area),
            gtk_widget_get_width(pa->gl_area),
            gtk_widget_get_height(pa->gl_area));
}


/* ============================================================
 * WINDOW MAP
 * ============================================================ */

static void on_window_map(
    GtkWindow *window,
    PlayerApp *pa)
{
    fprintf(stderr,
            "[window] MAP\n");

    fprintf(stderr,
            "[window] visible=%d mapped=%d\n",
            gtk_widget_get_visible(
                GTK_WIDGET(window)),
            gtk_widget_get_mapped(
                GTK_WIDGET(window)));

    fprintf(stderr,
            "[window] size=%dx%d\n",
            gtk_widget_get_width(
                GTK_WIDGET(window)),
            gtk_widget_get_height(
                GTK_WIDGET(window)));

    fprintf(stderr,
            "[gl-area] realized=%d mapped=%d visible=%d size=%dx%d\n",
            gtk_widget_get_realized(pa->gl_area),
            gtk_widget_get_mapped(pa->gl_area),
            gtk_widget_get_visible(pa->gl_area),
            gtk_widget_get_width(pa->gl_area),
            gtk_widget_get_height(pa->gl_area));
}

/* ============================================================
 * ACTIVATE
 * ============================================================ */

static void on_activate(
    GtkApplication *app,
    PlayerApp *pa)
{
    fprintf(stderr,
            "[gtk] ACTIVATE\n");


    /* --------------------------------------------------------
     * WINDOW
     * -------------------------------------------------------- */

    pa->window =
        gtk_application_window_new(app);

    gtk_window_set_title(
        GTK_WINDOW(pa->window),
        "GtkGLArea TEST"
    );

    gtk_window_set_default_size(
        GTK_WINDOW(pa->window),
        960,
        540
    );


    /* --------------------------------------------------------
     * GtkGLArea
     * -------------------------------------------------------- */

    pa->gl_area =
        gtk_gl_area_new();


    gtk_gl_area_set_allowed_apis(
        GTK_GL_AREA(pa->gl_area),
        GDK_GL_API_GL
    );


    gtk_gl_area_set_required_version(
        GTK_GL_AREA(pa->gl_area),
        3,
        3
    );


    /*
     * O GtkGLArea deve ocupar toda a janela.
     */

    gtk_widget_set_hexpand(
        pa->gl_area,
        TRUE
    );

    gtk_widget_set_vexpand(
        pa->gl_area,
        TRUE
    );


    /*
     * Coloca GtkGLArea dentro da janela.
     */

    gtk_window_set_child(
        GTK_WINDOW(pa->window),
        pa->gl_area
    );


    /* --------------------------------------------------------
     * SIGNALS
     * -------------------------------------------------------- */

    g_signal_connect(
        pa->window,
        "realize",
        G_CALLBACK(on_window_realize),
        pa
    );


    g_signal_connect(
        pa->window,
        "map",
        G_CALLBACK(on_window_map),
        pa
    );


    g_signal_connect(
        pa->gl_area,
        "realize",
        G_CALLBACK(on_gl_realize),
        pa
    );


    g_signal_connect(
        pa->gl_area,
        "unrealize",
        G_CALLBACK(on_gl_unrealize),
        pa
    );


    g_signal_connect(
        pa->gl_area,
        "resize",
        G_CALLBACK(on_gl_resize),
        pa
    );


    g_signal_connect(
        pa->gl_area,
        "render",
        G_CALLBACK(on_gl_render),
        pa
    );


    /* --------------------------------------------------------
     * AUTO RENDER
     * -------------------------------------------------------- */

    gtk_gl_area_set_auto_render(
        GTK_GL_AREA(pa->gl_area),
        TRUE
    );

    /* --------------------------------------------------------
     * MOSTRA A JANELA
     * -------------------------------------------------------- */

    fprintf(stderr,
            "[gtk] chamando gtk_window_present()\n");

    gtk_window_present(
        GTK_WINDOW(pa->window)
    );

    /* --------------------------------------------------------
     * PRIMEIRO RENDER EXPLÍCITO
     * -------------------------------------------------------- */

    // fprintf(stderr,
    //         "[gtk] chamando gtk_gl_area_queue_render()\n");

    // gtk_gl_area_queue_render(
    //     GTK_GL_AREA(pa->gl_area)
    // );
}


/* ============================================================
 * MAIN
 * ============================================================ */

int main(int argc, char **argv)
{
    setlocale(LC_NUMERIC, "C");

    fprintf(stderr,
            "[MAIN] LC_NUMERIC = %s\n",
            setlocale(LC_NUMERIC, NULL));

    PlayerApp pa = {0};

    if (argc > 1) {
        pa.filename = argv[1];

        fprintf(stderr,
                "[MAIN] arquivo = %s\n",
                pa.filename);
    }

    pa.app =
        gtk_application_new(
            "dev.local.gpiv-test",
            G_APPLICATION_NON_UNIQUE
        );

    g_signal_connect(
        pa.app,
        "activate",
        G_CALLBACK(on_activate),
        &pa
    );

    /*
     * Não entregar o caminho do vídeo ao GApplication.
     */
    int gtk_argc = 1;
    char *gtk_argv[] = {
        argv[0],
        NULL
    };

    int status =
        g_application_run(
            G_APPLICATION(pa.app),
            gtk_argc,
            gtk_argv
        );

    g_object_unref(pa.app);

    return status;
}
