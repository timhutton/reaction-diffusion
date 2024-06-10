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

// Local:
class InfoPanel;
class MyFrame;

// wxWidgets:
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/html/htmlwin.h>

// define a child window for displaying HTML info
class HtmlInfo : public wxHtmlWindow
{
    public:

        HtmlInfo(wxWindow* parent, MyFrame* myframe, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxHW_SCROLLBAR_AUTO | wxWANTS_CHARS);

        virtual void OnLinkClicked(const wxHtmlLinkInfo& link);
        virtual void OnCellMouseHover(wxHtmlCell* cell, wxCoord x, wxCoord y);

        void ClearStatus();  // clear pane's status line

        void SetFontSizes(int size);
        void ChangeFontSizes(int size);

        wxRect linkrect;     // rect for cell containing link

        void SaveScrollPos() { GetViewStart(&scroll_x, &scroll_y); }
        void RestoreScrollPos() { Scroll(scroll_x, scroll_y); }
        void ResetScrollPos() { Scroll(0,0); }

        static const wxString change_prefix;

    private:

        void OnSize(wxSizeEvent& event);
        void OnMouseMotion(wxMouseEvent& event);
        void OnMouseLeave(wxMouseEvent& event);
        void OnMouseDown(wxMouseEvent& event);
        void OnHtmlCellClicked(wxHtmlCellEvent& event);

    private:

        MyFrame* frame;
        InfoPanel* panel;

        bool editlink;          // open clicked file in editor?
        int scroll_x,scroll_y;  // remember the scroll position

        DECLARE_EVENT_TABLE()
};
