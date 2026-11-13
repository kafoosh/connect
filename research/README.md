# Can the magician force a prediction?

`prop.c` is the perfect solver with one change: red (the magician, moving first)
only counts as winning if a stated prediction comes true. Real rules still
apply: any four ends the game, so red must never complete a "wrong" four, must
still block, and draws or yellow wins count as failure. `brute_prop.c` checks
it against a plain game-tree search (thousands of late positions, 0 mismatches).

## Results (red opens in the centre)

Against a perfect opponent, only the plain win is forced. Every more specific
prediction tested can be beaten: orientation of the winning four (horizontal,
vertical, diagonal, any pair of these, "not vertical"), passing through the
centre column, containing the first disc, touching the bottom row, avoiding
the edges, the column of the winning disc, never using the edge columns,
winning only late / with the last disc.

"Winning four passes through the centre column" and "winning four is not
vertical" become forced as soon as the spectator's first reply is in column
1, 2, 6 or 7.

`sim_prop.c`: magician plays to force the prediction whenever possible and
otherwise plays the normal perfect move. Over 200 games each:

| Prediction | random spectator | careful spectator | typically locked by ply |
|---|---|---|---|
| passes through the centre column | 200/200 | 200/200 | ~4 |
| not vertical | 199/200 | 200/200 | ~4 |
