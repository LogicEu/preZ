/*#include <preZ.h>
#include <stdlib.h>
#include <string.h>
#include <utopia/utopia.h>
#include <xstring/xstring.h>*/

/* Preprocessor #include */

/*static char* z_preprocess_include_path_extract(char* restrict str, const char* open, const char* end, const char* new_line)
{
    char* ret = NULL;
    
    char* parenth = strstr(str, open);
    if (!parenth) {
        z_log("preZ error: Missing file path in #include macro directive.\n");
        return NULL;
    }

    if (!new_line || parenth < new_line) {
        char* close = strstr(parenth, end);
        if (!close || (new_line && close > new_line)) {
            z_log("preZ error: Unclosed #include %s directive\n", open);
            return NULL;
        }
        ret = x_strdup_range(parenth, ++close);
    } else {
        z_log("preZ error: Unclosed #include %s directive\n", open);
        return NULL;
    }

    return ret;
}

static char* z_preprocess_include_replace_text(const char* str, const char* ptr, const char* restrict buff, const size_t size)
{
    const size_t buffsize = strlen(buff);
    const size_t strsize = strlen(str);
    const size_t addsize = buffsize + strsize;

    char* add = calloc(addsize + 1, sizeof(char));
    memcpy(add, str, strsize + 1);

    char* addptr = add + (size_t)(ptr - str);
    char* addinc = addptr + size + 1;
    char* addbuff = addptr + buffsize;
    memmove(addbuff, addinc, strlen(addinc) + 1);
    memcpy(addptr, buff, buffsize);

    return add;
}

static char** z_preprocess_include_replace(char** lines, const char* ptr, char* restrict filename, const size_t size)
{
    const char c = *filename;
    const size_t filename_size = strlen(filename) - 1;
    filename[filename_size] = '\0';
    memmove(filename, filename + 1, filename_size);

    char* buff = NULL;

    if (c == '<') {
        
        const size_t include_size = include_paths.size;
        const char** paths = include_paths.data;
        char* tmp;
        
        for (size_t i = 0; i < include_size; ++i) {
            tmp = x_strcat(paths[i], filename);
            if (!tmp) {
                continue;
            }

            z_log("Searching in: %s\n", tmp);

            buff = x_file_stream_read(tmp);
            free(tmp);

            if (buff) {
                break;
            }
        }
    }
    else buff = z_file_stream(filename);

    if (!buff) {
        z_log("preZ error: Unresolved path for #include '%s'\n", filename);
        return NULL;
    }

    char** bufflines = z_preprocess_text(buff);
    if (!bufflines) {
        return NULL;
    }
    free(buff);

    //char* add = z_preprocess_include_replace_text(*str, ptr, buff, size);
    char** sum = x_strget_inject();
    x_strget_free(bufflines);
    return Z_EXIT_SUCCESS;
}

static int z_preprocess_include(char** lines)
{
    static const char* include = "#include ";
    const size_t include_size = strlen(include);

    char* s = *str;
    while ((s = strstr(s, include))) {
        char* ptr = s;
        s += include_size;

        char* new_line = strstr(s, "\n");
        char* parenth = z_preprocess_include_path_extract(s, "<", ">", new_line);
        
        if (!parenth){
            parenth = z_preprocess_include_path_extract(s, "\"", "\"", new_line);
        }

        if (!parenth) {
            z_log("preZ error: #include directive error.\n");
            return Z_EXIT_FAILURE;
        }

        if (!new_line) {
            new_line = s + strlen(s) - 1;
        }

        z_log("#include %s\n", parenth);
        if (z_preprocess_include_replace(str, ptr, parenth, new_line - ptr)) {
            s = ptr + 1;
        } 
        else s = *str;
        
        free(parenth);
    }

    return Z_EXIT_SUCCESS;
}*/

/*static char** z_preprocess_directives(char** lines)
{
    char** strs = lines;
    for (size_t i = 0; lines[i]; ++i) {
        char* ch = lines[i];
        while (*ch == '\t' || *ch == ' ') {
            ++ch;
        }

        if (*ch == '#') {

        }
    }

    return strs;
}*/
