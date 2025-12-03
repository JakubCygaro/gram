#include <ctype.h>
#include <plap.h>
#include <stddef.h>
#include <stdio.h>
#define PLAP_IMPLEMENTATION
#include "plap.h"

typedef struct CSVFile {
    char* file_name;
    size_t header_count;
    char** headers;
    size_t col_count;
    float* columns;
} CSVFile;

char* read_line(FILE* f)
{
    size_t buf_p = 0;
    size_t buf_sz = 512;
    char* buf = calloc(buf_sz, sizeof(char));

    int c = 0;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') {
            break;
        } else if (isspace(c) && c != ' ') {
            continue;
        } else {
            if (buf_p + 1 >= buf_sz) {
                buf_sz *= 2;
                buf = realloc(buf, buf_sz);
            }
            buf[buf_p++] = c;
        }
    }
    if (!buf_p) {
        return NULL;
    }
    char* line = calloc(buf_p + 1, sizeof(char));
    strcpy(line, buf);
    return line;
}

int split_at_colon(const char* str)
{
    for (int i = 0; i < (int)strlen(str); i++) {
        if (str[i] == ',') {
            return i;
        }
    }
    return -1;
}

char** parse_headers(const char* hline, size_t* hcount)
{
    *hcount = 0;
    size_t headersz = 16;
    char** headers = calloc(headersz, sizeof(char*));

    int colon = 0;
    int start = 0;
    do {
        colon = split_at_colon(hline + start);
        if (colon - start >= 0 && !hcount) {
            fprintf(stderr, "Empty column\n");
            exit(-1);
        } else {
            colon = strlen(hline);
        }
        printf("start: %d, colon: %d\n", start, colon);
        if ((*hcount++) >= headersz) {
            headersz *= 2;
            headers = realloc(headers, headersz);
        }
        char* h = calloc(colon - start + 1, sizeof(char));
        memcpy(h, hline + start, colon - start);
        start = colon;
    } while (colon != -1);

    char** ret = calloc(*hcount, sizeof(char*));
    memcpy(ret, headers, *hcount * sizeof(char*));
    free(headers);
    return ret;
}

CSVFile parse_csv(FILE* csv)
{
    CSVFile ret = { 0 };

    char* line = read_line(csv);
    if (!line) {
        fprintf(stderr, "Empty file\n");
        exit(-1);
    }
    ret.headers = parse_headers(line, &ret.header_count);
    free(line);

    return ret;
}

int main(int argc, char** args)
{
    ArgsDef adef = plap_args_def();
    plap_positional_string(&adef, "input-csv-file-path");
    plap_positional_string(&adef, "output-header-file-path");
    Args a = plap_parse_args(adef, argc, args);

    char* in_path = plap_get_positional(&a, 0)->str;
    char* out_path = plap_get_positional(&a, 1)->str;

    FILE* in_file = fopen(in_path, "r");
    CSVFile csv = parse_csv(in_file);

    for(size_t i = 0; i < csv.header_count; i++){
        printf("%s\n", csv.headers[i]);
    }

    plap_free_args(a);
    return 0;
}
