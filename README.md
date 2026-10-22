# CommunityBridge

A C++ console program that helps a coordinator decide where surplus food should go.

Food is often wasted while nearby organisations go without it. CommunityBridge takes a donation, checks which recipients can actually receive it, ranks them, and recommends the best options. The coordinator makes the final choice. The program only recommends.

## Current stage

**Phase 2: the full pipeline works end to end.** Loading, checking feasibility, ranking, allocating and saving all run against real data, including real travel time over a road network. A small number of pieces (adding a donation from the console, one data structure used for a later comparison) are still not built.

## How it works

1. **Donation.** A donor offers some food, with a quantity, a pickup location and an expiry time.
2. **Feasibility check.** Each recipient is checked against six ordered rules (for example: does it accept this food type, is there enough time to deliver it before it expires). A recipient that fails is rejected and the reason is recorded, so the coordinator can see why.
3. **Ranking.** The recipients that pass are ranked by several keys, so the most suitable one comes first.
4. **Coordinator's choice.** The ranked list is shown. The coordinator picks one.
5. **Allocation and record.** The donation is allocated, a transaction is recorded, and the data is saved to CSV files.

Travel time between locations comes from a weighted graph of locations, using Dijkstra's shortest-path algorithm. The distances in the sample data are **simulated**, not real map data.

## Data structures

The project is also a study of core data structures, so the main ones are written by hand instead of taken from the standard library:

- priority queue (min-heap)
- stack
- hash index
- merge sort
- graph with shortest-path search (Dijkstra's algorithm)

## Requirements

- A C++ compiler such as g++ (MinGW on Windows)
- No external libraries

## Folder layout

```
Phase2/
  data/       sample CSV files
  include/    header files
  src/        source files
  tests/      test programs
```

## Code files

Files currently in the repository:

| File | What it does |
|------|--------------|
| `Duration` | A length of time, used for expiry and travel time |
| `EntityId` | Identifiers for donors, recipients, donations and transactions |
| `Exceptions` | Error types that carry the file, line and reason of a problem |
| `Models` | The main records: donor, recipient, donation, transaction |
| `Datamanager` | Loads, checks and saves the CSV files |
| `graph` | The road network, stored as a linked list of connections |
| `priorityqueue` | Keeps the most urgent donation, and the nearest location during routing, at the front |
| `intstack` | Builds a travel route from the result of the shortest-path search |
| `hashindex` | Looks up a donor, recipient or donation by its id |
| `traveltime` | Shortest travel time and route between two locations, using Dijkstra's algorithm |
| `RecommendationEngine` | Feasibility rules and ranking of recipients |
| `Sorting` | Merge sort, used to rank recipients |
| `allocationmanager` | Carries out an allocation and keeps the most urgent donation first |
| `ConsoleView` | The menu, and prints results and lists to the console |
| `main` | Loads the data and runs the menu, with no decision logic of its own |

Each module has a header in `include/` and a source file in `src/`. `Sorting` is header-only.

Still to come: adding a donation from the console, and one more data structure planned for a later comparison test.

## Tests

Each module has its own small test program that checks it on its own, separate from the rest of the project. A test file builds one module with known inputs and compares the result against expected values. This makes it easy to find which module a bug is in.

## Limits
- The dataset isi static.
- Travel times use simulated distances.
- The program recommends. It does not decide.
- Adding a donation from the console is not built yet.
