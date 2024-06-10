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
#include "HelpPanel.hpp"
#include "app.hpp"              // for wxGetApp
#include "frame.hpp"
#include "IDs.hpp"
#include "prefs.hpp"            // for GetShortcutTable, readydir, etc
#include "wxutils.hpp"          // for Warning, CopyTextToClipboard

// wxWidgets:
#include <wx/filename.h>        // for wxFileName
#include <wx/html/htmlwin.h>    // for wxHtmlWindow

const wxString helphome = _("Help/index.html");    // contents page

// -----------------------------------------------------------------------------

// define a child window for displaying HTML info
class HtmlView : public wxHtmlWindow
{
    public:

        HtmlView(wxWindow* parent, MyFrame* myframe, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxHW_SCROLLBAR_AUTO | wxWANTS_CHARS)
            : wxHtmlWindow(parent, id, pos, size, style)
        {
            frame = myframe;
            panel = (HelpPanel*)parent;
            editlink = false;
            linkrect = wxRect(0,0,0,0);
            inAbout = false;
        }

        virtual void OnLinkClicked(const wxHtmlLinkInfo& link);
        virtual void OnCellMouseHover(wxHtmlCell* cell, wxCoord x, wxCoord y);

        void ClearStatus();  // clear help pane's status line

        void SetFontSizes(int size);
        void ChangeFontSizes(int size);

        bool canreload;     // can OnSize call LoadPage?
        bool inAbout;       // in ShowAboutBox?

    private:

        void OnSize(wxSizeEvent& event);
        void OnMouseMotion(wxMouseEvent& event);
        void OnMouseLeave(wxMouseEvent& event);
        void OnMouseDown(wxMouseEvent& event);

        MyFrame* frame;
        HelpPanel* panel;

        bool editlink;       // open clicked file in editor?
        wxRect linkrect;     // rect for cell containing link

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(HtmlView, wxHtmlWindow)
    EVT_SIZE          (HtmlView::OnSize)
    EVT_MOTION        (HtmlView::OnMouseMotion)
    EVT_ENTER_WINDOW  (HtmlView::OnMouseMotion)
    EVT_LEAVE_WINDOW  (HtmlView::OnMouseLeave)
    EVT_LEFT_DOWN     (HtmlView::OnMouseDown)
    EVT_RIGHT_DOWN    (HtmlView::OnMouseDown)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

void HtmlView::OnLinkClicked(const wxHtmlLinkInfo& link)
{
    wxString url = link.GetHref();
    if (url.StartsWith(wxT("http:")) || url.StartsWith(wxT("https:")) || url.StartsWith(wxT("mailto:"))) {
        // pass http/mailto URL to user's preferred browser/emailer
        if ( !wxLaunchDefaultBrowser(url) )
            Warning(_("Could not open URL in browser!"));

    } else if ( url.StartsWith(_T("prefs:")) ) {
        // user clicked on link to Preferences dialog
        frame->ShowPrefsDialog( url.AfterFirst(':') );

    } else if ( url.StartsWith(wxT("edit:")) ) {
        // open clicked file in user's preferred text editor
        wxString path = url.AfterFirst(':');
        #ifdef __WXMSW__
            path.Replace(wxT("/"), wxT("\\"));
        #endif
        wxFileName fname(path);
        if (!fname.IsAbsolute()) path = readydir + path;
        frame->EditFile(path);


    } else if ( url.StartsWith(wxT("open:")) ) {
        // open clicked file
        wxString path = url.AfterFirst(':');
        #ifdef __WXMSW__
            path.Replace(wxT("/"), wxT("\\"));
        #endif
        wxFileName fname(path);
        if (!fname.IsAbsolute()) path = readydir + path;
        if (editlink) {
            frame->EditFile(path);
        } else {
            frame->Raise();
            frame->OpenFile(path);
        }

    } else {
        // assume it's a link to a local target or another help file
        panel->ShowHelp(url);
    }
}

// -----------------------------------------------------------------------------

void HtmlView::OnCellMouseHover(wxHtmlCell* cell, wxCoord x, wxCoord y)
{
    if (inAbout || cell == NULL) return;

    wxHtmlLinkInfo* link = cell->GetLink(x,y);
    if (link) {
        wxString href = link->GetHref();
        href.Replace(wxT("&"), wxT("&&"));
        panel->SetStatus(href);
        wxPoint pt = ScreenToClient( wxGetMousePosition() );
        linkrect = wxRect(pt.x-x, pt.y-y, cell->GetWidth(), cell->GetHeight());
    } else {
        ClearStatus();
    }
}

// -----------------------------------------------------------------------------

void HtmlView::OnMouseMotion(wxMouseEvent& event)
{
    if (!inAbout && !linkrect.IsEmpty()) {
        int x = event.GetX();
        int y = event.GetY();
        if (!linkrect.Contains(x,y)) ClearStatus();
    }
    event.Skip();
}

// -----------------------------------------------------------------------------

void HtmlView::OnMouseLeave(wxMouseEvent& event)
{
    ClearStatus();
    event.Skip();
}

// -----------------------------------------------------------------------------

void HtmlView::ClearStatus()
{
    if (inAbout) return;

    panel->SetStatus(wxEmptyString);
    linkrect = wxRect(0,0,0,0);
}

// -----------------------------------------------------------------------------

void HtmlView::OnMouseDown(wxMouseEvent& event)
{
    // set flag so ctrl/right-clicked file can be opened in editor
    editlink = event.ControlDown() || event.RightDown();
    event.Skip();
}

// -----------------------------------------------------------------------------

void HtmlView::OnSize(wxSizeEvent& event)
{
    // avoid scroll position being reset to top when wxHtmlWindow is resized
    int x, y;
    GetViewStart(&x, &y);            // save current position

    wxHtmlWindow::OnSize(event);

    wxString currpage = GetOpenedPage();
    if ( !currpage.IsEmpty() && canreload ) {
        LoadPage(currpage);         // reload page
        Scroll(x, y);               // scroll to old position
    }

    // prevent wxHtmlWindow::OnSize being called again
    event.Skip(false);
}

// -----------------------------------------------------------------------------

void HtmlView::SetFontSizes(int size)
{
    // set font sizes for <FONT SIZE=-2> to <FONT SIZE=+4>
    int f_sizes[7];
    f_sizes[0] = int(size * 0.6);
    f_sizes[1] = int(size * 0.8);
    f_sizes[2] = size;
    f_sizes[3] = int(size * 1.2);
    f_sizes[4] = int(size * 1.4);
    f_sizes[5] = int(size * 1.6);
    f_sizes[6] = int(size * 1.8);
    #ifdef __WXOSX_COCOA__
        // use Courier rather than Monaco to avoid baseline bug
        // (there are still problems with Courier, but not as bad)
        SetFonts(wxT("Lucida Grande"), wxT("Courier"), f_sizes);
    #else
        SetFonts(wxEmptyString, wxEmptyString, f_sizes);
    #endif
}

// -----------------------------------------------------------------------------

void HtmlView::ChangeFontSizes(int size)
{
    // changing font sizes resets pos to top, so save and restore pos
    int x, y;
    GetViewStart(&x, &y);
    SetFontSizes(size);
    Scroll(x, y);

    // this hack fixes display bug when size increases so that scroll bar appears
    // (curiously, we don't see any such bug in InfoPanel)
    wxRect r = GetRect();
    SetSize(r.x, r.y, r.width-1, r.height);
    SetSize(r);

    panel->UpdateHelpButtons();
}

// -----------------------------------------------------------------------------

// define a panel that contains a HtmlView window

BEGIN_EVENT_TABLE(HelpPanel, wxPanel)
    EVT_BUTTON (ID::BackButton,     HelpPanel::OnBackButton)
    EVT_BUTTON (ID::ForwardButton,  HelpPanel::OnForwardButton)
    EVT_BUTTON (ID::ContentsButton, HelpPanel::OnContentsButton)
    EVT_BUTTON (ID::SmallerButton,  HelpPanel::OnSmallerButton)
    EVT_BUTTON (ID::BiggerButton,   HelpPanel::OnBiggerButton)
END_EVENT_TABLE()

HelpPanel::HelpPanel(MyFrame* parent, wxWindowID id)
    : wxPanel(parent,id), frame(parent)
{
    html = new HtmlView(this, frame, wxID_ANY);

    html->SetFontSizes(helpfontsize);

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    // add buttons at top of html window
    backbutt = new wxButton(this, ID::BackButton, _("<"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    forwbutt = new wxButton(this, ID::ForwardButton, _(">"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    contbutt = new wxButton(this, ID::ContentsButton, _("Contents"));
    smallerbutt = new wxButton(this, ID::SmallerButton, _("-"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    biggerbutt = new wxButton(this, ID::BiggerButton, _("+"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

    #ifdef __WXMAC__
        // nicer to use smaller buttons -- also do for Win/Linux???
        backbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        forwbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        contbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        smallerbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        biggerbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    #endif

    hbox->Add(backbutt, 0, wxLEFT | wxTOP | wxBOTTOM | wxALIGN_LEFT, 10);
    hbox->Add(forwbutt, 0, wxTOP | wxBOTTOM | wxALIGN_LEFT, 10);
    hbox->Add(contbutt, 0, wxLEFT | wxTOP | wxBOTTOM | wxALIGN_LEFT, 10);
    hbox->Add(smallerbutt, 0, wxLEFT | wxTOP | wxBOTTOM | wxALIGN_LEFT, 10);
    hbox->Add(biggerbutt, 0, wxRIGHT | wxTOP | wxBOTTOM | wxALIGN_LEFT, 10);

    status = new wxStaticText(this, wxID_STATIC, wxEmptyString);
    #ifdef __WXMAC__
        status->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    #endif
    wxBoxSizer* statbox = new wxBoxSizer(wxHORIZONTAL);
    statbox->Add(status);
    hbox->Add(statbox, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 10);

    hbox->AddStretchSpacer(1);
    vbox->Add(hbox, 0, wxALL | wxEXPAND | wxALIGN_TOP, 0);
    vbox->Add(html, 1, wxEXPAND | wxALIGN_TOP, 0);
    SetSizer(vbox);

    // prevent HtmlView::OnSize calling LoadPage twice
    html->canreload = false;
    ShowHelp(helphome);
    html->canreload = true;

    // install event handlers to detect keyboard shortcuts when html window has focus
    html->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MyFrame::OnKeyDown), NULL, frame);
    html->Connect(wxEVT_CHAR, wxKeyEventHandler(MyFrame::OnChar), NULL, frame);
}

// -----------------------------------------------------------------------------

void HelpPanel::ShowHelp(const wxString& filepath)
{
    if (filepath == SHOW_KEYBOARD_SHORTCUTS) {
        // build HTML string to display current keyboard shortcuts
        wxString contents = GetShortcutTable();

        // write contents to file and call LoadPage so that back/forwards buttons work
        wxString htmlfile = tempdir + SHOW_KEYBOARD_SHORTCUTS;
        wxFile outfile(htmlfile, wxFile::write);
        if (outfile.IsOpened()) {
            outfile.Write(contents);
            outfile.Close();
            html->LoadPage(htmlfile);
        } else {
            Warning(_("Could not create file:\n") + htmlfile);
            // might as well show contents
            html->SetPage(contents);
        }

    } else if ( filepath.StartsWith(_("Help/")) ) {
        // safer to prepend location of app
        wxString fullpath = readydir + filepath;
        html->LoadPage(fullpath);

    } else {
        // assume full path or local link
        html->LoadPage(filepath);
    }

    UpdateHelpButtons();
}

// -----------------------------------------------------------------------------

void HelpPanel::OnBackButton(wxCommandEvent& WXUNUSED(event))
{
    if ( html->HistoryBack() ) UpdateHelpButtons();
}

// -----------------------------------------------------------------------------

void HelpPanel::OnForwardButton(wxCommandEvent& WXUNUSED(event))
{
    if ( html->HistoryForward() ) UpdateHelpButtons();
}

// -----------------------------------------------------------------------------

void HelpPanel::OnContentsButton(wxCommandEvent& WXUNUSED(event))
{
    ShowHelp(helphome);
}

// -----------------------------------------------------------------------------

void HelpPanel::OnSmallerButton(wxCommandEvent& WXUNUSED(event))
{
    if ( helpfontsize > minfontsize ) {
        helpfontsize--;
        html->ChangeFontSizes(helpfontsize);
    }
}

// -----------------------------------------------------------------------------

void HelpPanel::OnBiggerButton(wxCommandEvent& WXUNUSED(event))
{
    if ( helpfontsize < maxfontsize ) {
        helpfontsize++;
        html->ChangeFontSizes(helpfontsize);
    }
}

// -----------------------------------------------------------------------------

void HelpPanel::UpdateHelpButtons()
{
    backbutt->Enable( html->HistoryCanBack() );
    forwbutt->Enable( html->HistoryCanForward() );

    // check for title used in helphome
    contbutt->Enable( html->GetOpenedPageTitle() != _("Ready Help: Contents") );

    smallerbutt->Enable( helpfontsize > minfontsize );
    biggerbutt->Enable( helpfontsize < maxfontsize );

    html->ClearStatus();
    html->SetFocus();       // for keyboard shortcuts
}

// -----------------------------------------------------------------------------

bool HelpPanel::HtmlHasFocus()
{
    return html->HasFocus();
}

// -----------------------------------------------------------------------------

void HelpPanel::SelectAllText()
{
    html->SelectAll();
}

// -----------------------------------------------------------------------------

void HelpPanel::CopySelection()
{
    wxString text = html->SelectionToText();
    if (text.length() > 0) {
        CopyTextToClipboard(text);
    }
}

// -----------------------------------------------------------------------------

bool HelpPanel::DoKey(int key, int mods)
{
    // first look for keys that should be passed to the default handler
    if ( mods == wxMOD_NONE ) {
        if ( key == WXK_UP || key == WXK_DOWN || key == WXK_LEFT || key == WXK_RIGHT ||
             key == WXK_PAGEUP || key == WXK_PAGEDOWN ) {
            // let default handler see arrow keys and page up/down keys (for scroll bars)
            return false;
        }
    }

    // now do keyboard shortcuts specific to help pane
    if ( mods == wxMOD_NONE ) {
        if ( key == '[' ) {
            if ( html->HistoryBack() ) UpdateHelpButtons();
            return true;
        }
        if ( key == ']' ) {
            if ( html->HistoryForward() ) UpdateHelpButtons();
            return true;
        }
        if ( key == WXK_HOME ) {
            ShowHelp(helphome);
            return true;
        }
    }

    // finally do other keyboard shortcuts
    frame->ProcessKey(key, mods);
    return true;
}

// -----------------------------------------------------------------------------

void ShowAboutBox()
{
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    wxDialog dlg(wxGetApp().currframe, wxID_ANY, wxString(_("About Ready")));

    HtmlView* html = new HtmlView(&dlg, wxGetApp().currframe, wxID_ANY, wxDefaultPosition,
                                  #if wxCHECK_VERSION(2,9,0)
                                      // work around SetSize bug below
                                      wxSize(400, 600),
                                  #else
                                      wxSize(300, 300),
                                  #endif
                                  wxHW_SCROLLBAR_NEVER | wxSUNKEN_BORDER);
    html->inAbout = true;
    html->SetBorders(0);
    #ifdef __WXOSX_COCOA__
        html->SetFontSizes(13);
    #endif
    html->LoadPage(readydir + _("Help/about.html"));

    // avoid HtmlView::OnSize calling LoadPage
    html->canreload = false;

    // this call seems to be ignored in wx 2.9.x
    html->SetSize(html->GetInternalRepresentation()->GetWidth(),
                  html->GetInternalRepresentation()->GetHeight());

    topsizer->Add(html, 1, wxALL, 10);

    wxButton* okbutt = new wxButton(&dlg, wxID_OK, _("OK"));
    okbutt->SetDefault();
    topsizer->Add(okbutt, 0, wxBOTTOM | wxALIGN_CENTER, 10);

    dlg.SetSizer(topsizer);
    topsizer->Fit(&dlg);
    dlg.Centre();
    dlg.ShowModal();
}
