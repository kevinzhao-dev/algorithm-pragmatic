---
title: Baseball Elimination (which teams can still win the league)
unit: 07 · Network Flow (PS7)
source: 6.046J Lecture 14 (Baseball Elimination)
pdf: materials/6.046j/lectures/lec14b.pdf
read: materials/6.046j/lectures/lec14b.pdf
kind: code
minutes: 80
---

# 0703 · Baseball Elimination

**Based on:** [Baseball Elimination Notes](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bd6083516a6e9fd003be011d866c472f_MIT6_046JS15_lec14B.pdf) · **Read first:** [Baseball Elimination Notes](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bd6083516a6e9fd003be011d866c472f_MIT6_046JS15_lec14B.pdf)

> **Why this comes up in practice:** the "magic number" in leaderboard products, season scheduling (can the remaining games still let a team finish first?), and any question of the form "given the unfinished games, can this outcome still happen?" can all be decided with flow.

## Your task

`Standings` describes the current standings: `wins[i]` is the number of games team i has won, and `games_left[i][j]` is the number of games left between i and j (symmetric, with 0 on the diagonal). Every game ends in a win or a loss; there are no ties.

A team is **eliminated** when no outcome of the remaining games lets it finish with the most wins (a tie for first still counts as having a chance, so it is not eliminated).

You go from a quick check to the exact flow-based test from Lecture 14, then to a certificate a person can check, and finally to a tracker that updates as games are played.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Obviously eliminated

```cpp
struct Standings {
    std::vector<int> wins;
    std::vector<std::vector<int>> games_left;
};

bool trivially_eliminated(const Standings& s, int team);
```

`bool trivially_eliminated(const Standings& s, int team)`: compare only "`team`'s wins plus all of its remaining games won" against "the number of games each other team has already won".

## Stage 2 — Deciding with flow

```cpp
bool is_eliminated(const Standings& s, int team);
```

`bool is_eliminated(const Standings& s, int team)`: build a flow network to decide exactly. If some team's `wins[i]` already exceeds the most that `team` can reach, report it as eliminated right away (capacities can't be negative).

## Stage 3 — An elimination certificate

```cpp
std::vector<int> elimination_certificate(const Standings& s, int team);
```

`std::vector<int> elimination_certificate(const Standings& s, int team)`: if `team` is not eliminated, return an empty vector; otherwise return a subset `R` of the teams (not including `team`, in increasing order) such that

```
(wins(R) + games_within(R)) / |R| > wins[team] + remaining(team)
```

where `wins(R)` is the total number of wins of the teams in R, `games_within(R)` is the total number of games left between teams in R, and `remaining(team)` is the number of games `team` itself has left. A person can understand this set at a glance: however the games within R are split, some team in R ends up with more wins than `team`.

## Stage 4 — Real-world extension: a season tracker

```cpp
class SeasonTracker {
public:
    explicit SeasonTracker(Standings initial);
    void record_game(int winner, int loser);        // a game in which winner beat loser has been played
    std::vector<int> contenders() const;            // teams that are not eliminated yet, in increasing order
    bool is_contender(int team) const;
};
```

Each game passed to `record_game` must be a matchup that is still on the remaining schedule. You don't need to maintain incremental state; recomputing O(n) flows is fine.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is a saturated max flow equivalent to "every game can be assigned a winner without anyone going over team's maximum"? What role does flow integrality play here?
2. Why is the other side of the min cut exactly the elimination certificate R?
3. If the rules changed so that a win earns points and a tie earns each team 1 point (as in soccer), how would the model change? Why is the difficulty completely different?
