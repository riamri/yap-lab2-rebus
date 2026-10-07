#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <time.h>

#define MAX_LETTERS   26
#define MAX_TERMS      7
#define MAX_LEN       32
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

    int  n_columns;
    int  col_addend[MAX_LEN][MAX_TERMS];
    int  col_addend_count[MAX_LEN];
    int  col_result[MAX_LEN];

    int  order[MAX_LETTERS];
} Rebus;

static Rebus R;
static int   digit[MAX_LETTERS];
static int   used[10];
static int   solution_found;
static unsigned long long nodes_visited;


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

    if (R.n_terms < 2 || R.n_letters > 10) return 0;


    R.n_columns = R.result_len;
    for (int t = 0; t < R.n_terms; t++)
        if (R.term_len[t] > R.n_columns) R.n_columns = R.term_len[t];

    for (int k = 0; k < R.n_columns; k++) {
        R.col_addend_count[k] = 0;
        R.col_result[k] = -1;
    }

    for (int t = 0; t < R.n_terms; t++) {
        int L = R.term_len[t];
        for (int i = 0; i < L; i++) {
            int k = L - 1 - i;
            R.col_addend[k][R.col_addend_count[k]++] = R.terms[t][i];
        }
    }

    for (int i = 0; i < R.result_len; i++) {
        int k = R.result_len - 1 - i;
        R.col_result[k] = R.result[i];
    }

 

    int placed[MAX_LETTERS] = { 0 };
    int ord_size = 0;

    for (int k = 0; k < R.n_columns; k++) {
        for (int j = 0; j < R.col_addend_count[k]; j++) {
            int l = R.col_addend[k][j];
            if (!placed[l]) {
                placed[l] = 1;
                R.order[ord_size++] = l;
            }
        }
        if (R.col_result[k] != -1) {
            int l = R.col_result[k];
            if (!placed[l]) {
                placed[l] = 1;
                R.order[ord_size++] = l;
            }
        }
    }

    return (ord_size == R.n_letters) ? 1 : 0;
}

static int check_column(int k, int carry_in, int* carry_out) {
    int sum = 0;

    for (int j = 0; j < R.col_addend_count[k]; j++) {
        int l = R.col_addend[k][j];
        if (digit[l] < 0) return -1;
        sum += digit[l];
    }

    int rl = R.col_result[k];
    if (rl != -1 && digit[rl] < 0) return -1;

    int total = sum + carry_in;
    int d_res = total % 10;
    int c_out = total / 10;

    if (rl != -1) {
        if (digit[rl] != d_res) return 0;
    }
    else {
        if (d_res != 0) return 0;
    }

    *carry_out = c_out;
    return 1;
}

static int check_all(int* carry_out) {
    int c = 0;
    for (int k = 0; k < R.n_columns; k++) {
        int c_out;
        int r = check_column(k, c, &c_out);
        if (r == -1) {
            *carry_out = c;
            return -1;
        }
        if (r == 0) return 0;
        c = c_out;
    }
    *carry_out = c;
    return 1;
}

static int try_derive_letter(int k, int carry_in, int letter, int* value) {
    int known_sum = 0;
    int unknown_count = 0;
    int unknown_is_result = 0;

    for (int j = 0; j < R.col_addend_count[k]; j++) {
        int l = R.col_addend[k][j];
        if (l == letter) {
            unknown_count++;
        }
        else if (digit[l] < 0) {
            return 0;
        }
        else {
            known_sum += digit[l];
        }
    }

    int rl = R.col_result[k];
    if (rl != -1) {
        if (rl == letter) {
            unknown_count++;
            unknown_is_result = 1;
        }
        else if (digit[rl] < 0) {
            return 0;
        }
    }

    if (unknown_count != 1) return 0;

    int leading = R.is_leading[letter];

    if (!unknown_is_result) {
        if (rl == -1) return 0;
        int d_res = digit[rl];
        int target = (d_res - carry_in - known_sum) % 10;
        if (target < 0) target += 10;

        if (leading && target == 0) return 0;
        if (used[target]) return 0;

        int total = known_sum + target + carry_in;
        if (total % 10 != d_res) return 0;

        *value = target;
        return 1;
    }
    else {
        int total = known_sum + carry_in;
        int d_res = total % 10;

        if (leading && d_res == 0) return 0;
        if (used[d_res]) return 0;

        *value = d_res;
        return 1;
    }
}


static void recurse(int pos) {
    if (solution_found) return;
    nodes_visited++;

    if (pos == R.n_letters) {
        int carry;
        int r = check_all(&carry);
        if (r == 1 && carry == 0) {
            solution_found = 1;
        }
        return;
    }

    int l = R.order[pos];

   
    {
        int c = 0;
        for (int k = 0; k < R.n_columns; k++) {
            int c_out;
            int r = check_column(k, c, &c_out);
            if (r == -1) {
                int v;
                if (try_derive_letter(k, c, l, &v)) {
                    used[v] = 1;
                    digit[l] = v;
                    recurse(pos + 1);
                    if (solution_found) return;  
                    used[v] = 0;
                    digit[l] = -1;
                    return;
                }
                break;
            }
            if (r == 0) return;
            c = c_out;
        }
    }

   
    int leading = R.is_leading[l];
    int start_d = leading ? 1 : 0;

    for (int d = start_d; d <= 9; d++) {
        if (used[d]) continue;

        used[d] = 1;
        digit[l] = d;

     
        int all_ok = 1;
        int c = 0;
        for (int k = 0; k < R.n_columns; k++) {
            int c_out;
            int r = check_column(k, c, &c_out);
            if (r == -1) break;
            if (r == 0) { all_ok = 0; break; }
            c = c_out;
        }

        if (all_ok) recurse(pos + 1);

        if (solution_found) return;  

        used[d] = 0;
        digit[l] = -1;
    }
}


static long value_of(const int* ids, int len) {
    long v = 0;
    for (int i = 0; i < len; i++) v = v * 10 + digit[ids[i]];
    return v;
}

static void print_solution(void) {
    printf("Solution: ");
    for (int i = 0; i < R.n_letters; i++) {
        printf("%c=%d", R.letters[i], digit[i]);
        if (i + 1 < R.n_letters) printf(" ");
    }
    printf("\n");

    printf("Expression: ");
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
        printf("Invalid expression: %s\n", line);
        return 0;
    }
    printf("Rebus: %s\n", line);

    memset(used, 0, sizeof(used));
    for (int i = 0; i < MAX_LETTERS; i++) digit[i] = -1;
    solution_found = 0;
    nodes_visited = 0;

    double t0 = now_seconds();
    recurse(0);
    double t1 = now_seconds();

    if (solution_found) {
        print_solution();
    }
    else {
        printf("No solution.\n");
    }
    printf("Nodes: %llu   Time: %.6f sec\n",
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

    printf("Enter expressions, one per line (empty line to finish):\n");
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

