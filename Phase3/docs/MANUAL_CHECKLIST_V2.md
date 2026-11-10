# MANUAL CHECKLIST V2 (round two: one calm window)

How to use: start the program, do the steps of the version you are checking, and write OK or the problem next to each step.
Start with `Phase3\run_gui.bat` (it uses `Phase3\data`). Use a copy of the data if you do not want to change the sample data
(`Phase3\reset_data.bat` restores it).

## v2.0 shell and welcome page
Start: `Phase3\run_gui.bat` (or double click `Phase3\releases\v2.0\communitybridge_gui.exe` after copying it next to a `data` folder).
1. The program opens ONE window with the title "CommunityBridge". It shows the welcome page: a title, a grey line, three round buttons (Coordinator, Donor, Recipient) and a disabled Continue button. [ ]
2. If the data has start-up notes (the sample data has two), they appear in grey above Continue. [ ]
3. Click Donor: a list of donors appears ("DN001  name"). Continue is still disabled. Click a name: Continue turns on. [ ]
4. Press Continue (or double click a name): the same window now has a grey sidebar on the left with the program name, five items, who you are and a Switch user button. The window title says "CommunityBridge - Donor: <name>". [ ]
5. Click every sidebar item: the page title on the right changes at once, with no new window and no flicker. Every page says "This page comes in the next version." (the real pages come in v2.1 to v2.3). [ ]
6. Use the arrow keys in the sidebar list: the page changes with the selected item. Press Tab: the focus moves through the sidebar and the page. [ ]
7. Click Switch user: you are back on the welcome page, the sidebar is gone, and the title is "CommunityBridge" again. [ ]
8. Repeat with Coordinator (no name needed, six sidebar items) and with Recipient (a list of recipients). [ ]
9. Make the window bigger, smaller (it stops at a minimum size) and maximise it. The text stays readable and nothing overlaps. [ ]
10. Start a second copy of the program while the first is open: no second window appears, the first window comes to the front. [ ]
11. Start the program with a wrong data folder (for example `communitybridge_gui.exe C:\nothere`): a plain message appears and the program closes. [ ]
## v2.1 donor pages
Start the program, choose Donor, pick a name, Continue. Use a copy of the data if you want to keep the sample data (`Phase3\reset_data.bat` restores it).
1. Home shows four lines (My donations, Still open, Offers waiting, Food given out) and one button, Add donation. The numbers match the lists on the other pages. [ ]
2. My donations lists only your own donations, without a Donor column. The Add donation button is at the bottom right. [ ]
3. Offers and Given out list your offers and the food that was given out. If there is nothing yet, a grey line says so ("No offers yet."). [ ]
4. Add donation: leave Quantity empty and press Save: a red line says what to type and the page stays. Type 0 as the usable minutes: a red line says the minutes must be at least 1. [ ]
5. Type a quantity (25) and minutes (90), choose PACKAGED, Save: you land on My donations, the new donation is in the list, and the status line at the bottom says "Donation DONxxx added.". The line disappears when you open another page. [ ]
6. Open `data\donations.csv` (or the copy you use): the last row is the new donation. [ ]
7. Open Add donation again: the form is empty again. Press Cancel: you go back to the page you came from. [ ]
8. Make the window as small as it goes: the lists still show every column (long names end with ...), and the Add donation button is still visible. [ ]
## v2.2 coordinator pages and the Match page
Start the program, choose Coordinator, Continue. Use a copy of the data (`Phase3\reset_data.bat` restores the sample data).
1. Home: five numbers (Open donations, Waiting for an offer, Offers pending, Food given out, Food wasted), "Next donation to handle" with one line such as "DON006 - PACKAGED - 100 left - 8 min usable", and the button Process next donation. If the data has start-up notes they are listed in one small list. [ ]
2. Press Process next donation: the Match page opens INSIDE the same window. The sidebar still shows Donations selected. One line tells which donation it is (donor, food, left, already offered, usable minutes). [ ]
3. Two lists: "Recipients that can take it" and "Recipients that cannot" (with a plain reason such as "Too far for the usable time"). [ ]
4. Press Send offer with nobody chosen: a red line asks you to choose a recipient. Choose the first recipient and press Send offer with an empty quantity: a red line asks for the quantity. Type a quantity that is too big: the red line says "The quantity must be from 1 to N.". Nothing is saved by these. [ ]
5. Type a good quantity and press Send offer: the status line says "Offer OFFxxx sent to <name>.", the line at the top shows the new "Already offered", and the lists are updated. If nothing is left to offer the page returns to Donations by itself. [ ]
6. Press Back: you are on Donations. [ ]
7. Donations: Close selected donation and Process selected donation are grey until you choose a row of a donation that is still open. Close asks "Close this donation? ..." (Yes/No); after Yes the status line says how much was wasted and the donation shows as closed. [ ]
8. Offers: Withdraw selected offer is grey until you choose a waiting offer; after it the status line says "Offer OFFxxx withdrawn." and the offer shows as withdrawn. [ ]
9. Recipients, Transactions and Summary each show one list; Summary ends with three lines about the program. [ ]
10. Make the window as small as it goes: on the Match page the red line, the quantity box and the three buttons are all visible and do not overlap. [ ]
## v2.3 recipient pages
Start the program as Coordinator first and send an offer to a recipient (Home > Process next donation > choose the first recipient > a quantity > Send offer). Remember the recipient's name. Switch user, choose Recipient and that name.
1. Home: six lines (Capacity, Accepting donations, Can collect food, Food types, Offers waiting, Food received) and one button, Edit my details. "Offers waiting" shows 1. [ ]
2. Incoming offers: the offer is listed (without a Recipient column). Accept selected offer and Decline selected offer are grey until you choose the row. [ ]
3. Accept: a question "Accept 60 of PACKAGED from <donor>?" (Yes/No). After Yes the status line says the offer was accepted and recorded as a transaction number; the offer leaves the list; Received food lists the food; Past offers lists the offer; Home shows a lower Capacity. [ ]
4. As Coordinator send a second offer to a recipient, then as that recipient press Decline selected offer: "Decline this offer?" (Yes/No); after Yes the status line says "Offer OFFxxx declined." and it shows in Past offers. [ ]
5. Edit my details: the form shows the saved capacity. Clear the capacity and press Save: a red line asks for a whole number and the page stays. Type a new capacity, tick or untick the two boxes, Save: you are on Home, the status line says "Your details were saved.", and the values on Home changed. [ ]
6. An old offer after the recipient stopped accepting: as Coordinator send an offer to a recipient; as that recipient untick Accepting donations and Save; open Incoming offers and Accept: a plain message says the recipient can no longer take it and the offer stays waiting. [ ]
7. Cancel on the form goes back to the page you came from. [ ]
8. Make the window as small as it goes: all lists keep their columns visible and the buttons stay at the bottom right. [ ]
## v2.4 calm review (no new features)
1. Go through every page of the three roles. The look is the same everywhere: white page, light grey sidebar on the left, a blue title, grey helper lines, and one button with a blue outline at the bottom right. [ ]
2. Nothing is red except the one line that tells you what to correct. [ ]
3. There is no menu bar, no toolbar, no search box, no tooltip and no pop-up after a successful action. A short sentence appears at the bottom left instead (for example "Donation DON047 added.") and disappears when you open another page. [ ]
4. Press Tab on any page: the focus moves from the top to the bottom and from left to right, first the sidebar, then the page. Press Esc or F1 or Ctrl+anything: nothing happens. [ ]
5. Make the window as small as it goes and as large as your screen: no text is cut off, no controls overlap, and the lists show every column (long names are shortened with ...). [ ]
6. Switch between pages quickly with the arrow keys in the sidebar: no flicker, no delay. [ ]
## v2.5 icon, packaging, desktop shortcut
1. Open a command window in the Phase3 folder (or double click) and run `make_dist.bat`. It builds the release version and ends with "Done". The folder `Phase3\dist\CommunityBridge` now holds `communitybridge_gui.exe`, `data`, `sample_data` and `reset_data.bat`. [ ]
2. Run `create_desktop_shortcut.bat`. A CommunityBridge icon (a blue rounded square with the white letters CB) appears on the Desktop. [ ]
3. Double click it: the program opens on the welcome page. The same blue CB icon is in the title bar and on the taskbar. [ ]
4. Double click the Desktop icon again while the program is open: no second window appears; the open window comes to the front. [ ]
5. Choose Donor, a name, add a donation, close the program and open it again: the donation is still there (the changes are saved in `dist\CommunityBridge\data`). [ ]
6. Run `make_dist.bat` again: your changes in `data` are NOT overwritten. [ ]
7. Run `dist\CommunityBridge\reset_data.bat`: the data is the sample data again and there are no offers. [ ]
8. Right click `communitybridge_gui.exe` > Properties > Details: the product name is CommunityBridge and the version is 2.5.0.0. [ ]
9. Copy the whole `CommunityBridge` folder to another place or another computer and start the exe there: it works without any other file (Windows may show a SmartScreen warning because the program is not signed; choose More info > Run anyway). [ ]
10. Run `remove_desktop_shortcut.bat`: the Desktop icon is gone and the program in `dist` is still there. [ ]
## v3.0 design guide round (full width, scale change, sidebar)
Start the program with `Phase3\run_gui.bat --dpi-log` (the extra word makes it write `build\dpi_log.txt`).
1. Coordinator > Donations, then maximise the window. The list reaches the same distance from the left and the right edge of the page, and Close donation and Process donation end at the right margin. [ ]
2. Click every sidebar item with the mouse. The highlight fills the whole sidebar width, there is no dotted outline around the clicked item and no thin line at the right edge of the sidebar. [ ]
3. While the program runs, open Windows Settings > System > Display and change Scale (for example 100 to 150 percent, then back). Within a second the text, the sidebar and the buttons change size and nothing is cut off. Close the program and open `build\dpi_log.txt`: it has one line per change with the old and the new dpi. [ ]
4. Home pages show their numbers in a row of equal columns. The coordinator Home shows the notes about the data as pale yellow bands. [ ]
5. Process donation at full screen: the six facts sit in equal columns, the upper list stays short when it has few rows and the lower list takes the rest; Quantity has its label above the box. [ ]
6. Donor > Add donation and Recipient > My details: one column in the middle of the page, every label above its field, Save at the bottom right of the column. [ ]
## v2.7 visual round (scaling, soft palette, clean rendering)
Do these steps on a computer with a normal screen and, if you have one, a second monitor with another scale. Start the program with `Phase3\run_gui.bat` or with the Desktop icon.
1. Start on the normal screen scale. The page and the lists are a soft warm off-white, not pure white; the sidebar is a little deeper than the page and is one even tone from top to bottom, with no lighter strip at its right edge and no bands around Signed in as. [ ]
2. The welcome page: the title, the grey line and the three buttons are one block in the middle of the window, a little above the centre. Press the maximise button: the block stays in the middle and the window does not look empty or lopsided. [ ]
3. Maximise the window and open every sidebar item of the coordinator (Home, Donations, Offers, Recipients, Transactions, Summary). Look for: a row that is cut in half, short dotted or solid line pieces, a thick black frame around a list, a row cut by the bottom edge of a list. None of these may appear. Resize the window by dragging its edge for a few seconds and look again. [ ]
4. Summary: the list has only Item and Quantity, and the three grey sentences are below it as plain text (not rows of the list). [ ]
5. Open Windows Settings, System, Display, Scale and change it to 100, 125, 150 and 200 percent in turn while the program runs (Windows may ask you to sign out for some other programs, this one follows at once). After every change the text is sharp, nothing is cut or overlapping, list rows and the sidebar have the right size for the scale and the sidebar is still one even tone. [ ]
6. Second monitor with another scale: drag the window from one monitor to the other. The window and everything in it changes to the scale of the new monitor, the text stays sharp. [ ]
7. Look at every page of the donor (Home, My donations, Offers, Given out, Add donation) and of the recipient (Home, Incoming offers, Past offers, Received food, My details) at the normal scale and at the smallest window size. Nothing is cut, nothing overlaps, the forms are a column at the left. [ ]
8. Text is comfortable to read on the soft page: dark text, grey captions that are still readable, blue titles, a red error line. [ ]
9. Start the packaged program from the Desktop icon (make_dist.bat and create_desktop_shortcut.bat first): it looks the same and starts from any folder. [ ]
10. The colour of the title bar is the accent colour that you chose in Windows. The program does not set it. [ ]
## v2.6 revision round (welcome page, one home, no punctuation)
1. Start the program. The welcome page shows the title, one grey line and three equal buttons Coordinator, Donor and Recipient. There are no round radio dots, no list and no notes on this page. [ ]
2. Press Coordinator: you enter at once (no Continue). The window title says only CommunityBridge. [ ]
3. Switch user, press Donor: the page shows Pick your name, a list of the donors and the buttons Back and Continue. Continue is grey until you choose a name. Press Back: the three buttons are back. Press Donor again, double click a name: you are in the donor pages. [ ]
4. Switch user, press Recipient, choose a name, press Continue (or press Enter): you are in the recipient pages and the window title still says only CommunityBridge. [ ]
5. Coordinator Home: five numbers and, below them, Notes about the data with each note as a full plain line (for example "Mayur Vihar Isolated Shelter has no road to any donor so it can never be recommended"). Nothing is cut off, even when the window is as small as it goes. There is no button on this page. [ ]
6. Coordinator Donations: the most urgent donation is already chosen when the page opens, and the two buttons Close donation and Process donation are on. Process donation opens the Match page. [ ]
7. Match page: it has only Back and Send offer (no Close donation). When no recipient can take the donation the list says "No recipient can take this donation right now" and Send offer is grey. [ ]
8. Coordinator Offers: choose a waiting offer, the button Withdraw offer is on. It is the only place where an offer is withdrawn. [ ]
9. Donor: Add donation exists only as the last item of the sidebar. My donations and Home have no Add button. [ ]
10. Recipient: the sidebar has Home, Incoming offers, Past offers, Received food and My details. Home shows only Offers waiting and Food received. Incoming offers has Decline offer and Accept offer. My details shows Capacity, two check boxes, Food types and one Save button; after Save the page stays and the status line says Details saved. [ ]
11. Look at every page and every message (red lines, status line, the question before Close donation): no full stop, colon, bracket, semicolon or hyphen anywhere. Names of people and ids may keep their own marks. [ ]
## One full story (all roles, about 10 minutes)
Start from the sample data: run `Phase3\reset_data.bat` (or `dist\CommunityBridge\reset_data.bat`) and start the program (`Phase3\run_gui.bat` or the Desktop icon).
1. **Coordinator.** Press Coordinator (you enter at once). Donations: the most urgent donation is already chosen. Press Process donation. Choose the first recipient in the upper list, type its "Can take" number as the quantity, press Send offer. The status line says "Offer OFF001 sent to <name>" and "Already offered" at the top has grown. Remember the recipient's name. [ ]
2. **Recipient accepts.** Switch user > Recipient > that name > Continue. Home shows "Offers waiting 1". Incoming offers: choose the row, Accept offer, answer Yes. The status line says the offer was accepted. Received food lists the food, Past offers lists the offer, Home shows "Food received" with the new quantity. [ ]
3. **Second offer, declined.** Switch user > Coordinator. Donations > choose a donation > Process donation, send a second offer to the first recipient of the list. Switch user > that recipient > Incoming offers > choose the row > Decline offer > Yes. The status line says "Offer OFFxxx declined". [ ]
4. **Recipient edits the details.** My details: clear the capacity and Save (a red line asks for a number), then type 77, untick Accepting donations and Save. The page stays and the status line says "Details saved". Open Home: it shows only Offers waiting and Food received. [ ]
5. **Donor.** Switch user > Donor > a name. Home, My donations and Offers show only this donor's data. Add donation (sidebar): Save with nothing typed (red line), then PACKAGED, 25, 90, Save. The new donation is in My donations and the status line says so. [ ]
6. **Coordinator closes and withdraws.** Switch user > Coordinator. Send one more offer, then Offers > choose it > Withdraw offer (status line: "Offer OFFxxx withdrawn"). Donations > choose a donation that has a waiting offer > Close donation > Yes: the donation shows as closed and its waiting offer as withdrawn. Summary shows the new totals. [ ]
7. **The files.** Close the program, open `data\donations.csv`, `data\transactions.csv`, `data\offers.csv` and `data\recipients.csv` (Notepad): the new donation, the transaction of step 2, the offers with their statuses and the changed recipient are all there. Open the program again: everything is still there. [ ]
8. **Back to the start.** Run reset_data.bat: the sample data is back and there are no offers. [ ]
Write down anything that confused you: that is a finding for the report.
