# GROUPED IMAGEM AND VIDEO (gpiv)

```bash
make clean
make
./gpiv tst1.mp4
```

---

## A estrutura final ficaria assim

Eu visualizaria o projeto desta maneira:

```text
                         ┌──────────────┐
                         │   main.c     │
                         └──────┬───────┘
                                │
                                ▼
                         ┌──────────────┐
                         │    app.c     │
                         └──────┬───────┘
                                │
             ┌──────────────────┼──────────────────┐
             │                  │                  │
             ▼                  ▼                  ▼
       ┌───────────┐      ┌───────────┐      ┌───────────┐
       │ media.c   │      │ player.c  │      │   ui.c    │
       │           │      │           │      │           │
       │ playlist  │      │   mpv     │      │ GTK       │
       │ arquivos  │      │ comandos  │      │ widgets   │
       │ ordenação │      │ transform │      │ signals   │
       └─────┬─────┘      └─────┬─────┘      └─────┬─────┘
             │                  │                  │
             └──────────────────┼──────────────────┘
                                │
                                ▼
                         ┌──────────────┐
                         │  render.c   │
                         │ GTK + OpenGL│
                         │ + mpv render│
                         └──────────────┘

                         ┌──────────────┐
                         │ controls.c  │
                         │ teclado     │
                         │ mouse       │
                         │ scroll      │
                         └──────────────┘
```


### Estado atual

* **`player.c/h`** → mpv, reprodução, zoom, pan, rotate, brightness, screenshot.
* **`render.c/h`** → GtkGLArea + OpenGL + `mpv_render_context`.
* **`ui.c/h`** → widgets, overlay, label e mensagens.
* **`controls.c/h`** → teclado, mouse, scroll, drag.
* **`app.c/h`** → criação da aplicação e integração dos módulos.
* **`main.c`** → entrada do programa.
