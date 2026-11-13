/*
 * Perfect Connect 4 solver (7 columns x 6 rows).
 *
 * Connect 4 is solved (Allis / Allen, 1988): the first player wins with
 * perfect play by starting in the centre column. This solver computes the
 * exact game-theoretic score of any position, which lets us always pick a
 * move that wins as fast as possible (or, if winning is impossible, draws,
 * or loses as slowly as possible).
 *
 * Score convention (from the side to move):
 *   > 0  win;  the larger, the sooner the win  (score = 22 - stones the winner played)
 *   = 0  draw
 *   < 0  loss; the smaller, the sooner the loss
 *
 * Techniques: negamax + alpha-beta, bitboards, null-window binary search,
 * move ordering by threat count, anticipation of losing moves and a
 * transposition table storing upper and lower bounds.
 *
 * Freestanding C (no libc) so the same file builds natively (book
 * generation / tests) and to WebAssembly (the web page).
 */

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;

#define W 7
#define H 6
#define MIN_SCORE (-(W * H) / 2 + 3)
#define MAX_SCORE ((W * H + 1) / 2 - 3)

#ifndef TT_SIZE
#define TT_SIZE 8388617u /* prime; TT_SIZE * 2^32 > 2^49 so partial keys are unique */
#endif

#define BOTTOM_MASK 0x0040810204081ULL /* bit 0 of every column */
#define BOARD_MASK (BOTTOM_MASK * ((1ULL << H) - 1))

static u64 top_mask_col(int c) { return (1ULL << (H - 1)) << (c * (H + 1)); }
static u64 bottom_mask_col(int c) { return 1ULL << (c * (H + 1)); }
static u64 column_mask(int c) { return ((1ULL << H) - 1) << (c * (H + 1)); }

typedef struct {
    u64 cur;  /* stones of the player to move */
    u64 mask; /* all stones */
    int moves;
} Pos;

static int popcount(u64 m) {
    int c = 0;
    for (; m; c++) m &= m - 1;
    return c;
}

static u64 winning_spots(u64 p, u64 mask) {
    u64 r = (p << 1) & (p << 2) & (p << 3); /* vertical */
    u64 q;
    /* horizontal */
    q = (p << (H + 1)) & (p << 2 * (H + 1));
    r |= q & (p << 3 * (H + 1));
    r |= q & (p >> (H + 1));
    q = (p >> (H + 1)) & (p >> 2 * (H + 1));
    r |= q & (p << (H + 1));
    r |= q & (p >> 3 * (H + 1));
    /* diagonal 1 */
    q = (p << H) & (p << 2 * H);
    r |= q & (p << 3 * H);
    r |= q & (p >> H);
    q = (p >> H) & (p >> 2 * H);
    r |= q & (p << H);
    r |= q & (p >> 3 * H);
    /* diagonal 2 */
    q = (p << (H + 2)) & (p << 2 * (H + 2));
    r |= q & (p << 3 * (H + 2));
    r |= q & (p >> (H + 2));
    q = (p >> (H + 2)) & (p >> 2 * (H + 2));
    r |= q & (p << (H + 2));
    r |= q & (p >> 3 * (H + 2));
    return r & (BOARD_MASK ^ mask);
}

static u64 possible(const Pos *P) { return (P->mask + BOTTOM_MASK) & BOARD_MASK; }
static u64 key(const Pos *P) { return P->cur + P->mask; }
static int can_play(const Pos *P, int c) { return (P->mask & top_mask_col(c)) == 0; }
static void play_bit(Pos *P, u64 move) {
    P->cur ^= P->mask;
    P->mask |= move;
    P->moves++;
}
static u64 col_move(const Pos *P, int c) { return (P->mask + bottom_mask_col(c)) & column_mask(c); }
static int is_winning_move(const Pos *P, int c) {
    return (winning_spots(P->cur, P->mask) & possible(P) & column_mask(c)) != 0;
}
static int can_win_next(const Pos *P) { return (winning_spots(P->cur, P->mask) & possible(P)) != 0; }

static u64 non_losing_moves(const Pos *P) {
    u64 pm = possible(P);
    u64 opp = winning_spots(P->cur ^ P->mask, P->mask);
    u64 forced = pm & opp;
    if (forced) {
        if (forced & (forced - 1)) return 0; /* two threats: cannot block both */
        pm = forced;
    }
    return pm & ~(opp >> 1); /* never play directly below an opponent threat */
}

static int move_score(const Pos *P, u64 move) { return popcount(winning_spots(P->cur | move, P->mask)); }

/* ---------- transposition table ---------- */
static u32 tt_key[TT_SIZE];
static u8 tt_val[TT_SIZE];
static void tt_reset(void) {
    for (u32 i = 0; i < TT_SIZE; i++) { tt_key[i] = 0; tt_val[i] = 0; }
}
static void tt_put(u64 k, u8 v) {
    u32 i = (u32)(k % TT_SIZE);
    tt_key[i] = (u32)k;
    tt_val[i] = v;
}
static u8 tt_get(u64 k) {
    u32 i = (u32)(k % TT_SIZE);
    return tt_key[i] == (u32)k ? tt_val[i] : 0;
}

static const int column_order[W] = {3, 2, 4, 1, 5, 0, 6};
static unsigned long long node_count;
static unsigned long long node_limit; /* 0 = unlimited */
static int aborted;

/* Precondition: side to move cannot win immediately. */
static int negamax(const Pos *P, int alpha, int beta) {
    if (aborted) return 0;
    if (++node_count > node_limit && node_limit) { aborted = 1; return 0; }
    u64 next = non_losing_moves(P);
    if (next == 0) return -(W * H - P->moves) / 2;
    if (P->moves >= W * H - 2) return 0;

    int min = -(W * H - 2 - P->moves) / 2;
    if (alpha < min) {
        alpha = min;
        if (alpha >= beta) return alpha;
    }
    int max = (W * H - 1 - P->moves) / 2;
    if (beta > max) {
        beta = max;
        if (alpha >= beta) return beta;
    }

    u64 k = key(P);
    int v = tt_get(k);
    if (v) {
        if (v > MAX_SCORE - MIN_SCORE + 1) { /* lower bound */
            min = v + 2 * MIN_SCORE - MAX_SCORE - 2;
            if (alpha < min) {
                alpha = min;
                if (alpha >= beta) return alpha;
            }
        } else { /* upper bound */
            max = v + MIN_SCORE - 1;
            if (beta > max) {
                beta = max;
                if (alpha >= beta) return beta;
            }
        }
    }

    /* order moves: more threats created first, centre first on ties */
    u64 mv[W];
    int sc[W], n = 0;
    for (int i = W - 1; i >= 0; i--) {
        u64 m = next & column_mask(column_order[i]);
        if (!m) continue;
        int s = move_score(P, m);
        int pos = n++;
        for (; pos && sc[pos - 1] > s; pos--) { mv[pos] = mv[pos - 1]; sc[pos] = sc[pos - 1]; }
        mv[pos] = m;
        sc[pos] = s;
    }

    while (n) {
        Pos P2 = *P;
        play_bit(&P2, mv[--n]);
        int score = -negamax(&P2, -beta, -alpha);
        if (aborted) return 0;
        if (score >= beta) {
            tt_put(k, (u8)(score + MAX_SCORE - 2 * MIN_SCORE + 2));
            return score;
        }
        if (score > alpha) alpha = score;
    }
    tt_put(k, (u8)(alpha - MIN_SCORE + 1));
    return alpha;
}

static int solve(const Pos *P) {
    if (can_win_next(P)) return (W * H + 1 - P->moves) / 2;
    int min = -(W * H - P->moves) / 2;
    int max = (W * H + 1 - P->moves) / 2;
    while (min < max) {
        int med = min + (max - min) / 2;
        if (med <= 0 && min / 2 < med) med = min / 2;
        else if (med >= 0 && max / 2 > med) med = max / 2;
        int r = negamax(P, med, med + 1);
        if (r <= med) max = r;
        else min = r;
    }
    return min;
}

/* Exact score of every column (-1000 = column full). */
static void analyze(const Pos *P, int *scores) {
    for (int c = 0; c < W; c++) {
        if (!can_play(P, c)) { scores[c] = -1000; continue; }
        if (is_winning_move(P, c)) { scores[c] = (W * H + 1 - P->moves) / 2; continue; }
        Pos P2 = *P;
        play_bit(&P2, col_move(P, c));
        scores[c] = -solve(&P2);
    }
}

/* Best column: maximises score; ties go to the most central column. */
static int best_move(const Pos *P, int *out_score) {
    int s = solve(P);
    if (out_score) *out_score = s;
    for (int i = 0; i < W; i++) {
        int c = column_order[i];
        if (!can_play(P, c)) continue;
        if (is_winning_move(P, c)) {
            if ((W * H + 1 - P->moves) / 2 == s) return c;
            continue;
        }
        Pos P2 = *P;
        play_bit(&P2, col_move(P, c));
        if (can_win_next(&P2)) continue; /* opponent would win at once */
        /* child score <= -s  <=>  this move achieves s */
        if (negamax(&P2, -s, -s + 1) <= -s) return c;
    }
    for (int c = 0; c < W; c++) if (can_play(P, c)) return c; /* unreachable */
    return -1;
}

/* Fallback when the search budget runs out: win now, block, else the
   non-losing move creating most threats (centre first on ties). */
static int heuristic_move(const Pos *P) {
    for (int c = 0; c < W; c++) if (can_play(P, c) && is_winning_move(P, c)) return c;
    u64 next = non_losing_moves(P);
    int best = -1, bs = -1;
    for (int i = 0; i < W; i++) {
        int c = column_order[i];
        u64 m = next & column_mask(c);
        if (!m) continue;
        int s = move_score(P, m);
        if (s > bs) { bs = s; best = c; }
    }
    if (best < 0) for (int i = 0; i < W; i++) if (can_play(P, column_order[i])) return column_order[i];
    return best;
}

/* ---------- small API (used by wasm and by the native tools) ---------- */
static Pos g;
static int g_score;

void c4_reset(void) { g.cur = g.mask = 0; g.moves = 0; }
void c4_clear_tt(void) { tt_reset(); }
int c4_can_play(int c) { return c >= 0 && c < W && can_play(&g, c); }
int c4_play(int c) {
    if (!c4_can_play(c)) return 0;
    play_bit(&g, col_move(&g, c));
    return 1;
}
int c4_moves(void) { return g.moves; }
int c4_best(void) { return best_move(&g, &g_score); }
/* Best move with a node budget; returns the heuristic move if exceeded
   (c4_score() is then -1000). */
int c4_best_limited(double limit) {
    node_count = 0;
    node_limit = (unsigned long long)limit;
    aborted = 0;
    int m = best_move(&g, &g_score);
    node_limit = 0;
    if (aborted) {
        aborted = 0;
        tt_reset(); /* entries written while unwinding are unreliable */
        g_score = -1000;
        m = heuristic_move(&g);
    }
    return m;
}
int c4_score(void) { return g_score; }
int c4_solve(void) { return solve(&g); }
double c4_nodes(void) { return (double)node_count; }
