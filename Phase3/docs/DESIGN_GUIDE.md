# Astryx design guide for the CommunityBridge window (plain C++ / Win32)

## What Astryx is and what we take from it
Astryx (`@astryxdesign/core` and `@astryxdesign/cli` 0.7.11, MIT licence, made by Meta, github.com/meta/astryx) is a design system for web apps written in React. Its components (`Button`, `Table`, `SideNav`, ...) are JavaScript and cannot run in our program.

We take only three things, and no code:
1. **The numbers**: colours, spacing steps, type sizes, control heights, border width and focus ring of the default light theme.
2. **The rules**: when to use which kind of element, and the do/don't lists of the components.
3. **The layout method**: frame first, then regions, then content.

Everything below is built from plain Win32 controls that we already use: static text, push buttons, edit boxes, check boxes, drop-down lists, list views and the status bar. There are no new libraries, no web view, no animation and no shadows. Where Astryx uses transparency, the colour below is already mixed over white, because GDI has no transparency for normal controls.

How these values were read: `astryx docs principles`, `color`, `spacing`, `typography/type-scale`, `typography/font-setup`, `shape`, `elevation`, `layout/scaffold`, `layout/layout-spacing`, `tokens/tokens-size`, `tokens/tokens-border`, `tokens/tokens-focus`, `tokens/tokens-text-size`, `tokens/tokens-font-weight`, and `astryx component Button`, `Table`, `Banner`, `StatusDot`, `SideNav`, `TextInput`, `EmptyState`, `Section`.

## 1. The eight Astryx rules, in our terms
| Astryx rule | What it means for our window |
|-------------|------------------------------|
| Use components | Use the standard Windows control for each job (list view for tables, edit box for text, check box for yes/no). Do not draw our own buttons or lists. |
| Frame first | Fix the window regions (sidebar, page, status bar) and their sizes before placing content. |
| Dense data = rows, not cards | Donations, offers, recipients and transactions are list-view rows. Boxes ("cards") are only for a few small number blocks, such as the Home totals. |
| Semantic tokens only | Every colour, gap and font comes from one place (`Theme.cpp` and `GuiLayout.cpp`), named by purpose (`text`, `muted`, `accent`). No `RGB(...)` or pixel numbers inside the page files. |
| One primary action per view | Each page has exactly one default button (`BS_DEFPUSHBUTTON`). Everything else is a normal button. |
| Destructive needs confirmation | Close donation, Withdraw and Decline ask first (a Yes/No message box). |
| Status never by colour alone | A status is always a word ("Waiting", "Accepted", "Declined"); colour only helps. |
| Read before inventing | New page parts copy an existing pattern from this guide instead of a new one-off style. |

## 2. Colours (Astryx light theme)
Light mode only, as the project has always been.

| Our name (suggested) | Astryx token | Value | Use |
|----------------------|--------------|-------|-----|
| page | `--color-background-surface` | `RGB(255, 255, 255)` | page, lists, edit boxes |
| sidebar | `--color-background-body` | `RGB(241, 244, 247)` | sidebar and the area behind the page |
| text | `--color-text-primary` | `RGB(10, 19, 23)` | normal text, headings |
| muted | `--color-text-secondary` | `RGB(78, 96, 111)` | captions, helper lines, empty-list text |
| disabled | `--color-text-disabled` | `RGB(164, 176, 188)` | only for disabled text (Windows draws disabled controls itself) |
| accent | `--color-accent` | `RGB(0, 100, 224)` | the program name, focus ring, the selected sidebar item mark |
| select | `--color-accent-muted` (mixed) | `RGB(204, 230, 254)` | the selected line in a list we draw ourselves |
| hover | `--color-overlay-hover` (mixed) | `RGB(243, 246, 247)` | hover line in a list we draw ourselves |
| border | `--color-border` (mixed) | `RGB(230, 235, 239)` | thin lines between regions |
| strongborder | `--color-border-emphasized` | `RGB(204, 211, 219)` | edit box and box outlines |
| errortext | `--color-text-red` | `RGB(123, 2, 16)` | the error line under a form |
| errorfill | `--color-error-muted` (mixed) | `RGB(249, 209, 216)` | background of an error banner |
| successtext | `--color-text-green` | `RGB(9, 68, 31)` | "saved" status text |
| successfill | `--color-success-muted` (mixed) | `RGB(206, 235, 210)` | background of a success banner |
| warningtext | `--color-text-yellow` | `RGB(117, 63, 7)` | start-up notes, warnings |
| warningfill | `--color-warning-muted` (mixed) | `RGB(249, 237, 204)` | background of a warning banner |
| infotext | `--color-text-blue` | `RGB(4, 47, 151)` | neutral information |
| infofill | `--color-info-muted` (mixed) | `RGB(204, 230, 254)` | background of an information banner |

Contrast (worked out from the values above, WCAG formula; 4.5:1 is the minimum for normal text):
- text on page 18.8:1, on sidebar 17.0:1
- muted on page 6.5:1, on sidebar 5.9:1
- accent on page 5.4:1; white text on an accent fill 5.4:1
- each status text on its own banner fill 7.3:1 or more
- disabled 2.2:1, on purpose: it is only for things that cannot be used.

Compared with the present `Theme.cpp` palette: the warm paper page `RGB(250, 248, 244)` becomes plain white, the sidebar becomes a cool light grey, the accent moves from navy `RGB(31, 78, 140)` to Astryx blue, and the error red becomes darker for better contrast.

## 3. Type (Astryx scale: base 14 px, ratio 1.2)
Astryx uses the system font stack; on Windows that is **Segoe UI**, the same face as the Windows message font we already read. Sizes are in design pixels (at 96 dpi). Multiply by the screen scaling like every other size: `lfHeight = -MulDiv(px, dpi, 96)`.

| Our font (suggested) | Astryx style | Size | Weight (`lfWeight`) | Use |
|----------------------|--------------|------|---------------------|-----|
| body | `text-body` | 14 px | 400 `FW_NORMAL` | all normal text, list rows, buttons, edits |
| label | `text-label` | 14 px | 500 `FW_MEDIUM` | field labels |
| caption | `text-supporting` | 12 px | 400 | helper lines, captions, start-up notes |
| section | `heading-3` | 17 px | 600 `FW_SEMIBOLD` | a section heading inside a page |
| title | `heading-1` | 24 px | 600 | the page title (one per page) |
| welcome | `display-2` | 35 px | 400 | only the big title of the welcome page |

Rules from `typography-best-practices`: one title per page; headings are semibold, not bold; large display text is normal weight; do not invent sizes between these steps.

## 4. Spacing, sizes, lines (4 px grid)
| Astryx | Value | Use |
|--------|-------|-----|
| `--spacing-1` | 4 px | between a label and its edit box |
| `--spacing-2` | 8 px | between buttons in a row, inside a group |
| `--spacing-3` | 12 px | between form fields |
| `--spacing-4` | 16 px | inner padding of a box or banner |
| `--spacing-6` | 24 px | page margin (the "content line") and space between sections |
| `--spacing-8` | 32 px | space above a page title |
| `--size-element-md` | 32 px | height of buttons, edit boxes and drop-down lists |
| `--size-element-sm` | 28 px | small buttons inside a toolbar row, if ever needed |
| `--border-width` | 1 px | every line we draw |
| `--focus-outline-*` | 2 px accent, 3 px away | the keyboard focus ring, only if we ever draw our own control |
| `--radius-element` | 8 px | only on shapes we draw ourselves (a banner); native controls stay square |

Region widths (from `layout/scaffold`): sidebar 240 to 280 px; a detail side panel 340 to 420 px; forms and text capped at about 640 px wide; tables fill their region.

Spacing rules (`layout/layout-spacing`):
- **One content line per region**: every label, title and list on a page starts at the same left edge (24 px).
- **The container owns the padding**: a list view starts on the content line; its own cell padding gives the text inset.
- **Tight inside, generous between**: 4 to 8 px inside a field or button group, 16 to 24 px between sections. Do not use one gap everywhere.
- **One control size per row**: all buttons in a row are 32 px high.

## 5. Elements: Astryx component to Win32 control
| Astryx component | Our control | Rules taken from its docs |
|------------------|-------------|---------------------------|
| `AppShell` + `SideNav` | main window: sidebar on the left, page on the right, status bar at the bottom | SideNav is for navigation only, never for filters; group items under small section captions. |
| `Heading`, `Text` | static controls with the title, section, body or caption font | One heading level per purpose; captions use muted colour. |
| `Button` primary | the one `BS_DEFPUSHBUTTON` of the page | Only one per view. Label names the action: "Send offer", "Save changes", never "OK" or "Submit". |
| `Button` secondary | `BS_PUSHBUTTON` | All other actions. |
| `Button` destructive | `BS_PUSHBUTTON` plus a Yes/No question | Never act without asking (Close donation, Withdraw, Decline). |
| `Table` | list view, report style, full row select, single selection | Balanced density; no stripes; only columns that have the same kind of value in every row; wide text columns get more room; no horizontal scroll at normal size. |
| `StatusDot`, `Badge` | a Status column with a word | Never colour alone; no decorative badges. |
| `TextInput` | edit box with a visible label above it | Never use the grey hint text as the label; size the box to the expected value (short for numbers); the error message says what is wrong ("The quantity must be at least 1"), not just "Invalid". |
| `CheckboxInput` | `BS_AUTOCHECKBOX` | Label on the right, says the yes state ("Accepting donations"). |
| `Select` | drop-down list (`CBS_DROPDOWNLIST`) | For a fixed set of choices (food kind, location). |
| `Banner` | a 1-line block with a status fill and status text, drawn by us | Title says the status in words; errors stay until fixed; success and info may go away on the next action; never stack two banners of the same kind. |
| `Toast` | the status bar text | Short "Offer OFF004 was sent." messages after an action. |
| `EmptyState` | muted text inside or under an empty list | Say exactly what is empty and what to do next ("No offers yet. Send one from Donations."), never just "No data". |
| `Section` | a heading plus its controls, with 24 px space above | Use for page regions; do not draw a box around each one. |
| `Card` | a box with a 1 px strong border, 16 px padding | Only for a few small number blocks (Home totals). Never one box per list item. |
| `AlertDialog` | `MessageBox` with Yes/No | For questions before something cannot be undone. |
| `Divider` | a 1 px line in the border colour | Only between regions that share the same background. |

## 6. Kept on purpose very basic
- No shadows (Astryx itself keeps every flat surface at elevation "none").
- No animation, no motion tokens.
- No rounded native controls; radius only on the banner we draw.
- No dark mode, no theme switching, no icons inside buttons.
- Windows keeps drawing its own controls (buttons, edits, check boxes, scroll bars); we only set fonts, colours of text and backgrounds, and sizes.

## 7. Suggested C++ shape (for the UI round, not applied yet)
The values belong in the existing theme code, named by purpose, so the page files never contain a colour or a pixel number. Team style: Allman braces, lower-case names, no templates.

```cpp
// The colours of the Astryx light theme. Colours that Astryx gives with transparency
// are already mixed over white, because GDI draws without transparency.
static const COLORREF palettepage = RGB(255, 255, 255);     // --color-background-surface
static const COLORREF palettesidebar = RGB(241, 244, 247);  // --color-background-body
static const COLORREF palettetext = RGB(10, 19, 23);        // --color-text-primary
static const COLORREF palettemuted = RGB(78, 96, 111);      // --color-text-secondary
static const COLORREF paletteaccent = RGB(0, 100, 224);     // --color-accent
static const COLORREF paletteerror = RGB(123, 2, 16);       // --color-text-red
static const COLORREF paletteselect = RGB(204, 230, 254);   // --color-accent-muted over white
static const COLORREF paletteborder = RGB(230, 235, 239);   // --color-border over white

// The spacing steps of the 4 px grid, in design pixels (scaled with px()).
static const int gaptight = 4;      // --spacing-1
static const int gapsmall = 8;      // --spacing-2
static const int gapfield = 12;     // --spacing-3
static const int gapbox = 16;       // --spacing-4
static const int gapsection = 24;   // --spacing-6, also the page margin
static const int controlheight = 32; // --size-element-md
```

## 8. Check list for every page (from Astryx "is-my-code-good", shortened)
1. One title, one default button, every label in plain words.
2. Every element starts on the 24 px content line.
3. Every colour and gap comes from the theme, none written in the page file.
4. Lists are rows; boxes only for small number blocks.
5. Every status is a word; every error says what to change.
6. Every destructive action asks first.
7. Every empty list says why it is empty and what to do.
8. Text contrast is at least 4.5:1 (the table in section 2 already meets it).
