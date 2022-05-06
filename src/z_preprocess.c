#include <preZ.h>
#include <stdlib.h>
#include <xstring/xstring.h>

static inline char** z_preprocess_first_pass(const char* restrict str)
{
    char** lines = z_preprocess_text_process(str);
    if (!lines) {
        return NULL;
    }

    char** tokens = z_preprocess_tokens(lines);
    x_strget_free(lines);

    return tokens;
}

char** z_preprocess_text(const char* restrict text, map_t* restrict defines)
{
    char** tokens = z_preprocess_first_pass(text);
    if (!tokens) {
        return NULL;
    }

    char** directives = z_preprocess_directives(tokens, defines);
    free(tokens);

    return directives;
}

char** z_preprocess_file(const char* restrict filename, map_t* restrict defines)
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

    char** tokens = z_preprocess_text(text, defines);
    free(text);

    return tokens;
}