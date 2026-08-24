#ifndef FILELIST_H
#define FILELIST_H

#include <stddef.h>
#include <time.h>


typedef struct _FileList FileList;


/* ============================================================
 * ORDEM
 * ============================================================ */

typedef enum {

    FILELIST_ORDER_DEFAULT,
    FILELIST_ORDER_NAME,
    FILELIST_ORDER_TIME

} FileListOrder;


/* ============================================================
 * CRIAÇÃO / DESTRUIÇÃO
 * ============================================================ */

FileList *filelist_new(void);

void filelist_free(
    FileList *list
);


/* ============================================================
 * CARREGAMENTO
 * ============================================================ */

int filelist_load_directory(
    FileList *list,
    const char *directory
);


/*
 * Adiciona diretamente um arquivo.
 *
 * Usado quando:
 *
 *     ./gpiv arquivo.mp4
 *
 * ou quando no futuro recebermos
 * arquivos individuais pela linha de comando.
 */

int filelist_add_path(
    FileList *list,
    const char *path
);


/* ============================================================
 * ORDENAÇÃO
 * ============================================================ */

void filelist_sort(
    FileList *list,
    FileListOrder order
);


/* ============================================================
 * CONSULTA
 * ============================================================ */

size_t filelist_count(
    const FileList *list
);

const char *filelist_get(
    const FileList *list,
    size_t index
);


/* ============================================================
 * DEBUG
 * ============================================================ */

void filelist_print(
    const FileList *list
);

#endif
