#include "gram_csv.h"
#include <plap.h>
#include <stddef.h>
#include <stdio.h>
#define PLAP_IMPLEMENTATION
#include "plap.h"

void print_csv(CSVFile* csv)
{
    printf("gram_csv\n");
    printf("CSV file: `%s`\n", csv->file_name);
    printf("Fields per record: %ld\n", csv->header_count);
    printf("Data per column: %ld\n", csv->col_len);
    printf("Headers:\n");
    for (size_t i = 0; i < csv->header_count; i++) {
        printf("`%s`\t", csv->headers[i]);
    }
    printf("\n\n");
    printf("Column Data:\n");
    for (size_t i = 0; i < csv->col_len; i++) {
        for (size_t c = 0; c < csv->col_count; c++) {
            printf("%lf\t", csv->columns[c][i]);
        }
        printf("\n");
    }
    printf("\n");
}

int main(int argc, char** args)
{
    ArgsDef adef = plap_args_def();
    plap_positional_string(&adef, "input-csv-file-path", "csv file to process", 1);
    plap_positional_string(&adef, "output-header-file-path", "header file to output", 0);
    plap_option_int(&adef, "p", "print", "print csv file contents summary", 0);
    Args a = plap_parse_args(adef, argc, args);

    char* in_path = plap_get_positional(&a, 0)->str;
    PositionalArg* out_path_a = plap_get_positional(&a, 1);

    CSVFile csv = {0};
    if(gram_csv_load_csv(in_path, &csv)){
        fprintf(stderr, "%s\n", gram_csv_err_msg());
        exit(-1);
    }

    if (!out_path_a || plap_get_option(&a, "p", "print")) {
        print_csv(&csv);
    }
    if (out_path_a) {
        const char* out_file = out_path_a->str;
        gram_csv_write_header_file(&csv, out_file);
        printf("File written to `%s`\n", out_path_a->str);
    }
    gram_csv_csv_file_free(csv);
    plap_free_args(a);
    return 0;
}
