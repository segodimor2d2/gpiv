# GROUPED IMAGEM AND VIDEO (gpiv)

```bash
make clean
make
./gpiv tst1.mp4
```

---

## estado assim

| Componente                    | Estado |
| ----------------------------- | ------ |
| GTK4                          | ✅      |
| GtkGLArea                     | ✅      |
| OpenGL                        | ✅      |
| AMD/Mesa                      | ✅      |
| `LC_NUMERIC=C`                | ✅      |
| `mpv_create()`                | ✅      |
| `mpv_initialize()`            | ✅      |
| `mpv_render_context_create()` | ✅      |
| `loadfile`                    | ✅      |
| abertura do arquivo           | ✅      |
| decoder                       | ✅      |
| geração de frames             | ✅      |
| update callback               | ✅      |
| `gtk_gl_area_queue_render()`  | ✅      |
| `GtkGLArea::render`           | ✅      |
| `mpv_render_context_render()` | ✅      |
| FBO 0                         | ✅      |
| vídeo na tela                 | **✅**  |
| reprodução até o fim          | **✅**  |
