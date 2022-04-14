#include <preZ.h>
#include <stdlib.h>
#include <string.h>
#include <utopia/utopia.h>
#include <xstring/xstring.h>

extern char* z_token_string_literal(const char* str, const char* symbol);

/* Pass 1: Line Splicing */
static char** z_preprocess_splice(const char* str)
{
    char* line;
    char* ch = (char*)(size_t)str;
    char* prev = ch;

    array_t lines = array_create(sizeof(char*));
    
    while ((ch = strstr(ch, "\n"))) {
        char* line = x_strdup_range(prev, ch);
        if (!line) line = calloc(2, sizeof(char));

        array_push(&lines, &line);
        prev = ++ch;
    }

    line = x_strdup(prev);
    array_push(&lines, &line);
    
    line = NULL;
    array_push(&lines, &line);

    array_cut(&lines);
    return lines.data;
}

/* Pass 2: Tirigrapgh Replacement */
static int z_preprocess_tirigraph(char** lines)
{
    #define TIRIGRAPH_COUNT 9

    static const char tirigraphs[TIRIGRAPH_COUNT] = {'=', '/',  '\'', '(', ')', '!', '<', '>', '-'};
    static const char replace[TIRIGRAPH_COUNT] =    {'#', '\\', '^',  '[', ']', '|', '{', '}', '~'};
    static const char* tirtok = "??";

    const size_t tirlen = strlen(tirtok);
    
    for (size_t i = 0; lines[i]; ++i) {
        char* ch = lines[i];
        while ((ch = strstr(ch, tirtok))) {
            
            const char c = ch[tirlen];
            char r = '\0';

            for (size_t j = 0; j < TIRIGRAPH_COUNT; ++j) {
                if (c == tirigraphs[j]) {
                    r = replace[j];
                    break;
                }
            }

            if (!r) {
                z_log("preZ warning: using multiple question marks ?? with invalid tirigraph replacement in line %zu.\n'%s'\n", i + 1, lines[i]);
                continue;
            }

            memset(ch, r, sizeof(char));
            memmove(ch + 1, ch + tirlen + 1, strlen(ch + tirlen + 1) + 1);
        }
    }
    
    return Z_EXIT_SUCCESS;
}

/* Pass 3: Line Joining */
static int z_preprocess_join(char** lines)
{
    for (size_t i = 0; lines[i + 1]; ++i) {
        const size_t len = strlen(lines[i]);
        if (len && lines[i][len - 1] == '\\') {
            
            char* tmp = x_strcat(lines[i], lines[i + 1]);
            if (!tmp) tmp = calloc(2, sizeof(char));
            
            free(lines[i]);
            free(lines[i + 1]);
            
            lines[i] = tmp;
            memmove(lines + i + 1, lines + i + 2, (x_strscnt(lines + i + 2) + 1) * sizeof(char*));
        }
    }
    return Z_EXIT_SUCCESS;
}

/* Pass 4: Remmove comments, account for string literals */
static int z_preprocess_comment_c(char* open, char** lines, const size_t index)
{
    static const char* close = "*/";

    char* comment_close = NULL;
    
    size_t i;
    for (i = index; lines[i]; ++i) {
        if ((comment_close = strstr(lines[i], close))) {
            break;
        }
    }

    if (!comment_close) {
        return Z_EXIT_FAILURE;
    }
    
    comment_close += strlen(close);
    
    *open = '\0';
    char* tmp = x_strcat_spaced(lines[index], comment_close);
    if (!tmp) tmp = calloc(2, sizeof(char));

    for (size_t j = index; j <= i; ++j) {
        free(lines[j]);
    }

    lines[index] = tmp;
    memmove(lines + index + 1, lines + i + 1, (x_strscnt(lines + i + 1) + 1) * sizeof(char*));

    return Z_EXIT_SUCCESS;
}

static char* z_token_block_next(const char* str)
{
    char* comment = x_strmin(strstr(str, "/*"), strstr(str, "//"));
    char* block = x_strmin(strstr(str, "\""), strstr(str, "'"));
    return x_strmin(comment, block);
}

static int z_preprocess_blocks(char** lines)
{
    for (size_t i = 0; lines[i]; ++i) {
        char* s = lines[i];
        
        while ((s = z_token_block_next(s))) {
            if (s[0] == '"') {
                if (!(s = z_token_string_literal(s, "\""))) {
                    z_log("preZ error: Unclosed \"\" block at line %zu.\n[%s]\n", i, lines[i]);
                    return Z_EXIT_FAILURE;
                }
            }
            else if (s[0] == '\'') {
                if (!(s = z_token_string_literal(s, "'"))) {
                    z_log("preZ error: Unclosed '' block at line %zu.\n[%s]\n", i, lines[i]);
                    return Z_EXIT_FAILURE;
                }
            }
            else if (s[1] == '*') {
                if (z_preprocess_comment_c(s, lines, i)) {
                    z_log("preZ error: Unclosed /* comment on line %zu.\n[%s]\n", i, lines[i]);
                    return Z_EXIT_FAILURE;
                }
            } 
            else if (s[1] == '/') {
                s[0] = ' ';
                s[1] = '\0';
            } 
        }
    }

    return Z_EXIT_SUCCESS;
}

/* First Preprocessing Pass: Text Transformation */

char** z_preprocess_text_process(const char* str)
{
    /* Pass 1: Line splicing */
    char** lines = z_preprocess_splice(str); 
    if (!lines) {
        goto z_preprocess_text_fatal;
    }

    /* Pass 2: Tirigrapgh replacement */
    if (z_preprocess_tirigraph(lines)) {
        goto z_preprocess_text_fail;
    }

    /* Pass 3: Join continued lines */
    if (z_preprocess_join(lines)) {
        goto z_preprocess_text_fail;
    }

    /* Pass 4: Comment out accounting for string literals */
    if (z_preprocess_blocks(lines)) {
        goto z_preprocess_text_fail;
    }

    return lines;

z_preprocess_text_fail:
    x_strget_free(lines);
z_preprocess_text_fatal:
    return NULL;
}