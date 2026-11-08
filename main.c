#include <stdio.h>
#include <string.h>
#include "table.h"
#include "parser.h"
#include "output.h"

int main(int argc, char *argv[]) {
    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Error:--help doesn't take extra arguments\n");
            return 2;
        }
        printf("csvstat - CSV Table Analyzer\n");
        printf("Use: csvstat <command> <csv-file> <output-file>\n");
        printf("Commands: schema, stats, report, --help\n");
        printf("The command \"--help\" takes only 2 arguments,\n");
        printf("\"stats\" and \"schema\" - 3 arguments, \"report\" - 4 arguments\n");
        return 0;
    }
    if (argc < 3) {
        fprintf(stderr, "Error: not enough arguments\n");
        return 2;
    }
    
    char *cmd = argv[1], *file = argv[2];
    int is_report = strcmp(cmd, "report") == 0;
    int is_schema = strcmp(cmd, "schema") == 0;
    int is_stats = strcmp(cmd, "stats") == 0;
    
    if (!is_report && !is_schema && !is_stats) {
        fprintf(stderr, "Error: unknown command\n");
        return 2;
    }
    
    int need = is_report ? 4 : 3;
    if (argc != need) {
        fprintf(stderr, "Error: wrong number of arguments\n");
        return 2;
    }
    
    FILE *f = fopen(file, "r");
    if (f == NULL) {
        fprintf(stderr, "Error: can't open file\n");
        return 1;
    }
    
    int n = count_columns(f);
    Table t;
    init_table(&t, n);
    read_header(f, &t);
    long long rows = read_data(f, &t);
    
    if (is_schema) print_schema(&t, rows);
    else if (is_stats) print_stats(&t, rows);
    else {
        FILE *out = fopen(argv[3], "w");
        if (out == NULL) {
            free_table(&t);
            fclose(f);
            fprintf(stderr, "Error: can't create report file\n");
            return 1;
        }
        write_report(out, &t, rows);
        fclose(out);
    }
    
    free_table(&t);
    fclose(f);
    return 0;
}