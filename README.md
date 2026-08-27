# GROUPED IMAGEM AND VIDEO (gpiv)

```bash
make clean
make
./gpiv tst1.mp4
```

---
### Estado atual

* **`player.c/h`** → mpv, reprodução, zoom, pan, rotate, brightness, screenshot.
* **`render.c/h`** → GtkGLArea + OpenGL + `mpv_render_context`.
* **`ui.c/h`** → widgets, overlay, label e mensagens.
* **`controls.c/h`** → teclado, mouse, scroll, drag.
* **`app.c/h`** → criação da aplicação e integração dos módulos.
* **`main.c`** → entrada do programa.

## Mapa atual do `controls.c`

### 1. Comandos normais

| Tecla             | Ação                 |
| ----------------- | -------------------- |
| `q` / `Q`         | Sair do programa     |
| `j`               | Próximo arquivo      |
| `k`               | Arquivo anterior     |
| `h`               | Avança 20 arquivos   |
| `l`               | Volta 20 arquivos    |
| `Space` / `Enter` | Play / Pause         |
| `n` / `N`         | Próximo frame        |
| `.`               | Frame anterior       |
| `m`               | Seek +3 segundos     |
| `,`               | Seek -3 segundos     |
| `M`               | Seek +10 segundos    |
| `<`               | Seek -10 segundos    |
| `V`               | Volume +5            |
| `C`               | Volume -5            |
| `X`               | Volume 0             |
| `y` / `Y`         | Copiar path do vídeo |
| `r` / `R`         | Rotacionar vídeo     |
| `z`               | Reset zoom/pan       |

---

# 2. Leader `f`

O `f` funciona como um **prefixo**.

```text
f + tecla
```

Atualmente temos:

| Comando | Ação                                      |
| ------- | ----------------------------------------- |
| `f b`   | Ativa controle de brightness              |
| `f c`   | Ativa controle de contrast                |
| `f s`   | Ativa controle de saturation              |
| `f g`   | Ativa controle de gamma                   |
| `f v`   | Ativa controle de volume                  |
| `f z`   | Ativa controle de zoom                    |
| `f r`   | Cria diretórios das tags                  |
| `f p`   | Screenshot                                |

Então:

```text
fb → brightness
fc → contrast
fs → saturation
fg → gamma
fv → volume
fz → zoom
fr → criar pastas das tags
fp → mover arquivos tagueados   ← NOVO
```

---

# 3. `f` + scroll

O leader `f` também modifica o comportamento do scroll.

| Comando | Scroll para cima | Scroll para baixo |
| ------- | ---------------: | ----------------: |
| `fb`    |     brightness + |      brightness - |
| `fc`    |       contrast + |        contrast - |
| `fs`    |     saturation + |      saturation - |
| `fg`    |          gamma + |           gamma - |
| `fv`    |        volume +5 |         volume -5 |
| `fz`    |             zoom |              zoom |

Então, por exemplo:

```text
fb + scroll ↑
```

aumenta brightness.

E:

```text
fz + scroll ↑/↓
```

controla o zoom.

---

# 4. Leader `t`

O `t` é diferente do `f`.

Ele é um **leader temporário de tags**.

```text
t
```

ativa:

```c
controls->leadt = TRUE;
```

Depois uma letra:

```text
t + a
```

gera:

```text
ta
```

e salva a tag.

Exemplos:

```text
ta
tb
tc
td
te
...
tz
```

O comportamento desejado atualmente é:

```text
t
 ↓
leadt = TRUE

a
 ↓
tag = "ta"
 ↓
salva
 ↓
leadt = FALSE
```

Ou seja, `t` sozinho não cria tag.

---

# 5. Escape

Temos também:

```text
Esc
```

que cancela os leaders.

Conceitualmente:

```text
Esc
 ↓
leadf = FALSE
leadt = FALSE
leadfvar = '\0'
```

Isso é importante porque evita ficar preso em um modo de leader.

---

# 6. Scroll sem leader

Quando não existe:

```text
fz
fb
fc
...
```

o scroll segue o comportamento normal do programa.

O código atualmente também deixa explícito que:

```text
Leader T não usa scroll.
```



---

# Mapa visual completo

Eu organizaria mentalmente o `controls.c` assim:

```text
                    TECLADO
                       │
          ┌────────────┴────────────┐
          │                         │
       NORMAL                    LEADERS
          │                         │
          │                  ┌──────┴──────┐
          │                  │             │
          │                  f             t
          │                  │             │
          │          ┌───────┼───────┐     │
          │          │       │       │     │
          │         fb      fc      fs    ta
          │         fg      fv      fz    tb
          │         fr      fp            tc
          │                               ...
          │
          ├── q       sair
          ├── j       próximo arquivo
          ├── k       anterior
          ├── h       +20 arquivos
          ├── l       -20 arquivos
          ├── Space   play/pause
          ├── n       próximo frame
          ├── .       frame anterior
          ├── m       +3s
          ├── ,       -3s
          ├── M       +10s
          ├── <       -10s
          ├── V       volume +
          ├── C       volume -
          ├── X       mute
          ├── y       copiar path
          ├── r       rotate
          └── z       reset view
```

---

# E especificamente para as tags

Temos uma divisão muito boa:

```text
t + letra
    │
    └── TAGUEAR arquivo

f + r
    │
    └── CRIAR pastas das tags

f + p
    │
    └── MOVER arquivos para pastas das tags
```

Isso dá uma lógica bastante clara:

```text
                    TAGS
                     │
          ┌──────────┼──────────┐
          │          │          │
          t          fr         fp
          │          │          │
       taguear     criar       mover
                    pastas      arquivos
```

### Exemplo completo

Começamos com:

```text
videos/
├── a.mp4
├── b.mp4
├── c.mp4
└── tags.csv
```

Você faz:

```text
t a
```

→ `a.mp4` recebe `ta`

Depois:

```text
t b
```

→ `b.mp4` recebe `tb`

Então:

```text
fr
```

cria:

```text
videos/
├── a.mp4
├── b.mp4
├── c.mp4
├── ta/
├── tb/
└── tags.csv
```

Finalmente:

```text
fp
```

faz:

```text
videos/
├── c.mp4
├── ta/
│   └── a.mp4
├── tb/
│   └── b.mp4
└── tags.csv
```

Esse mapa deixa o próximo passo bem definido: **não precisamos criar outro sistema de comandos**. Basta transformar o significado de `fp` de screenshot para **"processar/mover arquivos tagueados"**, mantendo `fr` como **"criar diretórios"** e `t + letra` como **"taguear"**.
