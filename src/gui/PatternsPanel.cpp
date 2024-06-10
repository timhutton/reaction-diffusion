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

// local:
#include "PatternsPanel.hpp"
#include "frame.hpp"
#include "IDs.hpp"
#include "prefs.hpp"        // for patterndir, userdir

// wxWidgets:
#include <wx/filename.h>    // for wxFileName

#if defined(__WXMAC__) && wxCHECK_VERSION(2,9,0)
    // ControlDown has been changed to mean Command key down
    #define ControlDown RawControlDown
#endif

BEGIN_EVENT_TABLE(PatternsPanel, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_TREECTRL, PatternsPanel::OnTreeSelChanged)
END_EVENT_TABLE()

PatternsPanel::PatternsPanel(MyFrame* parent,wxWindowID id)
    : wxPanel(parent,id), frame(parent)
{
    edit_file = false;

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    patternctrl = NULL;    // for 1st call of OnTreeSelChanged
    patternctrl = new wxGenericDirCtrl(this, wxID_ANY, wxEmptyString,
                                       wxDefaultPosition, wxDefaultSize,
                                       #ifdef __WXMSW__
                                           // speed up a bit
                                           wxDIRCTRL_DIR_ONLY | wxNO_BORDER | wxWANTS_CHARS,
                                       #else
                                           wxNO_BORDER | wxWANTS_CHARS,
                                       #endif
                                       wxEmptyString   // see all file types
                                      );

    sizer->Add(patternctrl,wxSizerFlags(1).Expand());
    this->SetSizer(sizer);

    #ifdef __WXMSW__
        // now remove wxDIRCTRL_DIR_ONLY so we'll see files
        patternctrl->SetWindowStyle(wxNO_BORDER | wxWANTS_CHARS);
    #endif

    wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();

    #if defined(__WXGTK__)
        // make sure background is white when using KDE's GTK theme
        #if wxCHECK_VERSION(2,9,0)
            treectrl->SetBackgroundStyle(wxBG_STYLE_ERASE);
        #else
            treectrl->SetBackgroundStyle(wxBG_STYLE_COLOUR);
        #endif
        treectrl->SetBackgroundColour(*wxWHITE);
        // reduce indent a bit
        treectrl->SetIndent(8);
    #elif defined(__WXMAC__)
        // reduce font size
        wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        font.SetPointSize(12);
        treectrl->SetFont(font);
        // ensure entire background is near white (avoids bug in wxMac 3.1.5 if bg is pure white)
        treectrl->SetBackgroundColour(wxColour(254,254,254));
    #elif defined(__WXMSW__)
        // reduce indent a lot on Windows
        treectrl->SetIndent(4);
    #endif

    BuildTree();

    // install event handler to detect clicking on a file
    treectrl->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(PatternsPanel::OnTreeClick), NULL, this);
    treectrl->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(PatternsPanel::OnTreeClick), NULL, this);
    treectrl->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(PatternsPanel::OnTreeClick), NULL, this);

    // install event handlers to detect keyboard shortcuts when treectrl has focus
    treectrl->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MyFrame::OnKeyDown), NULL, frame);
    treectrl->Connect(wxEVT_CHAR, wxKeyEventHandler(MyFrame::OnChar), NULL, frame);
}

void PatternsPanel::AppendDir(const wxString& indir, wxTreeCtrl* treectrl, wxTreeItemId root)
{
    wxString dir = indir;
    if (dir.Last() == wxFILE_SEP_PATH) dir.Truncate(dir.Length()-1);
    wxDirItemData* diritem = new wxDirItemData(dir, dir, true);
    wxTreeItemId id = treectrl->AppendItem(root, dir.AfterLast(wxFILE_SEP_PATH), 0, 0, diritem);

    // expand the root item
    if ( diritem->HasFiles() || diritem->HasSubDirs() ) {
        treectrl->SetItemHasChildren(id);
        treectrl->Expand(id);
    }
}

void PatternsPanel::BuildTree()
{
    // delete old tree (except root)
    wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();
    wxTreeItemId root = patternctrl->GetRootId();
    treectrl->DeleteChildren(root);

    if ( wxFileName::DirExists(patterndir) ) {
        // append Ready's pattern folder as first child of root
        AppendDir(patterndir, treectrl, root);
    }

    if ( wxFileName::DirExists(userdir) ) {
        // append user's pattern folder as another child of root
        AppendDir(userdir, treectrl, root);
    }

    // select top folder so hitting left arrow can collapse it and won't cause an assert
    wxTreeItemIdValue cookie;
    wxTreeItemId id = treectrl->GetFirstChild(root, cookie);
    if (id.IsOk()) treectrl->SelectItem(id);
    
    #if defined(__WXMAC__) && wxCHECK_VERSION(3,1,3)
        // wxTR_HIDE_ROOT is needed to hide the "Sections" directory
        treectrl->SetWindowStyle(wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
    #endif
}

void PatternsPanel::OnTreeClick(wxMouseEvent& event)
{
    // determine if an item was clicked
    wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();
    wxPoint pt = event.GetPosition();
    int flags;
    wxTreeItemId id = treectrl->HitTest(pt, flags);
    if (!id.IsOk()) {
        // click wasn't in any item
        event.Skip();
        return;
    }

    if (treectrl->ItemHasChildren(id)) {
        // click was in a folder item
        event.Skip();
        return;
    }

    // set global flag for testing in OnTreeSelChanged
    edit_file = event.ControlDown() || event.RightDown();

    // check for click in an already selected item
    if (id == treectrl->GetSelection()) {
        // force a selection change so OnTreeSelChanged gets called
        treectrl->Unselect();
    }

    treectrl->SelectItem(id);
    treectrl->SetFocus();
    // OnTreeSelChanged will be called -- don't call event.Skip()
}

void PatternsPanel::OnTreeSelChanged(wxTreeEvent& event)
{
    if (patternctrl == NULL) return;   // ignore 1st call

    wxTreeItemId id = event.GetItem();
    if (!id.IsOk()) return;

    wxString filepath = patternctrl->GetFilePath();

    if (filepath.IsEmpty()) {
        // user clicked on a folder name

    } else if (edit_file) {
        // open file in text editor
        frame->EditFile(filepath);

    } else {
        // user clicked on a pattern/html/txt file
        frame->OpenFile(filepath);
    }
}

bool PatternsPanel::TreeHasFocus()
{
    wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();
    return treectrl && treectrl->HasFocus();
}

bool PatternsPanel::DoKey(int key, int mods)
{
    // first look for keys that should be passed to the default handler
    if ( mods == wxMOD_NONE ) {
        if ( key == WXK_UP || key == WXK_DOWN || key == WXK_LEFT || key == WXK_RIGHT ) {
            // let default handler see arrow keys (to select files or open/close folders)
            return false;
        }
    }

    // finally do other keyboard shortcuts
    frame->ProcessKey(key, mods);
    return true;
}

void PatternsPanel::SelectPath(const wxString& path)
{
    wxFileName full_path(path);
    full_path.MakeAbsolute();
    this->patternctrl->ExpandPath(full_path.GetFullPath());
    this->patternctrl->SetPath(full_path.GetFullPath());
}
