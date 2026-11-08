#include "table.h"

void free_table(Table *t) {
    if (t == NULL || t->cols == NULL) return;
    for (int i = 0; i < t->count; i++) free(t->cols[i].name);
    free(t->cols);
    t->count = 0;
}

void fall(Table *t, char *buf, const char *msg) {
    free(buf);
    free_table(t);
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

void init_table(Table *t, int count) {
    t->count = count;
    t->cols = malloc(sizeof(Column) * count);
    if (t->cols == NULL) fall(NULL, NULL, "out of memory");
    
    for (int i = 0; i < t->count; i++) {
        Column *c = &t->cols[i];
        c->name = NULL;
        c->type = TYPE_NULL;
        memset(&c->st, 0, sizeof(Stats));
        c->st.min_int = 9223372036854775807LL;
        c->st.max_int = -9223372036854775807LL - 1;
        c->st.min_dbl = 1e308;
        c->st.max_dbl = -1e308;
        c->st.min_len = 1100000LL;
    }
}