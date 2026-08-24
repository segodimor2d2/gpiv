#include "render.h"
#include "player.h"

#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>


/* ============================================================
 * RENDER
 * ============================================================ */

struct _Render {

    GtkWidget *gl_area;

    Player *player;

    mpv_render_context *mpv_render;

    const char *filename;
};


/* ============================================================
 * PROTÓTIPOS INTERNOS
 * ============================================================ */

static void on_gl_realize(
    GtkGLArea *area,
    Render *render
);

static void on_gl_unrealize(
    GtkGLArea *area,
    Render *render
);

static void on_gl_resize(
    GtkGLArea *area,
    int width,
    int height,
    Render *render
);

static gboolean on_gl_render(
    GtkGLArea *area,
    GdkGLContext *ctx,
    Render *render
);

static void on_mpv_update(
    void *ctx
);

static gboolean mpv_queue_render_idle(
    gpointer data
);

static void *mpv_get_proc_address(
    void *ctx,
    const char *name
);


/* ============================================================
 * OPENGL
 * ============================================================ */

static void *mpv_get_proc_address(
    void *ctx,
    const char *name)
{
    (void)ctx;

    return (void *)glXGetProcAddressARB(
        (const GLubyte *)name
    );
}


/* ============================================================
 * MPV -> GTK RENDER
 * ============================================================ */

static gboolean mpv_queue_render_idle(
    gpointer data)
{
    Render *render = data;

    if (!render ||
        !render->gl_area)
        return G_SOURCE_REMOVE;

    gtk_gl_area_queue_render(
        GTK_GL_AREA(render->gl_area)
    );

    return G_SOURCE_REMOVE;
}


static void on_mpv_update(
    void *ctx)
{
    Render *render = ctx;

    if (!render)
        return;

    /*
     * O callback do mpv pode acontecer fora
     * da thread principal do GTK.
     *
     * Portanto colocamos o queue_render()
     * dentro do main context.
     */

    g_main_context_invoke(
        NULL,
        mpv_queue_render_idle,
        render
    );
}


/* ============================================================
 * CRIAÇÃO
 * ============================================================ */

Render *render_new(
    const char *filename)
{
    Render *render =
        calloc(
            1,
            sizeof(Render)
        );

    if (!render)
        return NULL;


    render->gl_area = NULL;
    render->player = NULL;
    render->mpv_render = NULL;
    render->filename = filename;


    /* --------------------------------------------------------
     * GTK GL AREA
     * -------------------------------------------------------- */

    render->gl_area =
        gtk_gl_area_new();


    gtk_gl_area_set_allowed_apis(
        GTK_GL_AREA(render->gl_area),
        GDK_GL_API_GL
    );


    gtk_gl_area_set_required_version(
        GTK_GL_AREA(render->gl_area),
        3,
        3
    );


    gtk_widget_set_hexpand(
        render->gl_area,
        TRUE
    );


    gtk_widget_set_vexpand(
        render->gl_area,
        TRUE
    );


    gtk_widget_set_focusable(
        render->gl_area,
        TRUE
    );


    /* --------------------------------------------------------
     * RENDER MANUAL
     * -------------------------------------------------------- */

    gtk_gl_area_set_auto_render(
        GTK_GL_AREA(render->gl_area),
        FALSE
    );


    /* --------------------------------------------------------
     * SIGNALS
     * -------------------------------------------------------- */

    g_signal_connect(
        render->gl_area,
        "realize",
        G_CALLBACK(on_gl_realize),
        render
    );


    g_signal_connect(
        render->gl_area,
        "unrealize",
        G_CALLBACK(on_gl_unrealize),
        render
    );


    g_signal_connect(
        render->gl_area,
        "resize",
        G_CALLBACK(on_gl_resize),
        render
    );


    g_signal_connect(
        render->gl_area,
        "render",
        G_CALLBACK(on_gl_render),
        render
    );


    return render;
}


/* ============================================================
 * GtkGLArea REALIZE
 * ============================================================ */

static void on_gl_realize(
    GtkGLArea *area,
    Render *render)
{

    gtk_gl_area_make_current(
        area
    );


    GError *error =
        gtk_gl_area_get_error(
            area
        );


    if (error != NULL) {

        fprintf(
            stderr,
            "[GL] ERRO ao criar contexto OpenGL: %s\n",
            error->message
        );

        return;
    }


    GdkGLContext *ctx =
        gtk_gl_area_get_context(
            area
        );


    if (!ctx) {

        fprintf(
            stderr,
            "[GL] contexto NULL\n"
        );

        return;
    }


    fprintf(
        stderr,
        "[GL] contexto OpenGL criado\n"
    );


    fprintf(
        stderr,
        "[GL] vendor   = %s\n",
        glGetString(GL_VENDOR)
    );


    fprintf(
        stderr,
        "[GL] renderer = %s\n",
        glGetString(GL_RENDERER)
    );


    fprintf(
        stderr,
        "[GL] version  = %s\n",
        glGetString(GL_VERSION)
    );


    /* --------------------------------------------------------
     * LC_NUMERIC
     * -------------------------------------------------------- */

    fprintf(
        stderr,
        "[MPV] LC_NUMERIC antes = %s\n",
        setlocale(
            LC_NUMERIC,
            NULL
        )
    );


    if (setlocale(
            LC_NUMERIC,
            "C") == NULL) {

        fprintf(
            stderr,
            "[MPV] ERRO: não foi possível definir "
            "LC_NUMERIC=C\n"
        );

        return;
    }


    fprintf(
        stderr,
        "[MPV] LC_NUMERIC depois = %s\n",
        setlocale(
            LC_NUMERIC,
            NULL
        )
    );


    /* --------------------------------------------------------
     * CRIA PLAYER
     * -------------------------------------------------------- */

    render->player =
        player_new();


    if (!render->player) {

        fprintf(
            stderr,
            "[PLAYER] ERRO: não foi possível criar Player\n"
        );

        return;
    }


    /* --------------------------------------------------------
     * INICIALIZA MPV
     * -------------------------------------------------------- */

    int status =
        player_initialize(
            render->player,
            render->filename
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[PLAYER] ERRO ao inicializar player\n"
        );

        player_free(
            render->player
        );

        render->player = NULL;

        return;
    }


    fprintf(
        stderr,
        "[PLAYER] inicializado com sucesso\n"
    );


    /* --------------------------------------------------------
     * OBTÉM MPV HANDLE
     * -------------------------------------------------------- */

    mpv_handle *mpv =
        player_get_mpv(
            render->player
        );


    if (!mpv) {

        fprintf(
            stderr,
            "[MPV-RENDER] ERRO: mpv_handle NULL\n"
        );

        player_free(
            render->player
        );

        render->player = NULL;

        return;
    }


    /* --------------------------------------------------------
     * MPV OPENGL INIT
     * -------------------------------------------------------- */

    mpv_opengl_init_params gl_init_params = {

        .get_proc_address =
            mpv_get_proc_address,

        .get_proc_address_ctx =
            NULL
    };


    mpv_render_param params[] = {

        {
            MPV_RENDER_PARAM_API_TYPE,
            MPV_RENDER_API_TYPE_OPENGL
        },

        {
            MPV_RENDER_PARAM_OPENGL_INIT_PARAMS,
            &gl_init_params
        },

        {
            MPV_RENDER_PARAM_INVALID,
            NULL
        }
    };


    /* --------------------------------------------------------
     * CRIA MPV RENDER CONTEXT
     * -------------------------------------------------------- */

    status =
        mpv_render_context_create(
            &render->mpv_render,
            mpv,
            params
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV-RENDER] ERRO criando contexto: %s\n",
            mpv_error_string(status)
        );

        render->mpv_render = NULL;

        player_free(
            render->player
        );

        render->player = NULL;

        return;
    }


    fprintf(
        stderr,
        "[MPV-RENDER] contexto criado: %p\n",
        (void *)render->mpv_render
    );


    /* --------------------------------------------------------
     * CALLBACK DE UPDATE
     * -------------------------------------------------------- */

    mpv_render_context_set_update_callback(
        render->mpv_render,
        on_mpv_update,
        render
    );


    /* --------------------------------------------------------
     * PRIMEIRO RENDER
     * -------------------------------------------------------- */

    gtk_gl_area_queue_render(
        GTK_GL_AREA(render->gl_area)
    );
}


/* ============================================================
 * GtkGLArea UNREALIZE
 * ============================================================ */

static void on_gl_unrealize(
    GtkGLArea *area,
    Render *render)
{
    (void)area;


    fprintf(
        stderr,
        "[GL] UNREALIZE GtkGLArea\n"
    );


    /* --------------------------------------------------------
     * MPV RENDER CONTEXT
     * -------------------------------------------------------- */

    if (render &&
        render->mpv_render) {

        fprintf(
            stderr,
            "[MPV-RENDER] destruindo contexto\n"
        );


        mpv_render_context_free(
            render->mpv_render
        );


        render->mpv_render = NULL;
    }


    /* --------------------------------------------------------
     * PLAYER
     * -------------------------------------------------------- */

    if (render &&
        render->player) {

        fprintf(
            stderr,
            "[PLAYER] destruindo player\n"
        );


        player_free(
            render->player
        );


        render->player = NULL;
    }
}


/* ============================================================
 * RESIZE
 * ============================================================ */

static void on_gl_resize(
    GtkGLArea *area,
    int width,
    int height,
    Render *render)
{
    (void)area;
    (void)render;


    fprintf(
        stderr,
        "[gtk-resize] size=%dx%d\n",
        width,
        height
    );
}


/* ============================================================
 * RENDER
 * ============================================================ */

static gboolean on_gl_render(
    GtkGLArea *area,
    GdkGLContext *ctx,
    Render *render)
{
    (void)ctx;

    int width =
        gtk_widget_get_width(
            GTK_WIDGET(area)
        );


    int height =
        gtk_widget_get_height(
            GTK_WIDGET(area)
        );


    if (width <= 0 ||
        height <= 0)
        return TRUE;


    /* --------------------------------------------------------
     * RENDER CONTEXT AINDA NÃO DISPONÍVEL
     * -------------------------------------------------------- */

    if (!render ||
        !render->mpv_render) {

        glClearColor(
            0.2f,
            0.2f,
            0.2f,
            1.0f
        );


        glClear(
            GL_COLOR_BUFFER_BIT
        );


        return TRUE;
    }


    /* --------------------------------------------------------
     * VIEWPORT
     * -------------------------------------------------------- */

    glViewport(
        0,
        0,
        width,
        height
    );


    /* --------------------------------------------------------
     * FBO DO GTK
     * -------------------------------------------------------- */

    GLint current_fbo = 0;


    glGetIntegerv(
        GL_DRAW_FRAMEBUFFER_BINDING,
        &current_fbo
    );


    mpv_opengl_fbo fbo = {

        .fbo = current_fbo,
        .w = width,
        .h = height,
        .internal_format = 0
    };


    /* --------------------------------------------------------
     * FLIP Y
     * -------------------------------------------------------- */

    int flip_y = 1;


    /* --------------------------------------------------------
     * PARÂMETROS MPV
     * -------------------------------------------------------- */

    mpv_render_param params[] = {

        {
            MPV_RENDER_PARAM_OPENGL_FBO,
            &fbo
        },

        {
            MPV_RENDER_PARAM_FLIP_Y,
            &flip_y
        },

        {
            MPV_RENDER_PARAM_INVALID,
            NULL
        }
    };


    /* --------------------------------------------------------
     * RENDER
     * -------------------------------------------------------- */

    int status =
        mpv_render_context_render(
            render->mpv_render,
            params
        );


    if (status < 0) {

        fprintf(
            stderr,
            "[MPV-RENDER] ERRO: %s\n",
            mpv_error_string(status)
        );

        return TRUE;
    }


    /* --------------------------------------------------------
     * REPORT SWAP
     * -------------------------------------------------------- */

    mpv_render_context_report_swap(
        render->mpv_render
    );


    return TRUE;
}


/* ============================================================
 * GET WIDGET
 * ============================================================ */

GtkWidget *render_get_widget(
    Render *render)
{
    if (!render)
        return NULL;

    return render->gl_area;
}


/* ============================================================
 * GET PLAYER
 * ============================================================ */

Player *render_get_player(
    Render *render)
{
    if (!render)
        return NULL;

    return render->player;
}


/* ============================================================
 * FREE
 * ============================================================ */

void render_free(
    Render *render)
{
    if (!render)
        return;


    /*
     * Normalmente o GtkGLArea já terá chamado
     * on_gl_unrealize().
     *
     * Mas mantemos esta proteção para evitar
     * vazamentos caso render_free() seja chamado
     * em outra situação.
     */

    if (render->mpv_render) {

        fprintf(
            stderr,
            "[MPV-RENDER] free() -> contexto\n"
        );


        mpv_render_context_free(
            render->mpv_render
        );


        render->mpv_render = NULL;
    }


    if (render->player) {

        fprintf(
            stderr,
            "[PLAYER] free() -> player\n"
        );


        player_free(
            render->player
        );


        render->player = NULL;
    }


    if (render->gl_area) {

        /*
         * O Gtk será responsável pela referência
         * do widget enquanto ele estiver na árvore.
         */

        render->gl_area = NULL;
    }


    free(render);
}
