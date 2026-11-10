// TextTools.cpp: texts without punctuation (plain C++, no window code).

#include "TextTools.h"

#include <ctype.h>

using namespace std;

// Cuts spaces, tabs and line ends off both ends of a text.
static string trimmed(const string& text)
{
    size_t first = 0;

    while (first < text.length() && isspace((unsigned char)text[first]))
    {
        first++;
    }

    size_t last = text.length();

    while (last > first && isspace((unsigned char)text[last - 1]))
    {
        last--;
    }

    return text.substr(first, last - first);
}

// Makes every run of spaces one space and trims both ends.
static string squeezed(const string& text)
{
    string result = "";
    bool pendingspace = false;

    for (size_t pos = 0; pos < text.length(); pos++)
    {
        if (isspace((unsigned char)text[pos]))
        {
            pendingspace = true;
        }
        else
        {
            if (pendingspace && !result.empty())
            {
                result = result + " ";
            }

            pendingspace = false;
            result = result + text[pos];
        }
    }

    return result;
}

// An apostrophe goes without a space (don't becomes dont), every other mark becomes a space.
string plaintext(const string& text)
{
    string source = trimmed(text);
    bool question = !source.empty() && source[source.length() - 1] == '?';
    string result = "";

    for (size_t pos = 0; pos < source.length(); pos++)
    {
        unsigned char letter = (unsigned char)source[pos];

        if (letter == '\'' || letter == '`')
        {
            continue;
        }

        if (ispunct(letter))
        {
            result = result + " ";
        }
        else
        {
            result = result + (char)letter;
        }
    }

    result = squeezed(result);

    if (question && !result.empty())
    {
        result = result + "?";
    }

    return result;
}

// A word with a file name or a path is kept; a mark at its end (a colon after a file name, a full stop) is cut off.
string plaintextkeepfiles(const string& text)
{
    string result = "";
    string word = "";
    string source = text + " ";

    for (size_t pos = 0; pos < source.length(); pos++)
    {
        if (!isspace((unsigned char)source[pos]))
        {
            word = word + source[pos];
            continue;
        }

        if (word.empty())
        {
            continue;
        }

        string piece = "";
        bool isfile = word.find(".csv") != string::npos || word.find(".tmp") != string::npos ||
                      word.find('\\') != string::npos || word.find('/') != string::npos;

        if (isfile)
        {
            piece = word;

            while (!piece.empty() && (piece[piece.length() - 1] == ':' || piece[piece.length() - 1] == ',' ||
                                      piece[piece.length() - 1] == '.' || piece[piece.length() - 1] == ';'))
            {
                piece = piece.substr(0, piece.length() - 1);
            }
        }
        else
        {
            piece = plaintext(word);
        }

        if (!piece.empty())
        {
            result = result + (result.empty() ? "" : " ") + piece;
        }

        word = "";
    }

    return result;
}

// Any mark counts, except one question mark at the very end.
bool haspunct(const string& text)
{
    string source = trimmed(text);

    if (!source.empty() && source[source.length() - 1] == '?')
    {
        source = source.substr(0, source.length() - 1);
    }

    for (size_t pos = 0; pos < source.length(); pos++)
    {
        if (ispunct((unsigned char)source[pos]))
        {
            return true;
        }
    }

    return false;
}

// Cuts every "(node 16)" out of a text, with the space in front of it.
static string withoutnodes(const string& text)
{
    string result = text;
    size_t open = result.find("(node ");

    while (open != string::npos)
    {
        size_t close = result.find(')', open);

        if (close == string::npos)
        {
            break;
        }

        size_t from = open;

        if (from > 0 && result[from - 1] == ' ')
        {
            from--;
        }

        result.erase(from, close - from + 1);
        open = result.find("(node ");
    }

    return result;
}

// True for an id such as R024 or DN003: letters, then at least three digits and nothing else.
static bool looksliketid(const string& word, string& prefix)
{
    size_t pos = 0;

    while (pos < word.length() && isalpha((unsigned char)word[pos]))
    {
        pos++;
    }

    if (pos == 0 || word.length() - pos < 3)
    {
        return false;
    }

    for (size_t digit = pos; digit < word.length(); digit++)
    {
        if (!isdigit((unsigned char)word[digit]))
        {
            return false;
        }
    }

    prefix = word.substr(0, pos);
    return true;
}

// Looks up the name of a recipient (R) or a donor (DN). Returns false when the id is not known.
static bool nameofid(const string& id, const dataset& data, string& name)
{
    string prefix = "";
    int pos = 0;

    if (!looksliketid(id, prefix))
    {
        return false;
    }

    if (prefix == "R" && data.recipientindex.get(id, pos))
    {
        name = data.recipients[pos].name();
        return true;
    }

    if (prefix == "DN" && data.donorindex.get(id, pos))
    {
        name = data.donors[pos].name();
        return true;
    }

    return false;
}

// Replaces the first word by the name (known sentence) or by the id and the name (any other warning).
vector<string> plainnotes(const list<string>& warnings, const dataset& data)
{
    vector<string> lines;

    for (list<string>::const_iterator at = warnings.begin(); at != warnings.end(); ++at)
    {
        string text = trimmed(withoutnodes(*at));
        size_t space = text.find(' ');
        string first = space == string::npos ? text : text.substr(0, space);
        string rest = space == string::npos ? "" : text.substr(space);
        string name = "";
        bool known = nameofid(first, data, name);

        // the road sentence of Phase 2, written in plain words
        const string roadsentence = " is not connected to any donor; it can never be recommended";
        size_t road = rest.find(roadsentence);

        if (road != string::npos)
        {
            rest = rest.substr(0, road) + " has no road to any donor so it can never be recommended" + rest.substr(road + roadsentence.length());
            text = (known ? name : first) + rest;
        }
        else if (known)
        {
            text = first + " " + name + rest;
        }

        // a file name loses its dot but keeps its meaning
        size_t csv = text.find(".csv");

        while (csv != string::npos)
        {
            text.replace(csv, 4, " file");
            csv = text.find(".csv");
        }

        string line = plaintext(text);

        if (!line.empty())
        {
            lines.push_back(line);
        }
    }

    return lines;
}
