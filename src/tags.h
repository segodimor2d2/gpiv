#ifndef TAGS_H
#define TAGS_H

#include <stddef.h>

#define TAGS_MAX 10000

typedef struct {

    char *filename;
    char tag;

} TagEntry;


typedef struct {

    TagEntry entries[TAGS_MAX];

    size_t count;

    char directory[4096];

} Tags;


/* ============================================================
 * CRIAÇÃO / DESTRUIÇÃO
 * ============================================================ */

Tags *tags_new(void);

void tags_free(
    Tags *tags
);


/* ============================================================
 * CARREGAMENTO
 * ============================================================ */

/*
 * Carrega tags.csv da pasta.
 *
 * Se tags.csv não existir, ele é criado.
 */
int tags_load(
    Tags *tags,
    const char *directory
);


/* ============================================================
 * TAG
 * ============================================================ */

/*
 * Define ou substitui a tag de um arquivo.
 */
int tags_set(
    Tags *tags,
    const char *filename,
    char tag
);


/*
 * Retorna a tag do arquivo.
 *
 * Retorna:
 *
 *     '\0' = arquivo sem tag
 *     'a'..'z' = tag
 */
char tags_get(
    Tags *tags,
    const char *filename
);


/* ============================================================
 * SALVAMENTO
 * ============================================================ */

int tags_save(
    Tags *tags
);

#endif
