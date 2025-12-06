#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gram_csv.h"

#define BUF_PUSH(VAL, BUF, BUFPTR, SIZE, OFTYPE)   \
    if (BUFPTR + 1 >= SIZE) {                      \
        SIZE *= 2;                                 \
        BUF = realloc(BUF, SIZE * sizeof(OFTYPE)); \
    }                                              \
    BUF[BUFPTR++] = VAL

#define SET_ERR(FMT, ...) \
    sprintf(__errbuf, #FMT ,##__VA_ARGS__ )

#define COMMA ','
#define QUOTE '"'
#define NEWLINE '\n'
#define CARRIAGERETURN '\r'

static char __errbuf[256] = { 0 };


static size_t line_count = 0;
static int IS_EOF = 0;

static char* get_file_name(const char* path)
{
    size_t len = strlen(path);
    size_t i = len;
    for (; i >= 1; i--) {
        char c = path[i - 1];
        if (c == '\\' || c == '/') {
            break;
        }
    }
    char* n = calloc(len - i, sizeof(char));
    strcpy(n, path + i);
    return n;
}
const char* gram_csv_err_msg(){
    return __errbuf;
}
void gram_csv_csv_file_free(CSVFile csv)
{
    if (csv.file_name) {
        free(csv.file_name);
    }
    for (size_t i = 0; i < csv.header_count; i++) {
        if (csv.headers[i]) {
            free(csv.headers[i]);
        }
    }
    for (size_t i = 0; i < csv.col_count; i++) {
        if (csv.columns[i]) {
            free(csv.columns[i]);
        }
    }
    free(csv.columns);
}


static void line_free(line_t l, size_t l_sz)
{
    for (size_t i = 0; i < l_sz; i++) {
        free(l[i]);
    }
}

static char* read_quoted(FILE* f, size_t* line_len)
{
    size_t start = *line_len;
    int closed = 0;
    size_t buf_p = 0;
    size_t buf_sz = 128;
    char* buf = calloc(buf_sz, sizeof(char));
    int c = 0;
    while ((c = fgetc(f)) != EOF) {
        (*line_len)++;
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
                (*line_len)--;
                break;
            } else {
                SET_ERR("Improper usage of quote (%ld:%ld)\n", line_count, *line_len);
                free(buf);
                return NULL;
            }
        } else {
            BUF_PUSH(c, buf, buf_p, buf_sz, char);
        }
    }
    if (!closed) {
        SET_ERR("Unclosed quotation (%ld:%ld)\n", line_count, start);
        free(buf);
        return NULL;
    }
    char* ret = calloc(buf_p + 1, sizeof(char));
    memcpy(ret, buf, buf_p * sizeof(char));
    free(buf);
    return ret;
}

static line_t read_line(FILE* f, size_t* size)
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
        line_len++;
        if (quoted && (c != COMMA && c != NEWLINE && c != CARRIAGERETURN && c != EOF)) {
            SET_ERR("Improper quoting in field (%ld:%ld)\n", line_count, line_len);
            free(buf);free(fbuf);
            return NULL;
        } else if (quoted && c == COMMA) {
            quoted = 0;
            continue;
        } else if (quoted && (c == NEWLINE || c == EOF)) {
            quoted = 0;
            break;
        } else if (quoted && c == CARRIAGERETURN) {
            c = fgetc(f);
            if (c != NEWLINE) {
                SET_ERR("Line ending not proper CRLF (CR present, but LF missing) (%ld:%ld)\n", line_count, line_len);
                free(buf);free(fbuf);
                return NULL;
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
            if (line_len-1 > 0) {
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
                SET_ERR("Line ending not proper CRLF (CR present, but LF missing) (%ld:%ld)\n", line_count, line_len);
                free(buf);free(fbuf);
                return NULL;
            }
            if (line_len-1 > 0) {
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
            char* field = read_quoted(f, &line_len);
            if(!field){
                free(buf);
                free(fbuf);
                return NULL;
            }
            BUF_PUSH(field, fbuf, fbuf_p, fbuf_sz, char*);
            quoted = 1;
            continue;
        } else if (c == QUOTE && buf > 0) {
            SET_ERR("Quote start inside of field (%ld:%ld)\n", line_count, line_len);
            free(buf);
            free(fbuf);
            return NULL;
        }
        BUF_PUSH(c, buf, buf_p, buf_sz, char);
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

int gram_csv_load_csv(const char* csv_file, CSVFile* ret)
{
    *ret = (CSVFile) { 0 };
    FILE* csv = fopen(csv_file, "r");
    if(!csv){
        SET_ERR("Could not open file for reading");
        return GRAMCSV_ERR_COULD_NOT_OPEN_FILE;
    }
    char* file_name = get_file_name(csv_file);
    char** headers = NULL;
    size_t fields_per_record = 0;
    while (1) {
        headers = read_line(csv, &fields_per_record);
        if (headers)
            break;
        if (IS_EOF) {
            fclose(csv);
            free(file_name);
            SET_ERR("File was empty");
            return GRAMCSV_ERR_FILE_EMPTY;
        }
    }
    ret->file_name = file_name;
    file_name = NULL;

    ret->headers = headers;
    ret->header_count = fields_per_record;

    ret->col_count = fields_per_record;
    ret->columns = calloc(ret->col_count, sizeof(double*));
    // columns
    size_t cbuf_sz = 16;
    size_t cbuf_p = 0;
    for (size_t i = 0; i < ret->col_count; i++) {
        ret->columns[i] = (double*)calloc(cbuf_sz, sizeof(double));
    }

    while (!IS_EOF) {
        size_t fields = 0;
        line_t line = NULL;
        while (!line && !IS_EOF) {
            line = read_line(csv, &fields);
            if(__errbuf[0] != '\0'){
                gram_csv_csv_file_free(*ret);
                return GRAMCSV_ERR_READ_LINE;
            }
        }
        if (line) {
            if (fields != fields_per_record) {
                SET_ERR("Mismatch between the number (%ld) of fields in line (%ld) and the number of headers (%ld)\n",
                        line_count, fields,fields_per_record);
                gram_csv_csv_file_free(*ret);
                return GRAMCSV_ERR_IRREGULAR_FIELD_NUMBER;
            }
            for (size_t i = 0; i < fields; i++) {
                char* field = line[i];
                double v = 0.0;
                if (field[0] != '\0') {
                    v = atof(field);
                }
                if (cbuf_p + 1 >= cbuf_sz) {
                    cbuf_sz *= 2;
                    ret->columns[i] = realloc(ret->columns[i], cbuf_sz * sizeof(double));
                }
                ret->columns[i][cbuf_p] = v;
                if (i == fields - 1)
                    cbuf_p++;
            }
            line_free(line, fields);
        }
    }
    fclose(csv);
    ret->col_len = cbuf_p;
    return 0;
}


static char* sanitize_guard(const char* str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str);
    if (len == 0) {
        return NULL;
    }
    char* n_next = calloc(len + 1, sizeof(char));
    size_t i = 0;
    while (i < len) {
        char c = str[i];
        if (c == '.')
            break;
        if (!isalnum(c)) {
            n_next[i] = '_';
        } else {
            n_next[i] = toupper(str[i]);
        }
        i++;
    }
    if (i == 0)
        return NULL;
    return n_next;
}
static char* sanitize_header(const char* str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str);
    if (len == 0) {
        return NULL;
    }
    char* head = calloc(len + 1, sizeof(char));
    size_t i = 0;
    while (i < len) {
        char c = str[i];
        if (!isalnum(c) || isspace(c)) {
            head[i] = '_';
        } else {
            head[i] = tolower(str[i]);
        }
        i++;
    }
    if (i == 0)
        return NULL;
    return head;
}

void gram_csv_write_header_file(CSVFile* csv, const char* header_file)
{
    FILE* f = fopen(header_file, "w");
    char* guard = sanitize_guard(csv->file_name);
    guard = guard ? guard : "UNNAMED";
    fprintf(f, "#ifndef _%s_DATASET_H\n"
               "#define _%s_DATASET_H\n",
        guard, guard);

    // define the struct
    fprintf(f, "struct _gram_dataset_%s {\n", guard);
    fprintf(f, "\t unsigned long column_count;\n");
    fprintf(f, "\t unsigned long data_count;\n");

    for (size_t h = 0; h < csv->header_count; h++) {
        char* header = sanitize_header(csv->headers[h]);
        fprintf(f, "\t double h%s[%ld];\n", header, csv->col_len);
        free(header);
    }
    fprintf(f, "} D%s = {\n", guard);

    fprintf(f, "\t .column_count = %ld,\n", csv->col_count);
    fprintf(f, "\t .data_count = %ld,\n", csv->col_len);
    // initialize columns
    for (size_t h = 0; h < csv->header_count; h++) {
        char* header = sanitize_header(csv->headers[h]);
        fprintf(f, "\t .h%s = {\n\t\t", header);
        free(header);
        for (size_t i = 0; i < csv->col_len; i++) {
            fprintf(f, "%lf", csv->columns[h][i]);
            if (i != csv->col_len - 1) {
                fprintf(f, ", ");
            }
        }
        fprintf(f, "\n\t},\n");
    }
    fprintf(f, "\n};\n");

    free(guard);
    fprintf(f, "#endif\n");
    fclose(f);
}
