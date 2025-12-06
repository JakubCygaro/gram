#ifndef GRAM_CSV_H
#define GRAM_CSV_H
#include <stddef.h>
#include <stdio.h>

#define GRAMCSV_ERR_COULD_NOT_OPEN_FILE 1
#define GRAMCSV_ERR_FILE_EMPTY 2
#define GRAMCSV_ERR_IRREGULAR_FIELD_NUMBER 3
#define GRAMCSV_ERR_READ_LINE 4

typedef struct CSVFile {
    char* file_name;
    size_t header_count;
    char** headers;
    size_t col_count;
    size_t col_len;
    double** columns;
} CSVFile;

typedef char** line_t;

/// returns 0 if no error
int gram_csv_load_csv(const char* csv_file, CSVFile* ret);
void gram_csv_csv_file_free(CSVFile csv);
void gram_csv_write_header_file(CSVFile* csv, const char* header_file);
/// returns a pointer to the error message (do not attempt to free this pointer)
const char* gram_csv_err_msg();


#endif
