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

// local:
class MyFrame;
class HtmlView;

/// The help panel displays .html files (stored in the Help folder or elsewhere).
class HelpPanel : public wxPanel
{
    public:

        HelpPanel(MyFrame* parent, wxWindowID id);

        // display given .html file
        void ShowHelp(const wxString& filepath);
        
        // update buttons at top of help panel
        void UpdateHelpButtons();
        
        // display link info in status line
        void SetStatus(const wxString& text) { status->SetLabel(text); }
        
        bool HtmlHasFocus();    // html window has keyboard focus?
        void SelectAllText();   // select all text in html window
        void CopySelection();   // copy selected text to clipboard
        
        // return false if key event should be passed to default handler
        bool DoKey(int key, int mods);
        
    private:

        MyFrame* frame;         // link to parent frame

        HtmlView* html;         // child window for rendering HTML data

        wxButton* backbutt;     // go back in history
        wxButton* forwbutt;     // go forwards in history
        wxButton* contbutt;     // go to Contents page
        wxButton* smallerbutt;  // smaller text
        wxButton* biggerbutt;   // bigger text
   
        wxStaticText* status;   // for link info
        
        // event handlers
        void OnBackButton(wxCommandEvent& event);
        void OnForwardButton(wxCommandEvent& event);
        void OnContentsButton(wxCommandEvent& event);
        void OnSmallerButton(wxCommandEvent& event);
        void OnBiggerButton(wxCommandEvent& event);

        DECLARE_EVENT_TABLE()
};

// If ShowHelp is called with this string then a temporary .html file
// is created to show the user's current keyboard shortcuts.
const wxString SHOW_KEYBOARD_SHORTCUTS = wxT("keyboard.html");

// Open a modal dialog and display info about the app.
void ShowAboutBox();
