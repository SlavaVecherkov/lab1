#include <ctype.h>
#include <math.h>
#include <errno.h>
#include "parser.h"

enum { TEXT, IN_QUOTES, CHECK_QUOTE };

int looks_like_int(const char *s) {
    int i = (s[0] == '-' || s[0] == '+');
    if (s[i] == '\0') return 0;
    while (isdigit(s[i])) i++;
    return s[i] == '\0';
}

int looks_like_double(const char *s) {
    int i = (s[0] == '-' || s[0] == '+');
    if (!isdigit(s[i])) return 0;
    while (isdigit(s[i])) i++;
    if (s[i] != '.') return 0;
    i++;
    if (!isdigit(s[i])) return 0;
    while (isdigit(s[i])) i++;
    return s[i] == '\0';
}

void save_name(Table *t, int idx, char *buf, int len) {
    if (idx >= t->count) fall(t, buf, "too many columns");
    if (len == 0) fall(t, buf, "empty column name");
    
    for (int i = 0; i < idx; i++) {
        if (strcmp(t->cols[i].name, buf) == 0) fall(t, buf, "duplicate column name");
    }
    
    t->cols[idx].name = malloc(len + 1);
    if (t->cols[idx].name == NULL) fall(t, buf, "out of memory");
    memcpy(t->cols[idx].name, buf, len + 1);
}

void process_cell(Table *t, int idx, const char *raw, int quoted) {
    if (idx < 0 || idx >= t->count) fall(t, NULL, "too many fields");
    Column *c = &t->cols[idx];
    int start = 0, end = strlen(raw);
    
    if (!quoted) {
        while (start < end && (raw[start] == ' ' || raw[start] == '\t')) start++;
        while (end > start && (raw[end-1] == ' ' || raw[end-1] == '\t')) end--;
    }
    int len = end - start;
    
    if (!quoted && len == 0) {
        c->st.nulls++;
        return;
    }
    
    char *val = malloc(len + 1);
    if (val == NULL) fall(t, NULL, "out of memory");
    memcpy(val, raw + start, len);
    val[len] = '\0';
    
    ColumnType ct = TYPE_STRING;
    long long iv = 0;
    double dv = 0.0;
    
    if (looks_like_int(val)) {
        errno = 0;
        iv = strtoll(val, NULL, 10);
        if (errno != ERANGE) ct = TYPE_INT;
    }
    else if (looks_like_double(val)) {
        errno = 0;
        dv = strtod(val, NULL);
        if (errno != ERANGE && isfinite(dv)) ct = TYPE_DOUBLE;
    }
    
    if (ct > c->type) c->type = ct;
    c->st.not_nulls++;
    
    if (ct == TYPE_INT) {
        if (iv < c->st.min_int) c->st.min_int = iv;
        if (iv > c->st.max_int) c->st.max_int = iv;
        c->st.sum_int += (long double)iv;
        if (iv > 0) c->st.pos_int++;
        if (iv < 0) c->st.neg_int++;
        if (iv % 2 == 0) c->st.even_int++;
    }
    
    if (ct == TYPE_INT || ct == TYPE_DOUBLE) {
        double d = (ct == TYPE_INT) ? (double)iv : dv;
        if (d < c->st.min_dbl) c->st.min_dbl = d;
        if (d > c->st.max_dbl) c->st.max_dbl = d;
        c->st.sum_dbl += (long double)d;
        if (d > 0) c->st.pos_dbl++;
        if (d < 0) c->st.neg_dbl++;
        if (d == floor(d)) c->st.int_dbl++;
    }
    
    if (len < c->st.min_len) c->st.min_len = len;
    if (len > c->st.max_len) c->st.max_len = len;
    c->st.sum_len += len;
    if (len == 0) c->st.empty++;
    
    free(val);
}

int count_columns(FILE *f) {
    int count = 1, in_quotes = 0, ch;
    while ((ch = fgetc(f)) != EOF && ch != '\n') {
        if (ch == '"') in_quotes = !in_quotes;
        else if (ch == ',' && !in_quotes) count++;
    }
    rewind(f);
    return count;
}

void read_header(FILE *f, Table *t) {
    char *buf = malloc(MAX_FIELD + 1);
    if (buf == NULL) fall(t, NULL, "out of memory");
    
    int idx = 0, cols_found = 0, len = 0, ch;
    int state = TEXT;
    int saw_space = 0;
    
    while ((ch = fgetc(f)) != EOF) {
        if (state == TEXT) {
            if (ch == '"') {
                if (len > 0) fall(t, buf, "quote in plain field");
                state = IN_QUOTES;
            } else if (ch == ',' || ch == '\n') {
                buf[len] = '\0';
                while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\t')) buf[--len] = '\0';
                save_name(t, idx, buf, len);
                idx++; cols_found++;
                len = 0;
                if (ch == '\n') {
                    free(buf);
                    if (cols_found != t->count) fall(t, NULL, "header mismatch");
                    return;
                }
            } else {
                if (len == 0 && (ch == ' ' || ch == '\t')) continue;
                if (len >= MAX_FIELD) fall(t, buf, "field too long");
                buf[len++] = ch;
            }
        }
        else if (state == IN_QUOTES) {
            if (ch == '"') state = CHECK_QUOTE;
            else if (ch == '\n') fall(t, buf, "newline inside quotes");
            else {
                if (len >= MAX_FIELD) fall(t, buf, "field too long");
                buf[len++] = ch;
            }
        }
        else {
            if (ch == '"') {
                if (!saw_space) {
                    if (len >= MAX_FIELD) fall(t, buf, "field too long");
                    buf[len++] = '"';
                    state = IN_QUOTES;
                } else fall(t, buf, "space or tabulation between quotes");
            } else if (ch == ',' || ch == '\n') {
                buf[len] = '\0';
                save_name(t, idx, buf, len);
                idx++; cols_found++;
                len = 0;
                state = TEXT;
                saw_space = 0;
                if (ch == '\n') {
                    free(buf);
                    if (cols_found != t->count) fall(t, NULL, "header mismatch");
                    return;
                }
            } else if (ch == ' ' || ch == '\t') {
                saw_space = 1;
                continue;
            } else fall(t, buf, "garbage after closing quote");
        }
    }
    
    if (state == IN_QUOTES) fall(t, buf, "unterminated quote");
    if (state == CHECK_QUOTE) {
        buf[len] = '\0';
        save_name(t, idx, buf, len);
        idx++; cols_found++;
    } else if (len > 0) {
        while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\t')) buf[--len] = '\0';
        buf[len] = '\0';
        save_name(t, idx, buf, len);
        idx++; cols_found++;
    } else if (cols_found > 0) {
        buf[0] = '\0';
        save_name(t, idx, buf, 0);
        idx++; cols_found++;
    }
    
    free(buf);
    if (cols_found != t->count) fall(t, NULL, "header mismatch");
}

long long read_data(FILE *f, Table *t) {
    char *buf = malloc(MAX_FIELD + 1);
    if (buf == NULL) fall(t, NULL, "out of memory");
    
    long long rows = 0, empty_rows = 0;
    int idx = 0, cols_found = 0, len = 0, has_data = 0, ch;
    int state = TEXT;
    int saw_space = 0;

    while ((ch = fgetc(f)) != EOF) {
        if (idx == 0 && len == 0 && !has_data && state == TEXT) {
            if (ch == '\n') {
                empty_rows++;
                continue;
            }
            while (empty_rows > 0) {
                if (t->count != 1) fall(t, buf, "empty line in multi-column");
                t->cols[0].st.nulls++;
                rows++; empty_rows--;
            }
            has_data = 1;
        }
        
        if (state == TEXT) {
            if (ch == '"') {
                if (len > 0) fall(t, buf, "quote in plain field");
                state = IN_QUOTES;
            } else if (ch == ',' || ch == '\n') {
                buf[len] = '\0';
                process_cell(t, idx, buf, 0);
                idx++; cols_found++;
                len = 0;
                if (ch == '\n') {
                    if (cols_found != t->count) fall(t, buf, "column count mismatch");
                    rows++;
                    idx = 0;
                    cols_found = 0;
                    has_data = 0;
                }
            } else {
                if (len == 0 && (ch == ' ' || ch == '\t')) continue;
                if (len >= MAX_FIELD) fall(t, buf, "field too long");
                buf[len++] = ch;
            }
        }
        else if (state == IN_QUOTES) {
            if (ch == '"') state = CHECK_QUOTE;
            else if (ch == '\n') fall(t, buf, "newline inside quotes");
            else {
                if (len >= MAX_FIELD) fall(t, buf, "field too long");
                buf[len++] = ch;
            }
        }
        else {
            if (ch == '"') {
                if (!saw_space) {
                    if (len >= MAX_FIELD) fall(t, buf, "field too long");
                    buf[len++] = '"';
                    state = IN_QUOTES;
                } else fall(t, buf, "space or tabulation between quotes");
            } else if (ch == ',' || ch == '\n') {
                buf[len] = '\0';
                process_cell(t, idx, buf, 1);
                idx++; cols_found++;
                len = 0;
                state = TEXT;
                saw_space = 0;
                if (ch == '\n') {
                    if (cols_found != t->count) fall(t, buf, "column count mismatch");
                    rows++;
                    idx = 0;
                    cols_found = 0;
                    has_data = 0;
                }
            } else if (ch == ' ' || ch == '\t') {
                saw_space = 1;
                continue;
            } else fall(t, buf, "garbage after closing quote");
        }
    }

    if (state == IN_QUOTES) fall(t, buf, "unterminated quote");

    if (state == CHECK_QUOTE) {
        buf[len] = '\0';
        process_cell(t, idx, buf, 1);
        idx++; cols_found++;
        if (cols_found != t->count) fall(t, buf, "column count mismatch");
        rows++;
    }
    else if (len > 0) {
        buf[len] = '\0';
        process_cell(t, idx, buf, 0);
        idx++; cols_found++;
        if (cols_found != t->count) fall(t, buf, "column count mismatch");
        rows++;
    }
    else if (cols_found > 0) {
        buf[0] = '\0';
        process_cell(t, idx, buf, 0);
        idx++; cols_found++;
        if (cols_found != t->count) fall(t, buf, "column count mismatch");
        rows++;
    }

    free(buf);
    return rows;
}