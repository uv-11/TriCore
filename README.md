# CommunityBridge

**Send surplus food to the recipient who can take it in time.**

CommunityBridge is a C++ program for a food donation coordinator. A donor has food that will spoil soon. The program finds which recipients can really take it, shows them in the best order, and explains why every other recipient was left out. The coordinator makes the final choice. The program only recommends.

It comes in two forms that share the same logic:

| Form | What it is | Start it with |
|------|------------|---------------|
| Console program | A menu in the terminal | `.\demo.bat` |
| Windows app | One window with a screen for each kind of user | `Phase3\run_gui.bat` |

---

## 1. The idea 

Food is often wasted while nearby organisations go without it. The hard part is not finding a recipient. It is finding one that:

1. is open to donations and takes this kind of food,
2. has room,
3. can collect the food,
4. and can be reached **before the food spoils**.

Doing this by hand for many donations is slow and easy to get wrong. CommunityBridge does the checking in a fixed order, finds real travel times over a road map, and keeps a record of every decision.

Two program assures :

- **Every rejection has a reason.** The coordinator never sees "no" without a why.
- **Same data, same answer.** The ranking order is fixed, so results can be repeated and checked.

## 2. Who uses it

| Role | What they do |
|------|--------------|
| Coordinator | Looks at waiting donations, sees the ranked recipients, and decides who gets how much |
| Donor | Gives food (a hotel, caterer or market) and follows what happened to it |
| Recipient | Receives food (a shelter, food bank or kitchen) and keeps its own details up to date |

In the console program one person plays the coordinator. In the Windows app each role has its own screens.

---

## 3. How a donation travels

```
 load and check the files
            |
            v
 open donations wait in an urgency queue  (least usable time first)
            |
            v
 travel time from the donor's location to every place on the map  (Dijkstra)
            |
            v
 six feasibility rules for every recipient  (first failure is the reason)
            |
            v
 rank the recipients that pass  (four keys)
            |
            v
 coordinator chooses a recipient and a quantity, or closes the donation
            |
            v
 checks run again, then the donation, the recipient and a transaction are saved
            |
            +--> food left? go back to the ranking step, with the new numbers
```

### The six rules (in this order)

| # | Reason | Meaning |
|---|--------|---------|
| 1 | `NOT_ACCEPTING` | The recipient is not taking donations right now |
| 2 | `FOOD_INCOMPATIBLE` | The recipient does not take this type of food |
| 3 | `NO_CAPACITY` | The recipient has no room left |
| 4 | `NO_PICKUP` | The recipient cannot collect the food |
| 5 | `UNREACHABLE` | No road connects the recipient to the donor |
| 6 | `TOO_FAR` | The travel time is more than the usable time (equal is allowed) |

The order matters. A recipient that fails rule 2 is never shown as unreachable, even if it also is.

### The four ranking keys (in this order)

1. More food accepted (the smaller of what is left of the donation and what the recipient can take)
2. Shorter travel time
3. Less unused capacity
4. Smaller recipient number

### Worked example: DON001 on the sample data

The same donation as above, 60 portions of cooked food with 12 usable minutes. One recipient for each rule:

| Recipient | Result | Why |
|-----------|--------|-----|
| R017 | Rejected `NOT_ACCEPTING` | It switched donations off |
| R002 | Rejected `FOOD_INCOMPATIBLE` | It takes packaged, produce and bakery only |
| R020 | Rejected `NO_CAPACITY` | Its capacity is 0 |
| R021 | Rejected `NO_PICKUP` | It cannot collect food |
| R024 | Rejected `UNREACHABLE` | It sits in a part of the map with no road to any donor |
| R003 | Rejected `TOO_FAR` | 22 minutes of travel against 12 usable |
| R013 | Ranked 1 | 8 minutes, all 60 portions fit |
| R004 | Ranked 2 | 12 minutes, exactly the usable time, so it still passes |

### Partial allocation

A donation can be split. If the coordinator gives 40 of 60 portions to the first recipient, the list is ranked again for the 20 that are left, with the new capacity of that recipient. When nothing is left the donation is `COMPLETED`. If it is closed with food left over, that food is recorded as wasted.

---

## 4. The two programs

### Console menu

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

- **Add a donation** lists the donors. Pick one by number or id or type `n` to create a new donor first.
- **Add a recipient** asks for name, type, accepted food types, capacity, whether it accepts donations and can collect food, and a location on the map.
- Typing `q` at any question cancels and saves nothing. Nothing is saved until every question is answered.
- New donors and recipients use a location that is already on the map. The program warns if that location is not connected to any recipient or donor.

### Windows app

One window. A list on the left switches the pages. Nothing opens in a separate window. The look is light and plain on purpose.

| Role | Pages |
|------|-------|
| Coordinator | Home, Donations, Offers, Recipients, Transactions, Summary and a Match page for one donation |
| Donor | Home, My donations, Offers, Given out, Add donation |
| Recipient | Home, Incoming offers, Past offers, Received food, My details |

Each action lives in exactly one place. For example a recipient changes its details only on My details.

**Offers.** In the Windows app the coordinator does not allocate food directly. The coordinator sends an **offer**, and the recipient accepts or declines.

```
Coordinator sends an offer              ->  PENDING
PENDING, recipient accepts              ->  ACCEPTED   (allocation runs, transaction saved)
PENDING, recipient declines             ->  DECLINED   (not offered this donation again)
PENDING, coordinator withdraws
         or the donation is closed      ->  WITHDRAWN
```

Offers are soft. Nothing is reserved while an offer waits, so accepting runs the checks again. An old offer that no longer fits fails with a clear message and the data stays correct. Food that is already offered is shown as "already offered", so the same food is not offered twice.

The Windows app compiles the console program's source files directly. Both programs therefore use exactly the same rules, and the app never changes the console code.

---

## 5. How the code is organised

### Building blocks, from bottom to top

Each part uses only the parts below it. The screens make no decisions.

```
Screens          ConsoleView + main        |   Windows app pages
                                           |   Workflow  (offers, queries, tables)
Changes          allocationmanager         (allocate, close, urgency order, new ids)
Decisions        RecommendationEngine      (six rules and the ranking)
Travel time      traveltime                (Dijkstra and routes)
Data             Datamanager               (load, check, save)
Structures       graph, priorityqueue, intstack, intqueue, hashindex, Sorting
Records          Models                    (donor, recipient, donation, transaction)
Basics           Duration, EntityId, Exceptions
```

The decision part receives only a list of travel times. It does not care how they were found, so the travel-time method can change without touching the rules.

### What each file does and why it exists

| File | What it does | Why it is separate |
|------|--------------|--------------------|
| `Duration` | A length of time in minutes, with a safe value for "cannot be reached" | Adding two times never overflows, and unreachable stays unreachable |
| `EntityId` | Ids such as `DN001` and `DON012` | One place decides the format, the order and the next number |
| `Exceptions` | Error types for bad text bad data and wrong use | The program can tell a data problem from a coding mistake |
| `Models` | Donor, recipient, donation and transaction records | The rest of the program talks about records, not text lines |
| `Datamanager` | Loads, checks and saves the data files, and holds the data in memory | All file work is in one place |
| `graph` | The road map, stored as a list of roads for each location | Only roads that exist take space |
| `priorityqueue` | Gives the most urgent item first | Used for the urgency order and for Dijkstra |
| `intstack` | Builds a route from the Dijkstra result | The path comes out backwards, a stack turns it around |
| `intqueue` | A first-in first-out list of numbers | Used by the tests to compare arrival order with urgency order |
| `hashindex` | Finds a donor, recipient or donation by its id | A direct jump instead of searching the whole list |
| `traveltime` | Shortest travel time and route between locations, plus two simple estimates | The estimates exist so the tests can compare them with the real method |
| `RecommendationEngine` | The six rules and the ranking | The decision logic can be tested without any screen |
| `Sorting` | Merge sort, used to rank the recipients | The same input always gives the same order |
| `allocationmanager` | Carries out an allocation, closes donations, keeps the urgency order, gives out new ids | The only part that changes donation and recipient numbers |
| `ConsoleView` | Every screen and question of the console program | Keeps typing and printing out of the logic |
| `main` | Starts the program, loads the data and runs the menu | Contains no decisions |

### The Windows app code (`Phase3`)

| Group | What it does |
|-------|--------------|
| Offers | Offer records, the offers file, and the send, accept, decline, withdraw and close steps (`Workflow`) |
| Queries and tables | Pick the right rows for each role and turn them into text tables for the lists |
| Text tools | Keep screen text plain and rewrite start-up warnings in simple words |
| Recipient details | Save a recipient's own capacity and flags safely |
| Window and pages | The main window, the sidebar and the pages of each role. Pages only read input, call the steps above and show the result |

### Data structures

The project is also a study of core data structures, so the main ones are built from scratch instead of using the standard library.

| Structure | Used for | Why this one |
|-----------|----------|--------------|
| Min-heap priority queue | Urgency order, Dijkstra | The smallest item is always at the front |
| Linked-list graph | The road map | Stores only the roads that exist |
| Stack | Route display | Reverses the path from Dijkstra |
| Hash index | Finding records by id | Fast lookup without searching |
| Merge sort | Ranking recipients | Same result every time |
| Circular queue | Arrival-order comparison in the tests | Not needed by the main program |

### Where to start reading

1. `main.cpp` shows the menu loop.
2. `ConsoleView.cpp`, function `processDonation`, shows the whole flow for one donation.
3. `RecommendationEngine.cpp` holds the six rules and the ranking.
4. `traveltime.cpp` holds Dijkstra.
5. `allocationmanager.cpp` holds the allocation checks.
6. `Datamanager.cpp` holds loading, checking and saving.

For the Windows app: start with `Workflow`, then open one page file.

---

## 6. The data

All data is in comma separated files. The first line of each file is a header that must match exactly.

| File | What it holds |
|------|---------------|
| `donors.csv` | The organisations that give food and where they are on the map |
| `recipients.csv` | The organisations that receive food: capacity, accepted food types, whether they accept donations and can collect food, and location |
| `donations.csv` | Each donation: donor, food type, quantity offered, quantity left, usable minutes and status |
| `transactions.csv` | Every allocation: donation, recipient, quantity and travel minutes |
| `graph_nodes.csv` | The locations on the map: number, label and coordinates |
| `graph_edges.csv` | The roads between locations, with travel minutes |
| `offers.csv` | Windows app only: every offer and its state. It appears when the first offer is saved |

- **Ids** are a prefix and a number: `DN001` donor, `R001` recipient, `DON001` donation, `T001` transaction, `OFF001` offer. The program gives out new ids, so an id is never repeated.
- **Food types:** `COOKED`, `PACKAGED`, `PRODUCE`, `BAKERY`.
- **Donation status:** `PENDING` (nothing given yet), `PARTIAL`, `COMPLETED` (everything given), `CLOSED` (closed with food left over).
- **Checks at start-up:** the header line, that ids are unique, that every reference exists, that quantities and statuses agree, and that every road joins two real locations. A problem stops the program with a message that names the file and the line.
- **Safe saving:** every save goes through a temporary file, so an interrupted save does not damage the last good data.

`Phase2/data` is a larger sample dataset: 12 donors, 26 recipients, 46 donations, 20 locations and 26 roads. It has edge cases on purpose: a very large donation, a donation that practically never expires, a case where the travel time equals the usable time exactly, and parts of the map that are cut off from the rest. The travel times are **simulated**, not real map data.

---

## 7. Build, run and test

**You need:** Windows, g++ (MinGW) with C++17. CMake is needed for the Windows app only. No other libraries.

```
.\demo.bat                  build the console program, run it on a fresh copy of the data
.\run.bat                   build it and run it on Phase2\data, so every change is saved there
.\run.bat Phase2\data_check use a different data folder
Phase3\run_gui.bat          build the Windows app and start it on Phase3\data
Phase3\reset_data.bat       put the sample data back for the Windows app
Phase3\make_dist.bat        build a packaged copy of the app with its own data
Phase3\create_desktop_shortcut.bat   put an app icon on the Desktop (remove_desktop_shortcut.bat removes it)
```

**Tests.** Each part has its own small test program. A test builds one part with known inputs and compares the result with the expected one, so a failing test points to the part that broke.

```
cmd /c "Phase2\tests\test_all.bat nopause"          console parts
ctest --test-dir Phase3\build --output-on-failure   Windows app parts
```

The tests cover the rules, the ranking, the shortest paths, loading and saving, the console screens with typed answers, the Windows app steps and pages, and the larger sample dataset. One evaluation test compares the real travel times with a straight-line estimate and with no travel check at all, over a grid of scenarios. The Windows app also has built-in check modes that walk every page at three window sizes without clicking.

**Folder layout**

```
Phase2/               console program
  data/                 sample data files
  include/ src/         headers and source files
  tests/                test programs and the test runner
Phase3/               Windows app
  include/ src/ res/    code and window resources
  tests/ tools/         tests and helper scripts
  data/                 sample data the app works on
  CMakeLists.txt        build description
run.bat  demo.bat     build and run the console program
```

---

## 8. Limits and next steps

**Limits**

- Travel times use simulated distances.
- The program recommends. The coordinator decides.
- One coordinator at a time, and no login. The role is chosen by name.
- Donors and recipients cannot be edited or deleted from the program.
- New locations and roads are added by editing the two map files, not from the program.
- Dietary needs such as vegetarian or non-vegetarian are not part of the data.

**Next**

- Real road data.
- Several coordinators at once, with login.
- Editing and deleting records, and adding roads from inside the program.
- Dietary needs as part of the rules.