#ifndef PREZ_PREPROCESSOR_H
#define PREZ_PREPROCESSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************
        preZ
--------------------
Tiny C preprocessor.
--------------------
by Eugenio Arteaga A
*******************/

#define Z_EXIT_SUCCESS 0
#define Z_EXIT_FAILURE 1

int z_log(const char* fmt, ...);
char* z_file_stream(const char* filename);

char** z_preprocess_file(const char* filename);
char** z_preprocess_text(const char* text);

char** z_preprocess_text_process(const char* text);
char** z_preprocess_tokens(char** lines);
char** z_preprocess_directives(char** tokens);

#define ZMARK z_log("File '%s' on line %zu.\n", __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif
#endif
