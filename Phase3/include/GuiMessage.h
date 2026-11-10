#ifndef GUIMESSAGE_H
#define GUIMESSAGE_H

// GuiMessage.h: the only way the program shows a message box. A box is for a real error or a Yes/No question.
// In a test mode no window is shown: the text is written to the test log and the question is answered Yes.

#include <windows.h>

#include <string>

using namespace std;

// Shows a real error with an OK button.
void guishowerror(HWND parent, const string& text);

// Asks a question with Yes and No buttons. Returns true for Yes.
bool guiaskyes(HWND parent, const string& text);

#endif
