# CommunityBridge

A C++ console program that helps a coordinator decide where surplus food should go.

Food is often wasted while nearby organisations go without it. CommunityBridge takes a donation, checks which recipients can actually receive it, ranks them, and recommends the best options. The coordinator makes the final choice. The program only recommends.

## Current stage

**Phase 2: prototype.** The core building blocks and the recommendation logic are written. The full menu-driven workflow, saving, and travel-time calculation are still being built.

## How it works

1. **Donation.** A donor offers some food, with a quantity, a pickup location and an expiry time.
2. **Feasibility check.** Each recipient is checked against a set of rules (for example: is there enough time before the food expires, is the quantity acceptable). A recipient that fails is rejected and the reason is recorded, so the coordinator can see why.
3. **Ranking.** The recipients that pass are ranked by several keys, so the most suitable one comes first.
4. **Coordinator's choice.** The ranked list is shown. The coordinator picks one.
5. **Allocation and record.** The donation is allocated, a transaction is recorded, and the data is saved to CSV files.

Travel time between locations is meant to come from a weighted graph of locations, using shortest-path search. The distances in the sample data are **simulated**, not real map data.

## Data structures

The project is also a study of core data structures, so the main ones are written by hand instead of taken from the standard library:

- priority queue (min-heap)
- stack and queue
- hash index
- merge sort
- graph with shortest-path search

## Requirements

- A C++ compiler such as g++ (MinGW on Windows)
- No external libraries

## Folder layout

```
Phase2/
  data/       sample CSV files
  include/    header files
  src/        source files
```

## Code files

Files currently in the repository:

| File | What it does |
|------|--------------|
| `Duration` | A length of time, used for expiry and travel time |
| `EntityId` | Identifiers for donors, recipients, donations and transactions |
| `Exceptions` | Error types that carry the file, line and reason of a problem |
| `Models` | The main records: donor, recipient, donation, transaction |
| `Sorting` | Hand-written merge sort |
| `RecommendationEngine` | Feasibility rules and ranking of recipients |
| `ConsoleView` | Prints results and lists to the console |

Each module has a header in `include/` and a source file in `src/`. `Sorting` is header-only.

Still to come: loading and saving CSV data, the graph and travel-time calculation, allocation, and the main menu program.

## Tests

Each module has its own small test program that checks it on its own, separate from the rest of the project. A test file builds one module with known inputs and compares the result against expected values. This makes it easy to find which module a bug is in.

## Limits

- Travel times use simulated distances.
- The program recommends. It does not decide.
- This is a prototype, so some menu options and modules are not built yet.git log --all --format=%B | Select-String -Pattern "claude|anthropic" -CaseSensitive:$false
git log --all --format="%h | %an <%ae> | %cn" -n 20