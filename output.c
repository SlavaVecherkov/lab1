#include "output.h"

void print_name(const char *name) {
    putchar('"');
    for (int i = 0; name[i] != '\0'; i++) {
        if (name[i] == '"') {
            putchar('"');
            putchar('"');
        }
        else putchar(name[i]);
    }
    putchar('"');
}

double no_minus_zero(double v) {
    if (v == 0.0) return 0.0;
    return v;
}

void print_schema(Table *t, long long rows) {
    printf("rows %lld\n", rows);
    printf("columns %d\n", t->count);
    for (int i = 0; i < t->count; i++) {
        printf("%d ", i);
        print_name(t->cols[i].name);
        printf(" ");
        if (t->cols[i].type == TYPE_NULL) printf("NULL\n");
        else if (t->cols[i].type == TYPE_INT) printf("INT\n");
        else if (t->cols[i].type == TYPE_DOUBLE) printf("DOUBLE\n");
        else printf("STRING\n");
    }
}

void print_stats(Table *t, long long rows) {
    printf("rows %lld\n", rows);
    printf("columns %d\n", t->count);

    for (int i = 0; i < t->count; i++) {
        Column *c = &t->cols[i];
        printf("\n");
        printf("column %d\n", i);
        printf("name ");
        print_name(c->name);
        printf("\n");

        if (c->type == TYPE_NULL) {
            printf("type NULL\n");
            printf("rows %lld\n", rows);
        }
        else if (c->type == TYPE_INT) {
            printf("type INT\n");
            printf("nulls %lld\n", c->st.nulls);
            if (c->st.not_nulls > 0) {
                double mean = (double)(c->st.sum_int / (long double)c->st.not_nulls);
                printf("min %lld\n", c->st.min_int);
                printf("max %lld\n", c->st.max_int);
                printf("mean %f\n", mean);
                printf("positive %lld\n", c->st.pos_int);
                printf("negative %lld\n", c->st.neg_int);
                printf("even %lld\n", c->st.even_int);
            }
        }
        else if (c->type == TYPE_DOUBLE) {
            printf("type DOUBLE\n");
            printf("nulls %lld\n", c->st.nulls);
            if (c->st.not_nulls > 0) {
                double mean = (double)(c->st.sum_dbl / (long double)c->st.not_nulls);
                printf("min %f\n", no_minus_zero(c->st.min_dbl));
                printf("max %f\n", no_minus_zero(c->st.max_dbl));
                printf("mean %f\n", no_minus_zero(mean));
                printf("positive %lld\n", c->st.pos_dbl);
                printf("negative %lld\n", c->st.neg_dbl);
                printf("integer %lld\n", c->st.int_dbl);
            }
        }
        else {
            printf("type STRING\n");
            printf("nulls %lld\n", c->st.nulls);
            if (c->st.not_nulls > 0) {
                double mean = (double)c->st.sum_len / (double)c->st.not_nulls;
                printf("min_length %lld\n", c->st.min_len);
                printf("max_length %lld\n", c->st.max_len);
                printf("mean_length %f\n", mean);
                printf("empty %lld\n", c->st.empty);
            }
        }
    }
}

void write_report(FILE *out, Table *t, long long rows) {
    fprintf(out, "========================================================\n");
    fprintf(out, "               CSV STAT ANALYZER REPORT\n");
    fprintf(out, "========================================================\n");
    fprintf(out, "Rows: %lld, Columns: %d\n", rows, t->count);
    fprintf(out, "--------------------------------------------------------\n\n");

    for (int i = 0; i < t->count; i++) {
        Column *c = &t->cols[i];
        fprintf(out, "Column № %d\n  Name:  \"", i);
        for (int j = 0; c->name[j]; j++) {
            if (c->name[j] == '"') fprintf(out, "\"\"");
            else                   fprintf(out, "%c", c->name[j]);
        }
        fprintf(out, "\"\n  Type:  ");
        if (c->type == TYPE_NULL)           fprintf(out, "NULL\n");
        else if (c->type == TYPE_INT)       fprintf(out, "INT\n");
        else if (c->type == TYPE_DOUBLE)    fprintf(out, "DOUBLE\n");
        else fprintf(out, "STRING\n");
        fprintf(out, "  Nulls: %lld\n", c->st.nulls);

        if (c->type == TYPE_INT && c->st.not_nulls > 0) {
            double mean = (double)(c->st.sum_int / (long double)c->st.not_nulls);
            fprintf(out, "    min      = %lld\n", c->st.min_int);
            fprintf(out, "    max      = %lld\n", c->st.max_int);
            fprintf(out, "    mean     = %f\n", mean);
            fprintf(out, "    positive = %lld\n", c->st.pos_int);
            fprintf(out, "    negative = %lld\n", c->st.neg_int);
            fprintf(out, "    even     = %lld\n", c->st.even_int);
        } else if (c->type == TYPE_DOUBLE && c->st.not_nulls > 0) {
            double mean = (double)(c->st.sum_dbl / (long double)c->st.not_nulls);
            fprintf(out, "    min      = %f\n", no_minus_zero(c->st.min_dbl));
            fprintf(out, "    max      = %f\n", no_minus_zero(c->st.max_dbl));
            fprintf(out, "    mean     = %f\n", no_minus_zero(mean));
            fprintf(out, "    positive = %lld\n", c->st.pos_dbl);
            fprintf(out, "    negative = %lld\n", c->st.neg_dbl);
            fprintf(out, "    integer  = %lld\n", c->st.int_dbl);
        } else if (c->type == TYPE_STRING && c->st.not_nulls > 0) {
            double mean = (double)c->st.sum_len / (double)c->st.not_nulls;
            fprintf(out, "    min_len  = %lld\n", c->st.min_len);
            fprintf(out, "    max_len  = %lld\n", c->st.max_len);
            fprintf(out, "    mean_len = %f\n", mean);
            fprintf(out, "    empty    = %lld\n", c->st.empty);
        }
        fprintf(out, "\n");
    }
}