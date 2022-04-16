#include <preZ.h>
#include <stdlib.h>
#include <xstring/xstring.h>

char** z_preprocess_text(const char* str)
{
    char** lines = z_preprocess_text_process(str);
    if (!lines) {
        return NULL;
    }

    char** tokens = z_preprocess_tokens(lines);
    x_strget_free(lines);

    if (!tokens) {
        return NULL;
    }

    char** directives = z_preprocess_directives(tokens);;
    free(tokens);
    
    return directives;
}

char** z_preprocess_file(const char* filename)
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

    char** tokens = z_preprocess_text(text);
    free(text);

    return tokens;
}