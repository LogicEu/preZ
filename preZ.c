#include <preZ.h>
#include <string.h>
#include <stdlib.h>
#include <xstring/xstring.h>

extern array_t include_paths;

static int preZ_help(const char* exename)
{
    z_log("preZ usage information:\n", exename);
    z_log("<file_path>\t:Add path to source files.\n");
    z_log("-o <file_path>\t:Redirect output to this file.\n");
    z_log("-I<dir_path>\t:Add directory to include paths to search.\n");
    z_log("--help, -h\t:Print usage information.\n");
    z_log("--version, -v\t:Print version information.\n");
    return Z_EXIT_SUCCESS;
}

static int preZ_version(const char* exename)
{
    z_log("preZ version 0.1.0.\n", exename);
    return Z_EXIT_SUCCESS;
}

static void z_log_strs(char** strs)
{
    for (size_t i = 0; strs[i]; ++i) {
        z_log("%s\n", strs[i]);
    }
}

static void z_log_tokens(char** strs)
{
    for (size_t i = 0; strs[i]; ++i) {
        z_log("'%s' ", strs[i]);
    }
}

static void z_log_map(const map_t* map)
{
    char** keys = map->keys;
    char*** values = map->values;

    const size_t size = map_size(map);
    z_log("Element Count: %zu\n", size);
    for (size_t i = 0; i < size; ++i) {
        z_log("Macro: '%s'", keys[i]);
        for (size_t j = 0; values[i][j]; ++j) {
            z_log(" '%s'", values[i][j]);
        }
        z_log("\n");
    }
}

int main(int argc, char** argv) 
{
    void* null = NULL;
    char* output_file = NULL;
    array_t source_files = array_create(sizeof(char*));
    include_paths = array_create(sizeof(char*));

    for (int i = 1; i < argc; ++i) {
        
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            return preZ_help(argv[0]);
        }
        if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
            return preZ_version(argv[0]);
        }

        if (!strcmp(argv[i], "-o")) {
            if (i == argc - 1) {
                z_log("Use a valid path for -o argument. See -h for more information.\n");
                return Z_EXIT_FAILURE;
            }
            output_file = argv[i + 1];
        }
        else if (strstr(argv[i], "-I") == argv[i]) {
            if (!argv[i][2]) {
                z_log("Invalid use of option -I. See -h for more information.\n");
                return Z_EXIT_FAILURE;
            }
            char* str = (argv[i][strlen(argv[i]) - 1] != '/') ? x_strcat(argv[i] + 2, "/") : x_strdup(argv[i] + 2);
            if (!str) {
                z_log("What=??\n");
            }
            array_push(&include_paths, &str);
        }
        else array_push(&source_files, &argv[i]);
    }

    if (!source_files.size) {
        z_log("Missing input file.\n");
        return Z_EXIT_FAILURE;
    }

    array_push(&source_files, &null);
    array_push(&include_paths, &null);

    char** strs = source_files.data;
    
    z_log("Source Files:\n");
    z_log_strs(strs);
    z_log("Include Paths:\n");
    z_log_strs(include_paths.data);
    if (output_file) {
        z_log("Output File: %s\n", output_file);
    }

    map_t defines = map_create(sizeof(char*), sizeof(char**));

    char** tokens = z_preprocess_file(strs[0], &defines);
    if (!tokens) {
        return Z_EXIT_FAILURE;
    }

    z_log_tokens(tokens);
    z_log("\n");
    z_log_map(&defines);

    x_strget_free(include_paths.data);
    x_strget_free(tokens);
    array_free(&source_files);

    return Z_EXIT_SUCCESS;
}
