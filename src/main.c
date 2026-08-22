#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <locale.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <libgen.h>

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *gl_area;
    GtkWidget *info_label;
    mpv_handle *mpv;
    mpv_render_context *mpv_render;
    const char *filename;
    guint mpv_event_source;
} PlayerApp;

/* Protótipos */
static gboolean restore_info_label(gpointer data);

/* ============================================================
* CLIPBOARD
* ============================================================ */

static void show_clipboard_message(PlayerApp *pa)
{
    if (!pa->info_label || !pa->filename)
        return;

    char message[8192];

    snprintf(
        message,
        sizeof(message),
        "%s\nPath copiado",
        pa->filename
    );

    gtk_label_set_text(
        GTK_LABEL(pa->info_label),
        message
    );
}

static void copy_video_path(PlayerApp *pa)
{
    if (!pa->filename || !pa->window)
        return;

    GdkDisplay *display =
        gtk_widget_get_display(pa->window);

    GdkClipboard *clipboard =
        gdk_display_get_clipboard(display);

    gdk_clipboard_set_text(
        clipboard,
        pa->filename
    );

    fprintf(stderr,
            "[CLIPBOARD] copiado: %s\n",
            pa->filename);

    show_clipboard_message(pa);

    g_timeout_add(
        2000,
        restore_info_label,
        pa
    );
}

/* ============================================================
* CSS
* ============================================================ */

static void setup_info_label_css(void)
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

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);
}


/* ============================================================
 * RESTORE_INFO_LABEL
 * ============================================================ */

static gboolean restore_info_label(gpointer data)
{
    PlayerApp *pa = data;

    if (!pa->info_label || !pa->filename)
        return G_SOURCE_REMOVE;

    gtk_label_set_text(
        GTK_LABEL(pa->info_label),
        pa->filename
    );

    return G_SOURCE_REMOVE;
}



static void mpv_check_events(PlayerApp *pa)
{
    if (!pa->mpv)
        return;

    while (1) {
        mpv_event *event = mpv_wait_event(pa->mpv, 0);

        if (!event)
            break;

        if (event->event_id == MPV_EVENT_NONE)
            break;

        fprintf(stderr,
                "[MPV-EVENT] id=%d name=%s\n",
                event->event_id,
                mpv_event_name(event->event_id));

        switch (event->event_id) {

        case MPV_EVENT_FILE_LOADED:
            fprintf(stderr,
                    "[MPV-EVENT] FILE_LOADED\n");
            break;

        case MPV_EVENT_VIDEO_RECONFIG:
            fprintf(stderr,
                    "[MPV-EVENT] VIDEO_RECONFIG\n");
            break;

        case MPV_EVENT_PLAYBACK_RESTART:
            fprintf(stderr,
                    "[MPV-EVENT] PLAYBACK_RESTART\n");
            break;

        case MPV_EVENT_END_FILE: {
            mpv_event_end_file *end =
                event->data;

        fprintf(stderr,
                "[MPV-EVENT] END_FILE reason=%d error=%s\n",
                end ? (int)end->reason : -1,
                end ? mpv_error_string(end->error) : "NULL");
            break;
        }

        case MPV_EVENT_LOG_MESSAGE: {
            mpv_event_log_message *msg =
                event->data;

            if (msg) {
                fprintf(stderr,
                        "[MPV-LOG] [%s] %s",
                        msg->prefix,
                        msg->text);
            }
            break;
        }

        default:
            break;
        }
    }
}


static gboolean mpv_event_timer(gpointer data)
{
    PlayerApp *pa = data;

    if (!pa->mpv)
        return G_SOURCE_REMOVE;

    mpv_check_events(pa);

    return G_SOURCE_CONTINUE;
}

/* ============================================================
 * MPV - enviar comando
 * ============================================================ */

static int mpv_send_command(
    PlayerApp *pa,
    const char *cmd,
    const char *arg)
{
    if (!pa->mpv) {
        fprintf(stderr,
                "[MPV-CMD] ERRO: mpv não está disponível\n");
        return -1;
    }

    const char *command[3];

    command[0] = cmd;
    command[1] = arg;
    command[2] = NULL;

    fprintf(stderr,
            "[MPV-CMD] %s%s%s\n",
            cmd,
            arg ? " " : "",
            arg ? arg : "");

    int status = mpv_command(
        pa->mpv,
        command
    );

    if (status < 0) {
        fprintf(stderr,
                "[MPV-CMD] ERRO: %s\n",
                mpv_error_string(status));
    } else {
        fprintf(stderr,
                "[MPV-CMD] OK\n");
    }

    return status;
}


/* ============================================================
* SAVE FRAME
* ============================================================ */

static void show_saved_message(PlayerApp *pa)
{
    if (!pa->info_label || !pa->filename)
        return;

    char path[4096];

    snprintf(
        path,
        sizeof(path),
        "%s",
        pa->filename
    );

    char *filename = basename(path);

    /*
     * Remove a extensão.
     *
     * tst2.mp4 -> tst2
     */

    char name[4096];

    snprintf(
        name,
        sizeof(name),
        "%s",
        filename
    );

    char *dot = strrchr(name, '.');

    if (dot)
        *dot = '\0';

    char message[8192];

    snprintf(
        message,
        sizeof(message),
        "%s\nFrame %.*s salvo",
        pa->filename,
        (int)(sizeof(message) - strlen(pa->filename) - 20),
        name
    );

    gtk_label_set_text(
        GTK_LABEL(pa->info_label),
        message
    );
}

static void mpv_save_frame(PlayerApp *pa)
{
    if (!pa->mpv || !pa->filename)
        return;

    char path[4096];

    snprintf(
        path,
        sizeof(path),
        "%s",
        pa->filename
    );

    char *dir = dirname(path);

    fprintf(stderr,
            "[SCREENSHOT] diretório = %s\n",
            dir);

    /*
     * Define onde o screenshot será salvo.
     */

    int status = mpv_set_option_string(
        pa->mpv,
        "screenshot-dir",
        dir
    );

    if (status < 0) {

        fprintf(stderr,
                "[SCREENSHOT] ERRO screenshot-dir: %s\n",
                mpv_error_string(status));

        return;
    }

    /*
     * Nome do arquivo:
     *
     * %F = nome do arquivo de vídeo
     * %n = número sequencial
     */

    status = mpv_set_option_string(
        pa->mpv,
        "screenshot-template",
        "%F_%n"
    );

    if (status < 0) {

        fprintf(stderr,
                "[SCREENSHOT] ERRO screenshot-template: %s\n",
                mpv_error_string(status));

        return;
    }

    /*
     * Captura o frame atual.
     */

    const char *command[] = {
        "screenshot",
        "video",
        NULL
    };

    fprintf(stderr,
            "[SCREENSHOT] salvando frame\n");

    status = mpv_command(
        pa->mpv,
        command
    );

    if (status < 0) {

        fprintf(stderr,
                "[SCREENSHOT] ERRO: %s\n",
                mpv_error_string(status));

    } else {

        fprintf(stderr,
                "[SCREENSHOT] OK\n");

        /*
         * Só mostra "Frame tst2 salvo"
         * depois que o comando screenshot
         * foi aceito pelo mpv.
         */

        show_saved_message(pa);

        /*
         * Depois de 2 segundos volta
         * a mostrar o caminho do vídeo.
         */

        g_timeout_add(
            2000,
            restore_info_label,
            pa
        );
    }
}


/* ============================================================
 * MPV - pause/play
 * ============================================================ */

static void mpv_toggle_pause(PlayerApp *pa)
{
    fprintf(stderr,
            "[MPV] PAUSE/PLAY\n");

    mpv_send_command(
        pa,
        "cycle",
        "pause"
    );
}


static gboolean on_key_pressed(
    GtkEventControllerKey *controller,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    PlayerApp *pa)
{
    (void)controller;
    (void)keycode;
    (void)state;

    fprintf(stderr,
            "[KEY] keyval=0x%x\n",
            keyval);

    /* --------------------------------------------------------
     * Q -> sair do programa
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_q ||
        keyval == GDK_KEY_Q) {

        fprintf(stderr,
                "[KEY] Q DETECTADO -> saindo\n");

        gtk_window_destroy(
            GTK_WINDOW(pa->window)
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * SPACE -> pause/play
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_space) {

        fprintf(stderr,
                "[KEY] SPACE DETECTADO!\n");

        mpv_toggle_pause(pa);

        return TRUE;
    }


    /* --------------------------------------------------------
     * K -> frame anterior
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_k ||
        keyval == GDK_KEY_K) {

        fprintf(stderr,
                "[KEY] K -> frame anterior\n");

        mpv_send_command(
            pa,
            "frame-back-step",
            NULL
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * J -> próximo frame
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_j ||
        keyval == GDK_KEY_J) {

        fprintf(stderr,
                "[KEY] J -> próximo frame\n");

        mpv_send_command(
            pa,
            "frame-step",
            NULL
        );

        return TRUE;
    }

    /* --------------------------------------------------------
     * s -> salvar frame
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_s ||
        keyval == GDK_KEY_S) {

        fprintf(stderr,
                "[KEY] S -> salvar frame\n");

        mpv_save_frame(pa);

        return TRUE;
    }


    /* --------------------------------------------------------
     * Y -> copiar path do vídeo
     * -------------------------------------------------------- */

    if (keyval == GDK_KEY_y ||
        keyval == GDK_KEY_Y) {

        fprintf(stderr,
                "[KEY] Y -> copiar path\n");

        copy_video_path(pa);

        return TRUE;
    }

    /* --------------------------------------------------------
     * fim
     * -------------------------------------------------------- */
    return FALSE;
}

/* ============================================================
 * mpv → preciso redesenhar transformamos isso em gtk_gl_area_queue_render()
 * ============================================================ */

static gboolean mpv_queue_render_idle(gpointer data)
{
    PlayerApp *pa = data;

    if (!pa->gl_area)
        return G_SOURCE_REMOVE;

    gtk_gl_area_queue_render(
        GTK_GL_AREA(pa->gl_area)
    );

    return G_SOURCE_REMOVE;
}


static void on_mpv_update(void *ctx)
{
    PlayerApp *pa = ctx;

    /*
     * IMPORTANTE:
     *
     * Este callback pode ser chamado pela thread do mpv.
     * Não chamamos GTK diretamente aqui.
     *
     * Apenas agendamos o queue_render() na main loop do GTK.
     */

    g_main_context_invoke(
        NULL,
        mpv_queue_render_idle,
        pa
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

    mpv_set_option_string(
        pa->mpv,
        "terminal",
        "yes"
    );

    mpv_set_option_string(
        pa->mpv,
        "msg-level",
        "all=warn"
    );

    /* --------------------------------------------------------
     * MPV sem janela própria
     * -------------------------------------------------------- */

    int status = mpv_set_option_string(
        pa->mpv,
        "vo",
        "libmpv"
    );

    if (status < 0) {
        fprintf(stderr,
                "[MPV] ERRO: vo=libmpv: %s\n",
                mpv_error_string(status));

        mpv_terminate_destroy(pa->mpv);
        pa->mpv = NULL;

        return;
    }

    fprintf(stderr,
            "[MPV] vo=libmpv configurado\n");


    /* --------------------------------------------------------
     * LOOP DO ARQUIVO
     * -------------------------------------------------------- */

    status = mpv_set_option_string(
        pa->mpv,
        "loop-file",
        "yes"
    );

    if (status < 0) {
        fprintf(stderr,
                "[MPV] ERRO: loop-file=yes: %s\n",
                mpv_error_string(status));
    } else {
        fprintf(stderr,
                "[MPV] loop-file=yes configurado\n");
    }


    /* --------------------------------------------------------
     * Decodificação por hardware
     * -------------------------------------------------------- */

    status = mpv_set_option_string(
        pa->mpv,
        "hwdec",
        "auto"
    );

    if (status < 0) {
        fprintf(stderr,
                "[MPV] ERRO: hwdec=auto: %s\n",
                mpv_error_string(status));

        mpv_terminate_destroy(pa->mpv);
        pa->mpv = NULL;

        return;
    }

    fprintf(stderr,
            "[MPV] hwdec=auto configurado\n");


    /* --------------------------------------------------------
     * INICIALIZA MPV
     * -------------------------------------------------------- */

    status = mpv_initialize(pa->mpv);

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

    if (pa->mpv_event_source) {

        g_source_remove(
            pa->mpv_event_source
        );

        pa->mpv_event_source = 0;
    }

    if (pa->mpv_render) {

        fprintf(stderr,
                "[MPV-RENDER] destruindo render context\n");

        mpv_render_context_free(
            pa->mpv_render
        );

        pa->mpv_render = NULL;
    }

    if (pa->mpv) {

        fprintf(stderr,
                "[MPV] destruindo mpv\n");

        mpv_terminate_destroy(
            pa->mpv
        );

        pa->mpv = NULL;
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

    /*
     * GtkGLArea pode receber um render enquanto
     * ainda possui tamanho 0x0.
     */
    if (width <= 0 || height <= 0) {
        fprintf(stderr,
                "[GTK-RENDER] tamanho inválido: %dx%d\n",
                width,
                height);

        return TRUE;
    }

    /*
     * Ainda não temos render context do mpv.
     */
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

    /*
     * Viewport do GtkGLArea.
     */
    glViewport(
        0,
        0,
        width,
        height
    );

    /*
     * FBO atualmente fornecido pelo GtkGLArea.
     */
    GLint current_fbo = 0;

    glGetIntegerv(
        GL_DRAW_FRAMEBUFFER_BINDING,
        &current_fbo
    );

    /*
     * FBO usado pelo libmpv.
     */
    mpv_opengl_fbo fbo = {
        .fbo = current_fbo,
        .w = width,
        .h = height,
        .internal_format = 0
    };

    int flip_y = 1;

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

    /*
     * Renderiza o frame.
     */
    int status =
        mpv_render_context_render(
            pa->mpv_render,
            params
        );

    if (status < 0) {

        fprintf(stderr,
                "[MPV-RENDER] ERRO: %s\n",
                mpv_error_string(status));
    }

    /*
     * IMPORTANTE:
     *
     * Informa ao libmpv que o frame foi
     * entregue ao ciclo de apresentação.
     */
    mpv_render_context_report_swap(
        pa->mpv_render
    );

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


static gboolean grab_gl_focus(gpointer data)
{
    PlayerApp *pa = data;

    fprintf(stderr,
            "[gtk] tentando colocar foco no GtkGLArea\n");

    gtk_widget_grab_focus(pa->gl_area);

    fprintf(stderr,
            "[gtk] foco GtkGLArea = %d\n",
            gtk_widget_has_focus(pa->gl_area));

    return G_SOURCE_REMOVE;
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

    /* --------------------------------------------------------
     * CONFIGURAÇÃO DO GtkGLArea
     * -------------------------------------------------------- */

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

    /* --------------------------------------------------------
     * OVERLAY
     * -------------------------------------------------------- */

    GtkWidget *overlay = gtk_overlay_new();

    /*
     * GtkGLArea fica como conteúdo principal.
     */
    gtk_overlay_set_child(
        GTK_OVERLAY(overlay),
        pa->gl_area
    );


    setup_info_label_css();

    /* --------------------------------------------------------
     * LABEL DE INFORMAÇÕES
     * -------------------------------------------------------- */

    pa->info_label = gtk_label_new(NULL);

    gtk_widget_add_css_class(
        pa->info_label,
        "video-info"
    );

    gtk_label_set_text(
        GTK_LABEL(pa->info_label),
        pa->filename
    );

    gtk_widget_set_halign(
        pa->info_label,
        GTK_ALIGN_END
    );

    gtk_widget_set_valign(
        pa->info_label,
        GTK_ALIGN_END
    );

    gtk_widget_set_margin_end(
        pa->info_label,
        15
    );

    gtk_widget_set_margin_bottom(
        pa->info_label,
        15
    );

    /*
     * Coloca o label sobre o vídeo.
     */
    gtk_overlay_add_overlay(
        GTK_OVERLAY(overlay),
        pa->info_label
    );


    /* --------------------------------------------------------
     * COLOCA O OVERLAY NA JANELA
     * -------------------------------------------------------- */

    gtk_window_set_child(
        GTK_WINDOW(pa->window),
        overlay
    );

    /* --------------------------------------------------------
     * TECLADO
     * -------------------------------------------------------- */

    gtk_widget_set_focusable(
        pa->gl_area,
        TRUE
    );

    GtkEventControllerKey *key_controller =
        GTK_EVENT_CONTROLLER_KEY(
            gtk_event_controller_key_new()
        );

    gtk_event_controller_set_propagation_phase(
        GTK_EVENT_CONTROLLER(key_controller),
        GTK_PHASE_CAPTURE
    );

    g_signal_connect(
        key_controller,
        "key-pressed",
        G_CALLBACK(on_key_pressed),
        pa
    );

    /*
     * O controlador fica na GtkWindow.
     * Como está em CAPTURE, ele deve observar
     * os eventos antes de chegarem ao widget focado.
     */
    gtk_widget_add_controller(
        pa->window,
        GTK_EVENT_CONTROLLER(key_controller)
    );

    fprintf(stderr,
            "[gtk] controlador de teclado instalado na WINDOW\n");


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
        FALSE
    );

    /* --------------------------------------------------------
     * MOSTRA A JANELA
     * -------------------------------------------------------- */

    fprintf(stderr,
            "[gtk] chamando gtk_window_present()\n");

    gtk_window_present(
        GTK_WINDOW(pa->window)
    );

    pa->mpv_event_source =
        g_timeout_add(
            10,
            mpv_event_timer,
            pa
        );

    fprintf(stderr,
            "[gtk] janela apresentada\n");

    g_idle_add(
        grab_gl_focus,
        pa
    );

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
