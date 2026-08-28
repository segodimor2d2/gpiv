#include "control_video.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <sys/wait.h>

/* ============================================================
 * MUDAR FORMATO DO VIDEO
 * ============================================================ */

void detect_video_format(
    Controls *controls,
    const char *filename
)
{
    if (!controls || !filename || !filename[0])
        return;

    char command[PATH_MAX + 512];

    int written =
        snprintf(
            command,
            sizeof(command),

            "ffprobe -v error "
            "-show_entries "
            "format=format_name:stream="
            "codec_name,pix_fmt,width,height "
            "-select_streams v:0 "
            "-of default=noprint_wrappers=1 "
            "-- \"%s\"",

            filename
        );

    if (written < 0 ||
        (size_t)written >= sizeof(command)) {

        show_message(
            controls,
            "Erro: caminho do video muito longo"
        );

        return;
    }

    /*
     * --------------------------------------------------------
     * VIDEO
     * --------------------------------------------------------
     */

    FILE *video_pipe =
        popen(
            command,
            "r"
        );

    if (!video_pipe) {

        show_message(
            controls,
            "Erro: nao foi possivel executar ffprobe"
        );

        return;
    }

    char line[1024];

    char video_codec[256] = "";
    char pixel_format[256] = "";
    char width[64] = "";
    char height[64] = "";

    while (fgets(line, sizeof(line), video_pipe)) {

        line[strcspn(line, "\r\n")] = '\0';

        if (strncmp(line, "codec_name=", 11) == 0) {

            snprintf(
                video_codec,
                sizeof(video_codec),
                "%s",
                line + 11
            );

        } else if (strncmp(line, "pix_fmt=", 8) == 0) {

            snprintf(
                pixel_format,
                sizeof(pixel_format),
                "%s",
                line + 8
            );

        } else if (strncmp(line, "width=", 6) == 0) {

            snprintf(
                width,
                sizeof(width),
                "%s",
                line + 6
            );

        } else if (strncmp(line, "height=", 7) == 0) {

            snprintf(
                height,
                sizeof(height),
                "%s",
                line + 7
            );
        }
    }

    int video_status =
        pclose(video_pipe);

    if (video_status == -1 ||
        !WIFEXITED(video_status) ||
        WEXITSTATUS(video_status) != 0) {

        show_message(
            controls,
            "Erro: ffprobe nao conseguiu ler o video"
        );

        return;
    }

    /*
     * --------------------------------------------------------
     * CONTAINER
     * --------------------------------------------------------
     */

    written =
        snprintf(
            command,
            sizeof(command),

            "ffprobe -v error "
            "-show_entries format=format_name "
            "-of default=noprint_wrappers=1:nokey=1 "
            "-- \"%s\"",

            filename
        );

    if (written < 0 ||
        (size_t)written >= sizeof(command)) {

        show_message(
            controls,
            "Erro: comando ffprobe muito longo"
        );

        return;
    }

    FILE *format_pipe =
        popen(
            command,
            "r"
        );

    if (!format_pipe) {

        show_message(
            controls,
            "Erro ao detectar container"
        );

        return;
    }

    char format[256] = "";

    if (fgets(format, sizeof(format), format_pipe)) {

        format[strcspn(format, "\r\n")] = '\0';
    }

    int format_status =
        pclose(format_pipe);

    if (format_status == -1 ||
        !WIFEXITED(format_status) ||
        WEXITSTATUS(format_status) != 0) {

        show_message(
            controls,
            "Erro ao detectar container"
        );

        return;
    }

    /*
     * --------------------------------------------------------
     * AUDIO
     * --------------------------------------------------------
     */

    written =
        snprintf(
            command,
            sizeof(command),

            "ffprobe -v error "
            "-select_streams a:0 "
            "-show_entries stream=codec_name "
            "-of default=noprint_wrappers=1:nokey=1 "
            "-- \"%s\"",

            filename
        );

    if (written < 0 ||
        (size_t)written >= sizeof(command)) {

        show_message(
            controls,
            "Erro: comando ffprobe muito longo"
        );

        return;
    }

    FILE *audio_pipe =
        popen(
            command,
            "r"
        );

    if (!audio_pipe) {

        show_message(
            controls,
            "Erro ao detectar audio"
        );

        return;
    }

    char audio_codec[256] = "";

    if (fgets(audio_codec, sizeof(audio_codec), audio_pipe)) {

        audio_codec[
            strcspn(audio_codec, "\r\n")
        ] = '\0';
    }

    int audio_status =
        pclose(audio_pipe);

    /*
     * Não consideramos ausência de áudio como erro.
     */

    if (audio_status == -1) {

        snprintf(
            audio_codec,
            sizeof(audio_codec),
            "?"
        );
    }

    /*
     * --------------------------------------------------------
     * RESULTADO
     * --------------------------------------------------------
     */

    char message[4096];

    written =
        snprintf(
            message,
            sizeof(message),

            "formato: %s | video: %s | "
            "pixel: %s | %sx%s | audio: %s",

            format[0] ?
                format : "?",

            video_codec[0] ?
                video_codec : "?",

            pixel_format[0] ?
                pixel_format : "?",

            width[0] ?
                width : "?",

            height[0] ?
                height : "?",

            audio_codec[0] ?
                audio_codec : "?"
        );

    if (written < 0 ||
        (size_t)written >= sizeof(message)) {

        show_message(
            controls,
            "Erro: informacoes do video muito longas"
        );

        return;
    }

    show_message(
        controls,
        message
    );

    fprintf(
        stderr,
        "[fW] %s\n",
        message
    );
}



/* ============================================================
 * CONVERTER VIDEO PARA FORMATO CONVENCIONAL
 *
 * Resultado:
 *   Container: MP4
 *   Video:    H.264
 *   Pixel:    yuv420p
 *   Audio:    AAC, quando houver audio
 *
 * O arquivo original NÃO é apagado.
 * Exemplo:
 *
 *   video_original
 *       ->
 *   video_original.mp4
 * ============================================================ */

void convert_video_format(
    Controls *controls,
    const char *filename
)
{
    if (!controls || !filename || !filename[0])
        return;

    /*
     * --------------------------------------------------------
     * NOME DO ARQUIVO DE SAIDA
     * --------------------------------------------------------
     */

    char output[PATH_MAX];

    int written =
        snprintf(
            output,
            sizeof(output),
            "%s.mp4",
            filename
        );

    if (written < 0 ||
        (size_t)written >= sizeof(output)) {

        show_message(
            controls,
            "Erro: caminho do arquivo de saida muito longo"
        );

        return;
    }

    /*
     * --------------------------------------------------------
     * COMANDO FFMPEG
     *
     * -y             permite substituir um .mp4 existente
     * -i             arquivo de entrada
     * -c:v libx264   video H.264
     * -pix_fmt       yuv420p
     * -c:a aac       audio AAC
     * -movflags      MP4 otimizado
     *
     * -map 0:v:0     primeiro video
     * -map 0:a:0?    primeiro audio, se existir
     * --------------------------------------------------------
     */

    char command[PATH_MAX * 2 + 1024];

    written =
        snprintf(
            command,
            sizeof(command),

            "ffmpeg -hide_banner -loglevel error "
            "-y "
            "-i \"%s\" "
            "-map 0:v:0 "
            "-map 0:a:0? "
            "-c:v libx264 "
            "-pix_fmt yuv420p "
            "-c:a aac "
            "-movflags +faststart "
            "\"%s\"",

            filename,
            output
        );

    if (written < 0 ||
        (size_t)written >= sizeof(command)) {

        show_message(
            controls,
            "Erro: comando ffmpeg muito longo"
        );

        return;
    }

    /*
     * --------------------------------------------------------
     * EXECUTAR FFMPEG
     * --------------------------------------------------------
     */

    fprintf(
        stderr,
        "[fW] convertendo:\n"
        "     entrada: %s\n"
        "     saida:   %s\n",
        filename,
        output
    );

    int status =
        system(command);

    /*
     * --------------------------------------------------------
     * VERIFICAR RESULTADO
     * --------------------------------------------------------
     */

    if (status == -1) {

        show_message(
            controls,
            "Erro: nao foi possivel executar ffmpeg"
        );

        return;
    }

    if (!WIFEXITED(status) ||
        WEXITSTATUS(status) != 0) {

        show_message(
            controls,
            "Erro: ffmpeg falhou ao converter o video"
        );

        fprintf(
            stderr,
            "[fW] ffmpeg falhou (status=%d)\n",
            status
        );

        return;
    }

    /*
     * --------------------------------------------------------
     * SUCESSO
     * --------------------------------------------------------
     */

    char message[4096];

    written =
        snprintf(
            message,
            sizeof(message),
            "video convertido: %s",
            output
        );

    if (written < 0 ||
        (size_t)written >= sizeof(message)) {

        show_message(
            controls,
            "Video convertido com sucesso"
        );

        return;
    }

    show_message(
        controls,
        message
    );

    fprintf(
        stderr,
        "[fW] conversao concluida: %s\n",
        output
    );
}
