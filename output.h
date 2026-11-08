#ifndef OUTPUT_H
#define OUTPUT_H

#include "table.h"

void print_schema(Table *t, long long rows);
void print_stats(Table *t, long long rows);
void write_report(FILE *out, Table *t, long long rows);

#endif