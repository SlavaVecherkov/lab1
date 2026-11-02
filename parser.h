#ifndef PARSER_H
#define PARSER_H

#include "table.h"

int count_columns(FILE *f);
void read_header(FILE *f, Table *t);
long long read_data(FILE *f, Table *t);

#endif