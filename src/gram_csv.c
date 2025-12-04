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
    size_t col_len;
    double** columns;
} CSVFile;

#define READFIELD 1
#define READQUOTED 2
#define COMMA ','
#define QUOTE '"'
#define NEWLINE '\n'
#define CARRIAGERETURN '\r'

#define BUF_PUSH(VAL, BUF, BUFPTR, SIZE, OFTYPE)   \
    if (BUFPTR + 1 >= SIZE) {                      \
        SIZE *= 2;                                 \
        BUF = realloc(BUF, SIZE * sizeof(OFTYPE)); \
    }                                              \
    BUF[BUFPTR++] = VAL

char* read_quoted(FILE* f)
{
    int closed = 0;
    size_t buf_p = 0;
    size_t buf_sz = 512;
    char* buf = calloc(buf_sz, sizeof(char));
    int c = 0;
    while ((c = fgetc(f)) != EOF) {
        // if quote is in the middle of the field
        if (c == QUOTE) {
            c = getc(f);
            if (c == QUOTE) {
                BUF_PUSH(c, buf, buf_p, buf_sz, char);
                continue;
            }
            if (c == NEWLINE || c == COMMA || c == CARRIAGERETURN) {
                closed = 1;
                ungetc(c, f);
                break;
            } else {
                fprintf(stderr, "Improper usage of quote\n");
                exit(-1);
            }
        } else {
            BUF_PUSH(c, buf, buf_p, buf_sz, char);
        }
    }
    if(!closed){
        fprintf(stderr, "Unclosed quotation\n");
        exit(-1);
    }
    char* ret = calloc(buf_p + 1, sizeof(char));
    memcpy(ret, buf, buf_p * sizeof(char));
    free(buf);
    return ret;
}

typedef char** line_t;

void line_free(line_t l, size_t l_sz)
{
    for (size_t i = 0; i < l_sz; i++) {
        free(l[i]);
    }
}

static int IS_EOF = 0;
static size_t line_count = 0;

line_t read_line(FILE* f, size_t* size)
{
    *size = 0;
    int quoted = 0;
    int c = 0;
    size_t line_len = 0;

    size_t fbuf_p = 0;
    size_t fbuf_sz = 4;
    char** fbuf = calloc(fbuf_sz, sizeof(char*));

    size_t buf_p = 0;
    size_t buf_sz = 512;
    char* buf = calloc(buf_sz, sizeof(char));

    while (1) {
        c = fgetc(f);
        if (quoted && (c != COMMA && c != NEWLINE && c != CARRIAGERETURN && c != EOF)) {
            fprintf(stderr, "Improper quoting in field\n");
            exit(-1);
        } else if (quoted && c == COMMA) {
            quoted = 0;
            continue;
        } else if (quoted && (c == NEWLINE || c == EOF)) {
            quoted = 0;
            break;
        } else if (quoted && c == CARRIAGERETURN) {
            c = fgetc(f);
            if (c != NEWLINE) {
                fprintf(stderr, "Line ending not proper CRLF (CR present, but LF missing)\n");
                exit(-1);
            }
            quoted = 0;
            break;
        }
        // fields can be empty
        if (c == COMMA) {
            char* field = calloc(buf_p + 1, sizeof(char));
            memcpy(field, buf, buf_p);
            memset(buf, 0, buf_p * sizeof(char));
            buf_p = 0;
            BUF_PUSH(field, fbuf, fbuf_p, fbuf_sz, char*);
            continue;
        }
        if (c == NEWLINE || c == EOF) {
            if (line_len > 0) {
                char* field = calloc(buf_p + 1, sizeof(char));
                memcpy(field, buf, buf_p);
                memset(buf, 0, buf_p * sizeof(char));
                buf_p = 0;
                BUF_PUSH(field, fbuf, fbuf_p, fbuf_sz, char*);
            }
            break;
        }
        if (c == CARRIAGERETURN) {
            c = fgetc(f);
            if (c != NEWLINE) {
                fprintf(stderr, "Line ending not proper CRLF (CR present, but LF missing)\n");
                exit(-1);
            }
            if (line_len > 0) {
                char* field = calloc(buf_p + 1, sizeof(char));
                memcpy(field, buf, buf_p);
                memset(buf, 0, buf_p * sizeof(char));
                buf_p = 0;
                BUF_PUSH(field, fbuf, fbuf_p, fbuf_sz, char*);
            }
            break;
        }
        // if quote is at the start of the field
        if (c == QUOTE && buf_p == 0) {
            char* field = read_quoted(f);
            BUF_PUSH(field, fbuf, fbuf_p, fbuf_sz, char*);
            quoted = 1;
            continue;
        } else if (c == QUOTE && buf > 0) {
            fprintf(stderr, "Quote start inside of field\n");
            exit(-1);
        }
        BUF_PUSH(c, buf, buf_p, buf_sz, char);
        line_len++;
    }
    line_count++;
    IS_EOF = c == EOF;
    char** fields = NULL;
    if (fbuf_p > 0) {
        fields = calloc(fbuf_p, sizeof(char*));
        memcpy(fields, fbuf, fbuf_p * sizeof(char*));
        *size = fbuf_p;
    }
    free(buf);
    free(fbuf);
    return fields;
}

CSVFile parse_csv(FILE* csv)
{
    CSVFile ret = { 0 };

    char** headers = NULL;
    size_t fields_per_record = 0;
    while (1) {
        headers = read_line(csv, &fields_per_record);
        if (headers)
            break;
        if (IS_EOF) {
            fprintf(stderr, "Empty file\n");
            exit(-1);
        }
    }

    ret.headers = headers;
    ret.header_count = fields_per_record;

    ret.col_count = fields_per_record;
    ret.columns = calloc(ret.col_count, sizeof(double*));
    // columns
    size_t cbuf_sz = 16;
    size_t cbuf_p = 0;
    for (size_t i = 0; i < ret.col_count; i++) {
        ret.columns[i] = (double*)calloc(cbuf_sz, sizeof(double));
    }

    while (!IS_EOF) {
        size_t fields = 0;
        line_t line = NULL;
        while (!line && !IS_EOF) {
            line = read_line(csv, &fields);
        }
        if (line) {
            if (fields != fields_per_record) {
                fprintf(stderr, "Mismatch between the number of fields in line and the number of headers\n");
                exit(-1);
            }
            for (size_t i = 0; i < fields; i++) {
                char* field = line[i];
                double v = 0.0;
                if (field[0] != '\0') {
                    v = atof(field);
                }
                if (cbuf_p + 1 >= cbuf_sz) {
                    cbuf_sz *= 2;
                    ret.columns[i] = realloc(ret.columns[i], cbuf_sz * sizeof(double));
                }
                ret.columns[i][cbuf_p] = v;
                if(i == fields - 1)
                    cbuf_p++;
            }
            line_free(line, fields);
        }
    }
    ret.col_len = cbuf_p;
    return ret;
}

char* get_file_name(const char* path){
    size_t len = strlen(path);
    size_t i = len;
    for(; i >= 1; i--){
        char c = path[i-1];
        if(c == '\\' || c == '/'){
            break;
        }
    }
    char* n = calloc(len - i, sizeof(char));
    strcpy(n, path + i);
    return n;
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
    csv.file_name = get_file_name(in_path);

    printf("CSV file: %s\n", csv.file_name);
    printf("fields per record: %ld\n", csv.header_count);
    printf("headers\n");
    for (size_t i = 0; i < csv.header_count; i++) {
        printf("`%s`\t", csv.headers[i]);
    }
    printf("\n");
    printf("data:\n");
    for (size_t i = 0; i < csv.col_len; i++) {
        for (size_t c = 0; c < csv.col_count; c++) {
            printf("%lf\t", csv.columns[c][i]);
        }
        printf("\n");
    }
    printf("\n");
    plap_free_args(a);
    return 0;
}
