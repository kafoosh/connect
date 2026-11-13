# Connect 4 — the magician never loses

A normal-looking Connect 4 page for two players on one device. The spectator
plays yellow; the magician plays red and **always goes first**. After every
spectator move the page secretly shows the magician the column that wins.

Open `index.html` (works offline, straight from the file, or on GitHub Pages).

## The maths

Connect 4 (7 × 6) is a solved game: James D. Allen and Victor Allis
independently proved in 1988 that **the first player wins with perfect play
by starting in the centre column**. Starting anywhere else lets a perfect
opponent draw (columns 3 or 5) or win (columns 1, 2, 6, 7).

So the magician:

1. always plays first,
2. always opens in the centre,
3. then plays the column the page signals.

Against any spectator, that line is a forced win. Among winning moves the
page always picks the **fastest** one, so games against weak play end quickly.

## The secret signal

Pick the tell with a URL parameter (it is remembered on that device, so the
address bar stays clean afterwards):

| `?tell=` | What the magician looks for |
|---|---|
| `rest` *(default)* | On red's turn the floating red disc above the board settles over the winning column. On touch screens it just sits there; with a mouse, move the pointer off the board to see it. |
| `screw` | The seven little screws on the bottom rail have slanted slots. The screw under the winning column has its slot **vertical** (`|`). |
| `shade` | The rim of the next empty hole in the winning column is a shade lighter. Very subtle; best on a large screen. |
| `off` | No signal (plain game). |

Nothing is shown during the spectator's turn.

Themes: `?theme=classic` (default, blue board), `?theme=flat`, `?theme=night`.

![classic, flat and night themes](docs/designs.png)

Example: `index.html?theme=classic&tell=screw`

## How it works

- `solver/solver.c` — a perfect solver (negamax + alpha-beta over bitboards,
  null-window search, threat-based move ordering, transposition table). It
  returns the exact score of a position, i.e. win/draw/loss *and* how soon.
  It is freestanding C, compiled natively for tooling and to WebAssembly
  (~4 KB) for the page, where it runs in a Web Worker.
- `solver/gen.c` — walks the magician's strategy tree (magician's best move,
  every possible spectator reply) and records the best move for every
  magician turn up to ply 12. Early positions are the expensive ones (tens of
  seconds each), so they are precomputed; the page embeds this opening book.
  Beyond the book the live solver answers in milliseconds.
- `solver/sim.c` — plays thousands of games (random, "smart" and perfect-
  resistance spectators) to check the magician never fails to win.
- `solver/brute.c` — independent brute-force check of solver scores.
- `solver/book12/` — the generated opening book (36,032 magician positions,
  every one of them a proven win).

### Verification

- Solver vs. brute force on 600 random late-game positions: 0 mismatches.
- 40,300 simulated games (20,000 random spectator, 20,000 spectator that
  takes wins and blocks threats, 300 spectator that plays perfectly to
  resist): the magician won every one.
- Full games in the real page (headless Chromium, incl. touch emulation):
  every game won, signal appears within ~30 ms of the spectator's disc
  landing.
- `build.py` — compiles the WebAssembly, packs the book and writes
  `index.html` from `web/index.src.html`.

### Rebuild

```sh
cd solver
gcc -O3 -march=native -o gen gen.c
for c in 1 2 3 4 5 6 7; do ./gen 12 4$c > book12/b4$c.txt; done   # slow: run in parallel
cd ..
BOOK_DIR=book12 python3 build.py
```
