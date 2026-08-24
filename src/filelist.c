#include "filelist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>


/* ============================================================
 * ITEM
 * ============================================================ */

typedef struct {

    char *path;

    time_t mtime;

} FileItem;


/* ============================================================
 * FILE LIST
 * ============================================================ */

struct _FileList {

    FileItem *items;

    size_t count;

    size_t capacity;
};


/* ============================================================
 * CRESCE LISTA
 * ============================================================ */

static int filelist_grow(
    FileList *list)
{
    if (list->count < list->capacity)
        return 0;


    size_t new_capacity;


    if (list->capacity == 0)
        new_capacity = 64;
    else
        new_capacity = list->capacity * 2;


    FileItem *items =
        realloc(
            list->items,
            new_capacity * sizeof(FileItem)
        );


    if (!items)
        return -1;


    list->items = items;

    list->capacity = new_capacity;


    return 0;
}


/* ============================================================
 * ADICIONA
 * ============================================================ */

static int filelist_add(
    FileList *list,
    const char *path,
    time_t mtime)
{
    if (!list ||
        !path)
        return -1;


    if (filelist_grow(list) < 0)
        return -1;


    char *copy =
        strdup(path);


    if (!copy)
        return -1;


    list->items[list->count].path =
        copy;

    list->items[list->count].mtime =
        mtime;

    list->count++;


    return 0;
}


/* ============================================================
 * ADICIONA PATH
 * ============================================================ */

int filelist_add_path(
    FileList *list,
    const char *path)
{
    if (!list ||
        !path)
        return -1;


    struct stat st;


    if (stat(path, &st) < 0) {

        fprintf(
            stderr,
            "[FILELIST] não foi possível acessar: %s: %s\n",
            path,
            strerror(errno)
        );

        return -1;
    }


    /*
     * Somente arquivos regulares.
     */

    if (!S_ISREG(st.st_mode)) {

        fprintf(
            stderr,
            "[FILELIST] não é arquivo regular: %s\n",
            path
        );

        return -1;
    }


    return filelist_add(
        list,
        path,
        st.st_mtime
    );
}


/* ============================================================
 * CRIA
 * ============================================================ */

FileList *filelist_new(void)
{
    FileList *list =
        calloc(
            1,
            sizeof(FileList)
        );


    if (!list)
        return NULL;


    return list;
}


/* ============================================================
 * CARREGA DIRETÓRIO
 * ============================================================ */

int filelist_load_directory(
    FileList *list,
    const char *directory)
{
    if (!list ||
        !directory)
        return -1;


    DIR *dir =
        opendir(directory);


    if (!dir) {

        fprintf(
            stderr,
            "[FILELIST] ERRO abrindo pasta '%s': %s\n",
            directory,
            strerror(errno)
        );

        return -1;
    }


    struct dirent *entry;


    while ((entry = readdir(dir)) != NULL) {

        /*
         * Ignora somente "." e "..".
         *
         * Arquivos ocultos continuam
         * sendo incluídos.
         */

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;


        char path[PATH_MAX];


        int written =
            snprintf(
                path,
                sizeof(path),
                "%s/%s",
                directory,
                entry->d_name
            );


        if (written < 0 ||
            (size_t)written >= sizeof(path)) {

            fprintf(
                stderr,
                "[FILELIST] caminho muito longo: %s\n",
                entry->d_name
            );

            continue;
        }


        /*
         * Tenta adicionar.
         *
         * Se não for um arquivo regular,
         * simplesmente continua.
         */

        if (filelist_add_path(
                list,
                path
            ) < 0) {

            continue;
        }
    }


    closedir(dir);


    return 0;
}


/* ============================================================
 * COMPARADOR NOME
 * ============================================================ */

static int compare_name(
    const void *a,
    const void *b)
{
    const FileItem *fa = a;

    const FileItem *fb = b;


    return strcmp(
        fa->path,
        fb->path
    );
}


/* ============================================================
 * COMPARADOR TEMPO
 * ============================================================ */

static int compare_time(
    const void *a,
    const void *b)
{
    const FileItem *fa = a;

    const FileItem *fb = b;


    if (fa->mtime < fb->mtime)
        return -1;


    if (fa->mtime > fb->mtime)
        return 1;


    return strcmp(
        fa->path,
        fb->path
    );
}


/* ============================================================
 * ORDENA
 * ============================================================ */

void filelist_sort(
    FileList *list,
    FileListOrder order)
{
    if (!list ||
        list->count < 2)
        return;


    switch (order) {

        case FILELIST_ORDER_NAME:

            qsort(
                list->items,
                list->count,
                sizeof(FileItem),
                compare_name
            );

            break;


        case FILELIST_ORDER_TIME:

            qsort(
                list->items,
                list->count,
                sizeof(FileItem),
                compare_time
            );

            break;


        case FILELIST_ORDER_DEFAULT:

        default:

            /*
             * Não altera a ordem encontrada
             * pelo diretório.
             */

            break;
    }
}


/* ============================================================
 * COUNT
 * ============================================================ */

size_t filelist_count(
    const FileList *list)
{
    if (!list)
        return 0;


    return list->count;
}


/* ============================================================
 * GET
 * ============================================================ */

const char *filelist_get(
    const FileList *list,
    size_t index)
{
    if (!list ||
        index >= list->count)
        return NULL;


    return list->items[index].path;
}


/* ============================================================
 * PRINT
 * ============================================================ */

void filelist_print(
    const FileList *list)
{
    if (!list)
        return;


    printf(
        "\n"
        "========================================\n"
        " FILELIST\n"
        "========================================\n"
    );


    printf(
        "Arquivos encontrados: %zu\n\n",
        list->count
    );


    for (size_t i = 0;
         i < list->count;
         i++) {

        printf(
            "%4zu  %s\n",
            i,
            list->items[i].path
        );
    }


    printf(
        "========================================\n\n"
    );
}


/* ============================================================
 * FREE
 * ============================================================ */

void filelist_free(
    FileList *list)
{
    if (!list)
        return;


    for (size_t i = 0;
         i < list->count;
         i++) {

        free(
            list->items[i].path
        );
    }


    free(
        list->items
    );


    free(
        list
    );
}
