# CHANGELOG

## v3.0 - design guide round: full width pages, scale change, clean sidebar
- The look follows docs/DESIGN_GUIDE.md (the Astryx light theme made basic for Win32): page (250,248,244), sidebar (240,237,230), text (10,19,23), grey text (78,96,111), accent (0,100,224) for the program name only; six fonts in design pixels (14, 24, 17, 12, 35 and 14 medium); spacing tokens 4, 8, 12, 16, 24 and 32 in GuiLayout.h. Page files hold no colour, font name or pixel number.
- Full width (P1): every page uses the content width with margins of 24 design pixels on both sides; table lists fill the width and the height down to the button row; buttons are 32 high with gaps of 8 and end at the right and bottom margins.
- Scale change (P2): the manifest alone sets per-monitor awareness; WM_DPICHANGED is handled in the shell and tested with a real message cycle (--selftest-dpichange, 96 to 192 and back); --dpi-log writes every dpi change to dpi_log.txt.
- Sidebar (P3): 240 design pixels, the item list exactly as wide as the sidebar, no column edge line (clipped by a window region), no focus outline after a mouse click.
- Pages: Home pages show a stat row of equal columns (new role ROLE_VALUE); the coordinator notes are warning bands (ROLE_BANNER); Match shows its facts in equal columns when they fit and gives the upper list only the rows it needs; Add donation and My details are one centred column of at most 640 design pixels with labels above the fields (ROLE_LABEL).
- Self-test: new checks SC1 to SC5 (sidebar), LC1 to LC5 and LC7 (margins, list, button row, empty message, stat row, labels and field heights); flows at 96 and 192 dpi.
- run_gui.bat prints the path and build time of the exe it starts and passes extra words on; run_gui.bat and make_dist.bat end with exit code 1 when the build fails.
## v2.7 - visual round: any screen scale, balanced layout, soft palette, clean rendering
- Every screen scale: the program is per-monitor DPI aware (manifest PerMonitorV2 with the older flags as fallback). Every size follows the dpi of the window, fonts are made again for the dpi, list rows and column widths follow it, and WM_DPICHANGED (a move to another monitor, or a scale change in Windows while the program runs) moves the window to the rectangle Windows suggests and lays out again.
- Balanced at every size: the welcome block is centred (middle at 42 percent of the height), Home pages and forms are a left aligned column of at most 560 design pixels, a list is as wide as its columns need and shows whole rows, the Match page keeps two lists as wide as they need.
- Soft palette: a warm off-white page (250,248,244) and a slightly deeper sidebar (240,237,230), near black text (34,34,34), grey helper text, blue titles. Contrast: body text 15.0 and 13.6 to 1, grey text 5.9 and 5.4, titles 7.8 and 7.1, error line 6.6. The palette is in Theme.cpp only.
- Clean rendering: no frame around lists, no column lines running down into the empty part of a list, no faint line at the right edge of the sidebar (found by reading pixels), whole rows only at the bottom edge of a list, the name list of the welcome page has the page colours.
- Summary page: the list has only Item and Quantity; the three lines about the program are grey text under it (backend: summarytable has 8 rows, new aboutlines()).
- Sidebar 210 design pixels wide with items of 44, list rows of 32, buttons of 36, page margin 32, title 20 pt, welcome title 26 pt.
- Test modes: new option --dpi n; the self-test runs every page at 96, 120, 144, 168 and 192 dpi and at four sizes up to 1920 by 1010, with the new checks (l) contrast table, (m) list rows, (n) even colour columns read from painted pixels, (o) body font height per dpi, (p) balanced layout, (q) synthetic WM_DPICHANGED cycle; flows run at dpi 96 and 192; pictures at 96, 144 and 192.
- The title bar colour is the Windows accent colour of the person and is not touched.
## v2.6 - revision round: one home for every action, two step welcome page, no punctuation
- The welcome page has two steps in the same window and no radio buttons. Step 1: three equal buttons Coordinator, Donor and Recipient. The Coordinator enters at once; Donor and Recipient open step 2 with the caption Pick your name, the name list, Back and Continue. The notes about the data are no longer on this page.
- The notes about the data (the start-up findings) are only on the coordinator Home, as plain lines in full (never cut off), at most six and then "and N more". The window caption is always CommunityBridge.
- Every action has one home. Coordinator: Donations has Close donation and Process donation (the most urgent donation is already chosen), Offers has Withdraw offer, Home has no button, the Match page has only Back and Send offer. Donor: Add donation only in the sidebar. Recipient: Incoming offers has Decline offer and Accept offer, My details only in the sidebar (Home shows Offers waiting and Food received only).
- My details (recipient) shows Capacity, Accepting donations, Can collect food and the Food types line, and has one Save button; after Save the page stays and the status line says Details saved.
- Rule P1: no punctuation on any screen (no full stops, colons, brackets, semicolons, hyphens or commas in labels, buttons, headers, messages and the status line). Exempt: a final question mark of a yes or no question, stored values (names, ids), and file names in the start-up error box. New backend files TextTools.h and TextTools.cpp (plaintext, plaintextkeepfiles, haspunct, plainnotes) with tests test_text (221 checks) and test_notes (27 checks).
- Self-test: welcome steps, preselected donation, new layout check (j) no punctuation and (k) every action once, flows F1 to F7. 12 of 12 ctest tests pass, 0 warnings.
- Veg and non veg food is not built: FUTURE_IDEAS.md in Phase3/docs/ui2 lists three options and the Phase 2 files each one would touch.
## v2.5 - icon, packaging, desktop shortcut
- The program has an icon (a blue rounded square with the white letters CB, drawn by tools/make_icon.ps1 in six sizes from 16 to 256 pixels): on the exe file, in the title bar and on the taskbar.
- `make_dist.bat` builds the Release version (-O2, stripped) and puts it in Phase3\dist\CommunityBridge with `data`, `sample_data` and `reset_data.bat`. Making the folder again never overwrites the data a person changed. The program starts by double click and finds its own `data` folder.
- `create_desktop_shortcut.bat` and `remove_desktop_shortcut.bat` make and remove the CommunityBridge icon on the Desktop.
- Version information (name, description, 2.5.0.0) is part of the exe.
## v2.4 - calm review (no new features)
- All 72 screenshots (every page, three window sizes, special states) reviewed against the calm design; the status line now has room at the left.
- The Tab key follows the reading order on every page (the coordinator Home page was fixed) and the self-test checks it.
- The placeholder pages are gone: all 18 pages are real; the self-test checks that each one exists.
- Searches for shortcuts, owner draw, custom painting, menus, tooltips, dark mode, timers, colour and font literals outside Theme.cpp: all clean.
## v2.3 - recipient pages
- Recipient Home, Incoming offers (Accept and Decline, both ask first), Past offers, Received food and the Edit my details form, all in the one window.
- Edit my details: a missing capacity shows one red line; a successful save goes to Home with "Your details were saved." in the status line.
- An offer that can no longer be accepted shows the plain message of the program and stays waiting.
- Self-test flows F3 (accept), F3b (decline), F4 (edit details) and F4b (accept after the recipient stopped accepting).
- All round-one dialogs and their resources moved out of the build (Phase3/legacy); the window program now has no dialog resources.
## v2.2 - coordinator pages and the Match page
- Coordinator Home (five numbers, the next donation, Process next donation, start-up notes), Donations, Offers, Recipients, Transactions and Summary in the one window.
- The Match page is a page of the same window (the sidebar keeps Donations selected): the recipients that can and cannot take the donation, a quantity, Send offer, Close donation and Back. Problems show as one red line; success shows in the status line.
- Close donation and Close selected donation ask first; Withdraw is only possible for a waiting offer.
- Self-test flows F2 (send an offer), F5 (withdraw) and F6 (close a donation with a waiting offer).
- The old coordinator and donor screens moved to Phase3/legacy.
## v2.1 - donor pages
- Donor Home (four numbers), My donations, Offers, Given out and the Add donation form, all inside the one window.
- Add donation: a wrong entry shows one plain red line and the page stays; success goes to My donations with "Donation DONxxx added." in the status line; Cancel goes back.
- Lists fit their columns to the window (long texts end with "..."), no horizontal scroll bar; the Donor column is left out on donor pages.
- New backend file Dashboard (Home numbers) with its own test; self-test flow F1 (add a donation through the real window).
## v2.0 - one window, shell and welcome page (round two)
- The program is now ONE main window. A grey sidebar on the left switches pages inside the window; no page opens as its own window.
- Welcome page: choose Coordinator, Donor or Recipient, pick a name, Continue. The first start-up notes are shown in grey.
- Sidebar (a standard list), page title, a single status line, Switch user, window title "CommunityBridge - <Role>: <name>".
- Calm light look (white, light grey sidebar, one blue for titles), system dpi aware, themed controls (Common Controls 6), version information.
- One running copy only: a second start brings the first window to the front.
- Test modes: --selftest (layout checks at three sizes and behaviour checks), --screenshots <folder>; registered in CTest as gui_selftest.
- The pages of the three roles are placeholders ("This page comes in the next version.") until v2.1 to v2.3.
- The old start window (GuiStart.cpp) moved to Phase3/legacy.
## v1.1 - optional extras
Added on top of v1.0 (each one has a Workflow function and a test first):
- Add recipient (button on the coordinator screen): name, type, food kinds (check boxes), capacity, accepting and can-collect check boxes, and a location chosen from a list of the map locations. Plain messages for every wrong value; the message also says when no donor is connected to the chosen location. After saving the Recipients view is shown. Workflow: `createrecipient`.
- New donor (button inside Add donation): opens a small dialog for name, type and location (checked at once, nothing saved yet). Save then adds the donor and its donation: the donor is saved BEFORE the donation, and a bad value saves nothing. Workflow: `checkdonordetails`, `createdonor`, `createdonationwithnewdonor`.
- Allocate my donation (button on the donor screen): active when an open donation is selected in My donations; opens the match dialog for that donation.
Tests: test_workflow 207 -> 252 PASS (new records, validation, failed saves, ids not used up, files consistent after a reload). All 7 programs pass: 516 PASS lines in total.
Checked in the real window on a data copy (docs/scratch/drive_v11.ps1): R027 added to recipients.csv, DN013 and DON047 added in the right order, the match dialog opens for the selected donation. Regression: the donor, coordinator and story scripts pass again.
Known gaps: the New donor button is on the screen of an existing donor, so the new donor's donation does not appear in that donor's own list (the message names the new donor and its id).

## v1.0 - whole app verified
Same program as v0.4, verified as a whole (no feature changes).
- Clean rebuild from an empty build folder: 0 warnings, 0 errors. ctest: 7 of 7 tests pass (471 PASS lines).
- `--selftest` on a copy of the data: every screen line present and matching the backend; on a copy changed by a backend-only scenario (two donations, an accepted offer, a waiting offer) the lists change as expected (donations 48, transactions 4, offers 2, donor DN001 rows 7, recipient R002 rows 1).
- Phase 2 files: SHA256 of all 72 files identical to the Stage 0 baseline.
- Searches: no windows.h in the backend, no README.md, no git call, colour calls only in GuiCommon.cpp, no manifest, no dark-mode or theme calls, no icons or images, no templates or lambdas in GUI code, every dialog procedure calls the colour helper (8 of 8).
- The three window-driving scripts were run again on the final build (donor, coordinator, full story) and passed.
- Comment in Workflow.cpp reworded (no code change).
Known gaps: see KNOWN_ISSUES.md.

## v0.4 - recipient screen
Works now:
- Recipient opens the real screen: banner "Recipient: <name> (R003)", the lines "Capacity: n    Accepting: yes    Can collect food: yes" and "Food types: COOKED;PACKAGED", three views (Incoming offers, Past offers, Received food), and the buttons Accept selected offer, Decline selected offer, Edit my details, Close.
- Accept asks "Accept <qty> of <food> from <donor>?" (Yes/No), then runs the allocation and records the transaction. A failure shows the plain message (for example "The recipient is no longer feasible for this donation.") and the offer stays in Incoming offers.
- Decline asks "Decline this offer?", then moves the offer to Past offers. The coordinator then sees that recipient as "Declined this donation earlier" and not as a candidate.
- Edit my details: capacity (numbers only), "Accepting donations" and "Can collect food" check boxes; an empty or negative capacity gives a plain message; saved values show on the screen and in recipients.csv.
- Self-test line `screen=recipient person=<id> rows=<n>` plus a check against `offersforrecipient`.
Changed: all three roles now have their own screens; "not built yet" is gone.
Tests: no new backend code; the 7 test programs still pass. The full offer story was driven in the real window on a data copy (see TESTING.md).
Known gaps: none new.

## v0.3 - coordinator screen and match dialog
Works now:
- Coordinator opens the real screen: five views of one list (Donations, Recipients, Offers, Transactions, Summary), "Showing: ..." line, and the buttons Process most urgent donation, Process selected donation, Close selected donation, Withdraw selected offer, Refresh, Close.
- The buttons that need a selection are disabled until the right row is selected (a donation row for the first two, a waiting offer for Withdraw). No urgent donation left: "No donation is waiting for an offer."
- Match dialog: donation and donor, food, left, already offered, usable minutes; the recipients that can take it (with route names) and the ones that cannot (plain reasons, declined ones marked); Quantity, Send offer, Close donation (asks first), Back. Problems (no recipient chosen, empty quantity, too big) are plain messages. After a send the numbers update; when nothing is left to offer the dialog says so and closes.
- Close selected donation asks "Close this donation? The food that is left will be recorded as wasted." and withdraws its waiting offers.
- Self-test lines: `screen=coordinator rows=<n>` and `screen=match donation=<id> candidates=<n> rejected=<n>`, each checked against the backend.
Changed: Coordinator no longer shows "not built yet". Only Recipient does.
Tests: no new backend code; the 7 test programs still pass. The window was also driven by script on a data copy (see TESTING.md).
Known gaps: offers can be sent but not yet answered (the recipient screen comes in v0.4).

## v0.2 - donor screen
Works now:
- Donor opens the real donor screen: banner "Donor: <name> (DN001)", the line "Donations: n    Food offered: x    Food given out: y", three buttons that switch the list (My donations, Offers, Given out), and a line "Showing: ..." that says which one is shown.
- Add donation dialog: food type (drop-down), quantity and usable minutes (numbers only), a plain message line. Wrong input keeps the dialog open with a message; a good one adds the donation (new id after the highest, status Waiting), shows "Donation DON047 was added." and refreshes the list.
- Self-test line `screen=donor person=<first donor> rows=<n>` plus a check against `donationsofdonor`.
Changed: Donor no longer shows "not built yet". Coordinator and Recipient still do.
Tests: no new backend code; the 7 test programs still pass. The window was also driven by script on a data copy (see TESTING.md).
Known gaps: none new.

## v0.1 - start window
Works now:
- Window "CommunityBridge" with the blue banner on a white background, three role buttons (Coordinator, Donor, Recipient), a list of the start-up warnings (or "No warnings.") and an Exit button.
- Donor and Recipient open the pick-name list (donors "DN001  name" or recipients "R003  name"); OK is disabled until a line is selected; a double click is OK; Cancel/Esc closes.
- The three screens are not built yet: each shows "This screen is not built yet."
- A start-up error (bad data file) shows a message box with the problem and the data folder, exit code 1.
- `--selftest` for automated checks; `run_gui.bat` builds and starts; `reset_data.bat` restores the sample data.
Backend (plain C++, tested): Offer, offerstore, Queries, Tables, RecipientUpdate, Workflow (all scenarios S1 to S8).
Tests: 7 programs, all pass (test_p2link 17, test_offer 45, test_offerstore 76, test_queries 35, test_tables 61, test_recipientupdate 30, test_workflow 207 PASS lines).
Known gaps: the three role screens; the window itself was checked by self-test and one screenshot, not by clicking.






