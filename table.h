#ifndef TABLE_H
#define TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FIELD (1024 * 1024)

enum { TYPE_NULL, TYPE_INT, TYPE_DOUBLE, TYPE_STRING };
typedef int ColumnType;

typedef struct {
    long long nulls, not_nulls;
    long long min_int, max_int;
    long double sum_int;
    long long pos_int, neg_int, even_int;
    double min_dbl, max_dbl;
    long double sum_dbl;
    long long pos_dbl, neg_dbl, int_dbl;
    long long min_len, max_len, sum_len, empty;
} Stats;

typedef struct {
    char *name;
    ColumnType type;
    Stats st;
} Column;

typedef struct {
    Column *cols;
    int count;
} Table;

void free_table(Table *t);
void fall(Table *t, char *buf, const char *msg);
void init_table(Table *t, int count);

#endif