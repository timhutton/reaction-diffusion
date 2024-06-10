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

#include "HtmlInfo.hpp"

// Local:
#include "frame.hpp"
#include "InfoPanel.hpp"
#include "prefs.hpp"
#include "wxutils.hpp"

// -----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(HtmlInfo, wxHtmlWindow)
  EVT_SIZE(HtmlInfo::OnSize)
  EVT_MOTION(HtmlInfo::OnMouseMotion)
  EVT_ENTER_WINDOW(HtmlInfo::OnMouseMotion)
  EVT_LEAVE_WINDOW(HtmlInfo::OnMouseLeave)
  EVT_LEFT_DOWN(HtmlInfo::OnMouseDown)
  EVT_RIGHT_DOWN(HtmlInfo::OnMouseDown)
  EVT_HTML_CELL_CLICKED(wxID_ANY, HtmlInfo::OnHtmlCellClicked)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

const wxString HtmlInfo::change_prefix = "change: ";

HtmlInfo::HtmlInfo(wxWindow* parent, MyFrame* myframe, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxHtmlWindow(parent, id, pos, size, style)
{
    frame = myframe;
    panel = (InfoPanel*)parent;
    editlink = false;
    linkrect = wxRect(0, 0, 0, 0);
    scroll_x = scroll_y = 0;
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnLinkClicked(const wxHtmlLinkInfo& link)
{
    wxString url = link.GetHref();
    if (url.StartsWith(wxT("http:")) || url.StartsWith(wxT("https:")) || url.StartsWith(wxT("mailto:"))) {
        // pass http/mailto URL to user's preferred browser/emailer
        if (!wxLaunchDefaultBrowser(url))
            Warning(_("Could not open URL in browser!"));

    }
    else if (url.StartsWith(change_prefix)) {
        panel->ChangeInfo(url.Mid(change_prefix.size()));
        // best to reset focus after dialog closes
        SetFocus();

    }
    else if (url.StartsWith(wxT("prefs:"))) {
        // user clicked on link to Preferences dialog
        frame->ShowPrefsDialog(url.AfterFirst(':'));

    }
    else if (url.StartsWith(wxT("edit:"))) {
        // open clicked file in user's preferred text editor
        wxString path = url.AfterFirst(':');
#ifdef __WXMSW__
        path.Replace(wxT("/"), wxT("\\"));
#endif
        wxFileName fname(path);
        if (!fname.IsAbsolute()) path = readydir + path;
        frame->EditFile(path);


    }
    else if (url.StartsWith(wxT("open:"))) {
        // open clicked file
        wxString path = url.AfterFirst(':');
#ifdef __WXMSW__
        path.Replace(wxT("/"), wxT("\\"));
#endif
        wxFileName fname(path);
        if (!fname.IsAbsolute()) path = readydir + path;
        if (editlink) {
            frame->EditFile(path);
        }
        else {
            frame->Raise();
            frame->OpenFile(path);
        }

    }
    else {
        // assume it's a link to a local target
        LoadPage(url);
    }
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnHtmlCellClicked(wxHtmlCellEvent& event)
{
    wxHtmlCell* cell = event.GetCell();
    int x = event.GetPoint().x;
    int y = event.GetPoint().y;

    wxHtmlLinkInfo* link = cell->GetLink(x, y);
    if (link) {
        // set linkrect to avoid bug in wxHTML if click was in outer edge of link
        // (bug is in htmlwin.cpp in wxHtmlWindowMouseHelper::HandleIdle;
        // OnCellMouseHover needs to be called if cell != m_tmpLastCell)
        wxPoint pt = ScreenToClient(wxGetMousePosition());
        linkrect = wxRect(pt.x - x, pt.y - y, cell->GetWidth(), cell->GetHeight());
    }

    event.Skip();   // call OnLinkClicked
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnCellMouseHover(wxHtmlCell* cell, wxCoord x, wxCoord y)
{
    wxHtmlLinkInfo* link = cell->GetLink(x, y);
    if (link) {
        wxString href = link->GetHref();
        href.Replace(wxT("&"), wxT("&&"));
        panel->SetStatus(href);
        wxPoint pt = ScreenToClient(wxGetMousePosition());
        linkrect = wxRect(pt.x - x, pt.y - y, cell->GetWidth(), cell->GetHeight());
    }
    else {
        ClearStatus();
    }
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnMouseMotion(wxMouseEvent& event)
{
    if (!linkrect.IsEmpty()) {
        int x = event.GetX();
        int y = event.GetY();
        if (!linkrect.Contains(x, y)) ClearStatus();
    }
    event.Skip();
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnMouseLeave(wxMouseEvent& event)
{
    ClearStatus();
    event.Skip();
}

// -----------------------------------------------------------------------------

void HtmlInfo::ClearStatus()
{
    panel->SetStatus(wxEmptyString);
    linkrect = wxRect(0, 0, 0, 0);
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnMouseDown(wxMouseEvent& event)
{
    // set flag so ctrl/right-clicked file can be opened in editor
    editlink = event.ControlDown() || event.RightDown();
    event.Skip();
}

// -----------------------------------------------------------------------------

// avoid scroll position being reset to top when wxHtmlWindow is resized
void HtmlInfo::OnSize(wxSizeEvent& event)
{
    SaveScrollPos();

    Freeze(); // prevent flicker

    wxHtmlWindow::OnSize(event);

    wxString currpage = GetOpenedPage();
    if (!currpage.IsEmpty()) {
        LoadPage(currpage);         // reload page
    }

    RestoreScrollPos();
    Thaw();

    // prevent wxHtmlWindow::OnSize being called again
    event.Skip(false);
}

// -----------------------------------------------------------------------------

void HtmlInfo::SetFontSizes(int size)
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
    SetFonts(wxT("Lucida Grande"), wxT("Monaco"), f_sizes);
#else
    SetFonts(wxEmptyString, wxEmptyString, f_sizes);
#endif
}

// -----------------------------------------------------------------------------

void HtmlInfo::ChangeFontSizes(int size)
{
    // changing font sizes resets pos to top, so save and restore pos
    SaveScrollPos();
    SetFontSizes(size);
    RestoreScrollPos();
    panel->UpdateButtons();
}

// -----------------------------------------------------------------------------

