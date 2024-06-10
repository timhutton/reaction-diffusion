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
#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

class MyFrame;

/// The wxApp-derived application class for the Ready GUI.
class MyApp : public wxApp
{
public:
    virtual bool OnInit();

    #ifdef __WXMAC__
        // called in response to an open-document event which is sent
        // if a .vti file is double-clicked or dropped onto the app icon
        virtual void MacOpenFile(const wxString& fullPath);
    #endif

    // we only support one frame window at the moment,
    // but eventually we might allow multiple frames
    MyFrame* currframe;
};

DECLARE_APP(MyApp)   // so other files can use wxGetApp
