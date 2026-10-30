#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t lo = (uint32_t)challenge;
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);

    uint32_t a = (lo ^ UINT32_C(0x9a32))
               + UINT32_C(4) * hi;

    uint32_t b = (UINT32_C(5) * lo + mid)
               ^ UINT32_C(0xdf27);

    values[0] = (a + UINT32_C(0xd4ba)) ^ b;

    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;

    values[0] = UINT32_C(3) *
        (uint32_t)((challenge >> 20) & UINT64_C(63));

    for (uint32_t i = 1; i < 6; ++i) {
        unsigned shift = 20 + 2 * (i - 1);
        uint32_t part =
            (uint32_t)((challenge >> shift) & UINT64_C(63));

        values[i] = UINT32_C(4) * values[i - 1] + (part ^ i);
    }

    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;

    uint32_t index = (uint32_t)challenge & UINT32_C(7);
    uint32_t x = (uint32_t)(challenge >> 32)
               ^ (uint32_t)(challenge >> 3);
    uint32_t result;

    switch (index) {
        case 0:
            result = UINT32_C(5) * x + UINT32_C(0x4bec9);
            break;
        case 1:
            result = UINT32_C(5) * x + UINT32_C(0x19799);
            break;
        case 2:
            result = (x ^ UINT32_C(0xf0f0)) + (x >> 2);
            break;
        case 3:
            result = (UINT32_C(2) * x) ^ UINT32_C(0xbb73);
            break;
        case 4:
            result = UINT32_C(3) * x + UINT32_C(0x6fd9);
            break;
        case 5:
            result = (UINT32_C(4) * x) ^ UINT32_C(0x1d17);
            break;
        case 6:
            result = (x ^ UINT32_C(0x6d05)) + (x >> 1);
            break;
        case 7:
            result = UINT32_C(5) * x + UINT32_C(0xc903);
            break;
        default:
            return 1;
    }

    values[0] = index;
    values[1] = result;
    return write_answer(out, cap, values, count);
}

static uint32_t phase4_walk(uint32_t target, uint32_t low,
                            uint32_t high, uint32_t key) {
    uint32_t mid = low + ((high - low) >> 1);

    if (target == mid) {
        return (mid + UINT32_C(0x9b)) ^ key;
    }

    uint32_t child;
    uint32_t tag;

    if (target < mid) {
        child = phase4_walk(target, low, mid - 1, key);
        tag = UINT32_C(0x10);
    } else {
        child = phase4_walk(target, mid + 1, high, key);
        tag = UINT32_C(0x18);
    }

    return (UINT32_C(5) * child) ^ (mid + key) ^ tag;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t target = (uint32_t)challenge & UINT32_C(127);
    uint32_t key = (uint32_t)(challenge >> 7)
                 ^ (uint32_t)(challenge >> 32);

    values[0] = phase4_walk(target, 0, 127, key);

    return write_answer(out, cap, values, count);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;

    static const struct {
        uint32_t value;
        uint32_t id;
        int next;
    } nodes[12] = {
        {32059, 2157, 10},
        {22904, 2320, -1},
        {10469, 1341,  4},
        {48744,  345,  8},
        {31148, 2943,  7},
        {19877,  805,  0},
        {35937, 1118,  5},
        {11771,  594, 11},
        {65095, 1623,  1},
        {58837, 1815,  3},
        {56827,  116,  2},
        {7119,  2657,  9}
    };

    static const int heads[8] = {6, 5, 0, 10, 2, 4, 7, 11};

    int node = heads[(unsigned)(challenge & UINT64_C(7))];
    unsigned steps = (unsigned)((challenge >> 3) & UINT64_C(3));

    uint32_t acc = (uint32_t)(challenge >> 6)
                 ^ (uint32_t)(challenge >> 32)
                 ^ UINT32_C(0xdbc6);

    for (unsigned i = 0; i <= steps; ++i) {
        if (node < 0 || node >= 12)
            return 1;

        acc = (acc ^ nodes[node].value) + nodes[node].id;
        values[1] = nodes[node].id;

        if (i < steps)
            node = nodes[node].next;
    }

    values[0] = acc;
    return write_answer(out, cap, values, count);
}

static uint32_t phase6_op(unsigned index, uint32_t x) {
    /* 按函数指针表的下标分派，而不是按函数名编号 */
    switch (index) {
        case 0:
        case 4:  /* op_2 */
            return (x + UINT32_C(0x7259)) ^ (x >> 1);

        case 1:  /* op_4 */
            return ((x << 5) + UINT32_C(0xaf77))
                 ^ (UINT32_C(7) * x);

        case 2:  /* op_5 */
            return (x >> 7) | (x << 25);

        case 3:  /* op_3 */
            return ((x >> 3) ^ x) + UINT32_C(0xe4ef);

        case 5:
        case 7:  /* op_0 */
            return ((x << 5) | (x >> 27)) ^ UINT32_C(0x9aba);

        case 6:  /* op_1 */
            return (x ^ UINT32_C(0xa229)) + UINT32_C(8) * x;

        default:
            return 0;
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    unsigned i = (unsigned)(challenge & UINT64_C(7));
    unsigned j = (unsigned)((challenge >> 3) & UINT64_C(7));

    uint32_t x = (uint32_t)(challenge >> 6)
               ^ (uint32_t)(challenge >> 32);

    uint32_t a = phase6_op(i, x);
    values[0] = phase6_op(j, a ^ UINT32_C(0x29a0))
              + UINT32_C(0xa31f);

    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context,
                 char *out, size_t cap) {
    uint32_t s = (uint32_t)challenge
               ^ (uint32_t)(challenge >> 32);
    uint32_t k = (uint32_t)context
               ^ (uint32_t)(context >> 32);

    uint32_t a = (uint32_t)context & UINT32_C(3);

    uint32_t x = s ^ k;
    uint32_t rotated = (x >> 5) | (x << 27);

    uint32_t b = rotated
               ^ UINT32_C(0x3546)
               ^ (a * UINT32_C(0xe4d2))
               ^ (uint32_t)(context >> 16);

    uint32_t c = b ^ (k + a + UINT32_C(0xd194) + (s >> 5));

    /* 32字节填充，随后是OPEN，再随后是三个整数 */
    static const char prefix[] =
        "AAAAAAAA"
        "AAAAAAAA"
        "AAAAAAAA"
        "AAAAAAAA";

    int size = snprintf(out, cap,
                        "%sOPEN%" PRIu32 " %" PRIu32 " %" PRIu32,
                        prefix, a, b, c);

    return size < 0 || (size_t)size >= cap;
}
