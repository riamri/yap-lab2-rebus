#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <time.h>

#define MAX_LETTERS   26
#define MAX_TERMS      7
#define MAX_LEN       32 // это кол-во букв букв в одном слове, можно поменять но не меньше 10 (тк цифры от 0 до 9)
#define MAX_LINE     256

typedef struct {
    int  n_terms;
    int  n_letters;
    char letters[MAX_LETTERS];
    int  idx[MAX_LETTERS];

    int  terms[MAX_TERMS][MAX_LEN];
    int  term_len[MAX_TERMS];
    int  result[MAX_LEN];
    int  result_len;

    int  is_leading[MAX_LETTERS];
} Rebus;

static Rebus R;
static int   digit[MAX_LETTERS];
static int   used[10];
static int   solution_found;

static int letter_id(char c) {
    return R.idx[(unsigned char)c - 'A'];
}

static void add_letter(char c) {
    int id = (unsigned char)c - 'A';
    if (R.idx[id] == -1) {
        R.idx[id] = R.n_letters;
        R.letters[R.n_letters] = c;
        R.n_letters++;
    }
}

static int parse(const char* s) {
    R.n_terms = 0;
    R.n_letters = 0;
    for (int i = 0; i < MAX_LETTERS; i++) R.idx[i] = -1;
    memset(R.is_leading, 0, sizeof(R.is_leading));

    const char* p = s;
    int term = 0;
    int len = 0;

    while (*p && *p != '=') {
        if (isupper((unsigned char)*p)) {
            if (len >= MAX_LEN) return 0;
            add_letter(*p);
            R.terms[term][len++] = letter_id(*p);
            p++;
        }
        else if (*p == '+') {
            if (len == 0) return 0;
            R.term_len[term] = len;
            R.is_leading[R.terms[term][0]] = 1;
            term++;
            if (term >= MAX_TERMS) return 0;
            len = 0;
            p++;
        }
        else {
            p++;
        }
    }
    if (*p != '=' || len == 0) return 0;
    R.term_len[term] = len;
    R.is_leading[R.terms[term][0]] = 1;
    R.n_terms = term + 1;
    p++;

    len = 0;
    while (*p) {
        if (isupper((unsigned char)*p)) {
            if (len >= MAX_LEN) return 0;
            add_letter(*p);
            R.result[len++] = letter_id(*p);
            p++;
        }
        else {
            p++;
        }
    }
    if (len == 0) return 0;
    R.result_len = len;
    R.is_leading[R.result[0]] = 1;

    return (R.n_terms >= 2 && R.n_letters <= 10) ? 1 : 0;
}

static long value_of(const int* ids, int len) {
    long v = 0;
    for (int i = 0; i < len; i++) v = v * 10 + digit[ids[i]];
    return v;
}

static int check_full(void) {
    long sum = 0;
    for (int t = 0; t < R.n_terms; t++)
        sum += value_of(R.terms[t], R.term_len[t]);
    long res = value_of(R.result, R.result_len);
    return sum == res;
}

static unsigned long long nodes_visited;

static void recurse(int pos) {
    if (solution_found) return;
    nodes_visited++;

    if (pos == R.n_letters) {
        if (check_full()) solution_found = 1;
        return;
    }

    int leading = R.is_leading[pos];
    for (int d = (leading ? 1 : 0); d <= 9; d++) {
        if (used[d]) continue;
        used[d] = 1;
        digit[pos] = d;
        recurse(pos + 1);
        used[d] = 0;
        if (solution_found) return;
    }
}

static void print_solution(void) {
    printf("Значения:");
    for (int i = 0; i < R.n_letters; i++) {
        printf("%c=%d", R.letters[i], digit[i]);
        if (i + 1 < R.n_letters) printf(" ");
    }
    printf("\n");

    printf("Решение:");
    for (int t = 0; t < R.n_terms; t++) {
        printf("%ld", value_of(R.terms[t], R.term_len[t]));
        if (t + 1 < R.n_terms) printf(" + ");
    }
    printf(" = %ld\n", value_of(R.result, R.result_len));
}

static double now_seconds(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

static int solve(const char* line) {
    if (!parse(line)) {
        printf("Ошибка разбора: %s\n", line);
        return 0;
    }
    printf("Ребус: %s\n", line);

    memset(used, 0, sizeof(used));
    solution_found = 0;
    nodes_visited = 0;

    double t0 = now_seconds();
    recurse(0);
    double t1 = now_seconds();

    if (solution_found) {
        print_solution();
    }
    else {
        printf("Решение не найдено.\n");
    }
    printf("Узлов: %llu   Время: %.6f с\n",
        nodes_visited, t1 - t0);
    return solution_found;
}

int main(int argc, char** argv) {
    
    setlocale(LC_ALL, "Russian");

    if (argc >= 2) {
        char buf[MAX_LINE] = { 0 };
        size_t used_len = 0;
        for (int i = 1; i < argc; i++) {
            size_t l = strlen(argv[i]);
            if (used_len + l + 2 >= sizeof(buf)) break;
            if (i > 1) buf[used_len++] = ' ';
            memcpy(buf + used_len, argv[i], l);
            used_len += l;
        }
        buf[used_len] = '\0';
        return solve(buf) ? 0 : 1;
    }

    printf("Вводите головоломки, по одной на строку (пустая строка для завершения):\n");
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), stdin)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';
        if (n == 0) break;
        solve(line);
        printf("\n");
    }
    return 0;
}