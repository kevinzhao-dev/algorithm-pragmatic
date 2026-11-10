# algorithm-pragmatic

A C++ training path that turns the assignments of three MIT courses into exercises you work through in order. It targets the kind of coding interview frontier labs run: get the code right, extend it round after round, then test and debug it.

This is an unofficial adaptation made for personal study, and it is not affiliated with MIT. See [License and attribution](#license-and-attribution) at the end.

| Order | Course | What you practice | Size |
| --- | --- | --- | --- |
| 1 | MIT 6.006 Introduction to Algorithms (Spring 2020) | Data structures, sorting, graphs, dynamic programming | 80 exercises, about 102 hours |
| 2 | MIT 6.046J Design and Analysis of Algorithms (Spring 2015) | Advanced algorithm design and analysis | 50 exercises, about 106 hours |
| 3 | MIT 6.172 Performance Engineering (Fall 2018) | Measurement, caches, parallelism, allocators | 50 exercises, about 118 hours |

Each unit starts with core implementations, where you write the data structures from the lecture notes yourself. Then come all of that unit's official assignment problems. Later exercises often reuse code you wrote earlier: once your AVL tree works, the leaderboard and the text buffer are built on top of it.

6.172 doesn't officially require 6.046J, so you can start it right after 6.006, or interleave the two. The full list of exercises is in [ROADMAP.md](ROADMAP.md).

## Getting started

You need Apple clang (or another clang with C++20 support), CMake and Ninja.

```bash
./practice next
```

It tells you which exercise is next, which lecture notes to read first, and where the exercise lives. Implement it in that exercise's `solution.hpp`, then run:

```bash
./practice test 0114
```

`0114` is the exercise number. A fragment of the slug works too, such as `doubly`. When a number exists in more than one course, add the course, as in `./practice test 6.006/0111`.

To follow a single course:

```bash
./practice next --course 6.172
```

## Stages

Each exercise's `README.md` is split into stages. The requirements arrive one stage at a time, the way an interviewer adds new requirements after you finish.

- Earlier tests keep running, so you find out right away when a new requirement breaks an old feature.
- Functional tests run under AddressSanitizer and UBSan, so memory errors don't slip through.
- Performance tests in a Release build enforce the complexity requirements; an O(n²) solution times out.
- Each exercise ends with interview follow-ups, which you should be able to answer without looking at your code.

Every README links to its MIT source on OCW. Assignment problems also restate the original problem in full, and each stage lists the exact C++ declarations it asks for, so you never need the PDF to get started.

```
6.006/0114-doubly-linked-list — Doubly Linked List (O(1) range moves)

  stage 1  ✓ 10/10
  stage 2  ✗ 2/6  ← current stage
  stage 3  · locked
```

There are four kinds of exercises:

- `code`: implementation.
- `perf`: performance engineering, measured as a speedup.
- `project`: a large exercise with many stages.
- `paper`: a pure proof or analysis. Once you've written your answer and compared it with the official solution, mark it done with `./practice done <number>`.

## Practicing like it's an interview

- Read one stage at a time, and finish it before reading on.
- To simulate live coding, turn off AI completion and time yourself against the README's `minutes`. Explain the requirements and your data structures before you start writing code.
- When you're stuck, go back to the lecture notes first. `solutions/` holds the reference solutions and **gives everything away**, so answer the interview follow-ups before you look.
- To practice agentic coding, write the acceptance criteria yourself and let an agent implement them. Then review its diff and add tests. Make sure you can explain every key decision yourself.

## Layout

| Path | Contents |
| --- | --- |
| `problems/` | Exercises, skeletons and tests, by course and number |
| `problems/<course>/_shared/` | Helpers shared by the tests. Files marked `SPOILER` contain the solution to an exercise, and that exercise's README warns you |
| `solutions/` | Reference solutions (spoilers) |
| `materials/` | MIT's lecture notes, slides, assignment PDFs and code bundles |
| `harness/` | The test framework, plus the fork-join runtime that replaces Cilk |
| `docs/AUTHORING.md` | The exercise format, for adding exercises yourself |
| `progress.json` | Your progress, updated automatically by `./practice` |

Exercise numbers have the form `UUNN`, where `UU` is the unit and `NN` is the exercise:

- `01`–`09`: core implementations.
- `11`–`19`: assignment problems; the last digit is the problem number.
- `21` onward: extensions.

## Course materials

`materials/` holds the slides or lecture notes and the assignments, without videos. PDFs and ZIPs are not in version control. `MANIFEST.csv` records each file's source and SHA-256. To download the files again:

```bash
python3 -I tools/fetch_materials.py
```

To verify them against `MANIFEST.csv`:

```bash
python3 -I tools/fetch_materials.py --verify
```

These files are copyrighted by MIT and their authors. The repo only ships the download script and does not redistribute the files themselves. See [License and attribution](#license-and-attribution) for sources and licensing.

## Environment

Some of the original courses' tools don't exist on Apple Silicon, so the exercises substitute them:

| Original course | Here |
| --- | --- |
| Python templates | C++20 |
| Cilk | `fork2` / `parallel_for` from `harness/forkjoin.hpp` |
| Cilksan | ThreadSanitizer |
| perf / cachegrind | Timers, counters, a deterministic cache simulator, and `sample` / `xctrace` |
| Valgrind | ASan / UBSan, plus the `Tracked` type for leak checks |
| AVX2 / x86 assembly | Compiler vectorization reports and ARM NEON |

## License and attribution

Everything in this repo (exercises, tests, reference solutions, tools and documentation) is licensed under [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/). The full text is in [LICENSE](LICENSE). You may share and adapt it freely, as long as you:

- give credit;
- don't use it for commercial purposes;
- use the same license when you share an adapted version.

The exercises are adapted from these three MIT OpenCourseWare courses, whose materials are also licensed under CC BY-NC-SA 4.0:

- Erik Demaine, Jason Ku, and Justin Solomon. *6.006 Introduction to Algorithms*. Spring 2020. Massachusetts Institute of Technology: MIT OpenCourseWare, <https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/>. License: Creative Commons BY-NC-SA.
- Erik Demaine, Srini Devadas, and Nancy Lynch. *6.046J Design and Analysis of Algorithms*. Spring 2015. Massachusetts Institute of Technology: MIT OpenCourseWare, <https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/>. License: Creative Commons BY-NC-SA.
- Charles Leiserson and Julian Shun. *6.172 Performance Engineering of Software Systems*. Fall 2018. Massachusetts Institute of Technology: MIT OpenCourseWare, <https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/>. License: Creative Commons BY-NC-SA.

How the material was adapted:

- **Rewritten:** the problem statements, interfaces, tests and reference solutions were rewritten in C++.
- **Restated:** each README restates its original problem in its own words and links to the original PDF on OCW.
- **Tools substituted:** the original courses' Python templates, Cilk and x86 tools were replaced with the substitutes listed under [Environment](#environment).
- **New game:** 6.172's Leiserchess project was replaced with an original game.
- **Not redistributed:** the repo does not include the original lecture notes or assignment files. Files downloaded with `tools/fetch_materials.py` remain subject to the [OCW terms of use](https://ocw.mit.edu/pages/privacy-and-terms-of-use/).

This is an unofficial adaptation for personal study. It is not affiliated with MIT or MIT OpenCourseWare, and has not been reviewed or endorsed by them.
