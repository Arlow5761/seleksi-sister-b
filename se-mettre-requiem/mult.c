#include <stdio.h>

#define bool        unsigned char
#define true        1
#define false       0

#define u8          unsigned char
#define u16         unsigned short
#define u32         unsigned int
#define u64         unsigned long long

#define MAX_DIGITS  1000000
#define N_MAX       2097152
#define PRIME       81788929

u64 w[] = {
    1,
    81788928,
    57807995,
    1977387,
    34192649,
    51800346,
    58177483,
    994921,
    45945133,
    58071073,
    17073102,
    21012575,
    24378151,
    20135734,
    38888378,
    19146616,
    7626772,
    1417215,
    39445061,
    15774326,
    80102761,
    22285958
};

u64 n_inv[] = {
    1,
    40894465,
    61341697,
    71565313,
    76677121,
    79233025,
    80510977,
    81149953,
    81469441,
    81629185,
    81709057,
    81748993,
    81768961,
    81778945,
    81783937,
    81786433,
    81787681,
    81788305,
    81788617,
    81788773,
    81788851,
    81788890
};

u64 w_inv[] = {
    1,
    81788928,
    23980934,
    1883838,
    49739338,
    76852948,
    23726402,
    56705191,
    15230354,
    12699317,
    8917364,
    61080809,
    48391651,
    9933144,
    63213817,
    33297963,
    38697572,
    31050320,
    79275988,
    35354232,
    76650467,
    75577748
};

// INTEGER COMPARISON OPERATIONS

bool eq(u64 a, u64 b) {
    return !(a ^ b);
}

bool neq(u64 a, u64 b) {
    return !eq(a, b);
}

bool leq(u64 a, u64 b) {
    u64 diff = a ^ b;
    diff = diff | (diff >> 1);
    diff = diff | (diff >> 2);
    diff = diff | (diff >> 4);
    diff = diff | (diff >> 8);
    diff = diff | (diff >> 16);
    diff = diff | (diff >> 32);
    diff = diff ^ (diff >> 1);

    return !(a & diff);
}

bool lt(u64 a, u64 b) {
    return leq(a, b) & neq(a, b);
}

bool geq(u64 a, u64 b) {
    return !lt(a, b);
}

bool gt(u64 a, u64 b) {
    return !leq(a, b);
}

// ELEMENTARY ARITHEMATIC OPERATIONS

u64 add(u64 a, u64 b) {
    u64 result = a ^ b;
    u64 carry = a & b;
    u64 shift = carry << 1;

    loop:
    if (carry) {
        carry = shift & result;
        result = result ^ shift;
        shift = carry << 1;

        goto loop;
    }

    return result;
}

u64 sub(u64 a, u64 b) {
    u64 negb = add(~b, 1);
    return add(a, negb);
}

u64 mul(u64 a, u64 b) {
    u64 result = 0;
    
    loop:
    if (b) {
        if (b & 1) {
            result = add(result, a);
        }
        
        b = b >> 1;
        a = a << 1;

        goto loop;
    }

    return result;
}

void divmod(u64 *result, u64 a, u64 b) {
    if (!b) return;

    u64 divider = b;
    u64 base = 1;
    u64 quotient = 0;

    loop1:
    if (gt(a, divider)) {
        divider = divider << 1;
        base = base << 1;

        if (gt(a, divider)) {
            goto loop1;
        }

        if (neq(a, divider)) {
            divider = divider >> 1;
            base = base >> 1;
        }
    }

    loop2:
    if (geq(a, b)) {
        a = sub(a, divider);
        quotient = add(quotient, base);

        loop3:
        if (lt(a, divider)) {
            divider = divider >> 1;
            base = base >> 1;

            goto loop3;
        }

        if (!divider) {
            goto end;
        }

        goto loop2;
    }

    end:
    result[0] = quotient;
    result[1] = a;
}

u64 div(u64 a, u64 b) {
    u64 res[2];
    divmod(res, a, b);
    return res[0];
}

u64 mod(u64 a, u64 b) {
    u64 res[2];
    divmod(res, a, b);
    return res[1];
}

u64 round_pow_2(u64 n) {
    n = sub(n, 1);
    n = n | n >> 1;
    n = n | n >> 2;
    n = n | n >> 4;
    n = n | n >> 8;
    n = n | n >> 16;
    n = n | n >> 32;
    return add(n, 1);
}

u64 get_exp_2(u64 n) {
    u64 i = -1;

    step:
    if (n) {
        i = add(i, 1);
        n = n >> 1;
        goto step;
    }

    return i;
}

// NTT IMPLEMENTATION

u64 addmod(u64 a, u64 b) {
    return mod(add(a, b), PRIME);
}

u64 submod(u64 a, u64 b) {
    return addmod(a, sub(PRIME, b));
}

u64 mulmod(u64 a, u64 b) {
    return mod(mul(a, b), PRIME);
}

u64 powmod(u64 base, u64 exp) {
    u64 result = 1;
    base = mod(base, PRIME);

    loop:
    if (exp) {
        if (exp & 1) {
            result = mulmod(result, base);
        }
        base = mulmod(base, base);
        exp >>= 1;
        goto loop;
    }

    return result;
}

void bit_reverse_permute(u64 *arr, u64 n) {
    u64 j = 0;
    u64 i = 1;

    loop1:
    if (lt(i, n)) {
        u64 bit = n >> 1;
        
        loop2:
        if (j & bit) {
            j = j ^ bit;
            bit >>= 1;

            goto loop2;
        }
        
        j = j ^ bit;

        if (lt(i, j)) {
            u64 temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }

        i = add(i, 1);
        goto loop1;
    }
}

void iterative_ntt(u64 *arr, u64 n, u64 w) {
    bit_reverse_permute(arr, n);

    u64 len = 2;

    loop1:
    if (leq(len, n)) {
        u64 wlen = powmod(w, div(n, len));
        u64 i = 0;

        loop2:
        if (i < n) {
            u64 w_cur = 1;
            u64 j = 0;
            u64 half_len = div(len, 2);

            loop3:
            if (j < half_len) {
                u64 t = add(i, j);
                u64 k = add(add(i, j), half_len);

                u64 u = arr[t];
                u64 v = mulmod(arr[k], w_cur);

                arr[t] = addmod(u, v);
                arr[k] = submod(u, v);

                w_cur = mulmod(w_cur, wlen);

                j = add(j, 1);
                goto loop3;
            }

            i = add(i, len);
            goto loop2;
        }

        len = len << 1;
        goto loop1;
    }
}

void ntt(u64 *arr, u64 n, u64 w) {
    iterative_ntt(arr, n, w);
}

void intt(u64 *arr, u64 n, u64 n_inv, u64 w_inv) {
    iterative_ntt(arr, n, w_inv);

    for (int j = 0; j < n; j++) {
        printf("%lld ", arr[j]);
    }
    printf("\n");

    u64 i = 0;
    mult:
    if (lt(i, n)) {
        arr[i] = mulmod(arr[i], n_inv);

        i = add(i, 1);
        goto mult;
    }
}

// MAIN PROGRAM

u64 base_buffer[N_MAX] = {0};
u64 other_buffer[N_MAX] = {0};

u64 input_a_len = 0;
u64 input_b_len = 0;
u64 output_len = 0;

void read_input(u64 *buffer, u64 *len) {
    *len = 0;

    scanloop:
    scanf("%c", &buffer[*len]);
    buffer[*len] = sub(buffer[*len], '0');
    if (leq(buffer[*len], '9')) {
        *len = add(*len, 1);
        goto scanloop;
    }

    buffer[*len] = 0;

    u64 i = 0;
    u64 last = *len >> 1;

    reorderloop:
    if (lt(i, last)) {
        u64 j = sub(sub(*len, 1), i);
        u64 temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;

        i = add(i, 1);
        goto reorderloop;
    }
}

void print_output(u64 *buffer, u64 len) {
    bool printed = false;
    
    loop1:
    if (len) {
        len = sub(len, 1);
        if (!buffer[len]) {
            goto loop1;
        }

        goto idfk;
    }

    loop2:
    if (len) {
        len = sub(len, 1);
        
        idfk:
        printf("%lld", buffer[len]);
        printed = true;

        goto loop2;
    }

    if (!printed) {
        printf("0");
    }

    printf("\n");
}

void long_mult() {
    u64 a_norm_len = input_a_len;
    u64 b_norm_len = input_b_len;

    output_len = add(a_norm_len, b_norm_len);
    u64 output_norm_len = round_pow_2(output_len);
    u64 input_norm_len = output_norm_len;

    u64 idx = get_exp_2(output_norm_len);

    for (int j = 0; j < output_norm_len; j++) {
        printf("%lld ", base_buffer[j]);
    }
    printf("\n");

    for (int j = 0; j < output_norm_len; j++) {
        printf("%lld ", other_buffer[j]);
    }
    printf("\n");

    ntt(base_buffer, input_norm_len, w[idx]);
    ntt(other_buffer, input_norm_len, w[idx]);

    for (int j = 0; j < output_norm_len; j++) {
        printf("%lld ", base_buffer[j]);
    }
    printf("\n");

    for (int j = 0; j < output_norm_len; j++) {
        printf("%lld ", other_buffer[j]);
    }
    printf("\n");

    u64 i = 0;
    mult_loop:
    if (lt(i, output_norm_len)) {
        base_buffer[i] = mulmod(base_buffer[i], other_buffer[i]);

        i = add(i, 1);
        goto mult_loop;
    }

    for (int j = 0; j < output_norm_len; j++) {
        printf("%lld ", base_buffer[j]);
    }
    printf("\n");

    intt(base_buffer, output_norm_len, n_inv[idx], w_inv[idx]);

    for (int j = 0; j < output_norm_len; j++) {
        printf("%lld ", base_buffer[j]);
    }
    printf("\n");

    i = 0;
    u64 carry = 0;
    carry_loop:
    if (lt(i, output_norm_len)) {
        base_buffer[i] = add(base_buffer[i], carry);

        u64 op[2];
        divmod(op, base_buffer[i], 10);

        base_buffer[i] = op[1];
        carry = op[0];

        i = add(i, 1);
        goto carry_loop;
    }
}

int main() {
    printf("Enter the first integer:\n");
    read_input(base_buffer, &input_a_len);

    printf("Enter the second integer:\n");
    read_input(other_buffer, &input_b_len);

    long_mult();

    printf("Result:\n");
    print_output(base_buffer, output_len);

    return 0;
}