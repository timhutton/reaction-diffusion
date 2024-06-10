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
#include "app.hpp"
#include "frame.hpp"
#include "utils.hpp"        // for STR
#include "prefs.hpp"        // for readydir, maximize

// wxWidgets:
#include <wx/stdpaths.h>    // for wxStandardPaths
#include <wx/filename.h>    // for wxFileName

// STL:
#include <stdexcept>

using namespace std;

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution and also implements the
// accessor function wxGetApp() which will return the reference of the correct
// type (ie. MyApp and not wxApp).
IMPLEMENT_APP(MyApp)

static void SetAppDirectory(const char* argv0)
{
    #if defined(__WXMSW__)
        // on Windows we need to reset current directory to app directory if user
        // dropped file from somewhere else onto app to start it up (otherwise we
        // can't find Help files)
        wxString appdir = wxStandardPaths::Get().GetDataDir();
        wxString currdir = wxGetCwd();
        if ( currdir.CmpNoCase(appdir) != 0 )
            wxSetWorkingDirectory(appdir);
        // avoid VC++ warning
        wxUnusedVar(argv0);
    #elif defined(__WXMAC__)
        // wxMac has set current directory to location of .app bundle so no need
        // to do anything
    #elif defined(__WXGTK__)
        // first, try to switch to READYDIR if that is set to a sensible value:
        static const char *gd = STR(READYDIR);
        if ( *gd == '/' && wxSetWorkingDirectory(wxString(gd,wxConvLocal)) ) {
            return;
        }
        // otherwise, use the executable directory as the application directory.
        // user might have started app from a different directory so find
        // last "/" in argv0 and change cwd if "/" isn't part of "./" prefix
        unsigned int pos = strlen(argv0);
        while (pos > 0) {
            pos--;
            if (argv0[pos] == '/') break;
        }
        if ( pos > 0 && !(pos == 1 && argv0[0] == '.') ) {
            char appdir[2048];
            if (pos < sizeof(appdir)) {
                strncpy(appdir, argv0, pos);
                appdir[pos] = 0;
                wxSetWorkingDirectory(wxString(appdir,wxConvLocal));
            }
        }
    #endif
}

#ifdef __WXMAC__
// open a .vti file that was double-clicked or dropped onto app icon
void MyApp::MacOpenFile(const wxString& fullPath)
{
    currframe->Raise();
    currframe->OpenFile(fullPath);
}
#endif

bool MyApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    SetAppName(_("Ready"));    // for use in Warning/Fatal dialogs

#if defined(__WXMAC__) && wxCHECK_VERSION(3,1,4)
    // avoid Show Tab Bar etc appearing in View menu
    OSXEnableAutomaticTabbing(false);
#endif

    srand((unsigned int)time(NULL));

    // get current working directory before calling SetAppDirectory
    wxString initdir = wxGetCwd();
    if (initdir.Last() != wxFILE_SEP_PATH) initdir += wxFILE_SEP_PATH;

    // make sure current working directory contains application otherwise
    // we can't open Help files
    SetAppDirectory( wxString(argv[0]).mb_str(wxConvLocal) );

    // now set global readydir for use elsewhere
    readydir = wxGetCwd();
    if (readydir.Last() != wxFILE_SEP_PATH) readydir += wxFILE_SEP_PATH;

    ::wxInitAllImageHandlers(); // make sure we can handle all sorts of images

    currframe = new MyFrame(_("Ready"));

    // prefs file has now been loaded
    if (maximize) currframe->Maximize(true);
    currframe->Show(true);
    SetTopWindow(currframe);

    // argc is > 1 if command line has one or more pattern files
    for (int n = 1; n < argc; n++) {
        wxFileName filename(argv[n]);
        // convert given path to a full path if not one already; this allows users
        // to do things like "../ready foo.vti" from within Patterns folder
        if (!filename.IsAbsolute()) filename = initdir + argv[n];
        currframe->OpenFile(filename.GetFullPath());
    }

    return true;
}

// homepage for doxygen docs:

/*! \mainpage
 *
 * The links above can be used to explore the classes and source code files.
 *
 * \section main_classes_sec Important base classes:
 *
 *  * AbstractRD - the base class for all our reaction-diffusion implementations
 *  * XML_Object - many classes can be serialized to/from XML files
 *  * MyFrame is the top-level window for the GUI
 *
 * \section external_sec External source code:
 *
 *  * <a href="http://www.vtk.org/doc/nightly/html/classes.html">VTK</a>
 *  * <a href="http://docs.wxwidgets.org/trunk/classes.html">wxWidgets</a>
 *
 * \section homepage_sec Homepage:
 *
 * <https://github.com/GollyGang/ready>
 *
 */
