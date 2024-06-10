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
#include "wx/wxprec.h"           // for compilers that support precompilation
#ifndef WX_PRECOMP
  #include "wx/wx.h"               // for all others include the necessary headers
#endif
#include <wx/stdpaths.h>         // for wxStandardPaths
#include <wx/filename.h>         // for wxFileName
#include <wx/propdlg.h>          // for wxPropertySheetDialog
#include <wx/bookctrl.h>         // for wxBookCtrlBase
#include <wx/notebook.h>         // for wxNotebookEvent
#include <wx/spinctrl.h>         // for wxSpinCtrl
#if wxUSE_TOOLTIPS
    #include <wx/tooltip.h>      // for wxToolTip
#endif

// local:
#include "IDs.hpp"               // for MAX_RECENT, etc
#include "app.hpp"               // for wxGetApp
#include "frame.hpp"             // for MyFrame
#include "utils.hpp"             // for STR
#include "wxutils.hpp"           // for Warning, Fatal, Beep
#include "prefs.hpp"

// STL:
#include <algorithm>

// -----------------------------------------------------------------------------

// Ready's preferences file is a simple text file.  It's initially created in
// a user-specific data directory (datadir) but we look in the application
// directory (readydir) first because this makes uninstalling simple and allows
// multiple copies/versions of the app to have separate preferences.

const wxString PREFS_NAME = wxT("ReadyPrefs");

wxString prefspath;              // full path to prefs file
wxString choosedir;              // directory used by Choose File button

// location of supplied pattern collection (relative to app)
const wxString PATT_DIR = wxT("Patterns");

const int PREFS_VERSION = 2;     // increment if necessary due to changes in syntax/semantics
int currversion = PREFS_VERSION; // might be changed by prefs_version
const int PREF_LINE_SIZE = 5000; // must be quite long for storing file paths

const int minmainwd = 200;       // main window's minimum width
const int minmainht = 100;       // main window's minimum height

wxString readydir;               // directory containing app
wxString datadir;                // directory for user-specific data
wxString tempdir;                // directory for temporary data
wxString patterndir;             // directory for supplied patterns
wxString recordingdir;           // directory for recording images

// initialize exported preferences:

int mainx = 30;                  // main window's initial location
int mainy = 40;
int mainwd = 1000;               // main window's initial size
int mainht = 800;
bool maximize = false;           // maximize main window?
wxString auilayout = wxT("");    // wxAUI layout info
int opencl_platform = 0;         // current OpenCL platform
int opencl_device = 0;           // current OpenCL device
int debuglevel = 0;              // for displaying debug info if > 0
#ifdef __WXMAC__
    int infofontsize = 12;       // font size in Mac info pane
    int helpfontsize = 12;       // font size in Mac help pane
#else
    int infofontsize = 10;       // font size in Win/Linux info pane
    int helpfontsize = 10;       // font size in Win/Linux help pane
#endif
int textdlgwd = 800;             // width of multi-line text dialog
int textdlght = 600;             // height of multi-line text dialog
bool showtips = true;            // show button tips?
bool repaint_to_erase = false;   // whether painting over the current color reverts to low
bool allowbeep = true;           // okay to play beep sound?
bool askonnew = true;            // ask to save changes before creating new pattern?
bool askonload = true;           // ask to save changes before loading pattern file?
bool askonquit = true;           // ask to save changes before quitting app?
wxString opensavedir;            // directory for Open/Save Pattern dialogs
wxString screenshotdir;          // directory for Save Screenshot dialog
wxString userdir;                // directory for user's patterns
wxString texteditor;             // path of user's preferred text editor
wxMenu* patternSubMenu = NULL;   // submenu of recent pattern files
int numpatterns = 0;             // current number of recent pattern files
int maxpatterns = 20;            // maximum number of recent pattern files (1..MAX_RECENT)
int current_brush_size = 1;      // small, medium or large brush

// local (ie. non-exported) globals:

// set of modifier keys (note that MSVC didn't like MK_*)
const int mk_CMD     = 1;        // command key on Mac, control key on Win/Linux
const int mk_ALT     = 2;        // option key on Mac
const int mk_SHIFT   = 4;
#ifdef __WXMAC__
const int mk_CTRL    = 8;        // control key is separate modifier on Mac
const int MAX_MODS   = 16;
#else
const int MAX_MODS   = 8;
#endif

// WXK_* key codes like WXK_F1 have values > 300, so to minimize the
// size of the keyaction table (see below) we use our own internal
// key codes for function keys and other special keys
const int IK_NULL       = 0;     // probably best never to use this
const int IK_HOME       = 1;
const int IK_END        = 2;
const int IK_PAGEUP     = 3;
const int IK_PAGEDOWN   = 4;
const int IK_HELP       = 5;
const int IK_INSERT     = 6;
const int IK_DELETE     = 8;     // treat delete like backspace (consistent with GSF_dokey)
const int IK_TAB        = 9;
const int IK_RETURN     = 13;
const int IK_LEFT       = 28;
const int IK_RIGHT      = 29;
const int IK_UP         = 30;
const int IK_DOWN       = 31;
const int IK_F1         = 'A';   // we use shift+a for the real A, etc
const int IK_F24        = 'X';
const int MAX_KEYCODES  = 128;

// names of the non-displayable keys we currently support;
// note that these names can be used in menu item accelerator strings
// so they must match legal wx names (listed in wxMenu::Append docs)
const char NK_HOME[]    = "Home";
const char NK_END[]     = "End";
const char NK_PGUP[]    = "PgUp";
const char NK_PGDN[]    = "PgDn";
const char NK_HELP[]    = "Help";
const char NK_INSERT[]  = "Insert";
const char NK_DELETE[]  = "Delete";
const char NK_TAB[]     = "Tab";
#ifdef __WXMSW__
    const char NK_RETURN[]  = "Enter";
#else
    const char NK_RETURN[]  = "Return";
#endif
const char NK_LEFT[]    = "Left";
const char NK_RIGHT[]   = "Right";
const char NK_UP[]      = "Up";
const char NK_DOWN[]    = "Down";
const char NK_SPACE[]   = "Space";

const action_info nullaction = { DO_NOTHING, wxEmptyString };

// table for converting key combinations into actions
action_info keyaction[MAX_KEYCODES][MAX_MODS] = { { nullaction } };

// strings for setting menu item accelerators
wxString accelerator[MAX_ACTIONS];

// -----------------------------------------------------------------------------

bool ConvertKeyAndModifiers(int wxkey, int wxmods, int* newkey, int* newmods)
{
    // first convert given wx modifiers (set by wxKeyEvent::GetModifiers)
    // to a corresponding set of mk_* values
    int ourmods = 0;
    if (wxmods & wxMOD_CMD)       ourmods |= mk_CMD;
    if (wxmods & wxMOD_ALT)       ourmods |= mk_ALT;
    if (wxmods & wxMOD_SHIFT)     ourmods |= mk_SHIFT;
#ifdef __WXMAC__
    if (wxmods & wxMOD_CONTROL)   ourmods |= mk_CTRL;
#endif

    // now convert given wx key code to corresponding IK_* code
    int ourkey;
    if (wxkey >= 'A' && wxkey <= 'Z') {
        // convert A..Z to shift+a..shift+z so we can use A..X
        // for our internal function keys (IK_F1 to IK_F24)
        ourkey = wxkey + 32;
        ourmods |= mk_SHIFT;

    }
    else if (wxkey >= WXK_F1 && wxkey <= WXK_F24) {
        // convert wx function key code to IK_F1..IK_F24
        ourkey = IK_F1 + (wxkey - WXK_F1);

    }
    else if (wxkey >= WXK_NUMPAD0 && wxkey <= WXK_NUMPAD9) {
        // treat numpad digits like ordinary digits
        ourkey = '0' + (wxkey - WXK_NUMPAD0);

    }
    else {
        switch (wxkey) {
            case WXK_HOME:          ourkey = IK_HOME; break;
            case WXK_END:           ourkey = IK_END; break;
            case WXK_PAGEUP:        ourkey = IK_PAGEUP; break;
            case WXK_PAGEDOWN:      ourkey = IK_PAGEDOWN; break;
            case WXK_HELP:          ourkey = IK_HELP; break;
            case WXK_INSERT:        ourkey = IK_INSERT; break;
            case WXK_BACK:          // treat backspace like delete
            case WXK_DELETE:        ourkey = IK_DELETE; break;
            case WXK_TAB:           ourkey = IK_TAB; break;
            case WXK_NUMPAD_ENTER:  // treat enter like return
            case WXK_RETURN:        ourkey = IK_RETURN; break;
            case WXK_LEFT:          ourkey = IK_LEFT; break;
            case WXK_RIGHT:         ourkey = IK_RIGHT; break;
            case WXK_UP:            ourkey = IK_UP; break;
            case WXK_DOWN:          ourkey = IK_DOWN; break;
            case WXK_ADD:           ourkey = '+'; break;
            case WXK_SUBTRACT:      ourkey = '-'; break;
            case WXK_DIVIDE:        ourkey = '/'; break;
            case WXK_MULTIPLY:      ourkey = '*'; break;
            default:                ourkey = wxkey;
        }
    }

    if (ourkey < 0 || ourkey >= MAX_KEYCODES) return false;

    *newkey = ourkey;
    *newmods = ourmods;
    return true;
}

// -----------------------------------------------------------------------------

action_info FindAction(int wxkey, int wxmods)
{
    // convert given wx key code and modifier set to our internal values
    // and return the corresponding action
    int ourkey, ourmods;
    if ( ConvertKeyAndModifiers(wxkey, wxmods, &ourkey, &ourmods) ) {
        return keyaction[ourkey][ourmods];
    }
    else {
        return nullaction;
    }
}

// -----------------------------------------------------------------------------

void AddDefaultKeyActions()
{
    // File menu
    keyaction[(int)'n'][mk_CMD].id =    DO_NEWPATT;
    keyaction[(int)'o'][mk_CMD].id =    DO_OPENPATT;
    keyaction[(int)'s'][mk_CMD].id =    DO_SAVE;
    keyaction[IK_F1+2][0].id =          DO_SCREENSHOT;
#ifdef __WXMSW__
    // Windows does not support ctrl+non-alphanumeric
#else
    keyaction[(int)','][mk_CMD].id =    DO_PREFS;
#endif
    keyaction[(int)','][0].id =         DO_PREFS;
    keyaction[(int)'q'][mk_CMD].id =    DO_QUIT;

    // Edit menu
    keyaction[(int)'z'][mk_CMD].id =    DO_UNDO;
#ifdef __WXMAC__
    keyaction[(int)'z'][mk_SHIFT+mk_CMD].id = DO_REDO;
#else
    keyaction[(int)'y'][mk_CMD].id =    DO_REDO;
#endif
    keyaction[(int)'x'][mk_CMD].id =    DO_CUT;
    keyaction[(int)'c'][mk_CMD].id =    DO_COPY;
    keyaction[(int)'v'][mk_CMD].id =    DO_PASTE;
    keyaction[(int)'a'][mk_CMD].id =    DO_SELALL;

    // View menu
    keyaction[IK_F1+10][0].id =         DO_FULLSCREEN;
    keyaction[(int)'f'][mk_CMD].id =    DO_FIT;
    keyaction[(int)'w'][0].id =         DO_WIREFRAME;
    keyaction[(int)'p'][mk_CMD].id =    DO_PATTERNS;
    keyaction[(int)'i'][mk_CMD].id =    DO_INFO;
    keyaction[(int)'h'][0].id =         DO_HELP;
    keyaction[(int)'?'][0].id =         DO_HELP;
    keyaction[IK_HELP][0].id =          DO_HELP;
#ifdef __WXMAC__
    // cmd-? is the usual shortcut in Mac apps
    keyaction[(int)'?'][mk_CMD].id =    DO_HELP;
    // we can only detect shift+cmd+/ so we have to assume '?' is above '/' -- yuk
    keyaction[(int)'/'][mk_SHIFT+mk_CMD].id = DO_HELP;
#else
    // F1 is the usual shortcut in Win/Linux apps
    keyaction[IK_F1][0].id =            DO_HELP;
#endif

    // Action menu
    keyaction[(int)' '][0].id =         DO_STEP1;
    keyaction[IK_TAB][0].id =           DO_STEPN;
    keyaction[IK_RETURN][0].id =        DO_RUNSTOP;
    keyaction[(int)'r'][mk_CMD].id =    DO_RESET;
    keyaction[(int)'+'][0].id =         DO_FASTER;
    keyaction[(int)'+'][mk_SHIFT].id =  DO_FASTER;
    keyaction[(int)'='][0].id =         DO_FASTER;
    keyaction[(int)'_'][0].id =         DO_SLOWER;
    keyaction[(int)'_'][mk_SHIFT].id =  DO_SLOWER;
    keyaction[(int)'-'][0].id =         DO_SLOWER;
}

// -----------------------------------------------------------------------------

const char* GetActionName(action_id action)
{
    switch (action) {
        case DO_NOTHING:        return "NONE";
        case DO_OPENFILE:       return "Open:";
        // File menu
        case DO_NEWPATT:        return "New Pattern";
        case DO_OPENPATT:       return "Open Pattern...";
        case DO_RELOAD:         return "Reload from Disk";
        case DO_IMPORTMESH:     return "Import Mesh...";
        case DO_EXPORTMESH:     return "Export Mesh...";
        case DO_IMPORTIMAGE:    return "Import Image...";
        case DO_EXPORTIMAGE:    return "Export Image...";
        case DO_SAVE:           return "Save Pattern...";
        case DO_SAVECOMPACT:    return "Save Compact...";
        case DO_SCREENSHOT:     return "Save Screenshot...";
        case DO_RECORDFRAMES:   return "Start Recording...";
        case DO_ADDPATTS:       return "Add My Patterns...";
        case DO_PREFS:          return "Preferences...";
        case DO_QUIT:           return "Quit Ready";
        // Edit menu
        case DO_UNDO:           return "Undo";
        case DO_REDO:           return "Redo";
        case DO_CUT:            return "Cut Selection";
        case DO_COPY:           return "Copy Selection";
        case DO_PASTE:          return "Paste";
        case DO_CLEAR:          return "Clear Selection";
        case DO_SELALL:         return "Select All";
        case DO_POINTER:        return "Select Pointer Tool";
        case DO_PENCIL:         return "Select Pencil Tool";
        case DO_BRUSH:          return "Select Brush Tool";
        case DO_PICKER:         return "Select Color Picker Tool";
        case DO_BRUSHEXTRASMALL:return "Select Extra Small Brush";
        case DO_BRUSHSMALL:     return "Select Small Brush";
        case DO_BRUSHMEDIUM:    return "Select Medium Brush";
        case DO_BRUSHLARGE:     return "Select Large Brush";
        case DO_BRUSHEXTRALARGE:return "Select Extra Large Brush";
        // View menu
        case DO_FULLSCREEN:     return "Full Screen";
        case DO_FIT:            return "Fit Pattern";
        case DO_WIREFRAME:      return "Wireframe";
        case DO_PATTERNS:       return "Show Patterns Pane";
        case DO_INFO:           return "Show Info Pane";
        case DO_HELP:           return "Show Help Pane";
        case DO_FILETOOLBAR:    return "Show File Toolbar";
        case DO_ACTIONTOOLBAR:  return "Show Action Toolbar";
        case DO_PAINTTOOLBAR:   return "Show Paint Toolbar";
        case DO_RESTORE:        return "Restore Default Layout";
        case DO_CHEMICAL:       return "Change Active Chemical...";
        // Action menu
        case DO_STEP1:          return "Step by 1";
        case DO_STEPN:          return "Step by N";
        case DO_RUNSTOP:        return "Run/Stop";
        case DO_FASTER:         return "Run Faster";
        case DO_SLOWER:         return "Run Slower";
        case DO_CHANGESPEED:    return "Change Running Speed...";
        case DO_RESET:          return "Reset";
        case DO_GENPATT:        return "Generate Initial Pattern";
        case DO_BLANK:          return "Blank";
        case DO_ADDPARAM:       return "Add Parameter...";
        case DO_DELPARAM:       return "Delete Parameter...";
        case DO_VIEWKERNEL:     return "View Full Kernel";
        case DO_CONVERTTOKERNEL:return "Convert to Full Kernel";
        case DO_DEVICE:         return "Select OpenCL Device...";
        case DO_OPENCL:         return "Show OpenCL Diagnostics...";
        // Help menu
        case DO_ABOUT:          return "About Ready";
        default:                Warning(_("Bug detected in GetActionName!"));
    }
    return "BUG";
}

// -----------------------------------------------------------------------------

// is there really no C++ standard for case-insensitive string comparison???
#ifdef __WXMSW__
#define ISTRCMP stricmp
#else
#define ISTRCMP strcasecmp
#endif

void GetKeyAction(char* value)
{
    // parse strings like "z undo" or "space+ctrl advance selection";
    // note that some errors detected here can be Fatal because the user
    // has to quit Ready anyway to edit the prefs file
    char* start = value;
    char* p = start;
    int modset = 0;
    int key = -1;

    // extract key, skipping first char in case it's '+'
    if (*p > 0) p++;
    while (1) {
        if (*p == 0) {
            Fatal(wxString::Format(_("Bad key_action value: %s"),
                wxString(value,wxConvLocal).c_str()));
        }
        if (*p == ' ' || *p == '+') {
            // we found end of key
            char oldp = *p;
            *p = 0;
            int len = strlen(start);
            if (len == 1) {
                key = start[0];
                if (key < ' ' || key > '~') {
                    Fatal(wxString::Format(_("Non-displayable key in key_action: code = %d"), key));
                }
                if (key >= 'A' && key <= 'Z') {
                    // convert A..Z to shift+a..shift+z so we can use A..X
                    // for our internal function keys (IK_F1 to IK_F24)
                    key += 32;
                    modset |= mk_SHIFT;
                }
            }
            else if (len > 1) {
                if ((start[0] == 'f' || start[0] == 'F') && start[1] >= '1' && start[1] <= '9') {
                    // we have a function key
                    char* p = &start[1];
                    int num;
                    sscanf(p, "%d", &num);
                    if (num >= 1 && num <= 24) key = IK_F1 + num - 1;
                }
                else {
                    if      (ISTRCMP(start, NK_HOME) == 0)    key = IK_HOME;
                    else if (ISTRCMP(start, NK_END) == 0)     key = IK_END;
                    else if (ISTRCMP(start, NK_PGUP) == 0)    key = IK_PAGEUP;
                    else if (ISTRCMP(start, NK_PGDN) == 0)    key = IK_PAGEDOWN;
                    else if (ISTRCMP(start, NK_HELP) == 0)    key = IK_HELP;
                    else if (ISTRCMP(start, NK_INSERT) == 0)  key = IK_INSERT;
                    else if (ISTRCMP(start, NK_DELETE) == 0)  key = IK_DELETE;
                    else if (ISTRCMP(start, NK_TAB) == 0)     key = IK_TAB;
                    else if (ISTRCMP(start, NK_RETURN) == 0)  key = IK_RETURN;
                    else if (ISTRCMP(start, NK_LEFT) == 0)    key = IK_LEFT;
                    else if (ISTRCMP(start, NK_RIGHT) == 0)   key = IK_RIGHT;
                    else if (ISTRCMP(start, NK_UP) == 0)      key = IK_UP;
                    else if (ISTRCMP(start, NK_DOWN) == 0)    key = IK_DOWN;
                    else if (ISTRCMP(start, NK_SPACE) == 0)   key = ' ';
                }
                if (key < 0)
                    Fatal(wxString::Format(_("Unknown key in key_action: %s"),
                        wxString(start,wxConvLocal).c_str()));
            }
            *p = oldp;           // restore ' ' or '+'
            start = p;
            start++;
            break;
        }
        p++;
    }

    // *p is ' ' or '+' so extract zero or more modifiers
    while (*p != ' ') {
        p++;
        if (*p == 0) {
            Fatal(wxString::Format(_("No action in key_action value: %s"),
                wxString(value,wxConvLocal).c_str()));
        }
        if (*p == ' ' || *p == '+') {
            // we found end of modifier
            char oldp = *p;
            *p = 0;
#ifdef __WXMAC__
            if      (ISTRCMP(start, "cmd") == 0)   modset |= mk_CMD;
            else if (ISTRCMP(start, "opt") == 0)   modset |= mk_ALT;
            else if (ISTRCMP(start, "ctrl") == 0)  modset |= mk_CTRL;
#else
            if      (ISTRCMP(start, "ctrl") == 0)  modset |= mk_CMD;
            else if (ISTRCMP(start, "alt") == 0)   modset |= mk_ALT;
#endif
            else if    (ISTRCMP(start, "shift") == 0) modset |= mk_SHIFT;
            else
                Fatal(wxString::Format(_("Unknown modifier in key_action: %s"),
                    wxString(start,wxConvLocal).c_str()));
            *p = oldp;           // restore ' ' or '+'
            start = p;
            start++;
        }
    }

    // *p is ' ' so skip and check the action string
    p++;
    action_info action = nullaction;

    // first look for "Open:" followed by file path
    if (strncmp(p, "Open:", 5) == 0) {
        action.id = DO_OPENFILE;
        action.file = wxString(&p[5], wxConvLocal);
    }
    else {
        // assume DO_NOTHING is 0 and start with action 1
        for (int i = 1; i < MAX_ACTIONS; i++) {
            if (strcmp(p, GetActionName((action_id) i)) == 0) {
                action.id = (action_id) i;
                break;
            }
        }
    }

    // test for some deprecated actions
    /*
    if (action.id == DO_NOTHING) {
        if (strcmp(p, "Swap Cell Colors") == 0) action.id = DO_INVERT;
    }
    */

    // probably best to silently ignore an unknown action
    // (friendlier if old Ready is reading newer prefs)
    /*
    if (action.id == DO_NOTHING)
       Warning(wxString::Format(_("Unknown action in key_action: %s"),
                                wxString(p,wxConvLocal).c_str()));
    */

    keyaction[key][modset] = action;
}

// -----------------------------------------------------------------------------

wxString GetKeyCombo(int key, int modset)
{
    // build a key combo string for display in prefs dialog and help window
    wxString result = wxEmptyString;

#ifdef __WXMAC__
    if (mk_ALT & modset)    result += wxT("Option-");
    if (mk_SHIFT & modset)  result += wxT("Shift-");
    if (mk_CTRL & modset)   result += wxT("Control-");
    if (mk_CMD & modset)    result += wxT("Command-");
#else
    if (mk_ALT & modset)    result += wxT("Alt+");
    if (mk_SHIFT & modset)  result += wxT("Shift+");
    if (mk_CMD & modset)    result += wxT("Control+");
#endif

    if (key >= IK_F1 && key <= IK_F24) {
        // function key
        result += wxString::Format(wxT("F%d"), key - IK_F1 + 1);

    }
    else if (key >= 'a' && key <= 'z') {
        // display A..Z rather than a..z
        result += wxChar(key - 32);

    }
    else if (key > ' ' && key <= '~') {
        // displayable char, but excluding space (that's handled below)
        result += wxChar(key);

    }
    else {
        // non-displayable char
        switch (key) {
            // these strings can be more descriptive than the NK_* strings
            case IK_HOME:     result += _("Home"); break;
            case IK_END:      result += _("End"); break;
            case IK_PAGEUP:   result += _("PageUp"); break;
            case IK_PAGEDOWN: result += _("PageDown"); break;
            case IK_HELP:     result += _("Help"); break;
            case IK_INSERT:   result += _("Insert"); break;
            case IK_DELETE:   result += _("Delete"); break;
            case IK_TAB:      result += _("Tab"); break;
#ifdef __WXMSW__
            case IK_RETURN:   result += _("Enter"); break;
#else
            case IK_RETURN:   result += _("Return"); break;
#endif
            case IK_LEFT:     result += _("Left"); break;
            case IK_RIGHT:    result += _("Right"); break;
            case IK_UP:       result += _("Up"); break;
            case IK_DOWN:     result += _("Down"); break;
            case ' ':         result += _("Space"); break;
            default:          result = wxEmptyString;
        }
    }

    return result;
}

// -----------------------------------------------------------------------------

wxString GetShortcutTable()
{
    // return HTML data to display current keyboard shortcuts in help window
    wxString result;
    result += _("<html><title>Ready Help: Keyboard Shortcuts</title>");
    result += _("<body>");
    result += _("<p><font size=+1><b>Keyboard shortcuts</b></font>");
    result += _("<p>Use <a href=\"prefs:keyboard\">Preferences > Keyboard</a>");
    result += _(" to change the following keyboard shortcuts:");
    result += _("<p><center>");
    result += _("<table cellspacing=1 border=2 cols=2 width=\"90%\">");
    result += _("<tr><td align=center>Key Combination</td><td align=center>Action</td></tr>");

    bool assigned[MAX_ACTIONS] = {false};

    for (int key = 0; key < MAX_KEYCODES; key++) {
        for (int modset = 0; modset < MAX_MODS; modset++) {
            action_info action = keyaction[key][modset];
            if ( action.id != DO_NOTHING ) {
                assigned[action.id] = true;
                wxString keystring = GetKeyCombo(key, modset);
                if (key == '<') {
                    keystring.Replace(_("<"), _("&lt;"));
                }
                result += _("<tr><td align=right>");
                result += keystring;
                result += _("&nbsp;</td><td>&nbsp;");
                result += wxString(GetActionName(action.id), wxConvLocal);
                if (action.id == DO_OPENFILE) {
                    result += _("&nbsp;");
                    result += action.file;
                }
                result += _("</td></tr>");
            }
        }
    }

    result += _("</table></center>");

    // also list unassigned actions
    result += _("<p>The following actions currently have no keyboard shortcuts:<p>");
    for (int i = 1; i < MAX_ACTIONS; i++) {
        if (!assigned[i]) {
            wxString name = wxString(GetActionName((action_id) i),wxConvLocal);
            result += wxString::Format(_("<dd>%s</dd>"), name.c_str());
        }
    }

    result += _("</body></html>");
    return result;
}

// -----------------------------------------------------------------------------

wxString GetModifiers(int modset)
{
    wxString modkeys = wxEmptyString;
#ifdef __WXMAC__
    if (mk_ALT & modset)    modkeys += wxT("+opt");
    if (mk_SHIFT & modset)  modkeys += wxT("+shift");
    if (mk_CTRL & modset)   modkeys += wxT("+ctrl");
    if (mk_CMD & modset)    modkeys += wxT("+cmd");
#else
    if (mk_ALT & modset)    modkeys += wxT("+alt");
    if (mk_SHIFT & modset)  modkeys += wxT("+shift");
    if (mk_CMD & modset)    modkeys += wxT("+ctrl");
#endif
    return modkeys;
}

// -----------------------------------------------------------------------------

wxString GetKeyName(int key)
{
    wxString result;

    if (key >= IK_F1 && key <= IK_F24) {
        // function key
        result.Printf(wxT("F%d"), key - IK_F1 + 1);

    }
    else if (key > ' ' && key <= '~') {
        // displayable char, but excluding space (that's handled below)
        result = wxChar(key);

    }
    else {
        // non-displayable char
        switch (key) {
            case IK_HOME:     result = wxString(NK_HOME, wxConvLocal); break;
            case IK_END:      result = wxString(NK_END, wxConvLocal); break;
            case IK_PAGEUP:   result = wxString(NK_PGUP, wxConvLocal); break;
            case IK_PAGEDOWN: result = wxString(NK_PGDN, wxConvLocal); break;
            case IK_HELP:     result = wxString(NK_HELP, wxConvLocal); break;
            case IK_INSERT:   result = wxString(NK_INSERT, wxConvLocal); break;
            case IK_DELETE:   result = wxString(NK_DELETE, wxConvLocal); break;
            case IK_TAB:      result = wxString(NK_TAB, wxConvLocal); break;
            case IK_RETURN:   result = wxString(NK_RETURN, wxConvLocal); break;
            case IK_LEFT:     result = wxString(NK_LEFT, wxConvLocal); break;
            case IK_RIGHT:    result = wxString(NK_RIGHT, wxConvLocal); break;
            case IK_UP:       result = wxString(NK_UP, wxConvLocal); break;
            case IK_DOWN:     result = wxString(NK_DOWN, wxConvLocal); break;
            case ' ':         result = wxString(NK_SPACE, wxConvLocal); break;
            default:          result = wxEmptyString;
        }
    }

    return result;
}

// -----------------------------------------------------------------------------

void SaveKeyActions(FILE* f)
{
    bool assigned[MAX_ACTIONS] = {false};

    fputs("\n", f);
    for (int key = 0; key < MAX_KEYCODES; key++) {
        for (int modset = 0; modset < MAX_MODS; modset++) {
            action_info action = keyaction[key][modset];
            if ( action.id != DO_NOTHING ) {
                assigned[action.id] = true;
                fprintf(f, "key_action=%s%s %s%s\n",
                    (const char*) GetKeyName(key).mb_str(wxConvLocal),
                    (const char*) GetModifiers(modset).mb_str(wxConvLocal),
                    GetActionName(action.id),
                    (const char*) action.file.mb_str(wxConvLocal));
            }
        }
    }

    // list all unassigned actions in comment lines
    fputs("# unassigned actions:\n", f);
    for (int i = 1; i < MAX_ACTIONS; i++) {
        if ( !assigned[i] ) {
            fprintf(f, "# key_action=key+mods %s", GetActionName((action_id) i));
            if ( i == DO_OPENFILE ) fputs("file", f);
            fputs("\n", f);
        }
    }
    fputs("\n", f);
}

// -----------------------------------------------------------------------------

void CreateAccelerator(action_id action, int modset, int key)
{
    accelerator[action] = wxT("\t");
#ifdef __WXMAC__
    if (modset & mk_CTRL) accelerator[action] += wxT("RawCtrl+");
#endif
    if (modset & mk_CMD) accelerator[action] += wxT("Ctrl+");
    if (modset & mk_ALT) accelerator[action] += wxT("Alt+");
    if (modset & mk_SHIFT) accelerator[action] += wxT("Shift+");
    if (key >= 'a' && key <= 'z') {
        // convert a..z to A..Z
        accelerator[action] += wxChar(key - 32);
#ifdef __WXMAC__
    }
    else if (key == IK_DELETE) {
        // must use "Back" to get correct symbol (<+ rather than +>)
        accelerator[action] += wxT("Back");
#endif
    }
    else {
        accelerator[action] += GetKeyName(key);
    }
}

// -----------------------------------------------------------------------------

void UpdateAcceleratorStrings()
{
    for (int i = 0; i < MAX_ACTIONS; i++)
        accelerator[i] = wxEmptyString;

    // go thru keyaction table looking for key combos that are valid menu item
    // accelerators and construct suitable strings like "\tCtrl+Alt+Shift+K"
    // or "\tF12" or "\tReturn" etc
    for (int key = 0; key < MAX_KEYCODES; key++) {
        for (int modset = 0; modset < MAX_MODS; modset++) {
            action_info info = keyaction[key][modset];
            action_id action = info.id;
            if (action != DO_NOTHING && accelerator[action].IsEmpty()) {
                // check if key can be used as an accelerator
                if ((key >= ' ' && key <= '~') ||
                    (key >= IK_F1 && key <= IK_F24) ||
                    (key >= IK_LEFT && key <= IK_DOWN) ||
                    key == IK_HOME ||
                    key == IK_END ||
                    key == IK_PAGEUP ||
                    key == IK_PAGEDOWN ||
                    key == IK_DELETE ||
                    key == IK_TAB ||
                    key == IK_RETURN)
                    CreateAccelerator(action, modset, key);
             }
        }
    }

    // go thru keyaction table again looking only for key combos containing Ctrl;
    // we do this so that the Paste menu item will have the standard Ctrl+V
    // shortcut rather than a plain V if both those shortcuts are assigned
    for (int key = 0; key < MAX_KEYCODES; key++) {
        for (int modset = 0; modset < MAX_MODS; modset++) {
            action_info info = keyaction[key][modset];
            action_id action = info.id;
            if (action != DO_NOTHING && (modset & mk_CMD))
                CreateAccelerator(action, modset, key);
         }
    }
}

// -----------------------------------------------------------------------------

wxString GetAccelerator(action_id action)
{
    return accelerator[action];
}

// -----------------------------------------------------------------------------

// some method names have changed in wx 2.9
#if wxCHECK_VERSION(2,9,0)
#define GetLabelFromText GetLabelText
#endif

void RemoveAccelerator(wxMenuBar* mbar, int item, action_id action)
{
    if (!accelerator[action].IsEmpty()) {
        // remove accelerator from given menu item
        mbar->SetLabel(item, wxMenuItem::GetLabelFromText(mbar->GetLabel(item)));
    }
}

// -----------------------------------------------------------------------------

void SetAccelerator(wxMenuBar* mbar, int item, action_id action)
{
    wxString accel = accelerator[action];

    // we need to remove old accelerator string from GetLabel text
    mbar->SetLabel(item, wxMenuItem::GetLabelFromText(mbar->GetLabel(item)) + accel);
}

// -----------------------------------------------------------------------------

void GetRelPath(const char* value, wxString& path,
                const wxString& defdir = wxEmptyString,
                bool isdir = true)
{
    path = wxString(value, wxConvLocal);
    wxFileName fname(path);

    // if path isn't absolute then prepend Ready directory
    if (!fname.IsAbsolute()) path = readydir + path;

    if (defdir.length() > 0) {
        // if path doesn't exist then reset to default directory
        if (!wxFileName::DirExists(path)) path = readydir + defdir;
    }

    // nicer if directory path ends with separator
    if (isdir && path.Last() != wxFILE_SEP_PATH) path += wxFILE_SEP_PATH;
}

// -----------------------------------------------------------------------------

void SaveRelPath(FILE* f, const char* name, wxString path)
{
    // if given path is inside Ready directory then save as a relative path
    if (path.StartsWith(readydir)) {
        // remove readydir from start of path
        path.erase(0, readydir.length());
    }
    fprintf(f, "%s=%s\n", name, (const char*)path.mb_str(wxConvLocal));
}

// -----------------------------------------------------------------------------

void SavePrefs()
{
    FILE* f = fopen((const char*)prefspath.mb_str(wxConvLocal), "w");
    if (f == NULL) {
        Warning(_("Could not save preferences file:\n") + prefspath);
        return;
    }

    fprintf(f, "# NOTE: If you edit this file then do so when Ready isn't running\n");
    fprintf(f, "# otherwise all your changes will be clobbered when Ready quits.\n\n");
    fprintf(f, "prefs_version=%d\n", PREFS_VERSION);
    fprintf(f, "ready_version=%s\n", STR(READY_VERSION));
    wxString wxversion = wxVERSION_STRING;
    fprintf(f, "wx_version=%s\n", (const char*)wxversion.mb_str(wxConvLocal));
#if defined(__WXMAC__)
    fprintf(f, "platform=Mac\n");
#elif defined(__WXMSW__)
    fprintf(f, "platform=Windows\n");
#elif defined(__WXGTK__)
    fprintf(f, "platform=Linux\n");
#else
    fprintf(f, "platform=unknown\n");
#endif
    fprintf(f, "debug_level=%d\n", debuglevel);

    // save main window's location and size
    MyFrame* frameptr = wxGetApp().currframe;
    #ifdef __WXMSW__
    if (frameptr->IsFullScreen() || frameptr->IsIconized()) {
        // use mainx, mainy, mainwd, mainht set by frameptr->OnFullScreen()
        // or by frameptr->OnSize
    #else
    if (frameptr->IsFullScreen()) {
        // use mainx, mainy, mainwd, mainht set by frameptr->OnFullScreen()
    #endif
    } else {
        wxRect r = frameptr->GetRect();
        mainx = r.x;
        mainy = r.y;
        mainwd = r.width;
        mainht = r.height;
    }
    fprintf(f, "main_window=%d,%d,%d,%d\n", mainx, mainy, mainwd, mainht);
    fprintf(f, "maximize=%d\n", frameptr->IsMaximized() ? 1 : 0);

    fprintf(f, "aui_layout=%s\n", (const char*)auilayout.mb_str(wxConvLocal));
    fprintf(f, "opencl_platform=%d\n", opencl_platform);
    fprintf(f, "opencl_device=%d\n", opencl_device);

    SaveKeyActions(f);

    fprintf(f, "info_font_size=%d (%d..%d)\n", infofontsize, minfontsize, maxfontsize);
    fprintf(f, "help_font_size=%d (%d..%d)\n", helpfontsize, minfontsize, maxfontsize);
    fprintf(f, "text_dlg_wd=%d\n", textdlgwd);
    fprintf(f, "text_dlg_ht=%d\n", textdlght);
    fprintf(f, "show_tips=%d\n", showtips ? 1 : 0);
    fprintf(f, "repaint_to_erase=%d\n", repaint_to_erase ? 1 : 0);
    fprintf(f, "current_brush_size=%d\n", current_brush_size);
    fprintf(f, "allow_beep=%d\n", allowbeep ? 1 : 0);
    fprintf(f, "ask_on_new=%d\n", askonnew ? 1 : 0);
    fprintf(f, "ask_on_load=%d\n", askonload ? 1 : 0);
    fprintf(f, "ask_on_quit=%d\n", askonquit ? 1 : 0);

    fputs("\n", f);

    SaveRelPath(f, "open_save_dir", opensavedir);
    SaveRelPath(f, "screenshot_dir", screenshotdir);
    SaveRelPath(f, "choose_dir", choosedir);
    SaveRelPath(f, "user_dir", userdir);
    SaveRelPath(f, "recording_dir", recordingdir);

    fputs("\n", f);

    fprintf(f, "text_editor=%s\n", (const char*)texteditor.mb_str(wxConvLocal));
    fprintf(f, "max_patterns=%d (1..%d)\n", maxpatterns, ID::MAX_RECENT);

    if (numpatterns > 0) {
        fputs("\n", f);
        int i;
        for (i = 0; i < numpatterns; i++) {
            wxMenuItem* item = patternSubMenu->FindItemByPosition(i);
            if (item) {
#if wxCHECK_VERSION(2,9,0)
                wxString path = item->GetItemLabel();
#else
                wxString path = item->GetText();
#endif
#ifdef __WXGTK__
                // remove duplicate underscores
                path.Replace(wxT("__"), wxT("_"));
#endif
                // remove duplicate ampersands
                path.Replace(wxT("&&"), wxT("&"));
                fprintf(f, "recent_pattern=%s\n", (const char*)path.mb_str(wxConvLocal));
            }
        }
    }

    fclose(f);
}

// -----------------------------------------------------------------------------

// the linereader class handles all line endings (CR, CR+LF, LF)
// and terminates a line buffer with \0

const int LF = 10;
const int CR = 13;

class linereader {
    public:
        linereader(FILE* f) { setfile(f); }
        ~linereader() { if (closeonfree) close(); }
        char *fgets(char* buf, int maxlen);
        void setfile(FILE* f);
        void setcloseonfree() { closeonfree = 1; }
        int close();
    private:
        int lastchar;
        int closeonfree;
        FILE* fp;
};

void linereader::setfile(FILE* f) {
    fp = f;
    lastchar = 0;
    closeonfree = 0;
}

int linereader::close() {
     if (fp) {
        int ret = fclose(fp);
        fp = 0;
        return ret;
    }
    return 0;
}

char* linereader::fgets(char* buf, int maxlen)
{
    int i = 0;
    for (;;) {
        if (i+1 >= maxlen) {
            buf[i] = 0;
            return buf;
        }
        int c = getc(fp);
        switch (c) {
        case EOF:
            if (i == 0) return 0;
            buf[i] = 0;
            return buf;
        case LF:
            if (lastchar != CR) {
                lastchar = LF;
                buf[i] = 0;
                return buf;
            }
            break;
        case CR:
            lastchar = CR;
            buf[i] = 0;
            return buf;
        default:
            lastchar = c;
            buf[i++] = (char)c;
            break;
        }
    }
}

// -----------------------------------------------------------------------------

bool GetKeywordAndValue(linereader& lr, char* line, char** keyword, char** value)
{
    while ( lr.fgets(line, PREF_LINE_SIZE) != 0 ) {
        if ( line[0] == '#' || line[0] == 0 ) {
            // skip comment line or empty line
        }
        else {
            // line should have format keyword=value
            *keyword = line;
            *value = line;
            while ( **value != '=' && **value != 0 ) *value += 1;
            **value = 0;         // terminate keyword
            *value += 1;
            return true;
        }
    }
    return false;
}

// -----------------------------------------------------------------------------

void CheckVisibility(int* x, int* y, int* wd, int* ht)
{
    wxRect maxrect = wxGetClientDisplayRect();
    // reset x,y if title bar isn't clearly visible
    if ( *y + 10 < maxrect.y || *y + 10 > maxrect.GetBottom() ||
         *x + 10 > maxrect.GetRight() || *x + *wd - 10 < maxrect.x ) {
        *x = wxDefaultCoord;
        *y = wxDefaultCoord;
    }
    // reduce wd,ht if too big for screen
    if (*wd > maxrect.width) *wd = maxrect.width;
    if (*ht > maxrect.height) *ht = maxrect.height;
}

// -----------------------------------------------------------------------------

void InitPaths()
{
#ifdef __WXGTK__
    // on Linux we want datadir to be "~/.ready" rather than "~/.Ready"
    wxGetApp().SetAppName(_("ready"));
#endif

    // init datadir and create the directory if it doesn't exist;
    // the directory will probably be:
    // Win: C:\Documents and Settings\username\Application Data\Ready
    // Mac: ~/Library/Application Support/Ready
    // Unix: ~/.ready
    datadir = wxStandardPaths::Get().GetUserDataDir();
    if ( !wxFileName::DirExists(datadir) ) {
        if ( !wxFileName::Mkdir(datadir, 0777, wxPATH_MKDIR_FULL) ) {
            Warning(_("Could not create a user-specific data directory!\nWill try to use the application directory instead."));
            datadir = readydir;
        }
    }
    if (datadir.Last() != wxFILE_SEP_PATH) datadir += wxFILE_SEP_PATH;

    // init tempdir to a temporary directory unique to this process
    tempdir = wxFileName::CreateTempFileName(wxT("ready_"));
    // on Linux the file is in /tmp;
    // on my Mac the file is in /private/var/tmp/folders.502/TemporaryItems;
    // on WinXP the file is in C:\Documents and Settings\Andy\Local Settings\Temp
    // (or shorter equivalent C:\DOCUME~1\Andy\LOCALS~1\Temp) but the file name
    // is rea*.tmp (ie. only 1st 3 chars of the prefix are used, and .tmp is added)
    wxRemoveFile(tempdir);
    if ( !wxFileName::Mkdir(tempdir, 0777, wxPATH_MKDIR_FULL) ) {
        Warning(_("Could not create temporary directory:\n") + tempdir);
        // use standard directory for temp files
        tempdir = wxStandardPaths::Get().GetTempDir();
        if ( !wxFileName::DirExists(tempdir) ) {
            // should never happen, but play safe
            Fatal(_("Sorry, temporary directory does not exist:\n") + tempdir);
        }
    }
    if (tempdir.Last() != wxFILE_SEP_PATH) tempdir += wxFILE_SEP_PATH;

#ifdef __WXGTK__
    // "Ready" is nicer for warning dialogs etc
    wxGetApp().SetAppName(_("Ready"));
#endif

    // init prefspath -- look in readydir first, then in datadir
    prefspath = readydir + PREFS_NAME;
    if ( !wxFileExists(prefspath) ) {
        prefspath = datadir + PREFS_NAME;
    }

    patterndir = readydir + PATT_DIR;

    recordingdir = _T(".");
}

// -----------------------------------------------------------------------------

void GetPrefs()
{
    bool sawkeyaction = false;   // saw at least one key_action entry?

    InitPaths();                 // init datadir, tempdir, prefspath, patterndir

    opensavedir = readydir + PATT_DIR;
    screenshotdir = readydir + PATT_DIR;
    choosedir = readydir;
    userdir = wxT(":\\/:");     // better if initially an illegal dir name

    // init the text editor to something reasonable
#ifdef __WXMSW__
    texteditor = wxT("Notepad");
#elif defined(__WXMAC__)
    texteditor = wxT("/Applications/TextEdit.app");
#else
    // assume Linux
    // don't attempt to guess which editor might be available;
    // set the string empty so the user is asked to choose their
    // preferred editor the first time texteditor is used
    texteditor = wxEmptyString;
#endif

    // initialize Open Recent submenu
    patternSubMenu = new wxMenu();
    patternSubMenu->AppendSeparator();
    patternSubMenu->Append(ID::ClearMissingPatterns, _("Clear Missing Files"));
    patternSubMenu->Append(ID::ClearAllPatterns, _("Clear All Files"));

    if ( !wxFileExists(prefspath) ) {
        AddDefaultKeyActions();
        UpdateAcceleratorStrings();
        return;
    }

    FILE* f = fopen((const char*)prefspath.mb_str(wxConvLocal), "r");
    if (f == NULL) {
        Warning(_("Could not read preferences file!"));
        return;
    }

    linereader reader(f);
    char line[PREF_LINE_SIZE];
    char* keyword;
    char* value;
    while ( GetKeywordAndValue(reader, line, &keyword, &value) ) {

        if (strcmp(keyword, "prefs_version") == 0) {
            sscanf(value, "%d", &currversion);
            /* should we prevent older Ready clobbering newer Ready's prefs???
            if (currversion > PREFS_VERSION) {
               Fatal(...);
            }
            */

        } else if (strcmp(keyword, "debug_level") == 0) {
            sscanf(value, "%d", &debuglevel);

        } else if (strcmp(keyword, "main_window") == 0) {
            sscanf(value, "%d,%d,%d,%d", &mainx, &mainy, &mainwd, &mainht);
            // avoid very small window
            if (mainwd < minmainwd) mainwd = minmainwd;
            if (mainht < minmainht) mainht = minmainht;
            CheckVisibility(&mainx, &mainy, &mainwd, &mainht);

        } else if (strcmp(keyword, "maximize") == 0) {
            maximize = value[0] == '1';

        } else if (strcmp(keyword, "aui_layout") == 0) {
            auilayout = wxString(value,wxConvLocal);

        } else if (strcmp(keyword, "opencl_platform") == 0) {
            sscanf(value, "%d", &opencl_platform);

        } else if (strcmp(keyword, "opencl_device") == 0) {
            sscanf(value, "%d", &opencl_device);

        } else if (strcmp(keyword, "key_action") == 0) {
            GetKeyAction(value);
            sawkeyaction = true;

        } else if (strcmp(keyword, "info_font_size") == 0) {
            sscanf(value, "%d", &infofontsize);
            if (infofontsize < minfontsize) infofontsize = minfontsize;
            if (infofontsize > maxfontsize) infofontsize = maxfontsize;

        } else if (strcmp(keyword, "help_font_size") == 0) {
            sscanf(value, "%d", &helpfontsize);
            if (helpfontsize < minfontsize) helpfontsize = minfontsize;
            if (helpfontsize > maxfontsize) helpfontsize = maxfontsize;

        } else if (strcmp(keyword, "text_dlg_wd") == 0) {
            sscanf(value, "%d", &textdlgwd);
            if (textdlgwd < 200) textdlgwd = 200;

        } else if (strcmp(keyword, "text_dlg_ht") == 0) {
            sscanf(value, "%d", &textdlght);
            if (textdlght < 100) textdlght = 100;

        } else if (strcmp(keyword, "show_tips") == 0)   { showtips = value[0] == '1';
        } else if (strcmp(keyword, "repaint_to_erase") == 0)  { repaint_to_erase = value[0] == '1';
        } else if (strcmp(keyword, "current_brush_size") == 0) {
            sscanf(value, "%d", &current_brush_size);
            current_brush_size = std::min(2,std::max(0,current_brush_size));
        } else if (strcmp(keyword, "allow_beep") == 0)  { allowbeep = value[0] == '1';
        } else if (strcmp(keyword, "ask_on_new") == 0)  { askonnew = value[0] == '1';
        } else if (strcmp(keyword, "ask_on_load") == 0) { askonload = value[0] == '1';
        } else if (strcmp(keyword, "ask_on_quit") == 0) { askonquit = value[0] == '1';

        } else if (strcmp(keyword, "open_save_dir") == 0)  { GetRelPath(value, opensavedir, PATT_DIR);
        } else if (strcmp(keyword, "screenshot_dir") == 0) { GetRelPath(value, screenshotdir, PATT_DIR);
        } else if (strcmp(keyword, "choose_dir") == 0)     { GetRelPath(value, choosedir);
        } else if (strcmp(keyword, "user_dir") == 0)       { GetRelPath(value, userdir);
        } else if (strcmp(keyword, "recording_dir") == 0)  { GetRelPath(value, recordingdir);

        } else if (strcmp(keyword, "text_editor") == 0) {
            texteditor = wxString(value,wxConvLocal);

        } else if (strcmp(keyword, "max_patterns") == 0) {
            sscanf(value, "%d", &maxpatterns);
            if (maxpatterns < 1) maxpatterns = 1;
            if (maxpatterns > ID::MAX_RECENT) maxpatterns = ID::MAX_RECENT;

        } else if (strcmp(keyword, "recent_pattern") == 0) {
            // append path to Open Recent submenu
            if (numpatterns < maxpatterns && value[0]) {
                numpatterns++;
                wxString path(value, wxConvLocal);
                // duplicate ampersands so they appear in menu
                path.Replace(wxT("&"), wxT("&&"));
                patternSubMenu->Insert(numpatterns - 1, ID::OpenRecent + numpatterns, path);
            }
        }
    }

    reader.close();

    // if no key_action entries then use default shortcuts
    if (!sawkeyaction) AddDefaultKeyActions();

    // initialize accelerator array
    UpdateAcceleratorStrings();
}

// -----------------------------------------------------------------------------

// Preferences dialog:

#if defined(__WXMAC__) && wxCHECK_VERSION(2,7,2)
// fix wxMac 2.7.2+ bug in wxTextCtrl::SetSelection
#define ALL_TEXT 0,999
#else
#define ALL_TEXT -1,-1
#endif

#if defined(__WXMAC__) && wxCHECK_VERSION(2,8,0) && !wxCHECK_VERSION(2,9,0)
// fix wxALIGN_CENTER_VERTICAL bug in wxMac 2.8.x;
// eg. when a wxStaticText box is next to a wxChoice box
#define FIX_ALIGN_BUG wxBOTTOM,4
#else
#define FIX_ALIGN_BUG wxALL,0
#endif

static size_t currpage = 0;      // current page in PrefsDialog

// these are global so we can remember current key combination
static int currkey = ' ';
static int currmods = mk_ALT + mk_SHIFT + mk_CMD;

enum
{
    // these *_PAGE values must correspond to currpage values
    FILE_PAGE = 0,
    EDIT_PAGE,
    VIEW_PAGE,
    ACTION_PAGE,
    KEYBOARD_PAGE
};

enum
{
    // File prefs
    PREF_MAX_PATTERNS = wxID_HIGHEST + 1,
    PREF_EDITOR_BUTT,
    PREF_EDITOR_BOX,
    #ifdef __WXOSX__
        PREF_HIDDEN,    // needed to fix wxOSX bug
    #endif
    // Edit prefs
    PREF_ERASE,
    PREF_BEEP,
    // View prefs
    PREF_SHOW_TIPS,
    // Action prefs
    // none as yet
    // Keyboard prefs
    PREF_KEYCOMBO,
    PREF_ACTION,
    PREF_CHOOSE,
    PREF_FILE_BOX
};

// define a multi-page dialog for changing various preferences

class PrefsDialog : public wxPropertySheetDialog
{
    public:
        PrefsDialog(MyFrame* parent, const wxString& page);
        ~PrefsDialog() { delete onetimer; }

        wxPanel* CreateFilePrefs(wxWindow* parent);
        wxPanel* CreateEditPrefs(wxWindow* parent);
        wxPanel* CreateViewPrefs(wxWindow* parent);
        wxPanel* CreateActionPrefs(wxWindow* parent);
        wxPanel* CreateKeyboardPrefs(wxWindow* parent);

        // called when user hits OK
        virtual bool TransferDataFromWindow();

        #ifdef __WXMAC__
            void OnSpinCtrlChar(wxKeyEvent& event);
        #endif

        static void UpdateChosenFile();

    private:
        bool GetCheckVal(long id);
        int GetChoiceVal(long id);
        int GetSpinVal(long id);
        int GetRadioVal(long firstid, int numbuttons);
        bool BadSpinVal(int id, int minval, int maxval, const wxString& prefix);
        bool ValidatePage();

        void OnCheckBoxClicked(wxCommandEvent& event);
        void OnPageChanging(wxNotebookEvent& event);
        void OnPageChanged(wxNotebookEvent& event);
        void OnChoice(wxCommandEvent& event);
        void OnButton(wxCommandEvent& event);
        void OnOneTimer(wxTimerEvent& event);

        bool ignore_page_event;  // used to prevent currpage being changed

        wxString neweditor;      // new text editor
        wxString newdownloaddir; // new directory for downloaded files
        wxString newuserrules;   // new directory for user's rules

        wxTimer* onetimer;       // one shot timer (see OnOneTimer)

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(PrefsDialog, wxPropertySheetDialog)
EVT_CHECKBOX               (wxID_ANY, PrefsDialog::OnCheckBoxClicked)
EVT_NOTEBOOK_PAGE_CHANGING (wxID_ANY, PrefsDialog::OnPageChanging)
EVT_NOTEBOOK_PAGE_CHANGED  (wxID_ANY, PrefsDialog::OnPageChanged)
EVT_CHOICE                 (wxID_ANY, PrefsDialog::OnChoice)
EVT_BUTTON                 (wxID_ANY, PrefsDialog::OnButton)
EVT_TIMER                  (wxID_ANY, PrefsDialog::OnOneTimer)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

// define a text control for showing current key combination

class KeyComboCtrl : public wxTextCtrl
{
    public:
        KeyComboCtrl(wxWindow* parent, wxWindowID id, const wxString& value,
            const wxPoint& pos, const wxSize& size, int style = 0)
            : wxTextCtrl(parent, id, value, pos, size, style) {}
        ~KeyComboCtrl() {}

        // handlers to intercept keyboard events
        void OnKeyDown(wxKeyEvent& event);
        void OnChar(wxKeyEvent& event);

    private:
        int realkey;             // key code set by OnKeyDown
        wxString debugkey;       // display debug info for OnKeyDown and OnChar

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(KeyComboCtrl, wxTextCtrl)
EVT_KEY_DOWN  (KeyComboCtrl::OnKeyDown)
EVT_CHAR      (KeyComboCtrl::OnChar)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

void KeyComboCtrl::OnKeyDown(wxKeyEvent& event)
{
    realkey = event.GetKeyCode();
    int mods = event.GetModifiers();

    if (debuglevel == 1) {
        // set debugkey now but don't show it until OnChar
        debugkey = wxString::Format(_("OnKeyDown: key=%d (%c) mods=%d"),
            realkey, realkey < 128 ? wxChar(realkey) : wxChar('?'), mods);
    }

    if (realkey == WXK_ESCAPE) {
        // escape key is reserved for other uses
        Beep();
        return;
    }

#ifdef __WXOSX__
    // pass arrow key or function key or delete key directly to OnChar
    if ( (realkey >=  WXK_LEFT && realkey <= WXK_DOWN) ||
    (realkey >= WXK_F1 && realkey <= WXK_F24) || realkey == WXK_BACK ) {
        OnChar(event);
        return;
    }
#endif

    // WARNING: logic must match that in MyFrame::OnKeyDown
    if (mods == wxMOD_NONE || realkey > 127) {
        // tell OnChar handler to ignore realkey
        realkey = 0;
    }

#ifdef __WXOSX__
    // pass ctrl/cmd-key combos directly to OnChar
    if (realkey > 0 && ((mods & wxMOD_CONTROL) || (mods & wxMOD_CMD))) {
        OnChar(event);
        return;
    }
#endif

#ifdef __WXMAC__
    // avoid translating option-E/I/N/U/`
    if (mods == wxMOD_ALT && (realkey == 'E' || realkey == 'I' || realkey == 'N' ||
                              realkey == 'U' || realkey == '`')) {
        OnChar(event);
        return;
    }
#endif

#ifdef __WXMSW__
    // on Windows, OnChar is NOT called for some ctrl-key combos like
    // ctrl-0..9 or ctrl-alt-key, so we call OnChar ourselves
    if (realkey > 0 && (mods & wxMOD_CONTROL)) {
        OnChar(event);
        return;
    }
#endif

    event.Skip();
}

// -----------------------------------------------------------------------------

#ifdef __WXOSX__
    static bool inonchar = false;
#endif

void KeyComboCtrl::OnChar(wxKeyEvent& event)
{
    #ifdef __WXOSX__
        // avoid infinite recursion in wxOSX due to ChangeValue call below
        if (inonchar) { event.Skip(); return; }
        inonchar = true;
    #endif

    int key = event.GetKeyCode();
    int mods = event.GetModifiers();

    if (debuglevel == 1) {
        debugkey += wxString::Format(_("\nOnChar: key=%d (%c) mods=%d"),
            key, key < 128 ? wxChar(key) : wxChar('?'), mods);
        Warning(debugkey);
    }

    // WARNING: logic must match that in MyFrame::OnChar
    if (realkey > 0 && mods != wxMOD_NONE) {
        #ifdef __WXGTK__
            // sigh... wxGTK returns inconsistent results for shift-comma combos
            // so we assume that '<' is produced by pressing shift-comma
            // (which might only be true for US keyboards)
            if (key == '<' && (mods & wxMOD_SHIFT)) realkey = ',';
        #endif
        #ifdef __WXMSW__
            // sigh... wxMSW returns inconsistent results for some shift-key combos
            // so again we assume we're using a US keyboard
            if (key == '~' && (mods & wxMOD_SHIFT)) realkey = '`';
            if (key == '+' && (mods & wxMOD_SHIFT)) realkey = '=';
        #endif
        if (mods == wxMOD_SHIFT && key != realkey) {
            // use translated key code but remove shift key;
            // eg. we want shift-'/' to be seen as '?'
            mods = wxMOD_NONE;
        } else {
            // use key code seen by OnKeyDown
            key = realkey;
            if (key >= 'A' && key <= 'Z') key += 32;   // convert A..Z to a..z
        }
    }

    // convert wx key and mods to our internal key code and modifiers
    // and, if they are valid, display the key combo and update the action
    if ( ConvertKeyAndModifiers(key, mods, &currkey, &currmods) ) {
        wxChoice* actionmenu = (wxChoice*) FindWindowById(PREF_ACTION);
        if (actionmenu) {
            wxString keystring = GetKeyCombo(currkey, currmods);
            if (!keystring.IsEmpty()) {
                ChangeValue(keystring);
            }
            else {
                currkey = 0;
                currmods = 0;
                ChangeValue(_("UNKNOWN KEY"));
            }
            actionmenu->SetSelection(keyaction[currkey][currmods].id);
            PrefsDialog::UpdateChosenFile();
            SetFocus();
            SetSelection(ALL_TEXT);
        }
        else {
            Warning(_("Failed to find wxChoice control!"));
        }
    }
    else {
        // unsupported key combo
        Beep();
    }

    // do NOT pass event on to next handler
    // event.Skip();

    #ifdef __WXOSX__
        inonchar = false;
    #endif
}

// -----------------------------------------------------------------------------

#ifdef __WXMAC__

// override key event handler for wxSpinCtrl to allow key checking
// and to get tab key navigation to work correctly
class MySpinCtrl : public wxSpinCtrl
{
    public:
        MySpinCtrl(wxWindow* parent, wxWindowID id, const wxString& str,
            const wxPoint& pos, const wxSize& size)
        : wxSpinCtrl(parent, id, str, pos, size) {
            // create a dynamic event handler for the underlying wxTextCtrl
            wxTextCtrl* textctrl = GetText();
            if (textctrl) {
                textctrl->Connect(wxID_ANY, wxEVT_CHAR,
                    wxKeyEventHandler(PrefsDialog::OnSpinCtrlChar));
            }
        }
};

void PrefsDialog::OnSpinCtrlChar(wxKeyEvent& event)
{
    int key = event.GetKeyCode();

    if (event.CmdDown()) {
        // allow handling of cmd-x/v/etc
        event.Skip();

    }
    else if ( key == WXK_TAB ) {
        // note that FindFocus() returns pointer to wxTextCtrl window in wxSpinCtrl
        if ( currpage == FILE_PAGE ) {
            // only one spin ctrl on this page
            wxSpinCtrl* s1 = (wxSpinCtrl*) FindWindowById(PREF_MAX_PATTERNS);
            if ( s1 ) { s1->SetFocus(); s1->SetSelection(ALL_TEXT); }
        }
        else if ( currpage == EDIT_PAGE ) {
            // no spin ctrls on this page
        }
        else if ( currpage == VIEW_PAGE ) {
            // no spin ctrls on this page
        }
        else if ( currpage == ACTION_PAGE ) {
            // no spin ctrls on this page
        }
        else if ( currpage == KEYBOARD_PAGE ) {
            // no spin ctrls on this page
        }

    }
    else if ( key >= ' ' && key <= '~' ) {
        if ( key >= '0' && key <= '9' ) {
            // allow digits
            event.Skip();
        }
        else {
            // disallow any other displayable ascii char
            Beep();
        }

    }
    else {
        event.Skip();
    }
}


#else

#define MySpinCtrl wxSpinCtrl

#endif      // !__WXMAC__

// -----------------------------------------------------------------------------

PrefsDialog::PrefsDialog(MyFrame* parent, const wxString& page)
{
    // not using validators so no need for this:
    // SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);

    Create(parent, wxID_ANY, _("Preferences"));
    CreateButtons(wxOK | wxCANCEL);

    wxBookCtrlBase* notebook = GetBookCtrl();

    wxPanel* filePrefs = CreateFilePrefs(notebook);
    wxPanel* editPrefs = CreateEditPrefs(notebook);
    wxPanel* viewPrefs = CreateViewPrefs(notebook);
    wxPanel* actionPrefs = CreateActionPrefs(notebook);
    wxPanel* keyboardPrefs = CreateKeyboardPrefs(notebook);

    // AddPage and SetSelection cause OnPageChanging and OnPageChanged to be called
    // so we use a flag to prevent currpage being changed (and unnecessary validation)
    ignore_page_event = true;

    notebook->AddPage(filePrefs, _("File"));
    notebook->AddPage(editPrefs, _("Edit"));
    notebook->AddPage(viewPrefs, _("View"));
    notebook->AddPage(actionPrefs, _("Action"));
    notebook->AddPage(keyboardPrefs, _("Keyboard"));

    if (!page.IsEmpty()) {
        if (page == wxT("file"))            currpage = FILE_PAGE;
        else if (page == wxT("edit"))       currpage = EDIT_PAGE;
        else if (page == wxT("view"))       currpage = VIEW_PAGE;
        else if (page == wxT("action"))     currpage = ACTION_PAGE;
        else if (page == wxT("keyboard"))   currpage = KEYBOARD_PAGE;
    }

    // show the desired page
    notebook->SetSelection(currpage);

    ignore_page_event = false;

    LayoutDialog();

    // ensure top text box has focus and text is selected by creating
    // a one-shot timer which will call OnOneTimer after short delay
    onetimer = new wxTimer(this, wxID_ANY);
    if (onetimer) onetimer->Start(10, wxTIMER_ONE_SHOT);
}

// -----------------------------------------------------------------------------

void PrefsDialog::OnOneTimer(wxTimerEvent& WXUNUSED(event))
{
    MySpinCtrl* s1 = NULL;
    MySpinCtrl* s2 = NULL;

    if (currpage == FILE_PAGE) {
        s1 = (MySpinCtrl*) FindWindowById(PREF_MAX_PATTERNS);
        #ifdef __WXOSX__
            // have to set s2 to hidden spin ctrl... sigh
            s2 = (MySpinCtrl*) FindWindowById(PREF_HIDDEN);
        #else
            s2 = s1;
        #endif
    }
    else if (currpage == KEYBOARD_PAGE) {
        KeyComboCtrl* k = (KeyComboCtrl*) FindWindowById(PREF_KEYCOMBO);
        if (k) {
            // don't need to change focus to some other control if wxOSX
            k->SetFocus();
            k->SetSelection(ALL_TEXT);
        }
        return;
    }

    if (s1 && s2) {
#ifdef __WXOSX__
        // first need to change focus to some other control
        s2->SetFocus();
#endif
        s1->SetFocus();
        s1->SetSelection(ALL_TEXT);
    }
}

// -----------------------------------------------------------------------------

// these consts are used to get nicely spaced controls on each platform:

#ifdef __WXMAC__
#define GROUPGAP (12)            // vertical gap between a group of controls
#define SBTOPGAP (2)             // vertical gap before first item in wxStaticBoxSizer
#define SBBOTGAP (2)             // vertical gap after last item in wxStaticBoxSizer
#define SVGAP (4)                // vertical gap above wxSpinCtrl box
#define S2VGAP (0)               // vertical gap between 2 wxSpinCtrl boxes
#define CH2VGAP (6)              // vertical gap between 2 check/radio boxes
#define CVGAP (9)                // vertical gap above wxChoice box
#define LRGAP (5)                // space left and right of vertically stacked boxes
#define SPINGAP (3)              // horizontal gap around each wxSpinCtrl box
#define CHOICEGAP (6)            // horizontal gap to left of wxChoice box
#elif defined(__WXMSW__)
#define GROUPGAP (10)
#define SBTOPGAP (7)
#define SBBOTGAP (7)
#define SVGAP (7)
#define S2VGAP (5)
#define CH2VGAP (8)
#define CVGAP (7)
#define LRGAP (5)
#define SPINGAP (6)
#define CHOICEGAP (6)
#else                            // assume Linux
#define GROUPGAP (10)
#define SBTOPGAP (12)
#define SBBOTGAP (7)
#define SVGAP (7)
#define S2VGAP (5)
#define CH2VGAP (8)
#define CVGAP (7)
#define LRGAP (5)
#define SPINGAP (6)
#define CHOICEGAP (6)
#endif

// -----------------------------------------------------------------------------

wxPanel* PrefsDialog::CreateFilePrefs(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    #ifdef __WXOSX__
        // create hidden spin ctrl for use in OnOneTimer
        wxSpinCtrl* hidden = new MySpinCtrl(panel, PREF_HIDDEN, wxEmptyString,
            wxPoint(-1000,-1000), wxSize(70, wxDefaultCoord));
        hidden->SetValue(666);
    #endif

    // max_patterns and max_scripts

    wxBoxSizer* maxbox = new wxBoxSizer(wxHORIZONTAL);
    maxbox->Add(new wxStaticText(panel, wxID_STATIC, _("Maximum number of recent patterns:")),
        0, wxALL, 0);

    wxSpinCtrl* spin1 = new MySpinCtrl(panel, PREF_MAX_PATTERNS, wxEmptyString,
        wxDefaultPosition, wxSize(70, wxDefaultCoord));

    wxBoxSizer* hpbox = new wxBoxSizer(wxHORIZONTAL);
    hpbox->Add(maxbox, 0, wxALIGN_CENTER_VERTICAL, 0);
    hpbox->Add(spin1, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, SPINGAP);

    wxButton* editorbutt = new wxButton(panel, PREF_EDITOR_BUTT, _("Text Editor..."));
    wxStaticText* editorbox = new wxStaticText(panel, PREF_EDITOR_BOX, texteditor);
    neweditor = texteditor;

    wxBoxSizer* hebox = new wxBoxSizer(wxHORIZONTAL);
    hebox->Add(editorbutt, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 0);
    hebox->Add(editorbox, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, LRGAP);

    // position things
    vbox->AddSpacer(5);
    vbox->Add(hpbox, 0, wxLEFT | wxRIGHT, LRGAP);
    vbox->AddSpacer(10);
    vbox->Add(hebox, 0, wxLEFT | wxRIGHT, LRGAP);
    vbox->AddSpacer(5);

    // init control values
    spin1->SetRange(1, ID::MAX_RECENT); spin1->SetValue(maxpatterns);
    spin1->SetFocus();
    spin1->SetSelection(ALL_TEXT);

    topSizer->Add(vbox, 1, wxGROW | wxALL, 5);
    panel->SetSizer(topSizer);
    topSizer->Fit(panel);
    return panel;
}

// -----------------------------------------------------------------------------

wxPanel* PrefsDialog::CreateEditPrefs(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    // repaint_to_erase

    wxCheckBox* erasecheck = new wxCheckBox(panel, PREF_ERASE,
        _("Pencil tool erases (to low value) if cell is redrawn with same value"));

    // allow_beep

    wxCheckBox* beepcheck = new wxCheckBox(panel, PREF_BEEP, _("Allow beep sound"));

    // position things
    vbox->AddSpacer(5);
    vbox->Add(erasecheck, 0, wxLEFT | wxRIGHT, LRGAP);
    vbox->AddSpacer(5);
    vbox->Add(beepcheck, 0, wxLEFT | wxRIGHT, LRGAP);

    // init control values
    erasecheck->SetValue(repaint_to_erase);
    beepcheck->SetValue(allowbeep);

    topSizer->Add(vbox, 1, wxGROW | wxALL, 5);
    panel->SetSizer(topSizer);
    topSizer->Fit(panel);
    return panel;
}

// -----------------------------------------------------------------------------

wxPanel* PrefsDialog::CreateViewPrefs(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    // show_tips

#if wxUSE_TOOLTIPS
    wxCheckBox* toolcheck = new wxCheckBox(panel, PREF_SHOW_TIPS, _("Show tool tips"));
#endif

    // position things
    vbox->AddSpacer(5);
#if wxUSE_TOOLTIPS
    vbox->Add(toolcheck, 0, wxLEFT | wxRIGHT, LRGAP);
#endif

    // init control values
#if wxUSE_TOOLTIPS
    toolcheck->SetValue(showtips);
#endif

    topSizer->Add(vbox, 1, wxGROW | wxALL, 5);
    panel->SetSizer(topSizer);
    topSizer->Fit(panel);
    return panel;
}

// -----------------------------------------------------------------------------

wxPanel* PrefsDialog::CreateActionPrefs(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    wxString note = _("No settings here as yet.");
    wxBoxSizer* notebox = new wxBoxSizer(wxHORIZONTAL);
    notebox->Add(new wxStaticText(panel, wxID_STATIC, note));

    // position things
    vbox->AddSpacer(5);
    vbox->Add(notebox, 0, wxALIGN_CENTER, LRGAP);

    topSizer->Add(vbox, 1, wxGROW | wxALL, 5);
    panel->SetSizer(topSizer);
    topSizer->Fit(panel);
    return panel;
}

// -----------------------------------------------------------------------------

wxPanel* PrefsDialog::CreateKeyboardPrefs(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    // make sure this is the first control added so it gets focus on a page change
    KeyComboCtrl* keycombo = new KeyComboCtrl(panel, PREF_KEYCOMBO, wxEmptyString,
        wxDefaultPosition, wxSize(230, wxDefaultCoord),
        wxTE_CENTER
        | wxTE_PROCESS_TAB
        | wxTE_PROCESS_ENTER     // so enter key won't select OK on Windows
    #ifdef __WXOSX__
        // avoid wxTE_RICH2 otherwise we see scroll bar
    #else
        // better for Windows
        | wxTE_RICH2
    #endif
    );

    wxArrayString actionChoices;
    for (int i = 0; i < MAX_ACTIONS; i++) {
        actionChoices.Add( wxString(GetActionName((action_id) i), wxConvLocal) );
    }
    actionChoices[DO_OPENFILE] = _("Open Chosen File");
    wxChoice* actionmenu = new wxChoice(panel, PREF_ACTION,
    wxDefaultPosition, wxDefaultSize, actionChoices);

    wxBoxSizer* hbox0 = new wxBoxSizer(wxHORIZONTAL);
    hbox0->Add(new wxStaticText(panel, wxID_STATIC,
                                _("Type a key combination, then select the desired action:")));

    wxBoxSizer* keybox = new wxBoxSizer(wxVERTICAL);
    keybox->Add(new wxStaticText(panel, wxID_STATIC, _("Key Combination")), 0, wxALIGN_CENTER, 0);
    keybox->AddSpacer(5);
    keybox->Add(keycombo, 0, wxALIGN_CENTER, 0);

    wxBoxSizer* actbox = new wxBoxSizer(wxVERTICAL);
    #if defined(__WXMAC__) && wxCHECK_VERSION(2,8,0) && !wxCHECK_VERSION(2,9,0)
        actbox->AddSpacer(2);
    #endif
    actbox->Add(new wxStaticText(panel, wxID_STATIC, _("Action")), 0, wxALIGN_CENTER, 0);
    actbox->AddSpacer(5);
    actbox->Add(actionmenu, 0, wxALIGN_CENTER, 0);

    wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(keybox, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, LRGAP);
    hbox1->AddSpacer(15);
    hbox1->Add(actbox, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, LRGAP);

    wxButton* choose = new wxButton(panel, PREF_CHOOSE, _("Choose File..."));
    wxStaticText* filebox = new wxStaticText(panel, PREF_FILE_BOX, wxEmptyString);

    wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(choose, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, LRGAP);
    hbox2->Add(filebox, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, LRGAP);

    wxBoxSizer* midbox = new wxBoxSizer(wxVERTICAL);
    midbox->Add(hbox1, 0, wxLEFT | wxRIGHT, LRGAP);
    midbox->AddSpacer(15);
    midbox->Add(hbox2, 0, wxLEFT, LRGAP);

    wxString notes = _("Note:");
    notes += _("\n- Different key combinations can be assigned to the same action.");
    notes += _("\n- The Escape key is reserved for hard-wired actions.");
    notes += _("\n- Click OK to save changes (the Return key can be assigned to an action).");
    wxBoxSizer* hbox3 = new wxBoxSizer(wxHORIZONTAL);
    hbox3->Add(new wxStaticText(panel, wxID_STATIC, notes));

    // position things
    vbox->AddSpacer(5);
    vbox->Add(hbox0, 0, wxLEFT, LRGAP);
    vbox->AddSpacer(15);
    vbox->Add(midbox, 0, wxALIGN_CENTER, 0);
    vbox->AddSpacer(30);
    vbox->Add(hbox3, 0, wxLEFT, LRGAP);

    // initialize controls
    keycombo->ChangeValue( GetKeyCombo(currkey, currmods) );
    actionmenu->SetSelection( keyaction[currkey][currmods].id );
    UpdateChosenFile();
    keycombo->SetFocus();
    keycombo->SetSelection(ALL_TEXT);

    topSizer->Add(vbox, 1, wxGROW | wxALL, 5);
    panel->SetSizer(topSizer);
    topSizer->Fit(panel);
    return panel;
}

// -----------------------------------------------------------------------------

void PrefsDialog::UpdateChosenFile()
{
    wxStaticText* filebox = (wxStaticText*) FindWindowById(PREF_FILE_BOX);
    if (filebox) {
        action_id action = keyaction[currkey][currmods].id;
        if (action == DO_OPENFILE) {
            // display current file name
            filebox->SetLabel(keyaction[currkey][currmods].file);
        }
        else {
            // clear file name; don't set keyaction[currkey][currmods].file empty
            // here because user might change their mind (TransferDataFromWindow
            // will eventually set the file empty)
            filebox->SetLabel(wxEmptyString);
        }
    }
}

// -----------------------------------------------------------------------------

void PrefsDialog::OnChoice(wxCommandEvent& event)
{
    int id = event.GetId();

    if ( id == PREF_ACTION ) {
        int i = event.GetSelection();
        if (i >= 0 && i < MAX_ACTIONS) {
            action_id action = (action_id) i;
            keyaction[currkey][currmods].id = action;
            if ( action == DO_OPENFILE && keyaction[currkey][currmods].file.IsEmpty() ) {
                // call OnButton (which will call UpdateChosenFile)
                wxCommandEvent buttevt(wxEVT_COMMAND_BUTTON_CLICKED, PREF_CHOOSE);
                OnButton(buttevt);
            } else {
                UpdateChosenFile();
            }
        }
    }
}

// -----------------------------------------------------------------------------

void ChooseTextEditor(wxWindow* parent, wxString& result)
{
    #ifdef __WXMSW__
        wxString filetypes = _("Applications (*.exe)|*.exe");
    #elif defined(__WXMAC__)
        wxString filetypes = _("Applications (*.app)|*.app");
    #else
        // assume Linux
        wxString filetypes = _("All files (*)|*");
    #endif

    wxFileDialog opendlg(parent, _("Choose a text editor"),
    wxEmptyString, wxEmptyString, filetypes,
    wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    #ifdef __WXMSW__
        opendlg.SetDirectory(_("C:\\Program Files"));
    #elif defined(__WXMAC__)
        opendlg.SetDirectory(_("/Applications"));
    #else
        // assume Linux
        opendlg.SetDirectory(_("/usr/bin"));
    #endif

    if ( opendlg.ShowModal() == wxID_OK ) {
        result = opendlg.GetPath();
    } else {
        result = wxEmptyString;
    }
}

// -----------------------------------------------------------------------------

void PrefsDialog::OnButton(wxCommandEvent& event)
{
    int id = event.GetId();

    if ( id == PREF_CHOOSE ) {
        // ask user to choose an appropriate file
        wxString filetypes = _("All files (*)|*");
        filetypes +=         _("|Pattern (*.vti)|*.vti");
        filetypes +=         _("|HTML (*.html;*.htm)|*.html;*.htm");

        wxFileDialog opendlg(this, _("Choose a pattern file or HTML file"),
                             choosedir, wxEmptyString, filetypes,
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    #ifdef __WXGTK__
        // choosedir is ignored above (bug in wxGTK 2.8.0???)
        opendlg.SetDirectory(choosedir);
    #endif
        if ( opendlg.ShowModal() == wxID_OK ) {
            wxFileName fullpath( opendlg.GetPath() );
            choosedir = fullpath.GetPath();
            wxString path = opendlg.GetPath();
            if (path.StartsWith(readydir)) {
                // remove readydir from start of path
                path.erase(0, readydir.length());
            }
            keyaction[currkey][currmods].file = path;
            keyaction[currkey][currmods].id = DO_OPENFILE;
            wxChoice* actionmenu = (wxChoice*) FindWindowById(PREF_ACTION);
            if (actionmenu) {
                actionmenu->SetSelection(DO_OPENFILE);
            }
        }

        UpdateChosenFile();

    }
    else if ( id == PREF_EDITOR_BUTT ) {
        // ask user to choose a text editor
        wxString result;
        ChooseTextEditor(this, result);
        if ( !result.IsEmpty() ) {
            neweditor = result;
            wxStaticText* editorbox = (wxStaticText*) FindWindowById(PREF_EDITOR_BOX);
            if (editorbox) {
                editorbox->SetLabel(neweditor);
            }
        }

    }

    event.Skip();   // need this so other buttons work correctly
}

// -----------------------------------------------------------------------------

void PrefsDialog::OnCheckBoxClicked(wxCommandEvent& event)
{
    //int id = event.GetId();

    // no need???
}

// -----------------------------------------------------------------------------

bool PrefsDialog::GetCheckVal(long id)
{
    wxCheckBox* checkbox = (wxCheckBox*) FindWindow(id);
    if (checkbox) {
        return checkbox->GetValue();
    } else {
        Warning(_("Bug in GetCheckVal!"));
        return false;
    }
}

// -----------------------------------------------------------------------------

int PrefsDialog::GetChoiceVal(long id)
{
    wxChoice* choice = (wxChoice*) FindWindow(id);
    if (choice) {
        return choice->GetSelection();
    } else {
        Warning(_("Bug in GetChoiceVal!"));
        return 0;
    }
}

// -----------------------------------------------------------------------------

int PrefsDialog::GetRadioVal(long firstid, int numbuttons)
{
    for (int i = 0; i < numbuttons; i++) {
        wxRadioButton* radio = (wxRadioButton*) FindWindow(firstid + i);
        if (radio->GetValue()) return i;
    }
    Warning(_("Bug in GetRadioVal!"));
    return 0;
}

// -----------------------------------------------------------------------------

int PrefsDialog::GetSpinVal(long id)
{
    wxSpinCtrl* spinctrl = (wxSpinCtrl*) FindWindow(id);
    if (spinctrl) {
        return spinctrl->GetValue();
    } else {
        Warning(_("Bug in GetSpinVal!"));
        return 0;
    }
}

// -----------------------------------------------------------------------------

bool PrefsDialog::BadSpinVal(int id, int minval, int maxval, const wxString& prefix)
{
    wxSpinCtrl* spinctrl = (wxSpinCtrl*) FindWindow(id);
    // spinctrl->GetValue() always returns a value within range even if
    // the text ctrl doesn't contain a valid number -- yuk!
    int i = spinctrl->GetValue();
    if (i < minval || i > maxval) {
        wxString msg;
        msg.Printf(_("%s must be from %d to %d."), prefix.c_str(), minval, maxval);
        Warning(msg);
        spinctrl->SetFocus();
        spinctrl->SetSelection(ALL_TEXT);
        return true;
    } else {
        return false;
    }
}

// -----------------------------------------------------------------------------

bool PrefsDialog::ValidatePage()
{
    // validate all spin control values on current page
    if (currpage == FILE_PAGE) {
        if ( BadSpinVal(PREF_MAX_PATTERNS, 1, ID::MAX_RECENT, _("Maximum number of recent patterns")) )
            return false;

    } else if (currpage == EDIT_PAGE) {
        // no spin ctrls on this page

    } else if (currpage == VIEW_PAGE) {
        // no spin ctrls on this page

    } else if (currpage == ACTION_PAGE) {
        // no spin ctrls on this page

    } else if (currpage == KEYBOARD_PAGE) {
        // no spin ctrls on this page

    } else {
        Warning(_("Bug in ValidatePage!"));
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------

void PrefsDialog::OnPageChanging(wxNotebookEvent& event)
{
    if (ignore_page_event) return;
    // validate current page and veto change if invalid
    if (!ValidatePage()) event.Veto();
}

// -----------------------------------------------------------------------------

void PrefsDialog::OnPageChanged(wxNotebookEvent& event)
{
    if (ignore_page_event) return;
    currpage = event.GetSelection();

#ifdef __WXMSW__
    // ensure key combo box has focus
    if (currpage == KEYBOARD_PAGE) {
        KeyComboCtrl* keycombo = (KeyComboCtrl*) FindWindowById(PREF_KEYCOMBO);
        if (keycombo) {
            keycombo->SetFocus();
            keycombo->SetSelection(ALL_TEXT);
        }
    }
#endif
}

// -----------------------------------------------------------------------------

bool PrefsDialog::TransferDataFromWindow()
{
    if (!ValidatePage()) return false;

    // set global prefs to current control values

    // FILE_PAGE
    maxpatterns   = GetSpinVal(PREF_MAX_PATTERNS);
    texteditor    = neweditor;

    // EDIT_PAGE
    repaint_to_erase = GetCheckVal(PREF_ERASE);
    allowbeep     = GetCheckVal(PREF_BEEP);

    // VIEW_PAGE
    #if wxUSE_TOOLTIPS
        showtips = GetCheckVal(PREF_SHOW_TIPS);
        wxToolTip::Enable(showtips);
    #endif

    // ACTION_PAGE
    // nothing yet

    // KEYBOARD_PAGE
    // go thru keyaction table and make sure the file field is empty
    // if the action isn't DO_OPENFILE
    for (int key = 0; key < MAX_KEYCODES; key++)
        for (int modset = 0; modset < MAX_MODS; modset++)
            if ( keyaction[key][modset].id != DO_OPENFILE &&
                 !keyaction[key][modset].file.IsEmpty() )
                keyaction[key][modset].file = wxEmptyString;

    return true;
}

// -----------------------------------------------------------------------------

bool ChangePrefs(const wxString& page)
{
    // save current keyboard shortcuts so we can restore them or detect a change
    action_info savekeyaction[MAX_KEYCODES][MAX_MODS];
    for (int key = 0; key < MAX_KEYCODES; key++)
        for (int modset = 0; modset < MAX_MODS; modset++)
            savekeyaction[key][modset] = keyaction[key][modset];

    MyFrame* frameptr = wxGetApp().currframe;

    PrefsDialog dialog(frameptr, page);

    bool result;
    if (dialog.ShowModal() == wxID_OK) {
        // TransferDataFromWindow has validated and updated all global prefs

        // if a keyboard shortcut changed then update menu item accelerators
        for (int key = 0; key < MAX_KEYCODES; key++)
            for (int modset = 0; modset < MAX_MODS; modset++)
                if (savekeyaction[key][modset].id != keyaction[key][modset].id) {
                    // first update accelerator array
                    UpdateAcceleratorStrings();
                    goto done;
                }
        done:

        // if maxpatterns was reduced then we may need to remove some paths
        while (numpatterns > maxpatterns) {
            numpatterns--;
            patternSubMenu->Delete( patternSubMenu->FindItemByPosition(numpatterns) );
        }

        result = true;
    }
    else {
        // user hit Cancel, so restore keyaction array in case it was changed
        for (int key = 0; key < MAX_KEYCODES; key++)
            for (int modset = 0; modset < MAX_MODS; modset++)
                keyaction[key][modset] = savekeyaction[key][modset];

        result = false;
    }

    frameptr->UpdateMenuAccelerators();

    return result;
}
