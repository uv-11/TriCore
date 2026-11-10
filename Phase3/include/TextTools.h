#ifndef TEXTTOOLS_H
#define TEXTTOOLS_H

// TextTools.h: the program writes its texts without punctuation (rule P1). These functions clean texts that come
// from somewhere else (Phase 2, exceptions, the data files) and check texts. Plain C++, no window code.

#include "Datamanager.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// Takes every punctuation mark out of a text. An apostrophe disappears, every other mark becomes a space so the
// words stay apart. Repeated spaces are made one, both ends are trimmed. A question mark at the very end stays.
string plaintext(const string& text);

// Like plaintext, but a word that holds a file name or a folder path (it has .csv, .tmp or a slash) is kept as it is,
// except for a mark at its very end. Used for the start-up error box, which may name the file.
string plaintextkeepfiles(const string& text);

// True when the text has any punctuation mark other than one question mark at the very end.
bool haspunct(const string& text);

// One plain line for each warning that the start-up found. The id of a recipient or donor at the start of a warning is
// replaced by its name, the words about nodes of the map are left out, and every mark is taken out.
vector<string> plainnotes(const list<string>& warnings, const dataset& data);

#endif
