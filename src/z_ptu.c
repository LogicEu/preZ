#include <preZ.h>
#include <stdlib.h>
#include <xstring/xstring.h>

ptu_t* z_ptu_preprocess_text(const char* restrict filename, const char* restrict text)
{
    map_t defines = map_create(sizeof(char*), sizeof(char**));
    char** tokens = z_preprocess_text(text, &defines);

    if (!tokens) {
        map_free(&defines);
        return NULL;
    }

    ptu_t* ptu = malloc(sizeof(ptu_t));
    ptu->filename = x_strdup(filename);
    ptu->defines = defines;
    ptu->tokens = tokens;
    
    return ptu;
}

ptu_t* z_ptu_preprocess_file(const char* filename)
{
    if (NULLSTR(filename)) {
        z_log("File name is invalid or null.\n");
        return NULL;
    }

    char* text = z_file_stream(filename);
    if (!text) {
        z_log("Could not found file '%s'.\n", filename);
        return NULL;
    }

    ptu_t* ptu = z_ptu_preprocess_text(filename, text);
    free(text);

    return ptu;
}

void z_ptu_free(ptu_t* ptu)
{
    if (!ptu) {
        return;
    }
    
    free(ptu->filename);
    x_strget_free(ptu->tokens);
    map_free(&ptu->defines);
    free(ptu);
}