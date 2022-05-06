#include <preZ.h>
#include <string.h>
#include <stdlib.h>
#include <xstring/xstring.h>

static inline bool z_chrbool_space(const char ch)
{
    return (ch == ' ' || ch == '\t');
}

static inline bool z_chrbool_number(const char ch)
{
    return (ch >= '0' && ch <= '9');
}

static inline bool z_chrbool_string_literal(const char ch)
{
    return (ch == '\'' || ch == '"');
}

static inline bool z_chrbool_identifier(const char ch)
{
    return ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_');
}

static bool z_chrbool_puntuator(const char ch)
{
    static const char puntuators[] = ".,:;{}[]()?!%%+-*/=<>~^&|#";
    for (size_t i = 0; puntuators[i]; ++i) {
        if (ch == puntuators[i]) {
            return true;
        }
    }
    return false;
}

static inline bool z_strbool_number(const char* str)
{
    return (z_chrbool_number(str[0]) || (str[0] == '.' && z_chrbool_number(str[1])));
}

static inline bool z_strbool_exponent(const char* str)
{
    return ((str[1] == '+' || str[1] == '-') && (str[0] == 'e' || str[0] == 'E' || str[0] == 'p' || str[0] == 'P'));
}

char* z_token_string_literal(const char* str, const char* symbol)
{
    char* end = strstr(str + 1, symbol);
    while (end && *(end - 1) == '\\') {
        end = strstr(++end, symbol);
    }

    if (!end) {
        return NULL;
    }

    return ++end;
}

static char* z_token_next(const char* str)
{
    char* s = (char*)(size_t)str;
    while (z_chrbool_space(*s)) {
        ++s;
    }
    return s;
}

static char* z_token_number(const char* str)
{
    char* s = (char*)(size_t)str;
    while (z_chrbool_identifier(s[0]) || z_chrbool_number(s[0]) || s[0] == '.') {
        if (z_strbool_exponent(++s)) {
            s += 2;
        }
    }
    return s;
}

static char* z_token_identifier(const char* str)
{
    char* s = (char*)(size_t)str;
    while (z_chrbool_identifier(*s) || z_chrbool_number(*s)) {
        ++s;
    }
    return s;
}

static int z_token_dirigrapgh(char* str)
{
    if (!strcmp(str, "<%%")) {
        strcpy(str, "{");
        return 1;
    }
    else if (!strcmp(str, "%%>")) {
        strcpy(str, "}");
        return 1;
    }
    else if (!strcmp(str, "<:")) {
        strcpy(str, "[");
        return 1;
    }
    else if (!strcmp(str, ":>")) {
        strcpy(str, "]");
        return 1;
    }
    else if (!strcmp(str, "%%:")) {
        strcpy(str, "#");
        return 1;
    }
    else if (!strcmp(str, "%%:%%:")) {
        strcpy(str, "##");
        return 1;
    }
    return 0;
}

static char* z_token_puntuator(const char* str)
{
    char* s = (char*)(size_t)str + 1;
    switch(str[0]) {
        case '#': {
            return s + (str[1] == '#');
        }
        case '|': {
            return s + (str[1] == '|' || str[1] == '=');
        }
        case '&': {
            return s + (str[1] == '&' || str[1] == '=');
        }
        case '!': {
            return s + (str[1] == '=');
        }
        case '^': {
            return s + (str[1] == '=');
        }
        case '~': {
            return s + (str[1] == '=');
        }
        case '+': {
            return s + (str[1] == '+' || str[1] == '=');
        }
        case '-': {
            return s + (str[1] == '-' || str[1] == '=' || str[1] == '>');
        }
        case '*': {
            return s + (str[1] == '=');
        }
        case '/': {
            return s + (str[1] == '=');
        }
        case '%': {
            return s + (str[1] == '=' || str[1] == '>' || str[1] == ':') + 2 * (str[1] == ':' && str[2] == '%' && str[3] == ':');
        }
        case '=': {
            return s + (str[1] == '=');
        }
        case '<': {
            if (str[1] == '=' || str[1] == '%' || str[1] == ':') {
                ++s;
            }
            else if (str[1] == '<') {
                ++s;
                if (str[2] == '=') {
                    ++s;
                }
            }
            return s;
        }
        case '>': {
            if (str[1] == '=') {
                ++s;
            }
            else if (str[1] == '>') {
                ++s;
                if (str[2] == '=') {
                    ++s;
                }
            }
            return s;
        }
        case ':': {
            return s + (str[1] == '>');
        }
        default: { 
            return s; 
        }
    }
}

static char* z_strtok(const char* str)
{
    if (z_chrbool_string_literal(*str)) {
        char symbol[2] = {*str, 0};
        return z_token_string_literal(str, symbol);
    }
    else if (z_chrbool_identifier(*str)) {
        return z_token_identifier(str);
    }
    else if (z_strbool_number(str)) {
        return z_token_number(str);
    }
    else if (z_chrbool_puntuator(*str)) {
        return z_token_puntuator(str);
    }
    else return NULL;
}

static int z_preprocess_tokenize_line(const char* restrict line, array_t* restrict tokens)
{
    static const char* new_line = "\n";

    char* end, *str, *s;

    if (!line[0] || !strcmp(line, new_line)) {
        str = x_strdup(new_line);
        array_push(tokens, &str);
        return Z_EXIT_SUCCESS;
    }

    s = z_token_next(line);
    while ((end = z_strtok(s))) {
        str = x_strdup_range(s, end);
        if (z_chrbool_puntuator(str[0])) {
            z_token_dirigrapgh(str);
        }
        array_push(tokens, &str);
        s = z_token_next(end);
    }

    if (*s) {
        str = x_strdup(s);
        array_push(tokens, &str);
    }

    str = x_strdup(new_line);
    array_push(tokens, &str);

    return Z_EXIT_SUCCESS;
}

/* Second Preprocessing Pass: Tokenization */

char** z_preprocess_tokens(char** lines)
{
    void* null = NULL;

    array_t tokens = array_create(sizeof(char*));
    for (size_t i = 0; lines[i] != 0; ++i) {
        z_preprocess_tokenize_line(lines[i], &tokens);
    }

    array_push(&tokens, &null);
    array_cut(&tokens);
    
    return tokens.data;
}