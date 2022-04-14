#include <preZ.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int z_log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int log = vfprintf(stdout, fmt, args);
    va_end(args);
    return log;
}

char* z_file_stream(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    char* str = calloc(size + 1, sizeof(char));
    
    fseek(file, 0, SEEK_SET);
    fread(str, sizeof(char), size, file);
    fclose(file);
    
    return str;
}