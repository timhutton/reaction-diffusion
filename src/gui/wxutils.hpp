/*  Copyright 2011-2024 The Ready Bunch

    This file is part of Ready.

    Ready is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ready is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ready. If not, see <http://www.gnu.org/licenses/>.         */

// wxWidgets:
#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/artprov.h>
#include <wx/filename.h>
#include <wx/string.h>

// VTK:
class vtkAlgorithmOutput;
class vtkUnsignedCharArray;

#if defined(__WXMAC__) && wxCHECK_VERSION(2,9,0)
    // wxMOD_CONTROL has been changed to mean Command key down
    #define wxMOD_CONTROL wxMOD_RAW_CONTROL
    #define ControlDown RawControlDown
#endif

// Various utility routines:

void Note(const wxString& msg);
// Display given message in a modal dialog.

void Warning(const wxString& msg);
// Beep and display message in a modal dialog.

void Fatal(const wxString& msg);
// Beep, display message in a modal dialog, then exit app.

void Beep();
// Play beep sound, depending on preference setting.

bool GetString(const wxString& title, const wxString& prompt,
               const wxString& instring, wxString& outstring,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize);
// Display a dialog box to get a string from the user.
// Returns false if user hits Cancel button.

bool GetInteger(const wxString& title, const wxString& prompt,
                int inval, int minval, int maxval, int* outval,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize);
// Display a dialog box to get an integer value from the user.
// Returns false if user hits Cancel button.

bool GetFloat(const wxString& title, const wxString& prompt,
              float inval, float* outval,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize);
// Display a dialog box to get a float value from the user.
// Returns false if user hits Cancel button.

bool IsHTMLFile(const wxString& filename);
// Return true if the given file's extension is .htm or .html
// (ignoring case).

bool IsTextFile(const wxString& filename);
// Return true if the given file's extension is .txt or .doc,
// or if it's not a HTML file and its name contains "readme"
// (ignoring case).

int SaveChanges(const wxString& query, const wxString& msg);
// Ask user if changes should be saved and return following result:
// wxYES if user selects Yes/Save button,
// wxNO if user selects No/Don't Save button,
// wxCANCEL if user selects Cancel button.

bool ClipboardHasText();
// Return true if the clipboard contains text.

bool CopyTextToClipboard(const wxString& text);
// Copy given text to the clipboard.

// strip trailing zeros
wxString FormatFloat(float f,int mdp=6);

// converts a wxFileName to a wxString, using the short form if needed as a workaround for unicode characters
wxString FileNameToString(const wxFileName& filename);

// reads an entire file into a string
wxString ReadEntireFile(const wxFileName& filename);

// finds a filename folder+wxString::Format(format,integer) that doesn't have an associated file
wxFileName FindUnusedFilename(const wxString& folder, const wxString& format);

// asks the user where to save a PNG or JPEG image, return false if they cancelled
bool AskUserWhereToSaveImage(wxFileName& filename);

// writes the GetOutputPort() of an image algorithm to the specified file
void WriteImageToFile(vtkAlgorithmOutput* image, const wxFileName& filename);

// writes an array of raw bytes to the specified file
void WriteBytesToFile(vtkUnsignedCharArray* bytes, const wxFileName& filename);

// load an image file and extract its min and max scalar values
void GetScalarRangeFromImage(wxFileName filename, double range[2]);
