# GROUPED IMAGEM AND VIDEO (gpiv)

```bash
make clean
make
./gpiv tst1.mp4
```


---

### Mapa de comandos

| Tecla             | Comando          | Comportamento             |
| ----------------- | ---------------- | ------------------------- |
| `q` / `Q`         | Sair             | Fecha o `gpiv`            |
| `Space` / `Enter` | Play/Pause       | Pausa ou continua o vídeo |
| `j`               | Próximo arquivo  | Avança 1 arquivo          |
| `k`               | Arquivo anterior | Volta 1 arquivo           |
| `h`               | Salto +20        | Avança 20 arquivos        |
| `l`               | Salto -20        | Volta 20 arquivos         |
| `n` / `N`         | Frame +1         | Avança 1 frame            |
| `.`               | Frame -1         | Volta 1 frame             |
| `m`               | Seek +3s         | Avança 3 segundos         |
| `,`               | Seek -3s         | Volta 3 segundos          |
| `M`               | Seek +10s        | Avança 10 segundos        |
| `<`               | Seek -10s        | Volta 10 segundos         |
| `V`               | Volume +5        | Aumenta volume            |
| `C`               | Volume -5        | Diminui volume            |
| `X`               | Volume 0         | Coloca volume em zero     |
| `y` / `Y`         | Copiar path      | Copia o caminho do vídeo  |
| `r` / `R`         | Rotacionar       | Rotaciona vídeo 90°       |
| `z`               | Reset view       | Reseta zoom e pan         |
| `Esc`             | Cancelar leader  | Desativa `f` e `t`        |

### Leader `t` — Tags

```text
t
```

Ativa o leader de tags.

Depois:

| Sequência | Resultado      |
| --------- | -------------- |
| `t` + `a` | Salva tag `ta` |
| `t` + `b` | Salva tag `tb` |
| `t` + `c` | Salva tag `tc` |
| ...       | ...            |
| `t` + `z` | Salva tag `tz` |

Depois da letra, o leader `t` é automaticamente desligado.

Exemplo:

```text
t → a
```

salva:

```text
arquivo.mp4,ta
```

---

### Leader `f`

Primeiro:

```text
f
```

ativa o leader `f`.

Depois:

| Sequência | Função                                              |
| --------- | --------------------------------------------------- |
| `fb`      | Brightness                                          |
| `fc`      | Contrast                                            |
| `fs`      | Saturation                                          |
| `fg`      | Gamma                                               |
| `fv`      | Volume                                              |
| `fp`      | Screenshot                                          |
| `fz`      | Zoom                                                |
| `fr`      | Criar diretórios das tags                           |
| `fR`      | Pré-validar/mover arquivos para diretórios das tags |

---

### Leader `fb` — Brightness

```text
f → b
```

Depois:

| Tecla     | Ação          |
| --------- | ------------- |
| `u` / `U` | Brightness +1 |
| `i` / `I` | Brightness -1 |
| Scroll ↑  | Brightness +1 |
| Scroll ↓  | Brightness -1 |

---

### Leader `fc` — Contrast

```text
f → c
```

| Tecla     | Ação        |
| --------- | ----------- |
| `u` / `U` | Contrast +1 |
| `i` / `I` | Contrast -1 |
| Scroll ↑  | Contrast +1 |
| Scroll ↓  | Contrast -1 |

---

### Leader `fs` — Saturation

```text
f → s
```

| Tecla     | Ação          |
| --------- | ------------- |
| `u` / `U` | Saturation +1 |
| `i` / `I` | Saturation -1 |
| Scroll ↑  | Saturation +1 |
| Scroll ↓  | Saturation -1 |

---

### Leader `fg` — Gamma

```text
f → g
```

| Tecla     | Ação     |
| --------- | -------- |
| `u` / `U` | Gamma +1 |
| `i` / `I` | Gamma -1 |
| Scroll ↑  | Gamma +1 |
| Scroll ↓  | Gamma -1 |

---

### Leader `fv` — Volume

```text
f → v
```

| Ação     | Resultado |
| -------- | --------- |
| Scroll ↑ | Volume +5 |
| Scroll ↓ | Volume -5 |

---

### Leader `fz` — Zoom

```text
f → z
```

| Tecla/Ação | Resultado |
| ---------- | --------- |
| `u` / `U`  | Zoom +    |
| `i` / `I`  | Zoom -    |
| Scroll ↑   | Zoom +    |
| Scroll ↓   | Zoom -    |

O `z` sozinho, fora do leader, continua sendo:

```text
z
```

→ reset de zoom/pan.

---

### Leader `fp` — Screenshot

```text
f → p
```

Executa imediatamente o screenshot.

O resultado aparece no label, por exemplo:

```text
screenshot salvo: /caminho/arquivo.jpg
```

---

### `fr` — Criar pastas das tags

```text
f → r
```

Percorre as tags existentes no `tags.csv`, elimina duplicadas e cria:

```text
/videos/ta/
/videos/tb/
/videos/tc/
```

Se a pasta já existir, ela não é recriada.

---

### `fR` — Organizar arquivos pelas tags

```text
f → R
```

Esse é o comando que você está implementando agora.

A intenção é:

```text
/videos/
├── video1.mp4   → ta
├── video2.mp4   → tb
├── video3.mp4   → ta
└── tags.csv
```

resultar em:

```text
/videos/
├── ta/
│   ├── video1.mp4
│   └── video3.mp4
├── tb/
│   └── video2.mp4
└── tags202608271657.md
```

Com colisão:

```text
video1.mp4
video1_bis_1.mp4
video1_bis_2.mp4
...
```

E a ideia é que o `fR` faça **pré-validação antes de mover qualquer arquivo**.

---

### Scroll normal

Sem nenhum leader ativo, o scroll não executa alteração.

Com leader:

```text
fb → brightness
fc → contrast
fs → saturation
fg → gamma
fv → volume
fz → zoom
```

---

### Arrastar com mouse

O `GtkGestureDrag` está configurado para:

```text
arrastar → pan do vídeo
```

Com:

```text
drag-begin
    ↓
player_pan_begin()

drag-update
    ↓
player_pan_update()

drag-end
    ↓
player_pan_end()
```

---

### Resumo visual

```text
CONTROLS
│
├── q/Q          sair
├── Space/Enter  play/pause
│
├── j            próximo arquivo
├── k            arquivo anterior
├── h            +20 arquivos
├── l            -20 arquivos
│
├── n/N          próximo frame
├── .            frame anterior
│
├── m            +3s
├── ,            -3s
├── M            +10s
├── <            -10s
│
├── V            volume +5
├── C            volume -5
├── X            volume 0
├── y/Y          copiar path
├── r/R          rotacionar
├── z            reset zoom/pan
│
├── t
│   └── letra    tag (ta, tb, tc...)
│
├── f
│   ├── b        brightness
│   │   ├── u    +
│   │   ├── i    -
│   │   └── scroll
│   │
│   ├── c        contrast
│   │   ├── u    +
│   │   ├── i    -
│   │   └── scroll
│   │
│   ├── s        saturation
│   │   ├── u    +
│   │   ├── i    -
│   │   └── scroll
│   │
│   ├── g        gamma
│   │   ├── u    +
│   │   ├── i    -
│   │   └── scroll
│   │
│   ├── v        volume
│   │   └── scroll
│   │
│   ├── p        screenshot
│   │
│   ├── z        zoom
│   │   ├── u    +
│   │   ├── i    -
│   │   └── scroll
│   │
│   ├── r        criar diretórios das tags
│   │
│   └── R        mover arquivos pelas tags
│
└── Esc          cancelar leaders
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



