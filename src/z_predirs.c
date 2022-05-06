#include <preZ.h>
#include <stdlib.h>
#include <string.h>
#include <xstring/xstring.h>

array_t include_paths;

static inline size_t z_token_count_line(char** tokens)
{
    size_t i;
    for (i = 0; *tokens[i] != '\n'; ++i) {}
    return i;
}

static char** z_preprocess_include(char** restrict line, map_t* restrict defines, const size_t line_count)
{
    if (*line[2] == '\n') {
        z_log("preZ error: Missing argument for #include in line %zu.\n", line_count);
        return NULL;
    }

    char filename[1024] = {0};
    char** tokens = NULL;

    if (*line[2] == '<') {
        
        bool closed = false;
        
        for (size_t i = 3; *line[i] != '\n'; ++i) {
            if (line[i][0] == '>') {
                if (*line[i + 1] != '\n') {
                    z_log("preZ error: Illegal token '%s' after #include directive in line %zu.\n", line[i + 1], line_count);
                    return NULL;
                }
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
            if ((tokens = z_preprocess_file(buffer, defines))) {
                strcpy(filename, buffer);
                break;
            }
        }
    } 
    else if (*line[2] == '"') {
        const size_t len = strlen(line[2]);
        memcpy(filename, line[2] + 1, len - 2);
        tokens = z_preprocess_file(filename, defines);
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

static char** z_preprocess_define(char** restrict line, map_t* restrict defines, const size_t line_count)
{
    char* identifier = line[2];
    char** expression = line + 3;

    if (*identifier == '\n') {
        z_log("preZ error: Missing token for #define in line %zu.\n", line_count);
        return NULL;
    }

    size_t search = map_search(defines, &identifier);
    if (search) {
        char* key = *(char**)map_key_at(defines, search - 1);
        if (!strcmp(key, identifier)) {
            map_remove(defines, &key);
        }
    }

    char* null = NULL;
    array_t expr = array_create(sizeof(char*));
    for (size_t i = 0; *expression[i] != '\n'; ++i) {
        char* dup = x_strdup(expression[i]);
        array_push(&expr, &dup);
    }
    array_push(&expr, &null);
    array_cut(&expr);

    char* id = x_strdup(identifier);
    map_push(defines, &id, &expr.data);

    char** tokens = malloc(sizeof(char*));
    tokens[0] = NULL;

    return tokens;
}

static char** z_preprocess_directive_line(char** restrict tokens, map_t* restrict defines, const size_t line_count)
{
    if (!strcmp(tokens[0], "#")) {
        if (!strcmp(tokens[1], "include")) {
            return z_preprocess_include(tokens, defines, line_count);
        }
        else if (!strcmp(tokens[1], "define")) {
            return z_preprocess_define(tokens, defines, line_count);
        }
    }

    return NULL;
}

static inline bool z_chrbool_identifier(const char ch)
{
    return ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_');
}

static size_t z_preprocess_parenth(char** restrict tokens, const size_t count, const char* restrict symbols, const size_t line_count)
{
    size_t i, stack_counter = (*tokens[count] == symbols[0]);
    for (i = count; stack_counter; ++i) {
        if (!tokens[i]) {
            z_log("preZ error: Parenthesis starting at line %zu is never closed.\n", line_count);
            return count;
        }

        stack_counter += (i > count && *tokens[i] == symbols[0]);
        stack_counter -= (*tokens[i] == symbols[1]);
    }

    return i;
}

static array_t z_preprocess_expand_function_macro(char** restrict macro, const size_t line_count)
{
    array_t args = array_create(sizeof(char*));

    bool expecting = true;
    size_t parenth = 0;
    for (size_t i = 1; macro[i]; ++i) {
        if (*macro[i] == ',') {
            if (expecting) {
                z_log("preZ error: Expected argument in #define on line %zu.\n", line_count);
                array_free(&args);
                return args;
            }
            expecting = true;
        }
        else if (*macro[i] == ')') {
            parenth = i + 1;
            break;
        }
        else if (expecting) {
            if (z_chrbool_identifier(*macro[i])) {
                array_push(&args, macro + i);
                expecting = false;
            }
            else {
                z_log("preZ error: Expected an identifier in #define at line %zu.\n", line_count);
                array_free(&args);
                return args;
            }
        }
        else {
            z_log("preZ error: Invalid #define statement at line %zu.", line_count);
            array_free(&args);
            return args;
        }
    }

    if (!parenth) {
        z_log("preZ error: Macro function definition must close parenthesis on line %zu.", line_count);
        array_free(&args);
    }

    return args;
}

/* major leak on error: args does not free inner copied strings */
static array_t z_preprocess_expand_function_expression(char** restrict expression, const size_t line_count)
{
    char* null = NULL;
    array_t args = array_create(sizeof(char**));
    array_t subexpr = array_create(sizeof(char*));

    bool expecting = true;
    size_t parenths = 0, brackets = 0, end = 0;
    for (size_t i = 2; expression[i]; ++i) {
        if (*expression[i] == ',' && !parenths && !brackets) {
            if (expecting) {
                z_log("preZ error: Expected argument in macro expansion on line %zu.\n", line_count);
                array_free(&args);
                array_free(&subexpr);
                return args;
            }

            array_push(&subexpr, &null);
            array_push(&args, &subexpr.data);
            subexpr = array_create(sizeof(char*));
            expecting = true;
        }
        else if (*expression[i] == ')' && !parenths) {
            array_push(&subexpr, &null);
            array_push(&args, &subexpr.data);
            end = i + 1;
            break;
        }
        else {
            array_push(&subexpr, expression + i);
            expecting = false;
        }

        parenths += (*expression[i] == '(') - (*expression[i] == ')'); 
        brackets += (*expression[i] == '{') - (*expression[i] == '}'); 
    }

    if (!end) {
        z_log("preZ error: Macro function call must close parenthesis on line %zu.\n", line_count);
        array_free(&subexpr);
        array_free(&args);
    }

    return args;
}

static char** z_preprocess_expand_function(char** restrict tokens, char** restrict macro, const size_t line_count)
{
    array_t macro_args = z_preprocess_expand_function_macro(macro, line_count);
    array_t expr_args = z_preprocess_expand_function_expression(tokens, line_count);
    
    if (macro_args.size != expr_args.size) {
        z_log("preZ error: Number of argument of macro function call (%zu) does not match definition (%zu) in line %zu.\n", expr_args.size, macro_args.size, line_count);
        array_free(&macro_args);
        array_free(&expr_args);
        return NULL;
    }

    map_t arguments = map_create(sizeof(char*), sizeof(char**));

    char** margs = macro_args.data;
    char*** eargs = expr_args.data;

    const size_t size = macro_args.size;
    for (size_t i = 0; i < size; ++i) {
        map_push(&arguments, margs + i, eargs + i);
    }

    array_free(&macro_args);
    array_free(&expr_args);

    size_t predindex = z_preprocess_parenth(macro, 0, "()", line_count);
    char** predicate = macro + predindex;

    eargs = arguments.values;

    array_t strings = array_create(sizeof(char*));
    for (size_t i = 0; predicate[i]; ++i) {
        size_t search = map_search(&arguments, predicate + i);
        if (search) {
            char** expansion = eargs[search - 1];
            for (size_t j = 0; expansion[j]; ++j) {
                array_push(&strings, expansion + j);
            }
        }
        else array_push(&strings, predicate + i);
    }

    map_free(&arguments);

    char* null = NULL;
    array_push(&strings, &null);
    array_cut(&strings);

    return strings.data;
}

static char** z_preprocess_expand(char** tokens, const map_t* defines, const size_t line_count);

static size_t z_preprocess_expand_token(char** restrict tokens, array_t* restrict strings, const map_t* restrict defines, const char endchar, size_t count, const size_t line_count)
{
   for (; tokens[count] && *tokens[count] != endchar; ++count) {
        char** expand = z_preprocess_expand(tokens + count, defines, line_count);
        if (expand) {
            for (size_t i = 0; expand[i]; ++i) {
                array_push(strings, &expand[i]);
            }
            free(expand);

            if (*tokens[count + 1] == '(') {
                count = z_preprocess_parenth(tokens, count + 1, "()", line_count) - 1;
            }
        }
        else array_push(strings, &tokens[count]);
    }

    return count;
}

static char** z_preprocess_expand(char** restrict tokens, const map_t* restrict defines, const size_t line_count)
{
    const char* identifier = tokens[0];

    if (!z_chrbool_identifier(*identifier)) {
        return NULL;
    }

    const size_t find = map_search(defines, &identifier);
    if (!find) {
        return NULL;
    }

    char** keys = defines->keys;
    if (strcmp(keys[find - 1], identifier)) {
        return NULL;
    }

    char*** values = defines->values;
    char** macro = values[find - 1];

    char** expansion = NULL;
    if (*macro[0] == '(') {
        if (!tokens[1] || *tokens[1] != '(') {
            return NULL;
        }
        
        if (*macro[1] == ')') {
            if (!tokens[2] || *tokens[2] != ')') {
                return NULL;
            }
            expansion = x_strget_dup(macro + 2);
        }
        else expansion = z_preprocess_expand_function(tokens, macro, line_count);
    } 
    else expansion = x_strget_dup(macro);

    if (!expansion) {
        return NULL;
    }

    array_t expr = array_create(sizeof(char*));
    
    for (size_t i = 0; expansion[i]; ++i) {
        char** subexp = z_preprocess_expand(expansion + i, defines, line_count);
        if (subexp) {
            for (size_t j = 0; subexp[j]; ++j) {
                array_push(&expr, subexp + j);
            }
            free(subexp);

            if (*expansion[i + 1] == '(') {
                i = z_preprocess_parenth(expansion, i + 1, "()", line_count) - 1;
            }
        }
        else array_push(&expr, expansion + i);
    }

    if (expr.size) {
        char* null = NULL;
        array_push(&expr, &null);
    }

    return expr.data;
}

char** z_preprocess_directives(char** restrict tokens, map_t* restrict defines)
{
    void* null = NULL;
    array_t directives = array_create(sizeof(char*));

    size_t count = 0, line_count = 1;
    while (tokens[count]) {
        char** toks = z_preprocess_directive_line(tokens + count, defines, line_count);
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
                char** expand = z_preprocess_expand(tokens + count, defines, line_count);
                if (expand) {
                    for (size_t i = 0; expand[i]; ++i) {
                        array_push(&directives, &expand[i]);
                    }
                    free(expand);

                    if (*tokens[count + 1] == '(') {
                        count = z_preprocess_parenth(tokens, count + 1, "()", line_count) - 1;
                    }
                }
                else array_push(&directives, &tokens[count]);
            }
            array_push(&directives, &tokens[count++]);
        }
        ++line_count;
    }

    array_push(&directives, &null);
    array_cut(&directives);

    return directives.data;
}
