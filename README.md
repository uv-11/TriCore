# CommunityBridge

A C++ console program that helps a coordinator decide where surplus food should go.

Food is often wasted while nearby organisations go without it. CommunityBridge takes a donation, checks which recipients can actually receive it, ranks them, and recommends the best options. The coordinator makes the final choice. The program only recommends.

## Current stage

**Phase 2: working console program.** The whole workflow runs from start to end. The program loads and checks the data files, picks the most urgent donation, works out travel times over a road map, applies the feasibility rules, ranks the recipients, lets the coordinator allocate the food, and saves every change. New records can be added from the program too: a donation, a donor (while adding a donation) and a recipient.

## How it works

1. **Donation.** A donor offers some food, with a quantity, a pickup location on the map and a usable time in minutes. Waiting donations are ordered by urgency: the one with the least usable time comes first.
2. **Travel time.** A shortest-path search (Dijkstra's algorithm) finds the travel time and the route from the donor's location to every other location on a weighted road map.
3. **Feasibility check.** Every recipient is checked against six rules, in this order. The first rule a recipient fails is recorded as its reason, so the coordinator can see why it was rejected.

| Reason | Meaning |
|--------|---------|
| `NOT_ACCEPTING` | The recipient is not accepting donations right now |
| `FOOD_INCOMPATIBLE` | The recipient does not take this type of food |
| `NO_CAPACITY` | The recipient has no room left |
| `NO_PICKUP` | The recipient cannot collect the food |
| `UNREACHABLE` | No road connects the recipient to the donor |
| `TOO_FAR` | The travel time is more than the usable time of the food (equal is allowed) |

4. **Ranking.** The recipients that pass are ranked with four keys, in this order: more food accepted (the smaller of what is left of the donation and what the recipient can take), shorter travel time, less unused capacity, smaller recipient number.
5. **Coordinator's choice.** The ranked list is shown with the route to each recipient, and the rejected recipients are shown with their reasons. The coordinator types a candidate number and a quantity, or `c` to close the donation. A donation can be split between several recipients, and the list is ranked again after each allocation.
6. **Allocation and record.** The checks run again, the donation and the recipient are updated, a transaction is recorded and the files are saved. If nobody can take the food, or the coordinator closes the donation, the food that is left is recorded as wasted.

Travel times come from a weighted graph of locations. The distances in the sample data are **simulated**, not real map data.

## Menu

```
1. Process the most urgent donation
2. Add a donation
3. Add a recipient
4. Show donations
5. Show recipients
6. Show transactions
7. Show road network
8. Show travel times from a donor
9. Show summary
0. Exit
```

- **Add a donation** lists the donors. The coordinator picks one by number or by id, or types `n` to create a new donor first.
- **Add a recipient** asks for the name, type, accepted food types, capacity, whether it accepts donations and can collect food, and its location on the map.
- New donors and recipients use a location that is already on the map. The program warns if that location is not connected to any recipient or donor.
- Typing `q` at any question cancels and saves nothing. Nothing is saved until every question has been answered.
- Ids are given by the program (`DN`, `R`, `DON` or `T` followed by a number), so an id is never repeated.
- **Show summary** shows the total quantity offered, allocated, wasted and still open, the allocated quantity for each food type, and a short description of the program.

## Data files

| File | What it holds |
|------|---------------|
| `donors.csv` | The organisations that give food and where they are on the map |
| `recipients.csv` | The organisations that receive food: capacity, accepted food types, whether they accept donations and can collect food, and location |
| `donations.csv` | Each donation: donor, food type, quantity offered, quantity left, usable minutes and status |
| `transactions.csv` | Every allocation: donation, recipient, quantity and travel minutes |
| `graph_nodes.csv` | The locations on the map: number, label and coordinates |
| `graph_edges.csv` | The roads between locations, with travel minutes |

A donation is `PENDING` (nothing given yet), `PARTIAL`, `COMPLETED` (everything given) or `CLOSED` (closed with food left over).

The program checks every file when it starts: the header line, that ids are unique, that every reference exists, and that quantities and statuses agree. If something is wrong it stops with a message that names the file and the line. Every save goes through a temporary file, so an interrupted save does not damage the last good data.

`Phase2/data` holds a larger sample dataset with edge cases on purpose: a very large donation, a donation that practically never expires, a case where the travel time equals the usable time exactly, and parts of the map that are cut off from the rest.

## Data structures

The project is also a study of core data structures, so the main ones are written by hand instead of taken from the standard library:

- priority queue (min-heap), used for the urgency order and for the shortest-path search
- stack, used to build a route
- queue (built and tested, the main program does not need it)
- hash index, used to find a record by its id
- merge sort, used to rank the recipients
- graph with shortest-path search

## Requirements

- A C++ compiler such as g++ (MinGW on Windows), with C++17
- No external libraries

## Running

```
.\demo.bat
.\run.bat
cmd /c "Phase2\tests\test_all.bat nopause"
```

- `demo.bat` builds the program and runs it on a fresh copy of the sample data, so the real data is never changed.
- `run.bat` builds the program and runs it on `Phase2\data`, so every change is saved there. A different data folder can be given: `.\run.bat Phase2\data_check`.
- `test_all.bat` builds and runs every test and prints a summary.

## Folder layout

```
Phase2/
  data/       sample CSV files
  include/    header files
  src/        source files
  tests/      test programs and the test runner
run.bat       build and run the program
demo.bat      build and run on a fresh copy of the data
```

## Code files

| File | What it does |
|------|--------------|
| `Duration` | A length of time in minutes, with a safe value for a location that cannot be reached |
| `EntityId` | Ids for donors, recipients, donations and transactions |
| `Exceptions` | Error types that say what went wrong and where |
| `Models` | The main records: donor, recipient, donation, transaction |
| `Datamanager` | Loads, checks and saves the CSV files, and keeps everything in memory while the program runs |
| `graph` | The road map, stored as a list of roads for each location |
| `priorityqueue` | Gives the most urgent item first, for the urgency order and the shortest-path search |
| `intstack` | Builds a route from the result of the shortest-path search |
| `intqueue` | A first-in first-out list of numbers |
| `hashindex` | Finds a donor, recipient or donation by its id |
| `traveltime` | Shortest travel time and route between locations, plus two simple estimates that the tests use for comparison |
| `RecommendationEngine` | Feasibility rules and ranking of recipients |
| `Sorting` | Merge sort, used to rank the recipients |
| `allocationmanager` | Carries out an allocation, closes donations, keeps the urgency order and gives out new ids |
| `ConsoleView` | Every screen and question of the console program |
| `main` | Starts the program, loads the data and runs the menu |

Each module has a header in `include/` and a source file in `src/`. `Sorting` is header-only.

## Tests

Each module has its own small test program that checks it on its own, separate from the rest of the project. A test builds one part of the program with known inputs and compares the result against expected values. This makes it easy to find which part a bug is in.

Together the tests cover the feasibility rules, the ranking, the shortest paths, loading and saving, the console screens (with typed answers) and the larger sample dataset. One evaluation test compares the real travel times with a straight-line estimate and with no travel check at all, over a grid of scenarios.

## Limits

- Travel times use simulated distances.
- The program recommends. The coordinator decides.
- New locations and roads are added by editing the two map files, not from the program.
- Donors and recipients cannot be edited or deleted from the program.
- Names cannot contain a comma, because the data files are comma separated.
- The program runs in the console.

## Next

- A graphical window on top of the same code.
- Adding locations and roads from inside the program.