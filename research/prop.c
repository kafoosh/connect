/*
 * Which predictions can the first player (the magician, red) FORCE?
 *
 * Same perfect-play machinery as solver/solver.c, but red only "wins" when the
 * prediction comes true. Real Connect 4 rules still apply: any four ends the
 * game, so red must never complete a four that breaks the prediction, must
 * still block yellow, and a draw or a yellow win both count as failure.
 *
 * Usage: prop PROPERTY [PREFIX]   ->  prints FORCED or NOT FORCED
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;
#define W 7
#define H 6
#define BOTTOM_MASK 0x0040810204081ULL
#define BOARD_MASK (BOTTOM_MASK * ((1ULL << H) - 1))
#define TT_SIZE 8388617u
static u64 column_mask(int c) { return ((1ULL << H) - 1) << (c * (H + 1)); }
static u64 bottom_mask_col(int c) { return 1ULL << (c * (H + 1)); }
static u64 cell(int c, int r) { return 1ULL << (c * (H + 1) + r); }
static int popcount(u64 m) { return __builtin_popcountll(m); }

enum { DV = 1, DH = 2, DD1 = 4, DD2 = 8 };
/* empty cells that would complete a four of `p`, only along chosen directions */
static u64 spots_dirs(u64 p, u64 mask, int dirs) {
    u64 r = 0, q;
    if (dirs & DV) r |= (p << 1) & (p << 2) & (p << 3);
    if (dirs & DH) {
        q = (p << (H + 1)) & (p << 2 * (H + 1)); r |= q & (p << 3 * (H + 1)); r |= q & (p >> (H + 1));
        q = (p >> (H + 1)) & (p >> 2 * (H + 1)); r |= q & (p << (H + 1)); r |= q & (p >> 3 * (H + 1));
    }
    if (dirs & DD1) {
        q = (p << H) & (p << 2 * H); r |= q & (p << 3 * H); r |= q & (p >> H);
        q = (p >> H) & (p >> 2 * H); r |= q & (p << H); r |= q & (p >> 3 * H);
    }
    if (dirs & DD2) {
        q = (p << (H + 2)) & (p << 2 * (H + 2)); r |= q & (p << 3 * (H + 2)); r |= q & (p >> (H + 2));
        q = (p >> (H + 2)) & (p >> 2 * (H + 2)); r |= q & (p << (H + 2)); r |= q & (p >> 3 * (H + 2));
    }
    return r & (BOARD_MASK ^ mask);
}
static u64 spots(u64 p, u64 mask) { return spots_dirs(p, mask, 15); }

/* all 69 lines of four, for line-based properties */
static u64 lines[128]; static int nlines;
static void init_lines(void) {
    int d[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    for (int k = 0; k < 4; k++) for (int c = 0; c < W; c++) for (int r = 0; r < H; r++) {
        int ec = c + 3 * d[k][0], er = r + 3 * d[k][1];
        if (ec < 0 || ec >= W || er < 0 || er >= H) continue;
        u64 m = 0; for (int i = 0; i < 4; i++) m |= cell(c + i * d[k][0], r + i * d[k][1]);
        lines[nlines++] = m;
    }
}
static u64 qlines[128]; static int nq;
static u64 spots_lines(u64 p, u64 mask) {
    u64 r = 0;
    for (int i = 0; i < nq; i++) {
        u64 own = p & qlines[i], hole = qlines[i] & ~mask;
        if (popcount(own) == 3 && popcount(hole) == 1) r |= hole;
    }
    return r;
}

/* ---------------- the predictions ---------------- */
static int PROP;
static u64 allowed_cols = BOARD_MASK; /* columns red may use */
static const char *PROP_NAME[] = {
    "0  plain win (sanity check)",
    "1  winning four passes through the centre column",
    "2  winning four is horizontal",
    "3  winning four is vertical",
    "4  winning four is diagonal (either way)",
    "5  winning four is diagonal, rising to the right",
    "6  winning four is NOT vertical",
    "7  winning four includes my first disc (bottom of the centre)",
    "8  my winning disc goes in column 4",
    "9  I never use the two edge columns",
    "10 I only use the middle three columns",
    "11 I win with my very last disc (21st)",
    "12 winning four touches the bottom row",
    "13 my winning disc goes in column 3",
    "14 my winning disc goes in column 2",
    "15 my winning disc goes in column 1",
    "16 winning four is horizontal or vertical (not diagonal)",
    "17 winning four is NOT horizontal",
    "18 my winning disc goes in one of the middle three columns",
    "19 winning four stays clear of the two edge columns",
    "20 I don't win before my 11th disc",
    "21 I don't win before my 16th disc",
    "22 winning four is horizontal or rising-diagonal",
};
#define NPROP 23

/* Red's qualifying winning spots (completing a four that fulfils the prediction). */
static u64 red_q(u64 p, u64 mask, int moves) {
    switch (PROP) {
    case 1: return spots_dirs(p, mask, DH | DD1 | DD2) | (spots_dirs(p, mask, DV) & column_mask(3));
    case 2: return spots_dirs(p, mask, DH);
    case 3: return spots_dirs(p, mask, DV);
    case 4: return spots_dirs(p, mask, DD1 | DD2);
    case 5: return spots_dirs(p, mask, DD2);
    case 6: return spots_dirs(p, mask, DH | DD1 | DD2);
    case 7: case 12: return spots_lines(p, mask);
    case 8: return spots(p, mask) & column_mask(3);
    case 13: return spots(p, mask) & column_mask(2);
    case 14: return spots(p, mask) & column_mask(1);
    case 15: return spots(p, mask) & column_mask(0);
    case 9: case 10: return spots(p, mask) & allowed_cols;
    case 11: return moves == W * H - 2 ? spots(p, mask) : 0;
    case 16: return spots_dirs(p, mask, DH | DV);
    case 17: return spots_dirs(p, mask, DV | DD1 | DD2);
    case 18: return spots(p, mask) & (column_mask(2) | column_mask(3) | column_mask(4));
    case 19: return spots_lines(p, mask);
    case 20: return moves >= 20 ? spots(p, mask) : 0;
    case 21: return moves >= 30 ? spots(p, mask) : 0;
    case 22: return spots_dirs(p, mask, DH | DD2);
    default: return spots(p, mask);
    }
}

typedef struct { u64 cur, mask; int moves; } Pos;
static int is_red(const Pos *P) { return (P->moves & 1) == 0; }
static u64 possible(const Pos *P) { return (P->mask + BOTTOM_MASK) & BOARD_MASK; }
/* threats of the player to move / of the opponent, under the asymmetric rules */
static u64 my_spots(const Pos *P) { return is_red(P) ? red_q(P->cur, P->mask, P->moves) : spots(P->cur, P->mask); }
static u64 opp_spots(const Pos *P) {
    u64 o = P->cur ^ P->mask;
    return is_red(P) ? spots(o, P->mask) : red_q(o, P->mask, P->moves + 1);
}
/* cells red must never fill: they complete a "wrong" four, or are off-limits */
static u64 red_forbidden(const Pos *P) {
    u64 all = spots(P->cur, P->mask), q = red_q(P->cur, P->mask, P->moves);
    return (all & ~q) | (BOARD_MASK & ~allowed_cols);
}

static u64 non_losing_moves(const Pos *P) {
    u64 pm = possible(P);
    u64 forb = is_red(P) ? red_forbidden(P) : 0;
    u64 opp = opp_spots(P);
    u64 forced = pm & opp;
    if (forced) {
        if (forced & (forced - 1)) return 0;
        if (forced & forb) return 0; /* red would have to break the prediction */
        pm = forced;
    }
    return pm & ~forb & ~(opp >> 1);
}

static u32 *tt_key; static u8 *tt_val;
static u32 *tt_keys[64]; static u8 *tt_vals[64]; /* one table per prediction */
#define MIN_SCORE (-(W * H) / 2 + 3)
#define MAX_SCORE ((W * H + 1) / 2 - 3)
static const int order[W] = {3, 2, 4, 1, 5, 0, 6};
static unsigned long long nodes;

static int negamax(const Pos *P, int alpha, int beta) {
    nodes++;
    u64 next = non_losing_moves(P);
    if (next == 0) return -(W * H - P->moves) / 2;
    if (P->moves >= W * H - 2) return 0;
    int min = -(W * H - 2 - P->moves) / 2;
    if (alpha < min) { alpha = min; if (alpha >= beta) return alpha; }
    int max = (W * H - 1 - P->moves) / 2;
    if (beta > max) { beta = max; if (alpha >= beta) return beta; }
    u64 k = P->cur + P->mask; u32 i = (u32)(k % TT_SIZE);
    int v = tt_key[i] == (u32)k ? tt_val[i] : 0;
    if (v) {
        if (v > MAX_SCORE - MIN_SCORE + 1) { min = v + 2 * MIN_SCORE - MAX_SCORE - 2; if (alpha < min) { alpha = min; if (alpha >= beta) return alpha; } }
        else { max = v + MIN_SCORE - 1; if (beta > max) { beta = max; if (alpha >= beta) return beta; } }
    }
    u64 mv[W]; int sc[W], n = 0;
    for (int j = W - 1; j >= 0; j--) {
        u64 m = next & column_mask(order[j]); if (!m) continue;
        Pos T = *P; T.cur |= m; T.mask |= m; /* threats this move creates for the mover */
        int s = popcount(is_red(P) ? red_q(T.cur, T.mask, P->moves) : spots(T.cur, T.mask));
        int pos = n++;
        for (; pos && sc[pos - 1] > s; pos--) { mv[pos] = mv[pos - 1]; sc[pos] = sc[pos - 1]; }
        mv[pos] = m; sc[pos] = s;
    }
    while (n) {
        Pos P2 = *P; u64 m = mv[--n];
        P2.cur ^= P2.mask; P2.mask |= m; P2.moves++;
        int score = -negamax(&P2, -beta, -alpha);
        if (score >= beta) { tt_key[i] = (u32)k; tt_val[i] = (u8)(score + MAX_SCORE - 2 * MIN_SCORE + 2); return score; }
        if (score > alpha) alpha = score;
    }
    tt_key[i] = (u32)k; tt_val[i] = (u8)(alpha - MIN_SCORE + 1);
    return alpha;
}

/* Does red (the first player) succeed from P, whoever is to move?  1/0 */
static int red_succeeds(Pos P) {
    if (is_red(&P)) {
        if (my_spots(&P) & possible(&P)) return 1;
        return negamax(&P, 0, 1) > 0;
    } else {
        if (my_spots(&P) & possible(&P)) return 0; /* yellow wins at once */
        return negamax(&P, -1, 0) < 0;
    }
}

static int setup(int prop) {
    PROP = prop; nq = 0; allowed_cols = BOARD_MASK;
    if (prop == 9) allowed_cols = BOARD_MASK & ~column_mask(0) & ~column_mask(6);
    if (prop == 10) allowed_cols = column_mask(2) | column_mask(3) | column_mask(4);
    nq = 0;
    for (int i = 0; i < nlines; i++) {
        if (prop == 7 && (lines[i] & cell(3, 0))) qlines[nq++] = lines[i];
        if (prop == 12 && (lines[i] & BOTTOM_MASK)) qlines[nq++] = lines[i];
        if (prop == 19 && !(lines[i] & (column_mask(0) | column_mask(6)))) qlines[nq++] = lines[i];
    }
    if (!tt_keys[prop]) { tt_keys[prop] = calloc(TT_SIZE, 4); tt_vals[prop] = calloc(TT_SIZE, 1); }
    tt_key = tt_keys[prop]; tt_val = tt_vals[prop]; /* switching keeps each table's contents */
    return 0;
}

#ifndef NO_MAIN
int main(int argc, char **argv) {
    init_lines();
    int prop = atoi(argv[1]);
    setup(prop);
    Pos P = {0, 0, 0};
    const char *pre = argc > 2 ? argv[2] : "4"; /* red always opens in the centre */
    for (const char *s = pre; *s; s++) { u64 m = (P.mask + bottom_mask_col(*s - '1')) & column_mask(*s - '1'); P.cur ^= P.mask; P.mask |= m; P.moves++; }
    time_t t0 = time(0);
    int ok = red_succeeds(P);
    printf("%-62s after \"%s\": %s  (%lds, %llu nodes)\n", PROP_NAME[prop], pre, ok ? "FORCED" : "NOT FORCED", (long)(time(0) - t0), nodes);
    return 0;
}
#endif
