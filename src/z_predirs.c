#include <preZ.h>
#include <stdlib.h>
#include <string.h>
#include <utopia/utopia.h>
#include <xstring/xstring.h>

array_t include_paths;

static char** z_preprocess_include(char** line, const size_t line_count)
{
    if (line[2][0] == '\n') {
        z_log("preZ error: Missing argument for #include in line %zu.\n", line_count);
        return NULL;
    }

    char filename[1024] = {0};
    char** tokens = NULL;

    if (line[2][0] == '<') {
        
        bool closed = false;
        
        for (size_t i = 3; line[i][0] != '\n'; ++i) {
            if (line[i][0] == '>') {
                closed = true;
                break;
            }
            strcat(filename, line[i]);
        }

        if (!closed) {
            z_log("preZ error: Missing closing symbols in #include argument in line %zu.\n", line_count);
            return NULL;
        }

        char buffer[1024];
        char** includes = include_paths.data;

        for (size_t i = 0; includes[i]; ++i) {
            strcpy(buffer, includes[i]);
            strcat(buffer, filename);
            if ((tokens = z_preprocess_file(buffer))) {
                strcpy(filename, buffer);
                break;
            }
        }
    } 
    else if (line[2][0] == '"') {
        const size_t len = strlen(line[2]);
        memcpy(filename, line[2] + 1, len - 2);
        tokens = z_preprocess_file(filename);
    } 
    else {
        z_log("preZ error: Argument for #include must begin and end with either \"\" or <>. Line %zu.\n", line_count);
        return NULL;
    }
    
    if (!tokens) {
        z_log("preZ error: Unresolved path for #include %s in line %zu.\n", filename, line_count);
        return NULL;
    }

    return tokens;
}

static char** z_preprocess_directive_line(char** restrict tokens, const size_t line_count)
{
    if (!strcmp(tokens[0], "#")) {
        if (!strcmp(tokens[1], "include")) {
            return z_preprocess_include(tokens, line_count);
        }
        /*else if (!strcmp(tokens[count + 1], "define")) {

        }*/
    }

    return NULL;
}

char** z_preprocess_directives(char** tokens)
{
    void* null = NULL;
    array_t directives = array_create(sizeof(char*));

    size_t count = 0, line_count = 1;
    while (tokens[count]) {
        char** toks = z_preprocess_directive_line(tokens + count, line_count);
        if (toks) {

            for (; *tokens[count] != '\n'; ++count) {
                free(tokens[count]);
            }
            free(tokens[count++]);

            for (size_t i = 0; toks[i]; ++i) {
                array_push(&directives, &toks[i]);
            }
            free(toks);

        } else {
            for (; *tokens[count] != '\n'; ++count) {
                array_push(&directives, &tokens[count]);
            }
            array_push(&directives, &tokens[count++]);
        }
        ++line_count;
    }

    array_push(&directives, &null);
    array_cut(&directives);

    return directives.data;
}
