/*  Copyright 2011, 2012 The Ready Bunch

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
#include "frame.hpp"
#include "app.hpp"
#include "wxutils.hpp"
#include "prefs.hpp"
#include "PatternsPanel.hpp"
#include "InfoPanel.hpp"
#include "HelpPanel.hpp"
#include "IDs.hpp"
#include "vtk_pipeline.hpp"
#include "dialogs.hpp"
#include "RecordingDialog.hpp"

// readybase:
#include "utils.hpp"
#include "GrayScottImageRD.hpp"
#include "OpenCL_utils.hpp"
#include "IO_XML.hpp"
#include "GrayScottMeshRD.hpp"
#include "FormulaOpenCLImageRD.hpp"
#include "FullKernelOpenCLImageRD.hpp"
#include "FormulaOpenCLMeshRD.hpp"
#include "FullKernelOpenCLMeshRD.hpp"
#include "MeshGenerators.hpp"

// local resources:
#include "appicon16.xpm"

// wxWidgets:
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/dnd.h>
#if wxUSE_TOOLTIPS
   #include "wx/tooltip.h"
#endif

// wxVTK: (local copy)
#include "wxVTKRenderWindowInteractor.h"

// STL:
#include <string>
#include <algorithm>
using namespace std;

// VTK:
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkOBJReader.h>
#include <vtkCellArray.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkTriangleFilter.h>
#include <vtkPointData.h>
#include <vtkRendererCollection.h>
#include <vtkXMLGenericDataObjectReader.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCellPicker.h>

#ifdef __WXMAC__
    #include <Carbon/Carbon.h>  // for GetCurrentProcess, etc
#endif

#if wxCHECK_VERSION(2,9,0)
    // some wxMenuItem method names have changed in wx 2.9
    #define GetText GetItemLabel
    #define SetText SetItemLabel
#endif

// ---------------------------------------------------------------------

const char* PaneName(int id)
{
    switch(id)
    {
        case ID::FileToolbar:   return "FileToolbar";
        case ID::ActionToolbar: return "ActionToolbar";
        case ID::PaintToolbar:  return "PaintToolbar";
        case ID::PatternsPane:  return "PatternsPane";
        case ID::InfoPane:      return "InfoPane";
        case ID::HelpPane:      return "HelpPane";
        case ID::CanvasPane:    return "CanvasPane";
        default:                throw runtime_error("PaneName : unlisted ID");
    }
}

// ---------------------------------------------------------------------

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_ACTIVATE(MyFrame::OnActivate)
    EVT_IDLE(MyFrame::OnIdle)
    EVT_SIZE(MyFrame::OnSize)
    EVT_CLOSE(MyFrame::OnClose)
    // file menu
    EVT_MENU(wxID_NEW, MyFrame::OnNewPattern)
    EVT_MENU(wxID_OPEN, MyFrame::OnOpenPattern)
    EVT_MENU(ID::ReloadFromDisk, MyFrame::OnReloadFromDisk)
    EVT_MENU(wxID_SAVE, MyFrame::OnSavePattern)
    EVT_MENU(ID::ImportMesh, MyFrame::OnImportMesh)
    EVT_MENU(ID::ExportMesh, MyFrame::OnExportMesh)
    EVT_MENU(ID::ExportImage, MyFrame::OnExportImage)
    EVT_MENU(ID::Screenshot, MyFrame::OnScreenshot)
    EVT_MENU(ID::RecordFrames, MyFrame::OnRecordFrames)
    EVT_UPDATE_UI(ID::RecordFrames, MyFrame::OnUpdateRecordFrames)
    EVT_MENU(ID::AddMyPatterns, MyFrame::OnAddMyPatterns)
    EVT_MENU(wxID_PREFERENCES, MyFrame::OnPreferences)
    EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
    // edit menu
    EVT_MENU(wxID_UNDO, MyFrame::OnUndo)
    EVT_UPDATE_UI(wxID_UNDO, MyFrame::OnUpdateUndo)
    EVT_MENU(wxID_REDO, MyFrame::OnRedo)
    EVT_UPDATE_UI(wxID_REDO, MyFrame::OnUpdateRedo)
    EVT_MENU(wxID_CUT, MyFrame::OnCut)
    EVT_MENU(wxID_COPY, MyFrame::OnCopy)
    EVT_MENU(wxID_PASTE, MyFrame::OnPaste)
    EVT_UPDATE_UI(wxID_PASTE, MyFrame::OnUpdatePaste)
    EVT_MENU(wxID_CLEAR, MyFrame::OnClear)
    EVT_MENU(wxID_SELECTALL, MyFrame::OnSelectAll)
    EVT_MENU(ID::Pointer, MyFrame::OnSelectPointerTool)
    EVT_UPDATE_UI(ID::Pointer, MyFrame::OnUpdateSelectPointerTool)
    EVT_MENU(ID::Pencil, MyFrame::OnSelectPencilTool)
    EVT_UPDATE_UI(ID::Pencil, MyFrame::OnUpdateSelectPencilTool)
    EVT_MENU(ID::Brush, MyFrame::OnSelectBrushTool)
    EVT_UPDATE_UI(ID::Brush, MyFrame::OnUpdateSelectBrushTool)
    EVT_MENU(ID::Picker, MyFrame::OnSelectPickerTool)
    EVT_UPDATE_UI(ID::Picker, MyFrame::OnUpdateSelectPickerTool)
    // view menu
    EVT_MENU(ID::FullScreen, MyFrame::OnFullScreen)
    EVT_MENU(ID::FitPattern, MyFrame::OnFitPattern)
    EVT_MENU(ID::Wireframe, MyFrame::OnWireframe)
    EVT_UPDATE_UI(ID::Wireframe, MyFrame::OnUpdateWireframe)
    EVT_MENU(ID::PatternsPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::PatternsPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::InfoPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::InfoPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::HelpPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::HelpPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::FileToolbar, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::FileToolbar, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::ActionToolbar, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::ActionToolbar, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::PaintToolbar, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::PaintToolbar, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::RestoreDefaultPerspective, MyFrame::OnRestoreDefaultPerspective)
    EVT_MENU(ID::ChangeActiveChemical, MyFrame::OnChangeActiveChemical)
    // action menu
    EVT_MENU(ID::Step1, MyFrame::OnStep)
    EVT_MENU(ID::StepN, MyFrame::OnStep)
    EVT_UPDATE_UI(ID::Step1, MyFrame::OnUpdateStep)
    EVT_UPDATE_UI(ID::StepN, MyFrame::OnUpdateStep)
    EVT_MENU(ID::RunStop, MyFrame::OnRunStop)
    EVT_UPDATE_UI(ID::RunStop, MyFrame::OnUpdateRunStop)
    EVT_MENU(ID::Faster, MyFrame::OnRunFaster)
    EVT_MENU(ID::Slower, MyFrame::OnRunSlower)
    EVT_MENU(ID::ChangeRunningSpeed, MyFrame::OnChangeRunningSpeed)
    EVT_MENU(ID::Reset, MyFrame::OnReset)
    EVT_UPDATE_UI(ID::Reset, MyFrame::OnUpdateReset)
    EVT_MENU(ID::GenerateInitialPattern, MyFrame::OnGenerateInitialPattern)
    EVT_MENU(ID::Blank, MyFrame::OnBlank)
    EVT_MENU(ID::AddParameter,MyFrame::OnAddParameter)
    EVT_UPDATE_UI(ID::AddParameter, MyFrame::OnUpdateAddParameter)
    EVT_MENU(ID::DeleteParameter,MyFrame::OnDeleteParameter)
    EVT_UPDATE_UI(ID::DeleteParameter, MyFrame::OnUpdateDeleteParameter)
    EVT_MENU(ID::ViewFullKernel,MyFrame::OnViewFullKernel)
    EVT_UPDATE_UI(ID::ViewFullKernel, MyFrame::OnUpdateViewFullKernel)
    EVT_MENU(ID::SelectOpenCLDevice, MyFrame::OnSelectOpenCLDevice)
    EVT_MENU(ID::OpenCLDiagnostics, MyFrame::OnOpenCLDiagnostics)
    // help menu
    EVT_MENU(wxID_HELP, MyFrame::OnHelp)
    EVT_MENU(ID::HelpQuick, MyFrame::OnHelp)
    EVT_MENU(ID::HelpIntro, MyFrame::OnHelp)
    EVT_MENU(ID::HelpTips, MyFrame::OnHelp)
    EVT_MENU(ID::HelpKeyboard, MyFrame::OnHelp)
    EVT_MENU(ID::HelpMouse, MyFrame::OnHelp)
    EVT_MENU(ID::HelpFile, MyFrame::OnHelp)
    EVT_MENU(ID::HelpEdit, MyFrame::OnHelp)
    EVT_MENU(ID::HelpView, MyFrame::OnHelp)
    EVT_MENU(ID::HelpAction, MyFrame::OnHelp)
    EVT_MENU(ID::HelpHelp, MyFrame::OnHelp)
    EVT_MENU(ID::HelpFormats, MyFrame::OnHelp)
    EVT_MENU(ID::HelpProblems, MyFrame::OnHelp)
    EVT_MENU(ID::HelpChanges, MyFrame::OnHelp)
    EVT_MENU(ID::HelpCredits, MyFrame::OnHelp)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
    // paint toolbar:
    EVT_BUTTON(ID::CurrentValueColor, MyFrame::OnChangeCurrentColor)
    // items in Open Recent submenu must be handled last
    EVT_MENU(wxID_ANY, MyFrame::OnOpenRecent)
END_EVENT_TABLE()

const char* MyFrame::opencl_not_available_message = 
	"This file requires OpenCL, which has not been detected on your system.\n\n"
	"OpenCL allows Ready to take advantage of the many-core architectures on\n"
	"graphics cards and modern CPUs. OpenCL also allows rules to be written in\n"
	"a text format and compiled on the fly. It is available on every operating\n"
	"system, so please install it to get the most out of Ready. (If your OS is\n"
	"running in a virtual machine then it may not be possible to get OpenCL\n"
	"working.)\n\n"
	"You can load the files in the 'CPU-only' folder, which don't use OpenCL. Or\n"
	"use File > New Pattern or File > Import Mesh to make new examples.";
						
// ---------------------------------------------------------------------

// constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title),
       pVTKWindow(NULL),system(NULL),
       is_running(false),
       frames_per_second(0.0),
       million_cell_generations_per_second(0.0),
       fullscreen(false),
       render_settings("render_settings"),
       is_recording(false),
       CurrentCursor(POINTER),
       current_paint_value(0.5f),
       left_mouse_is_down(false),
       right_mouse_is_down(false)
{
    this->SetIcon(wxICON(appicon16));
    #ifdef __WXGTK__
        // advanced docking hints cause problems on xfce (and probably others)
        this->aui_mgr.SetFlags( wxAUI_MGR_ALLOW_FLOATING | wxAUI_MGR_RECTANGLE_HINT );
    #endif
    #ifdef __WXMAC__
        this->aui_mgr.SetFlags( wxAUI_MGR_ALLOW_FLOATING | wxAUI_MGR_TRANSPARENT_HINT | wxAUI_MGR_ALLOW_ACTIVE_PANE );
        this->icons_folder = _T("resources/Icons/32px/");
    #else
        this->icons_folder = _T("resources/Icons/22px/");
    #endif
    this->aui_mgr.SetManagedWindow(this);
    
    GetPrefs();     // must be called before InitializeMenus

    this->InitializeMenus();
    this->InitializeToolbars();
    this->InitializeCursors();

    CreateStatusBar(1);
    SetStatusText(_("Ready"));
    
    this->is_opencl_available = OpenCL_utils::IsOpenCLAvailable();

    this->InitializePatternsPane();
    this->InitializeInfoPane();
    this->InitializeHelpPane();
    this->InitializeRenderPane();

    this->default_perspective = this->aui_mgr.SavePerspective();
    this->LoadSettings();
    this->aui_mgr.Update();

    // enable/disable tool tips
    #if wxUSE_TOOLTIPS
        // AKT TODO!!! fix bug: can't disable tooltips in Mac app (bug is in wxOSX-Cocoa)
        wxToolTip::Enable(showtips);
    #endif

    // initialize an RD system to get us started
    const wxString initfile = _T("Patterns/CPU-only/grayscott_3D.vti");
    if (wxFileExists(initfile)) {
        this->OpenFile(initfile);
    } else {
        // create new pattern
        wxCommandEvent cmdevent(wxID_NEW);
        OnNewPattern(cmdevent);
    }
}

// ---------------------------------------------------------------------

MyFrame::~MyFrame()
{
    this->SaveSettings(); // save the current settings so it starts up the same next time
    this->aui_mgr.UnInit();
    this->pVTKWindow->Delete();
    delete this->pencil_cursor;
    delete this->brush_cursor;
    delete this->picker_cursor;
    delete this->system;
}

// ---------------------------------------------------------------------

void MyFrame::InitializeMenus()
{
    wxMenuBar *menuBar = new wxMenuBar();
    {   // file menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_NEW, _("New Pattern") + GetAccelerator(DO_NEWPATT), _("Create a new pattern"));
        menu->AppendSeparator();
        menu->Append(wxID_OPEN, _("Open Pattern...") + GetAccelerator(DO_OPENPATT), _("Choose a pattern file to open"));
        menu->Append(ID::OpenRecent, _("Open Recent"), patternSubMenu);
        menu->Append(ID::ReloadFromDisk, _("Reload from Disk") + GetAccelerator(DO_RELOAD), _("Reload the pattern file from disk"));
        menu->AppendSeparator();
        menu->Append(ID::ImportMesh, _("Import Mesh...") + GetAccelerator(DO_IMPORTMESH), _("Import a mesh"));
        menu->Append(ID::ExportMesh, _("Export Mesh...") + GetAccelerator(DO_EXPORTMESH), _("Export the mesh"));
        menu->Append(ID::ExportImage, _("Export Image...") + GetAccelerator(DO_EXPORTIMAGE), _("Export the image"));
        menu->AppendSeparator();
        menu->Append(wxID_SAVE, _("Save Pattern...") + GetAccelerator(DO_SAVE), _("Save the current pattern"));
        menu->Append(ID::Screenshot, _("Save Screenshot...") + GetAccelerator(DO_SCREENSHOT), _("Save a screenshot of the current view"));
        menu->AppendSeparator();
        menu->AppendCheckItem(ID::RecordFrames, _("Start Recording...") + GetAccelerator(DO_RECORDFRAMES), _("Record frames as images to disk"));
        menu->AppendSeparator();
        menu->Append(ID::AddMyPatterns, _("Add My Patterns...") + GetAccelerator(DO_ADDPATTS), _("Add chosen folder to patterns pane"));
        #if !defined(__WXOSX_COCOA__)
            menu->AppendSeparator();
        #endif
        // on the Mac the wxID_PREFERENCES item is moved to the app menu
        menu->Append(wxID_PREFERENCES, _("Preferences...") + GetAccelerator(DO_PREFS), _("Edit the preferences"));
        #if !defined(__WXOSX_COCOA__)
            menu->AppendSeparator();
        #endif
        // on the Mac the wxID_EXIT item is moved to the app menu and the app name is appended to "Quit "
        menu->Append(wxID_EXIT, _("Quit") + GetAccelerator(DO_QUIT));
        menuBar->Append(menu, _("&File"));
    }
    {   // edit menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_UNDO, _("Undo") + GetAccelerator(DO_UNDO), _("Undo an edit"));
        menu->Append(wxID_REDO, _("Redo") + GetAccelerator(DO_REDO), _("Redo what was undone"));
        menu->AppendSeparator();
        menu->Append(wxID_CUT, _("Cut") + GetAccelerator(DO_CUT), _("Cut the selection and save it to the clipboard"));
        menu->Append(wxID_COPY, _("Copy") + GetAccelerator(DO_COPY), _("Copy the selection to the clipboard"));
        menu->Append(wxID_PASTE, _("Paste") + GetAccelerator(DO_PASTE), _("Paste in the contents of the clipboard"));
        menu->Append(wxID_CLEAR, _("Clear") + GetAccelerator(DO_CLEAR), _("Clear the selection"));
        menu->AppendSeparator();
        menu->Append(wxID_SELECTALL, _("Select All") + GetAccelerator(DO_SELALL), _("Select everything"));
        menu->AppendSeparator();
        menu->AppendRadioItem(ID::Pointer, _("Select Pointer") + GetAccelerator(DO_POINTER), _("Select pointer tool"));
        menu->AppendRadioItem(ID::Pencil, _("Select Pencil") + GetAccelerator(DO_PENCIL), _("Select pencil tool"));
        menu->AppendRadioItem(ID::Brush, _("Select Brush") + GetAccelerator(DO_BRUSH), _("Select brush tool"));
        menu->AppendRadioItem(ID::Picker, _("Select Color Picker") + GetAccelerator(DO_PICKER), _("Select color picker tool"));
        menuBar->Append(menu, _("&Edit"));
    }
    {   // view menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::FullScreen, _("Full Screen") + GetAccelerator(DO_FULLSCREEN), _("Toggle full screen mode"));
        menu->Append(ID::FitPattern, _("Fit Pattern") + GetAccelerator(DO_FIT), _("Restore view so all of pattern is visible"));
        menu->AppendCheckItem(ID::Wireframe, _("Wireframe") + GetAccelerator(DO_WIREFRAME), _("Wireframe or surface view"));
        menu->AppendSeparator();
        menu->AppendCheckItem(ID::PatternsPane, _("&Patterns Pane") + GetAccelerator(DO_PATTERNS), _("View the patterns pane"));
        menu->AppendCheckItem(ID::InfoPane, _("&Info Pane") + GetAccelerator(DO_INFO), _("View the info pane"));
        menu->AppendCheckItem(ID::HelpPane, _("&Help Pane") + GetAccelerator(DO_HELP), _("View the help pane"));
        menu->AppendSeparator();
        menu->AppendCheckItem(ID::FileToolbar, _("File Toolbar") + GetAccelerator(DO_FILETOOLBAR), _("View the file toolbar"));
        menu->AppendCheckItem(ID::ActionToolbar, _("Action Toolbar") + GetAccelerator(DO_ACTIONTOOLBAR), _("View the action toolbar"));
        menu->AppendCheckItem(ID::PaintToolbar, _("Paint Toolbar") + GetAccelerator(DO_PAINTTOOLBAR), _("View the paint toolbar"));
        menu->AppendSeparator();
        menu->Append(ID::RestoreDefaultPerspective, _("&Restore Default Layout") + GetAccelerator(DO_RESTORE), _("Put the windows and toolbars back where they were"));
        menu->AppendSeparator();
        menu->Append(ID::ChangeActiveChemical, _("&Change Active Chemical...") + GetAccelerator(DO_CHEMICAL), _("Change which chemical is being visualized"));
        menuBar->Append(menu, _("&View"));
    }
    {   // action menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::Step1, _("Step by 1") + GetAccelerator(DO_STEP1), _("Advance the simulation by a single timestep"));
        menu->Append(ID::StepN, _("Step by N") + GetAccelerator(DO_STEPN), _("Advance the simulation by timesteps per render"));
        menu->Append(ID::RunStop, _("Run") + GetAccelerator(DO_RUNSTOP), _("Start running the simulation"));
        menu->AppendSeparator();
        menu->Append(ID::Faster, _("Run Faster") + GetAccelerator(DO_FASTER),_("Run with more timesteps between each render"));
        menu->Append(ID::Slower, _("Run Slower") + GetAccelerator(DO_SLOWER),_("Run with fewer timesteps between each render"));
        menu->Append(ID::ChangeRunningSpeed, _("Change Running Speed...") + GetAccelerator(DO_CHANGESPEED),_("Change the number of timesteps between each render"));
        menu->AppendSeparator();
        menu->Append(ID::Reset, _("Reset") + GetAccelerator(DO_RESET), _("Go back to the starting pattern"));
        menu->Append(ID::GenerateInitialPattern, _("Generate Initial &Pattern") + GetAccelerator(DO_GENPATT), _("Run the Initial Pattern Generator"));
        menu->Append(ID::Blank, _("&Blank") + GetAccelerator(DO_BLANK), _("Sets every value to zero"));
        menu->AppendSeparator();
        menu->Append(ID::AddParameter, _("&Add Parameter...") + GetAccelerator(DO_ADDPARAM),_("Add a new named parameter"));
        menu->Append(ID::DeleteParameter, _("&Delete Parameter...") + GetAccelerator(DO_DELPARAM),_("Delete one of the parameters"));
        menu->Append(ID::ViewFullKernel, _("View Full Kernel") + GetAccelerator(DO_VIEWKERNEL),_("Shows the full OpenCL kernel as expanded from the formula"));
        menu->AppendSeparator();
        menu->Append(ID::SelectOpenCLDevice, _("Select &OpenCL Device...") + GetAccelerator(DO_DEVICE), _("Choose which OpenCL device to run on"));
        menu->Append(ID::OpenCLDiagnostics, _("Show Open&CL Diagnostics...") + GetAccelerator(DO_OPENCL), _("Show the available OpenCL devices and their attributes"));
        menuBar->Append(menu, _("&Action"));
    }
    {   // help menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_HELP,        _("Contents"));
        menu->Append(ID::HelpQuick,    _("Quick Start"));
        menu->Append(ID::HelpIntro,    _("Introduction to RD"));
        menu->AppendSeparator();
        menu->Append(ID::HelpTips,     _("Hints and Tips"));
        menu->Append(ID::HelpKeyboard, _("Keyboard Shortcuts"));
        menu->Append(ID::HelpMouse,    _("Mouse Shortcuts"));
        menu->AppendSeparator();
        menu->Append(ID::HelpFile,     _("File Menu"));
        menu->Append(ID::HelpEdit,     _("Edit Menu"));
        menu->Append(ID::HelpView,     _("View Menu"));
        menu->Append(ID::HelpAction,   _("Action Menu"));
        menu->Append(ID::HelpHelp,     _("Help Menu"));
        menu->AppendSeparator();
        menu->Append(ID::HelpFormats,  _("File Formats"));
        menu->Append(ID::HelpProblems, _("Known Problems"));
        menu->Append(ID::HelpChanges,  _("Changes"));
        menu->Append(ID::HelpCredits,  _("Credits"));
        menu->AppendSeparator();
        menu->Append(wxID_ABOUT,       _("&About Ready") + GetAccelerator(DO_ABOUT));
        menuBar->Append(menu, _("&Help"));
    }
    SetMenuBar(menuBar);
}

// ---------------------------------------------------------------------

void MyFrame::InitializeToolbars()
{
    const int toolbar_padding = 5;

    {   // file menu items
        this->file_toolbar = new wxAuiToolBar(this,ID::FileToolbar);
        this->file_toolbar->AddTool(wxID_NEW,_("New Pattern"),wxBitmap(this->icons_folder + _T("document-new.png"),wxBITMAP_TYPE_PNG),
            _("New Pattern"));
        this->file_toolbar->AddTool(wxID_OPEN,_("Open Pattern..."),wxBitmap(this->icons_folder + _T("document-open.png"),wxBITMAP_TYPE_PNG),
            _("Open Pattern..."));
        this->file_toolbar->AddTool(ID::ReloadFromDisk,_("Reload from disk"),wxBitmap(this->icons_folder + _T("document-revert.png"),wxBITMAP_TYPE_PNG),
            _("Reload from disk"));
        this->file_toolbar->AddTool(wxID_SAVE,_("Save Pattern..."),wxBitmap(this->icons_folder + _T("document-save.png"),wxBITMAP_TYPE_PNG),
            _("Save Pattern..."));
        this->file_toolbar->AddTool(ID::Screenshot,_("Save Screenshot..."),wxBitmap(this->icons_folder + _T("camera-photo.png"),wxBITMAP_TYPE_PNG),
            _("Save Screenshot..."));
        this->file_toolbar->SetToolBorderPadding(toolbar_padding);
        this->aui_mgr.AddPane(this->file_toolbar,wxAuiPaneInfo().ToolbarPane().Top().Name(PaneName(ID::FileToolbar))
            .Position(0).Caption(_("File tools")));
    }
    {   // action menu items
        this->action_toolbar = new wxAuiToolBar(this,ID::ActionToolbar);
        this->action_toolbar->AddTool(ID::Step1, _("Step by 1"),wxBitmap(this->icons_folder + _T("list-add_gray.png"),wxBITMAP_TYPE_PNG),
            _("Step by 1"));
        this->action_toolbar->AddTool(ID::RunStop,_("Run"),wxBitmap(this->icons_folder + _T("media-playback-start_green.png"),wxBITMAP_TYPE_PNG),
            _("Run"));
        //this->action_toolbar->AddTool(ID::RecordFrames,_("Start Recording..."),wxBitmap(this->icons_folder + _T("media-record.png"),wxBITMAP_TYPE_PNG),
        //    _("Start Recording..."),wxITEM_CHECK);
        this->action_toolbar->AddTool(ID::Slower,_("Run Slower"),wxBitmap(this->icons_folder + _T("media-seek-backward.png"),wxBITMAP_TYPE_PNG),
            _("Run Slower"));
        this->action_toolbar->AddTool(ID::Faster,_("Run Faster"),wxBitmap(this->icons_folder + _T("media-seek-forward.png"),wxBITMAP_TYPE_PNG),
            _("Run Faster"));
        this->action_toolbar->AddTool(ID::Reset, _("Reset"),wxBitmap(this->icons_folder + _T("media-skip-backward_modified.png"),wxBITMAP_TYPE_PNG),
            _("Reset"));
        this->action_toolbar->AddTool(ID::GenerateInitialPattern,_("Generate Initial Pattern"),wxBitmap(this->icons_folder + _T("system-run.png"),wxBITMAP_TYPE_PNG),
            _("Generate Initial Pattern"));
        this->action_toolbar->SetToolBorderPadding(toolbar_padding);
        this->aui_mgr.AddPane(this->action_toolbar,wxAuiPaneInfo().ToolbarPane().Top()
            .Name(PaneName(ID::ActionToolbar)).Position(1).Caption(_("Action tools")));
    }
    {   // paint items
        this->paint_toolbar = new wxAuiToolBar(this,ID::PaintToolbar);
        this->paint_toolbar->AddTool(ID::Pointer,_("Pointer"),wxBitmap(this->icons_folder + _T("icon-pointer.png"),wxBITMAP_TYPE_PNG),
            _("Pointer"),wxITEM_RADIO);
        this->paint_toolbar->AddTool(ID::Pencil,_("Pencil"),wxBitmap(this->icons_folder + _T("draw-freehand.png"),wxBITMAP_TYPE_PNG),
            _("Pencil (right-click to pick color)"),wxITEM_RADIO);
        this->paint_toolbar->AddTool(ID::Brush,_("Brush"),wxBitmap(this->icons_folder + _T("draw-brush.png"),wxBITMAP_TYPE_PNG),
            _("Brush (right-click to pick color)"),wxITEM_RADIO);
        this->paint_toolbar->AddTool(ID::Picker,_("Color picker"),wxBitmap(this->icons_folder + _T("color-picker.png"),wxBITMAP_TYPE_PNG),
            _("Color picker"),wxITEM_RADIO);
        wxStaticText *st = new wxStaticText(this->paint_toolbar,ID::CurrentValueText,_("  1.000000  "),wxDefaultPosition,wxDefaultSize,wxALIGN_CENTRE_HORIZONTAL);
        st->SetToolTip(_("Current value to paint with"));
        this->paint_toolbar->AddControl(st,_("Color"));
        wxImage im(22,22);
        im.SetRGB(wxRect(0,0,22,22),255,0,0);
        wxBitmapButton *cb = new wxBitmapButton(this->paint_toolbar,ID::CurrentValueColor,wxBitmap(im));
        cb->SetToolTip(_("Color of the current paint value. Click to change the value."));
        this->paint_toolbar->AddControl(cb,_("Color"));
        this->paint_toolbar->SetToolBorderPadding(toolbar_padding);
        this->aui_mgr.AddPane(this->paint_toolbar,wxAuiPaneInfo().ToolbarPane().Top().Name(PaneName(ID::PaintToolbar))
            .Position(2).Caption(_("Paint tools")));
    }
}

// ---------------------------------------------------------------------

void MyFrame::InitializeCursors()
{
    const wxString cursors_folder(_T("resources/Cursors/"));

    wxImage im1(cursors_folder + _T("pencil-cursor.png"),wxBITMAP_TYPE_PNG);
    im1.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 3);
    im1.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 18);
    this->pencil_cursor = new wxCursor(im1);

    wxImage im2(cursors_folder + _T("brush-cursor.png"),wxBITMAP_TYPE_PNG);
    im2.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 3);
    im2.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 21);
    this->brush_cursor = new wxCursor(im2);

    wxImage im3(cursors_folder + _T("picker-cursor.png"),wxBITMAP_TYPE_PNG);
    im3.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 4);
    im3.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 14);
    this->picker_cursor = new wxCursor(im3);
}

// ---------------------------------------------------------------------

void MyFrame::InitializePatternsPane()
{
    this->patterns_panel = new PatternsPanel(this,wxID_ANY);
    this->aui_mgr.AddPane(this->patterns_panel,
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::PatternsPane))
                  .Caption(_("Patterns Pane"))
                  .Left()
                  .BestSize(220,600)
                  .Position(0)
                  );
}

// ---------------------------------------------------------------------

void MyFrame::InitializeInfoPane()
{
    this->info_panel = new InfoPanel(this,wxID_ANY);
    this->aui_mgr.AddPane(this->info_panel,
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::InfoPane))
                  .Caption(_("Info Pane"))
                  .Right()
                  .BestSize(500,300)
                  .Position(0)
                  );
}

// ---------------------------------------------------------------------

void MyFrame::UpdateInfoPane()
{
    this->info_panel->Update(this->system);
}

// ---------------------------------------------------------------------

void MyFrame::InitializeHelpPane()
{
    this->help_panel = new HelpPanel(this,wxID_ANY);
    this->aui_mgr.AddPane(this->help_panel,
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::HelpPane))
                  .Caption(_("Help Pane"))
                  .Right()
                  .BestSize(500,300)
                  .Position(1)
                  );
}

// ---------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

// derive a simple class for handling dropped files
class DnDFile : public wxFileDropTarget
{
public:
    DnDFile() {}
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
};

bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
{
    MyFrame* frameptr = wxGetApp().currframe;

    // bring app to front
    #ifdef __WXMAC__
        ProcessSerialNumber process;
        if ( GetCurrentProcess(&process) == noErr ) SetFrontProcess(&process);
    #endif
    #ifdef __WXMSW__
        SetForegroundWindow( (HWND)frameptr->GetHandle() );
    #endif
    frameptr->Raise();
   
    size_t numfiles = filenames.GetCount();
    for ( size_t n = 0; n < numfiles; n++ ) {
        frameptr->OpenFile(filenames[n]);
    }
    return true;
}

#endif // wxUSE_DRAG_AND_DROP

// ---------------------------------------------------------------------

void MyFrame::InitializeRenderPane()
{
    // for now the VTK window goes in the center pane (always visible) - we got problems when had in a floating pane
    vtkObject::GlobalWarningDisplayOff(); // (can turn on for debugging)
    this->pVTKWindow = new wxVTKRenderWindowInteractor(this,wxID_ANY);
    this->aui_mgr.AddPane(this->pVTKWindow,
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::CanvasPane))
                  // AKT TODO!!! why don't we see caption??? (it would be nice to see when it has focus)
                  .Caption(_("Render Pane")).CaptionVisible()
                  .CenterPane()
                  .BestSize(400,400)
                  );

    #if wxUSE_DRAG_AND_DROP
        // let users drag-and-drop pattern files onto the render pane
        this->pVTKWindow->SetDropTarget(new DnDFile());
    #endif

    // install event handlers to detect keyboard shortcuts when render window has focus
    this->pVTKWindow->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MyFrame::OnKeyDown), NULL, this);
    this->pVTKWindow->Connect(wxEVT_CHAR, wxKeyEventHandler(MyFrame::OnChar), NULL, this);
}

// ---------------------------------------------------------------------

void MyFrame::LoadSettings()
{
    // use global info set by GetPrefs()
    this->SetPosition(wxPoint(mainx,mainy));
    this->SetSize(mainwd,mainht);
    if (auilayout.length() > 0 
        && currversion > 1)  // one-off fix for issue of IDs being used as names
        this->aui_mgr.LoadPerspective(auilayout);
}

// ---------------------------------------------------------------------

void MyFrame::SaveSettings()
{
    if (fullscreen) {
        // use auilayout saved earlier in OnFullScreen
    } else {
        auilayout = this->aui_mgr.SavePerspective();
    }
    SavePrefs();
}

// ---------------------------------------------------------------------

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    if(UserWantsToCancelWhenAskedIfWantsToSave()) return;
    Close(true);
}

// ---------------------------------------------------------------------

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    ShowAboutBox();
}

// ---------------------------------------------------------------------

void MyFrame::OnCut(wxCommandEvent& event)
{
    // action depends on which pane has focus
    if (this->info_panel->HtmlHasFocus()) return;
    if (this->help_panel->HtmlHasFocus()) return;
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::OnCopy(wxCommandEvent& event)
{
    // action depends on which pane has focus
    if (this->info_panel->HtmlHasFocus()) {
        this->info_panel->CopySelection();
        return;
    }
    if (this->help_panel->HtmlHasFocus()) {
        this->help_panel->CopySelection();
        return;
    }
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::OnPaste(wxCommandEvent& event)
{
    // action depends on which pane has focus
    if (this->info_panel->HtmlHasFocus()) return;
    if (this->help_panel->HtmlHasFocus()) return;
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable(ClipboardHasText());
}

// ---------------------------------------------------------------------

void MyFrame::OnClear(wxCommandEvent& event)
{
    // action depends on which pane has focus
    if (this->info_panel->HtmlHasFocus()) return;
    if (this->help_panel->HtmlHasFocus()) return;
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectAll(wxCommandEvent& event)
{
    // action depends on which pane has focus
    if (this->info_panel->HtmlHasFocus()) {
        this->info_panel->SelectAllText();
        return;
    }
    if (this->help_panel->HtmlHasFocus()) {
        this->help_panel->SelectAllText();
        return;
    }
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::OnFullScreen(wxCommandEvent& event)
{
    static bool restorestatus;  // restore the status bar?
    
    wxStatusBar* statusbar = GetStatusBar();
    
    if (!fullscreen) {
        // save current location and size for use in SavePrefs
        wxRect r = GetRect();
        mainx = r.x;
        mainy = r.y;
        mainwd = r.width;
        mainht = r.height;
        // also save current perspective
        auilayout = this->aui_mgr.SavePerspective();
    } else {
        // restore status bar before calling ShowFullScreen (so we see status text in Mac app)
        if (restorestatus) statusbar->Show();
    }

    fullscreen = !fullscreen;
    ShowFullScreen(fullscreen, wxFULLSCREEN_NOMENUBAR | wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);

    if (fullscreen) {
        // hide the status bar
        restorestatus = statusbar && statusbar->IsShown();
        if (restorestatus) statusbar->Hide();

        // hide all currently shown panes
        wxAuiPaneInfo &pattpane = this->aui_mgr.GetPane(PaneName(ID::PatternsPane));
        wxAuiPaneInfo &infopane = this->aui_mgr.GetPane(PaneName(ID::InfoPane));
        wxAuiPaneInfo &helppane = this->aui_mgr.GetPane(PaneName(ID::HelpPane));
        wxAuiPaneInfo &filepane = this->aui_mgr.GetPane(PaneName(ID::FileToolbar));
        wxAuiPaneInfo &actionpane = this->aui_mgr.GetPane(PaneName(ID::ActionToolbar));
        
        if (pattpane.IsOk() && pattpane.IsShown()) pattpane.Show(false);
        if (infopane.IsOk() && infopane.IsShown()) infopane.Show(false);
        if (helppane.IsOk() && helppane.IsShown()) helppane.Show(false);
        if (filepane.IsOk() && filepane.IsShown()) filepane.Show(false);
        if (actionpane.IsOk() && actionpane.IsShown()) actionpane.Show(false);
        
        // ensure the render window sees keyboard shortcuts
        this->pVTKWindow->SetFocus();

    } else {
        // restore saved perspective
        this->aui_mgr.LoadPerspective(auilayout);
    }
    
    this->aui_mgr.Update();
}

// ---------------------------------------------------------------------

void MyFrame::OnFitPattern(wxCommandEvent& event)
{
    // reset the active camera in all the renderers in this render window
    vtkRenderWindow* renWin = this->pVTKWindow->GetRenderWindow();
    renWin->GetRenderers()->InitTraversal();
    vtkRenderer *ren;
    while(ren = renWin->GetRenderers()->GetNextItem())
        ren->ResetCamera();
    this->Refresh(false);
}

// ---------------------------------------------------------------------

void MyFrame::OnWireframe(wxCommandEvent& event)
{
    bool wireframe = this->render_settings.GetProperty("use_wireframe").GetBool();
    wireframe = !wireframe;
    this->render_settings.GetProperty("use_wireframe").SetBool(wireframe);
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->render_settings,false);
    this->UpdateInfoPane();
    this->Refresh(false);
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateWireframe(wxUpdateUIEvent& event)
{
    event.Check(this->render_settings.GetProperty("use_wireframe").GetBool());
}

// ---------------------------------------------------------------------

void MyFrame::OnToggleViewPane(wxCommandEvent& event)
{
    wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(event.GetId()));
    if(!pane.IsOk()) return;
    pane.Show(!pane.IsShown());
    this->aui_mgr.Update();
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateViewPane(wxUpdateUIEvent& event)
{
    wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(event.GetId()));
    if(!pane.IsOk()) return;
    event.Check(pane.IsShown());
}

// ---------------------------------------------------------------------

void MyFrame::OnOpenCLDiagnostics(wxCommandEvent& event)
{
    // TODO: merge this with SelectOpenCLDevice?
    wxString txt;
    {
        wxBusyCursor busy;
        txt = wxString(OpenCL_utils::GetOpenCLDiagnostics().c_str(),wxConvUTF8);
    }
    MonospaceMessageBox(txt,_("OpenCL diagnostics"),wxART_INFORMATION);
}

// ---------------------------------------------------------------------

void MyFrame::OnSize(wxSizeEvent& event)
{
#ifdef __WXMSW__
    if(this->pVTKWindow) {
        // save current location and size for use in SavePrefs if app
        // is closed when window is minimized
        wxRect r = GetRect();
        mainx = r.x;
        mainy = r.y;
        mainwd = r.width;
        mainht = r.height;
    }
#endif

    // trigger a redraw
    if(this->pVTKWindow) this->pVTKWindow->Refresh(false);
    
    // need this to move and resize status bar in Mac app
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::OnScreenshot(wxCommandEvent& event)
{
    // find an unused filename
    const wxString default_filename_root = _("Ready_screenshot_");
    const wxString default_filename_ext = _T("png");
    int unused_value = 0;
    wxString filename;
    wxString extension,folder;
    folder = screenshotdir;
    do {
        filename = default_filename_root;
        filename << wxString::Format(_("%04d."),unused_value) << default_filename_ext;
        unused_value++;
    } while(::wxFileExists(folder+_T("/")+filename));

    // ask the user for confirmation
    bool accepted = true;
    do {
        filename = wxFileSelector(_("Specify the screenshot filename"),folder,filename,default_filename_ext,
            _("PNG files (*.png)|*.png|JPG files (*.jpg)|*.jpg"),
            wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
        if(filename.empty()) return; // user cancelled
        // validate
        wxFileName::SplitPath(filename,&folder,NULL,&extension);
        if(extension!=_T("png") && extension!=_T("jpg"))
        {
            wxMessageBox(_("Unsupported format"));
            accepted = false;
        }
    } while(!accepted);

    screenshotdir = folder;

    vtkSmartPointer<vtkWindowToImageFilter> screenshot = vtkSmartPointer<vtkWindowToImageFilter>::New();
    screenshot->SetInput(this->pVTKWindow->GetRenderWindow());

    vtkSmartPointer<vtkImageWriter> writer;
    if(extension==_T("png")) writer = vtkSmartPointer<vtkPNGWriter>::New();
    else if(extension==_T("jpg")) writer = vtkSmartPointer<vtkJPEGWriter>::New();
    writer->SetFileName(filename.mb_str());
    writer->SetInputConnection(screenshot->GetOutputPort());
    writer->Write();
}

// ---------------------------------------------------------------------

void MyFrame::OnAddMyPatterns(wxCommandEvent& event)
{
    // first make sure the patterns pane is visible
    wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(ID::PatternsPane));
    if(pane.IsOk() && !pane.IsShown()) {
        pane.Show();
        this->aui_mgr.Update();
    }

    wxDirDialog dirdlg(this, _("Choose your pattern folder"), userdir, wxDD_NEW_DIR_BUTTON);
    if (dirdlg.ShowModal() == wxID_OK) {
        userdir = dirdlg.GetPath();
        this->patterns_panel->BuildTree();
    }
}

// ---------------------------------------------------------------------

void MyFrame::SetCurrentRDSystem(AbstractRD* sys)
{
    delete this->system;
    this->system = sys;
    int iChem = IndexFromChemicalName(this->render_settings.GetProperty("active_chemical").GetChemical());
    iChem = min(iChem,this->system->GetNumberOfChemicals()-1); // ensure is in valid range
    this->render_settings.GetProperty("active_chemical").SetChemical(GetChemicalName(iChem));
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->render_settings,true);
    this->is_running = false;
    this->info_panel->ResetPosition();
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::UpdateWindowTitle()
{
    wxString name = this->system->GetFilename();
    if (name.IsEmpty()) {
        // this should probably never happen
        name = _("unknown");
    } else {
        // just show file's name, not full path
        name = name.AfterLast(wxFILE_SEP_PATH);
    }
    
    if (this->system->IsModified()) {
        // prepend asterisk to indicate the current system has been modified
        // (this is consistent with Golly and other Win/Linux apps)
        name = _T("*") + name;
    }
    
    #ifdef __WXMAC__
        // Mac apps don't show app name in window title
        this->SetTitle(name);
    #else
        // Win/Linux apps usually append the app name to the file name
        this->SetTitle(name + _T(" - Ready"));
    #endif
}

// ---------------------------------------------------------------------

void MyFrame::UpdateWindows()
{
    this->SetStatusBarText();
    this->UpdateInfoPane();
    this->UpdateWindowTitle();
    this->UpdateToolbars();
    this->Refresh(false);
}

// ---------------------------------------------------------------------

void MyFrame::OnStep(wxCommandEvent& event)
{
    if (this->is_running)
        return;
    
    if (this->system->GetTimestepsTaken() == 0)
    {
        this->system->SaveStartingPattern();
    
        // reset the initial number of steps used by system->Update in OnIdle
        num_steps = 50;
        // 50 is half the initial timesteps_per_render value used in most
        // pattern files, but really we could choose any small number > 0
    }

    try
    {
        if (event.GetId() == ID::Step1) {
            this->system->Update(1);
        } else if (event.GetId() == ID::StepN) {
            // timesteps_per_render might be huge, so don't do this:
            // this->system->Update(this->render_settings.GetProperty("timesteps_per_render").GetInt());
            // instead we let OnIdle do the stepping, but stop at next render
            this->is_running = true;
            steps_since_last_render = 0;
            accumulated_time = 0.0;
            do_one_render = true;
        }
    }
    catch(const exception& e)
    {
        MonospaceMessageBox(_("An error occurred when running the simulation:\n\n")+wxString(e.what(),wxConvUTF8),_("Error"),wxART_ERROR);
    }
    catch(...)
    {
        wxMessageBox(_("An unknown error occurred when running the simulation"));
    }
    
    this->SetStatusBarText();
    Refresh(false);
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateStep(wxUpdateUIEvent& event)
{
    // Step1 or StepN
    event.Enable(!this->is_running);
}

// ---------------------------------------------------------------------

void MyFrame::OnRunStop(wxCommandEvent& event)
{
    if (this->is_running) {
        this->is_running = false;
        this->SetStatusBarText();
    } else {
        this->is_running = true;
    }
    this->UpdateToolbars();
    Refresh(false);
    
    if (this->is_running) {
        if (this->system->GetTimestepsTaken() == 0)
        {
            this->system->SaveStartingPattern();
    
            // reset the initial number of steps used by system->Update in OnIdle
            num_steps = 50;
            // 50 is half the initial timesteps_per_render value used in most
            // pattern files, but really we could choose any small number > 0
        }
        steps_since_last_render = 0;
        accumulated_time = 0.0;
        do_one_render = false;
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateRunStop(wxUpdateUIEvent& event)
{
    wxMenuBar* mbar = GetMenuBar();
    if (mbar) {
        if (this->is_running) {
            mbar->SetLabel(ID::RunStop, _("Stop") + GetAccelerator(DO_RUNSTOP));
            mbar->SetHelpString(ID::RunStop,_("Stop running the simulation"));
        } else {
            mbar->SetLabel(ID::RunStop, _("Run") + GetAccelerator(DO_RUNSTOP));
            mbar->SetHelpString(ID::RunStop,_("Start running the simulation"));
        }
    }
}

// ---------------------------------------------------------------------

void MyFrame::UpdateToolbars()
{
    this->action_toolbar->FindTool(ID::RunStop)->SetBitmap( 
        this->is_running ? wxBitmap(this->icons_folder + _T("media-playback-pause_green.png"),wxBITMAP_TYPE_PNG)
                         : wxBitmap(this->icons_folder + _T("media-playback-start_green.png"),wxBITMAP_TYPE_PNG) );
    
    this->action_toolbar->FindTool(ID::RunStop)->SetShortHelp( 
        this->is_running ? _("Stop running the simulation")
                         : _("Start running the simulation") );
    this->action_toolbar->FindTool(ID::RunStop)->SetLabel( 
        this->is_running ? _("Stop")
                         : _("Run") );
    this->paint_toolbar->FindControl(ID::CurrentValueText)->SetLabel( wxString::Format(_T("%f"),
        this->current_paint_value) );
    // update the color swatch with the current color
    wxImage im(22,22);
    float r1,g1,b1,r2,g2,b2,low,high,r,g,b;
    this->render_settings.GetProperty("color_low").GetColor(r1,g1,b1);
    this->render_settings.GetProperty("color_high").GetColor(r2,g2,b2);
    low = this->render_settings.GetProperty("low").GetFloat();
    high = this->render_settings.GetProperty("high").GetFloat();
    InterpolateInHSV(r1,g1,b1,r2,g2,b2,min(1.0f,max(0.0f,(this->current_paint_value-low)/(high-low))),r,g,b);
    im.SetRGB(wxRect(0,0,22,22),r*255,g*255,b*255);
    dynamic_cast<wxBitmapButton*>(this->paint_toolbar->FindControl(ID::CurrentValueColor))->SetBitmap(wxBitmap(im));
    this->aui_mgr.Update();
}

// ---------------------------------------------------------------------

void MyFrame::OnReset(wxCommandEvent& event)
{
    if(this->system->GetTimestepsTaken() > 0) 
    {
        // restore pattern and other info saved by SaveStartingPattern() which
        // was called in OnStep/OnRunStop when GetTimestepsTaken() was 0
        this->system->RestoreStartingPattern();
        this->is_running = false;
        this->UpdateWindows();
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateReset(wxUpdateUIEvent& event)
{
    event.Enable(this->system->GetTimestepsTaken() > 0);
}

// ---------------------------------------------------------------------

void MyFrame::CheckFocus()
{
    // ensure one of our panes has the focus so keyboard shortcuts always work
    if ( this->pVTKWindow->HasFocus() ||
         this->patterns_panel->TreeHasFocus() ||
         this->info_panel->HtmlHasFocus() ||
         this->help_panel->HtmlHasFocus() ) {
        // good, no need to change focus
    } else {
        // best to restore focus to render window
        this->pVTKWindow->SetFocus();
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnIdle(wxIdleEvent& event)
{
    #ifdef __WXMAC__
        // do NOT do this in the Win app (buttons in Info/Help pane won't work)
        if (this->IsActive()) this->CheckFocus();
    #endif
    
    // we drive our simulation loop via idle events
    if (this->is_running)
    {
        // ensure num_steps <= timesteps_per_render
        int timesteps_per_render = this->render_settings.GetProperty("timesteps_per_render").GetInt();
        if (num_steps > timesteps_per_render) num_steps = timesteps_per_render;

        // use temp_steps for the actual system->Update call because it might be < num_steps
        int temp_steps = num_steps;
        if (steps_since_last_render + temp_steps > timesteps_per_render) {
            // do final steps of this rendering phase
            temp_steps = timesteps_per_render - steps_since_last_render;
        }
        
        double time_before = get_time_in_seconds();
        
        try 
        {
            this->system->Update(temp_steps);
        }
        catch(const exception& e)
        {
            this->is_running = false;
            this->SetStatusBarText();
            this->UpdateToolbars();
            MonospaceMessageBox(_("An error occurred when running the simulation:\n\n")+wxString(e.what(),wxConvUTF8),_("Error"),wxART_ERROR);
        }
        catch(...)
        {
            this->is_running = false;
            this->SetStatusBarText();
            this->UpdateToolbars();
            wxMessageBox(_("An unknown error occurred when running the simulation"));
        }
        
        double time_diff = get_time_in_seconds() - time_before;
        
        // note that we don't change num_steps if temp_steps < num_steps
        if (num_steps == temp_steps) {
            // if the above system->Update was quick then we'll use more steps in the next call,
            // otherwise we'll use less steps so that the app remains responsive
            if (time_diff < 0.1) {
                num_steps *= 2;
                if (num_steps > timesteps_per_render) num_steps = timesteps_per_render;
            } else {
                num_steps /= 2;
                if (num_steps < 1) num_steps = 1;
            }
        }
        
        accumulated_time += time_diff;
        steps_since_last_render += temp_steps;
        
        if (steps_since_last_render >= timesteps_per_render) {
            // it's time to render what we've computed so far
            int n_cells = this->system->GetNumberOfCells();
            if (accumulated_time == 0.0)
                accumulated_time = 0.000001;  // unlikely, but play safe
            this->frames_per_second = steps_since_last_render / accumulated_time;
            this->million_cell_generations_per_second = this->frames_per_second * n_cells / 1e6;

            if(this->is_recording)
                this->RecordFrame();
       
            this->pVTKWindow->Refresh(false);
            this->SetStatusBarText();
            
            if (do_one_render) {
                // user selected Step by N so stop now
                this->is_running = false;
                this->SetStatusBarText();
                this->UpdateToolbars();
            } else {
                // keep simulating
                steps_since_last_render = 0;
                accumulated_time = 0.0;
            }
        }
        
        event.RequestMore(); // trigger another idle event
    }
    
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::SetStatusBarText()
{
    wxString txt;
    if(this->is_running) txt << _("Running.");
    else txt << _("Stopped.");
    txt << _(" Timesteps: ") << this->system->GetTimestepsTaken();
    txt << _T("   (") << wxString::Format(_T("%.0f"),this->frames_per_second)
        << _(" computed frames per second, ")
        << wxString::Format(_T("%.0f"),this->million_cell_generations_per_second)
        << _T(" mcgs)");
    /* DEBUG
    txt << wxString::Format(_T("  num_steps=%d"), num_steps)
        << wxString::Format(_T("  tpr=%d"), this->render_settings.GetProperty("timesteps_per_render").GetInt());
    */
    SetStatusText(txt);
}

// ---------------------------------------------------------------------

void MyFrame::OnRestoreDefaultPerspective(wxCommandEvent& event)
{
    this->aui_mgr.LoadPerspective(this->default_perspective);
}

// ---------------------------------------------------------------------

void MyFrame::OnGenerateInitialPattern(wxCommandEvent& event)
{
    try
    {
        this->system->GenerateInitialPattern();
    }
    catch(const exception& e)
    {
        MonospaceMessageBox(_("Generating an initial pattern caused an error:\n\n")+wxString(e.what(),wxConvUTF8),_("Error"),wxART_ERROR);
    }
    catch(...)
    {
        wxMessageBox(_("Generating an initial pattern caused an unknown error"));
    }
    // (we allow the user to proceed because they might now want to change other things to match)

    this->is_running = false;
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectOpenCLDevice(wxCommandEvent& event)
{
    // TODO: merge this with GetOpenCL diagnostics?
    wxArrayString choices;
    int iOldSelection;
    int np;
    try 
    {
        np = OpenCL_utils::GetNumberOfPlatforms();
    }
    catch(const exception& e)
    {
        wxMessageBox(_("OpenCL not available: ")+
            wxString(e.what(),wxConvUTF8));
        return;
    }
    catch(...)
    {
        wxMessageBox(_("OpenCL not available"));
        return;
    }
    for(int ip=0;ip<np;ip++)
    {
        int nd = OpenCL_utils::GetNumberOfDevices(ip);
        for(int id=0;id<nd;id++)
        {
            if(ip==opencl_platform && id==opencl_device)
                iOldSelection = (int)choices.size();
            wxString s(OpenCL_utils::GetPlatformDescription(ip).c_str(),wxConvUTF8);
            s << _T(" : ") << wxString(OpenCL_utils::GetDeviceDescription(ip,id).c_str(),wxConvUTF8);
            choices.Add(s);
        }
    }
    wxSingleChoiceDialog dlg(this,_("Select the OpenCL device to use:"),_("Select OpenCL device"),
        choices);
    dlg.SetSelection(iOldSelection);
    if(dlg.ShowModal()!=wxID_OK) return;
    int iNewSelection = dlg.GetSelection();
    if(iNewSelection != iOldSelection)
        wxMessageBox(_("The selected device will be used the next time an OpenCL pattern is loaded."));
    int dc = 0;
    for(int ip=0;ip<np;ip++)
    {
        int nd = OpenCL_utils::GetNumberOfDevices(ip);
        if(iNewSelection < nd)
        {
            opencl_platform = ip;
            opencl_device = iNewSelection;
            break;
        }
        iNewSelection -= nd;
    }
    // TODO: hot-change the current RD system
}

// ---------------------------------------------------------------------

void MyFrame::OnHelp(wxCommandEvent& event)
{
    int id = event.GetId();
    switch (id)
    {
        case wxID_HELP:         this->help_panel->ShowHelp(_("Help/index.html")); break;
        case ID::HelpQuick:     this->help_panel->ShowHelp(_("Help/quickstart.html")); break;
        case ID::HelpIntro:     this->help_panel->ShowHelp(_("Help/introduction.html")); break;
        case ID::HelpTips:      this->help_panel->ShowHelp(_("Help/tips.html")); break;
        case ID::HelpKeyboard:  this->help_panel->ShowHelp(SHOW_KEYBOARD_SHORTCUTS); break;
        case ID::HelpMouse:     this->help_panel->ShowHelp(_("Help/mouse.html")); break;
        case ID::HelpFile:      this->help_panel->ShowHelp(_("Help/file.html")); break;
        case ID::HelpEdit:      this->help_panel->ShowHelp(_("Help/edit.html")); break;
        case ID::HelpView:      this->help_panel->ShowHelp(_("Help/view.html")); break;
        case ID::HelpAction:    this->help_panel->ShowHelp(_("Help/action.html")); break;
        case ID::HelpHelp:      this->help_panel->ShowHelp(_("Help/help.html")); break;
        case ID::HelpFormats:   this->help_panel->ShowHelp(_("Help/formats.html")); break;
        case ID::HelpProblems:  this->help_panel->ShowHelp(_("Help/problems.html")); break;
        case ID::HelpChanges:   this->help_panel->ShowHelp(_("Help/changes.html")); break;
        case ID::HelpCredits:   this->help_panel->ShowHelp(_("Help/credits.html")); break;
        default:
            wxMessageBox(_("Bug: Unexpected ID in OnHelp!"));
            return;
    }
    
    wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(ID::HelpPane));
    if(pane.IsOk() && !pane.IsShown()) {
        pane.Show();
        this->aui_mgr.Update();
    }
}

// ---------------------------------------------------------------------

wxString MyFrame::SavePatternDialog()
{
    wxString filename = wxEmptyString;
    wxString currname = this->system->GetFilename();
    currname = currname.AfterLast(wxFILE_SEP_PATH);

    wxString extension(this->system->GetFileExtension().c_str(),wxConvUTF8);
    wxString extension_description = _("Extended VTK files (*.")+extension +_T(")|*.")+extension;
    
    wxFileDialog savedlg(this, _("Specify the pattern filename"), opensavedir, currname,
                         extension_description,
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    #ifdef __WXGTK__
        // opensavedir is ignored above (bug in wxGTK 2.8.0???)
        savedlg.SetDirectory(opensavedir);
    #endif
    if(savedlg.ShowModal() == wxID_OK) {
        wxFileName fullpath( savedlg.GetPath() );
        opensavedir = fullpath.GetPath();
        filename = savedlg.GetPath();
    }
    
    return filename;
}

// ---------------------------------------------------------------------

void MyFrame::OnSavePattern(wxCommandEvent& event)
{
    wxString filename = SavePatternDialog();
    if(!filename.empty()) SaveFile(filename);
}

// ---------------------------------------------------------------------

void MyFrame::SaveFile(const wxString& path)
{
    wxBusyCursor busy;

    this->system->SaveFile(path.mb_str(),this->render_settings);

    AddRecentPattern(path);
    this->system->SetFilename(string(path.mb_str()));
    this->system->SetModified(false);
    this->UpdateWindowTitle();
}

// ---------------------------------------------------------------------

void MyFrame::OnNewPattern(wxCommandEvent& event)
{
    this->InitializeDefaultRenderSettings();
    if(this->system == NULL) {
        // initial call from MyFrame::MyFrame
        GrayScottImageRD *s = new GrayScottImageRD();
        s->SetDimensionsAndNumberOfChemicals(30,25,20,2);
        s->SetModified(false);
        s->SetFilename("untitled");
        s->GenerateInitialPattern();
        this->SetCurrentRDSystem(s);
        return;
    }

    if(UserWantsToCancelWhenAskedIfWantsToSave()) return;

    // ask user what type of dataset to generate:
    int sel;
    {
        const int N_CHOICES = 11;
        wxString dataset_types[N_CHOICES] = { _("1D image strip"), _("2D image"), _("3D image volume"), 
            _("Geodesic sphere"), _("Torus"), _("Tetrahedral mesh"), _("Triangular mesh"), _("Hexagonal mesh"),
            _("Rhombille tiling"), _("Penrose tiling (rhombi)"), _("Penrose tiling (darts and kites)")  };
        wxSingleChoiceDialog dlg(this,_("Select a pattern type:"),_("New Pattern"),N_CHOICES,dataset_types);
        dlg.SetSelection(1); // default selection
        if(dlg.ShowModal()!=wxID_OK) return;
        sel = dlg.GetSelection();
    }
    AbstractRD *sys;
    try 
    {
        switch(sel)
        {
            case 0: // 1D image
            {
				ImageRD *image_sys;
				if(this->is_opencl_available)
					image_sys = new FormulaOpenCLImageRD(opencl_platform,opencl_device);
				else
					image_sys = new GrayScottImageRD();
                image_sys->SetDimensionsAndNumberOfChemicals(128,1,1,2);
                sys = image_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                wxMessageBox(_("Created a 128x1x1 image. The dimensions can be edited in the Info Pane."));
                break;
            }
            case 1: // 2D image
            {
				ImageRD *image_sys;
				if(this->is_opencl_available)
					image_sys = new FormulaOpenCLImageRD(opencl_platform,opencl_device);
				else
					image_sys = new GrayScottImageRD();
                image_sys->SetDimensionsAndNumberOfChemicals(128,128,1,2);
                sys = image_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                wxMessageBox(_("Created a 128x128x1 image. The dimensions can be edited in the Info Pane."));
                break;
            }
            case 2: // 3D image
            {
				ImageRD *image_sys;
				if(this->is_opencl_available)
					image_sys = new FormulaOpenCLImageRD(opencl_platform,opencl_device);
				else
					image_sys = new GrayScottImageRD();
                image_sys->SetDimensionsAndNumberOfChemicals(32,32,32,2);
                sys = image_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                wxMessageBox(_("Created a 32x32x32 image. The dimensions can be edited in the Info Pane."));
                break;
            }
            case 3: // geodesic sphere
            {
                int divs;
                {
                    const int N_CHOICES=9;
                    int div_choices[N_CHOICES] = {2,3,4,5,6,7,8,9,10};
                    wxString div_descriptions[N_CHOICES];
                    for(int i=0;i<N_CHOICES;i++)
                        div_descriptions[i] = wxString::Format("%d subdivisions - %d cells",div_choices[i],20<<(div_choices[i]*2));
                    wxSingleChoiceDialog dlg(this,_("Select the number of subdivisions:"),_("Geodesic sphere options"),N_CHOICES,div_descriptions);
                    dlg.SetSelection(0); // default selection
                    if(dlg.ShowModal()!=wxID_OK) return;
                    divs = div_choices[dlg.GetSelection()];
                }
                wxBusyCursor busy;
                vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
                MeshGenerators::GetGeodesicSphere(divs,mesh,2);
                MeshRD *mesh_sys;
                if(this->is_opencl_available)
					mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
				else
					mesh_sys = new GrayScottMeshRD();
                mesh_sys->CopyFromMesh(mesh);
                sys = mesh_sys;
                this->render_settings.GetProperty("slice_3D").SetBool(false);
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                break;
            }
            case 4: // torus
            {
                int nx,ny;
                {
                    const int N_CHOICES=4;
                    int x_choices[N_CHOICES] = {100,160,200,500};
                    int y_choices[N_CHOICES] = {125,200,250,625};
                    int cells[N_CHOICES] = {12500,32000,50000,312500};
                    wxString div_descriptions[N_CHOICES];
                    for(int i=0;i<N_CHOICES;i++)
                        div_descriptions[i] = wxString::Format("%dx%d - %d cells",x_choices[i],y_choices[i],cells[i]);
                    wxSingleChoiceDialog dlg(this,_("Select the resolution:"),_("Torus tiling options"),N_CHOICES,div_descriptions);
                    dlg.SetSelection(2); // default selection
                    if(dlg.ShowModal()!=wxID_OK) return;
                    nx = x_choices[dlg.GetSelection()];
                    ny = y_choices[dlg.GetSelection()];
                }
                vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
                MeshGenerators::GetTorus(nx,ny,mesh,2);
                MeshRD *mesh_sys;
                if(this->is_opencl_available)
					mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
				else
					mesh_sys = new GrayScottMeshRD();
                mesh_sys->CopyFromMesh(mesh);
                sys = mesh_sys;
                this->render_settings.GetProperty("slice_3D").SetBool(false);
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                break;
            }
            case 5: // tetrahedral mesh
            {
                int npts;
                {
                    const int N_CHOICES=5;
                    int choices[N_CHOICES] = {500,1000,1500,2000,5000};
                    wxString div_descriptions[N_CHOICES];
                    for(int i=0;i<N_CHOICES;i++)
                        div_descriptions[i] = wxString::Format("%d points - approximately %d cells",choices[i],choices[i]*6);
                    wxSingleChoiceDialog dlg(this,_("Select the number of points:"),_("Tetrahedral mesh options"),N_CHOICES,div_descriptions);
                    dlg.SetSelection(1); // default selection
                    if(dlg.ShowModal()!=wxID_OK) return;
                    npts = choices[dlg.GetSelection()];
                }
                vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
                MeshGenerators::GetTetrahedralMesh(npts,mesh,2);
                MeshRD *mesh_sys;
                if(this->is_opencl_available)
					mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
				else
					mesh_sys = new GrayScottMeshRD();
                mesh_sys->CopyFromMesh(mesh);
                sys = mesh_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                this->render_settings.GetProperty("slice_3D_axis").SetAxis("y");
                break;
            }
            case 6: // triangular mesh
            {
                int n;
                {
                    const int N_CHOICES=5;
                    int choices[N_CHOICES] = {30,50,100,200,500};
                    int cells[N_CHOICES] = {1682,4802,19602,79202,498002};
                    wxString div_descriptions[N_CHOICES];
                    for(int i=0;i<N_CHOICES;i++)
                        div_descriptions[i] = wxString::Format("%dx%d - %d cells",choices[i],choices[i],cells[i]);
                    wxSingleChoiceDialog dlg(this,_("Select the grid size:"),_("Triangular mesh options"),N_CHOICES,div_descriptions);
                    dlg.SetSelection(1); // default selection
                    if(dlg.ShowModal()!=wxID_OK) return;
                    n = choices[dlg.GetSelection()];
                }
                vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
                MeshGenerators::GetTriangularMesh(n,n,mesh,2);
                MeshRD *mesh_sys;
                if(this->is_opencl_available)
					mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
				else
					mesh_sys = new GrayScottMeshRD();
                mesh_sys->CopyFromMesh(mesh);
                sys = mesh_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                this->render_settings.GetProperty("slice_3D").SetBool(false);
                this->render_settings.GetProperty("show_cell_edges").SetBool(true);
                this->render_settings.GetProperty("use_image_interpolation").SetBool(false);
                break;
            }
            case 7: // hexagonal mesh
            {
                int n;
                {
                    const int N_CHOICES=4;
                    int choices[N_CHOICES] = {100,150,200,500};
                    int cells[N_CHOICES] = {3185,7326,13068,82668};
                    wxString div_descriptions[N_CHOICES];
                    for(int i=0;i<N_CHOICES;i++)
                        div_descriptions[i] = wxString::Format("%dx%d - %d cells",choices[i],choices[i],cells[i]);
                    wxSingleChoiceDialog dlg(this,_("Select the grid size:"),_("Hexagonal mesh options"),N_CHOICES,div_descriptions);
                    dlg.SetSelection(0); // default selection
                    if(dlg.ShowModal()!=wxID_OK) return;
                    n = choices[dlg.GetSelection()];
                }
                vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
                MeshGenerators::GetHexagonalMesh(n,n,mesh,2);
                MeshRD *mesh_sys;
                if(this->is_opencl_available)
					mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
				else
					mesh_sys = new GrayScottMeshRD();
                mesh_sys->CopyFromMesh(mesh);
                sys = mesh_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                this->render_settings.GetProperty("slice_3D").SetBool(false);
                this->render_settings.GetProperty("show_cell_edges").SetBool(true);
                this->render_settings.GetProperty("use_image_interpolation").SetBool(false);
                break;
            }
            case 8: // rhombille tiling
            {
                int n;
                {
                    const int N_CHOICES=5;
                    int choices[N_CHOICES] = {50,75,100,150,200};
                    int cells[N_CHOICES] = {2304,5367,9555,21978,39204};
                    wxString div_descriptions[N_CHOICES];
                    for(int i=0;i<N_CHOICES;i++)
                        div_descriptions[i] = wxString::Format("%dx%d - %d cells",choices[i],choices[i],cells[i]);
                    wxSingleChoiceDialog dlg(this,_("Select the grid size:"),_("Rhombille mesh options"),N_CHOICES,div_descriptions);
                    dlg.SetSelection(0); // default selection
                    if(dlg.ShowModal()!=wxID_OK) return;
                    n = choices[dlg.GetSelection()];
                }
                vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
                MeshGenerators::GetRhombilleTiling(n,n,mesh,2);
                MeshRD *mesh_sys;
                if(this->is_opencl_available)
					mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
				else
					mesh_sys = new GrayScottMeshRD();
                mesh_sys->CopyFromMesh(mesh);
                sys = mesh_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                this->render_settings.GetProperty("slice_3D").SetBool(false);
                this->render_settings.GetProperty("show_cell_edges").SetBool(true);
                this->render_settings.GetProperty("use_image_interpolation").SetBool(false);
                break;
            }
            case 9: // Penrose rhombi tiling
            {
                int divs;
                {
                    const int N_CHOICES=5;
                    int div_choices[N_CHOICES] = {5,7,8,9,10};
                    int cells[N_CHOICES] = {430,3010,7920,20800,54560};
                    wxString div_descriptions[N_CHOICES];
                    for(int i=0;i<N_CHOICES;i++)
                        div_descriptions[i] = wxString::Format("%d subdivisions - %d cells",div_choices[i],cells[i]);
                    wxSingleChoiceDialog dlg(this,_("Select the number of subdivisions:"),_("Penrose tiling options"),N_CHOICES,div_descriptions);
                    dlg.SetSelection(1); // default selection
                    if(dlg.ShowModal()!=wxID_OK) return;
                    divs = div_choices[dlg.GetSelection()];
                }
                vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
                MeshGenerators::GetPenroseTiling(divs,0,mesh,2);
                MeshRD *mesh_sys;
                if(this->is_opencl_available)
					mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
				else
					mesh_sys = new GrayScottMeshRD();
                mesh_sys->CopyFromMesh(mesh);
                sys = mesh_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                this->render_settings.GetProperty("slice_3D").SetBool(false);
                this->render_settings.GetProperty("show_cell_edges").SetBool(true);
                this->render_settings.GetProperty("use_image_interpolation").SetBool(false);
                break;
            }
            case 10: // Penrose darts and kites tiling
            {
                int divs;
                {
                    const int N_CHOICES=5;
                    int div_choices[N_CHOICES] = {5,6,7,8,9};
                    int cells[N_CHOICES] = {705,1855,4885,12845,33705};
                    wxString div_descriptions[N_CHOICES];
                    for(int i=0;i<N_CHOICES;i++)
                        div_descriptions[i] = wxString::Format("%d subdivisions - %d cells",div_choices[i],cells[i]);
                    wxSingleChoiceDialog dlg(this,_("Select the number of subdivisions:"),_("Penrose tiling options"),N_CHOICES,div_descriptions);
                    dlg.SetSelection(2); // default selection
                    if(dlg.ShowModal()!=wxID_OK) return;
                    divs = div_choices[dlg.GetSelection()];
                }
                vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
                MeshGenerators::GetPenroseTiling(divs,1,mesh,2);
                MeshRD *mesh_sys;
                if(this->is_opencl_available)
					mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
				else
					mesh_sys = new GrayScottMeshRD();
                mesh_sys->CopyFromMesh(mesh);
                sys = mesh_sys;
                this->render_settings.GetProperty("active_chemical").SetChemical("b");
                this->render_settings.GetProperty("slice_3D").SetBool(false);
                this->render_settings.GetProperty("show_cell_edges").SetBool(true);
                this->render_settings.GetProperty("use_image_interpolation").SetBool(false);
                wxMessageBox(_("There's a problem with rendering concave polygons in OpenGL, so the display might be slightly corrupted."));
                break;
            }
            default:
            {
                wxMessageBox(_("Not currently supported"));
                return;
            }
        }
    }
    catch(const exception& e)
    {
        wxString message = _("Failed to create new pattern. Error:\n\n");
        message += wxString(e.what(),wxConvUTF8);
        MonospaceMessageBox(message,_("Error creating new pattern"),wxART_ERROR);
        return;
    }
    catch(...)
    {
        wxString message = _("Failed to create new pattern.");
        MonospaceMessageBox(message,_("Error creating pattern"),wxART_ERROR);
        return;
    }
    
	sys->CreateDefaultInitialPatternGenerator();
	sys->GenerateInitialPattern();
	this->SetCurrentRDSystem(sys);

    this->system->SetFilename("untitled");
    this->system->SetModified(false);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnOpenPattern(wxCommandEvent& event)
{
    wxFileDialog opendlg(this, _("Choose a pattern file"), opensavedir, wxEmptyString,
                         _("Extended VTK files (*.vti;*.vtu)|*.vti;*.vtu"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    #ifdef __WXGTK__
        // opensavedir is ignored above (bug in wxGTK 2.8.x???)
        opendlg.SetDirectory(opensavedir);
    #endif
    if(opendlg.ShowModal() == wxID_OK) {
        wxFileName fullpath( opendlg.GetPath() );
        opensavedir = fullpath.GetPath();
        OpenFile( opendlg.GetPath() );
    }
}

// ---------------------------------------------------------------------

void MyFrame::OpenFile(const wxString& path, bool remember)
{
    if (IsHTMLFile(path)) {
        // show HTML file in help pane
        this->help_panel->ShowHelp(path);
    
        wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(ID::HelpPane));
        if(pane.IsOk() && !pane.IsShown()) {
            pane.Show();
            this->aui_mgr.Update();
        }
        
        return;
    }
    
    if (IsTextFile(path)) {
        // open text file in user's preferred text editor
        EditFile(path);
        return;
    }

    if(!wxFileExists(path))
    {
        wxMessageBox(_("File doesn't exist: ")+path);
        return;
    }

    if(UserWantsToCancelWhenAskedIfWantsToSave()) return;

    if(remember) AddRecentPattern(path);
    
    wxBeginBusyCursor();

    // load pattern file
    bool warn_to_update = false;
    AbstractRD *target_system = NULL;
    try
    {
        // get the VTK data type from the file
        vtkSmartPointer<vtkXMLGenericDataObjectReader> generic_reader = vtkSmartPointer<vtkXMLGenericDataObjectReader>::New();
        bool parallel;
        int data_type = generic_reader->ReadOutputType(path.mb_str(),parallel);

        if( data_type == VTK_IMAGE_DATA )
        {
            vtkSmartPointer<RD_XMLImageReader> reader = vtkSmartPointer<RD_XMLImageReader>::New();
            reader->SetFileName(path.mb_str());
            reader->Update();
            vtkImageData *image = reader->GetOutput();

            string type = reader->GetType();
            string name = reader->GetName();

            ImageRD* image_system;
            if(type=="inbuilt")
            {
                if(name=="Gray-Scott")
                    image_system = new GrayScottImageRD();
                else 
                    throw runtime_error("Unsupported inbuilt implementation: "+name);
            }
            else if(type=="formula")
            {
                if(!this->is_opencl_available) 
                    throw runtime_error(this->opencl_not_available_message);
                image_system = new FormulaOpenCLImageRD(opencl_platform,opencl_device);
            }
            else if(type=="kernel")
            {
                if(!this->is_opencl_available) 
                    throw runtime_error(this->opencl_not_available_message);
                image_system = new FullKernelOpenCLImageRD(opencl_platform,opencl_device);
            }
            else throw runtime_error("Unsupported rule type: "+type);
            image_system->InitializeFromXML(reader->GetRDElement(),warn_to_update);

            // render settings
            this->InitializeDefaultRenderSettings();
            vtkSmartPointer<vtkXMLDataElement> xml_render_settings = reader->GetRDElement()->FindNestedElementWithName("render_settings");
            if(xml_render_settings) // optional
                this->render_settings.OverwriteFromXML(xml_render_settings);

            int dim[3];
            image->GetDimensions(dim);
            int nc = image->GetNumberOfScalarComponents() * image->GetPointData()->GetNumberOfArrays();
            image_system->SetDimensions(dim[0],dim[1],dim[2]);
            image_system->SetNumberOfChemicals(nc);
            if(reader->ShouldGenerateInitialPatternWhenLoading())
                image_system->GenerateInitialPattern();
            else
                image_system->CopyFromImage(image);
            target_system = image_system;
        }
        else if( data_type == VTK_UNSTRUCTURED_GRID )
        {
            vtkSmartPointer<RD_XMLUnstructuredGridReader> reader = vtkSmartPointer<RD_XMLUnstructuredGridReader>::New();
            reader->SetFileName(path.mb_str());
            reader->Update();
            vtkUnstructuredGrid *ugrid = reader->GetOutput();

            string type = reader->GetType();
            string name = reader->GetName();

            MeshRD* mesh_system;
            if(type=="inbuilt")
            {
                if(name=="Gray-Scott")
                    mesh_system = new GrayScottMeshRD();
                else 
                    throw runtime_error("Unsupported inbuilt implementation: "+name);
            }
            else if(type=="formula")
            {
                if(!this->is_opencl_available) 
                    throw runtime_error(this->opencl_not_available_message);
                mesh_system = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
            }
            else if(type=="kernel")
            {
                if(!this->is_opencl_available) 
                    throw runtime_error(this->opencl_not_available_message);
                mesh_system = new FullKernelOpenCLMeshRD(opencl_platform,opencl_device);
            }
            else throw runtime_error("Unsupported rule type: "+type);

            mesh_system->InitializeFromXML(reader->GetRDElement(),warn_to_update);
            mesh_system->CopyFromMesh(ugrid);
            // render settings
            this->InitializeDefaultRenderSettings();
            vtkSmartPointer<vtkXMLDataElement> xml_render_settings = reader->GetRDElement()->FindNestedElementWithName("render_settings");
            if(xml_render_settings) // optional
                this->render_settings.OverwriteFromXML(xml_render_settings);

            if(reader->ShouldGenerateInitialPatternWhenLoading())
                mesh_system->GenerateInitialPattern();

            target_system = mesh_system;
        }
        else
        {
            ostringstream oss;
            oss << "Unsupported data type: " << data_type;
            throw runtime_error(oss.str());
        }
        target_system->SetFilename(string(path.mb_str())); // TODO: display filetitle only (user option?)
        target_system->SetModified(false);
        this->SetCurrentRDSystem(target_system);
    }
    catch(const exception& e)
    {
        wxEndBusyCursor();
        wxString message = warn_to_update ? _("This file is from a more recent version of Ready. You should download a newer version.\n\n") : _("");
        message += _("Failed to open file. Error:\n\n");
        message += wxString(e.what(),wxConvUTF8);
        MonospaceMessageBox(message,_("Error reading file"),wxART_ERROR);
        delete target_system;
        return;
    }
    catch(...)
    {
        wxEndBusyCursor();
        wxString message = warn_to_update ? _("This file is from a more recent version of Ready. You should download a newer version.\n\n") : _("");
        message += _("Failed to open file.");
        MonospaceMessageBox(message,_("Error reading file"),wxART_ERROR);
        delete target_system;
        return;
    }
    wxEndBusyCursor();
    if(warn_to_update)
    {
        wxMessageBox("This file is from a more recent version of Ready. For best results you should download a newer version.");
        // TODO: allow user to stop this message from appearing every time
    }
    this->system->SetFilename(string(path.mb_str()));
}

// ---------------------------------------------------------------------

void MyFrame::OnOpenRecent(wxCommandEvent& event)
{
    int id = event.GetId();
    if (id == ID::ClearMissingPatterns) {
        ClearMissingPatterns();
    } else if (id == ID::ClearAllPatterns) {
        ClearAllPatterns();
    } else if ( id > ID::OpenRecent && id <= ID::OpenRecent + numpatterns ) {
        OpenRecentPattern(id);
    } else {
        event.Skip();
    }
}

// ---------------------------------------------------------------------

void MyFrame::AddRecentPattern(const wxString& inpath)
{
    if (inpath.IsEmpty()) return;
    wxString path = inpath;
    if (path.StartsWith(readydir)) {
        // remove readydir from start of path
        path.erase(0, readydir.length());
    }

    // duplicate any ampersands so they appear in menu
    path.Replace(wxT("&"), wxT("&&"));

    // put given path at start of patternSubMenu
    #ifdef __WXGTK__
        // avoid wxGTK bug in FindItem if path contains underscores
        int id = wxNOT_FOUND;
        for (int i = 0; i < numpatterns; i++) {
            wxMenuItem* item = patternSubMenu->FindItemByPosition(i);
            wxString temp = item->GetText();
            temp.Replace(wxT("__"), wxT("_"));
            temp.Replace(wxT("&"), wxT("&&"));
            if (temp == path) {
                id = ID::OpenRecent + 1 + i;
                break;
            }
        }
    #else
        int id = patternSubMenu->FindItem(path);
    #endif
    if ( id == wxNOT_FOUND ) {
        if ( numpatterns < maxpatterns ) {
            // add new path
            numpatterns++;
            id = ID::OpenRecent + numpatterns;
            patternSubMenu->Insert(numpatterns - 1, id, path);
        } else {
            // replace last item with new path
            wxMenuItem* item = patternSubMenu->FindItemByPosition(maxpatterns - 1);
            item->SetText(path);
            id = ID::OpenRecent + maxpatterns;
        }
    }
    
    // path exists in patternSubMenu
    if ( id > ID::OpenRecent + 1 ) {
        // move path to start of menu
        wxMenuItem* item;
        while ( id > ID::OpenRecent + 1 ) {
            wxMenuItem* previtem = patternSubMenu->FindItem(id - 1);
            wxString prevpath = previtem->GetText();
            #ifdef __WXGTK__
                // remove duplicate underscores
                prevpath.Replace(wxT("__"), wxT("_"));
                prevpath.Replace(wxT("&"), wxT("&&"));
            #endif
            item = patternSubMenu->FindItem(id);
            item->SetText(prevpath);
            id--;
        }
        item = patternSubMenu->FindItem(id);
        item->SetText(path);
    }
    
    wxMenuBar* mbar = GetMenuBar();
    if (mbar) mbar->Enable(ID::OpenRecent, numpatterns > 0);
}

// ---------------------------------------------------------------------

void MyFrame::OpenRecentPattern(int id)
{
    wxMenuItem* item = patternSubMenu->FindItem(id);
    if (item) {
        wxString path = item->GetText();
        #ifdef __WXGTK__
            // remove duplicate underscores
            path.Replace(wxT("__"), wxT("_"));
        #endif
        // remove duplicate ampersands
        path.Replace(wxT("&&"), wxT("&"));

        // if path isn't absolute then prepend Ready directory
        wxFileName fname(path);
        if (!fname.IsAbsolute()) path = readydir + path;

        OpenFile(path);
    }
}

// ---------------------------------------------------------------------

void MyFrame::ClearMissingPatterns()
{
    int pos = 0;
    while (pos < numpatterns) {
        wxMenuItem* item = patternSubMenu->FindItemByPosition(pos);
        wxString path = item->GetText();
        #ifdef __WXGTK__
            // remove duplicate underscores
            path.Replace(wxT("__"), wxT("_"));
        #endif
        // remove duplicate ampersands
        path.Replace(wxT("&&"), wxT("&"));

        // if path isn't absolute then prepend Ready directory
        wxFileName fname(path);
        if (!fname.IsAbsolute()) path = readydir + path;

        if (wxFileExists(path)) {
            // keep this item
            pos++;
        } else {
            // remove this item by shifting up later items
            int nextpos = pos + 1;
            while (nextpos < numpatterns) {
                wxMenuItem* nextitem = patternSubMenu->FindItemByPosition(nextpos);
                #ifdef __WXGTK__
                    // avoid wxGTK bug if item contains underscore
                    wxString temp = nextitem->GetText();
                    temp.Replace(wxT("__"), wxT("_"));
                    temp.Replace(wxT("&"), wxT("&&"));
                    item->SetText( temp );
                #else
                    item->SetText( nextitem->GetText() );
                #endif
                item = nextitem;
                nextpos++;
            }
            // delete last item
            patternSubMenu->Delete(item);
            numpatterns--;
        }
    }
    wxMenuBar* mbar = GetMenuBar();
    if (mbar) mbar->Enable(ID::OpenRecent, numpatterns > 0);
}

// ---------------------------------------------------------------------

void MyFrame::ClearAllPatterns()
{
    while (numpatterns > 0) {
        patternSubMenu->Delete( patternSubMenu->FindItemByPosition(0) );
        numpatterns--;
    }
    wxMenuBar* mbar = GetMenuBar();
    if (mbar) mbar->Enable(ID::OpenRecent, false);
}

// ---------------------------------------------------------------------

void MyFrame::EditFile(const wxString& path)
{
    // prompt user if text editor hasn't been set yet
    if (texteditor.IsEmpty()) {
        ChooseTextEditor(this, texteditor);
        if (texteditor.IsEmpty()) return;
    }
    
    // execute a command to open given file in user's preferred text editor
    wxString cmd = wxString::Format(wxT("\"%s\" \"%s\""), texteditor.c_str(), path.c_str());
    long result = wxExecute(cmd, wxEXEC_ASYNC);
    
#if defined(__WXMSW__)
    // on Windows, wxExecute returns 0 if cmd fails
    if (result == 0)
#elif defined(__WXMAC__)
    // on Mac, wxExecute returns -1 if cmd succeeds (bug, or wx docs are wrong)
    if (result != -1)
#elif defined(__WXGTK__)
    // on Linux, wxExecute always returns a +ve number (pid?) if cmd fails OR succeeds (sheesh!)
    // but if it fails an error message appears in shell window
    if (result <= 0)
#endif
    {
        wxString msg = _("Failed to open file in your preferred text editor.\n");
        msg += _("Try choosing a different editor in Preferences > File.");
        Warning(msg);
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnChangeActiveChemical(wxCommandEvent& event)
{
    wxArrayString choices;
    for(int i=0;i<this->system->GetNumberOfChemicals();i++)
        choices.Add(GetChemicalName(i));
    wxSingleChoiceDialog dlg(this,_("Select the chemical to render:"),_("Select active chemical"),
        choices);
    dlg.SetSelection(IndexFromChemicalName(this->render_settings.GetProperty("active_chemical").GetChemical()));
    if(dlg.ShowModal()!=wxID_OK) return;
    this->render_settings.GetProperty("active_chemical").SetChemical(GetChemicalName(dlg.GetSelection()));
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->render_settings,false);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::SetRuleName(string s)
{
    this->system->SetRuleName(s);
    this->UpdateWindowTitle();
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

void MyFrame::SetDescription(string s)
{
    this->system->SetDescription(s);
    this->UpdateWindowTitle();
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

void MyFrame::SetParameter(int iParam,float val)
{
    this->system->SetParameterValue(iParam,val);
    this->UpdateWindowTitle();
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

void MyFrame::SetParameterName(int iParam,std::string s)
{
    this->system->SetParameterName(iParam,s);
    this->UpdateWindowTitle();
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

void MyFrame::SetFormula(std::string s)
{
    this->system->SetFormula(s);
    this->UpdateWindowTitle();
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

bool MyFrame::UserWantsToCancelWhenAskedIfWantsToSave()
{
    if(this->system == NULL || !this->system->IsModified()) return false;
    
    int ret = SaveChanges(_("Save the current system?"),_("If you don't save, your changes will be lost."));
    if(ret==wxCANCEL) return true;
    if(ret==wxNO) return false;

    // ret == wxYES
    wxString filename = SavePatternDialog();
    if(filename.empty()) return true; // user cancelled

    SaveFile(filename);
    return false;
}

// ---------------------------------------------------------------------

void MyFrame::OnClose(wxCloseEvent& event)
{
    if(event.CanVeto() && this->UserWantsToCancelWhenAskedIfWantsToSave()) return;
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::ShowPrefsDialog(const wxString& page)
{
    if (ChangePrefs(page)) {
        // user hit OK button so might as well save prefs now
        SaveSettings();
    }
    // safer to update everything even if user hit Cancel
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnPreferences(wxCommandEvent& event)
{
    ShowPrefsDialog();
}

// ---------------------------------------------------------------------

void MyFrame::EnableAllMenus(bool enable)
{
    wxMenuBar* mbar = GetMenuBar();
    if (mbar) {
        int count = mbar->GetMenuCount();
        int i;
        for (i = 0; i < count; i++) {
            mbar->EnableTop(i, enable);
        }
        #ifdef __WXOSX_COCOA__
            // enable/disable items in app menu
            // AKT TODO!!! they fail to disable due to wxOSX-Cocoa bug
            mbar->Enable(wxID_ABOUT, enable);
            mbar->Enable(wxID_PREFERENCES, enable);
            mbar->Enable(wxID_EXIT, enable);
        #endif
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnActivate(wxActivateEvent& event)
{
    // we need to disable all menu items when frame becomes inactive
    // (eg. due to a modal dialog appearing) so that keys bound to menu items
    // get passed to wxTextCtrls
    EnableAllMenus(event.GetActive());
    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::UpdateMenuAccelerators()
{
    // keyboard shortcuts have changed, so update all menu item accelerators
    wxMenuBar* mbar = GetMenuBar();
    if (mbar) {
        // app menu (or file menu on Windows/Linux)
        // AKT TODO!!! wxOSX-Cocoa bug: these app menu items aren't updated
        // (but user isn't likely to change them so not critical)
        SetAccelerator(mbar, wxID_ABOUT,                    DO_ABOUT);
        SetAccelerator(mbar, wxID_PREFERENCES,              DO_PREFS);
        SetAccelerator(mbar, wxID_EXIT,                     DO_QUIT);
        
        // file menu
        SetAccelerator(mbar, wxID_NEW,                      DO_NEWPATT);
        SetAccelerator(mbar, wxID_OPEN,                     DO_OPENPATT);
        SetAccelerator(mbar, ID::ReloadFromDisk,            DO_RELOAD);
        SetAccelerator(mbar, ID::ImportMesh,                DO_IMPORTMESH);
        SetAccelerator(mbar, ID::ExportMesh,                DO_EXPORTMESH);
        SetAccelerator(mbar, ID::ExportImage,               DO_EXPORTIMAGE);
        SetAccelerator(mbar, wxID_SAVE,                     DO_SAVE);
        SetAccelerator(mbar, ID::Screenshot,                DO_SCREENSHOT);
        SetAccelerator(mbar, ID::RecordFrames,              DO_RECORDFRAMES);
        SetAccelerator(mbar, ID::AddMyPatterns,             DO_ADDPATTS);
        
        // edit menu
        SetAccelerator(mbar, wxID_UNDO,                     DO_UNDO);
        SetAccelerator(mbar, wxID_REDO,                     DO_REDO);
        SetAccelerator(mbar, wxID_CUT,                      DO_CUT);
        SetAccelerator(mbar, wxID_COPY,                     DO_COPY);
        SetAccelerator(mbar, wxID_PASTE,                    DO_PASTE);
        SetAccelerator(mbar, wxID_CLEAR,                    DO_CLEAR);
        SetAccelerator(mbar, wxID_SELECTALL,                DO_SELALL);
        SetAccelerator(mbar, ID::Pointer,                   DO_POINTER);
        SetAccelerator(mbar, ID::Pencil,                    DO_PENCIL);
        SetAccelerator(mbar, ID::Brush,                     DO_BRUSH);
        SetAccelerator(mbar, ID::Picker,                    DO_PICKER);
        
        // view menu
        SetAccelerator(mbar, ID::FullScreen,                DO_FULLSCREEN);
        SetAccelerator(mbar, ID::FitPattern,                DO_FIT);
        SetAccelerator(mbar, ID::Wireframe,                 DO_WIREFRAME);
        SetAccelerator(mbar, ID::PatternsPane,              DO_PATTERNS);
        SetAccelerator(mbar, ID::InfoPane,                  DO_INFO);
        SetAccelerator(mbar, ID::HelpPane,                  DO_HELP);
        SetAccelerator(mbar, ID::FileToolbar,               DO_FILETOOLBAR);
        SetAccelerator(mbar, ID::ActionToolbar,             DO_ACTIONTOOLBAR);
        SetAccelerator(mbar, ID::PaintToolbar,              DO_PAINTTOOLBAR);
        SetAccelerator(mbar, ID::RestoreDefaultPerspective, DO_RESTORE);
        SetAccelerator(mbar, ID::ChangeActiveChemical,      DO_CHEMICAL);
        
        // actions menu
        SetAccelerator(mbar, ID::Step1,                     DO_STEP1);
        SetAccelerator(mbar, ID::StepN,                     DO_STEPN);
        SetAccelerator(mbar, ID::RunStop,                   DO_RUNSTOP);
        SetAccelerator(mbar, ID::Faster,                    DO_FASTER);
        SetAccelerator(mbar, ID::Slower,                    DO_SLOWER);
        SetAccelerator(mbar, ID::ChangeRunningSpeed,        DO_CHANGESPEED);
        SetAccelerator(mbar, ID::Reset,                     DO_RESET);
        SetAccelerator(mbar, ID::GenerateInitialPattern,    DO_GENPATT);
        SetAccelerator(mbar, ID::Blank,                     DO_BLANK);
        SetAccelerator(mbar, ID::AddParameter,              DO_ADDPARAM);
        SetAccelerator(mbar, ID::DeleteParameter,           DO_DELPARAM);
        SetAccelerator(mbar, ID::ViewFullKernel,            DO_VIEWKERNEL);
        SetAccelerator(mbar, ID::SelectOpenCLDevice,        DO_DEVICE);
        SetAccelerator(mbar, ID::OpenCLDiagnostics,         DO_OPENCL);
    }
}

// ---------------------------------------------------------------------

void MyFrame::ProcessKey(int key, int modifiers)
{
    int cmdid = 0;
    action_info action = FindAction(key, modifiers);
    
    switch (action.id)
    {
        case DO_NOTHING:        // any unassigned key (including escape) turns off full screen mode
                                if (fullscreen) cmdid = ID::FullScreen; break;
        
        case DO_OPENFILE:       OpenFile(action.file);
                                return;
        
        // File menu
        case DO_NEWPATT:        cmdid = wxID_NEW; break;
        case DO_OPENPATT:       cmdid = wxID_OPEN; break;
        case DO_RELOAD:         cmdid = ID::ReloadFromDisk; break;
        case DO_IMPORTMESH:     cmdid = ID::ImportMesh; break;
        case DO_EXPORTMESH:     cmdid = ID::ExportMesh; break;
        case DO_EXPORTIMAGE:    cmdid = ID::ExportImage; break;
        case DO_SAVE:           cmdid = wxID_SAVE; break;
        case DO_SCREENSHOT:     cmdid = ID::Screenshot; break;
        case DO_RECORDFRAMES:   cmdid = ID::RecordFrames; break;
        case DO_ADDPATTS:       cmdid = ID::AddMyPatterns; break;
        case DO_PREFS:          cmdid = wxID_PREFERENCES; break;
        case DO_QUIT:           cmdid = wxID_EXIT; break;
        
        // Edit menu
        case DO_UNDO:           cmdid = wxID_UNDO; break;
        case DO_REDO:           cmdid = wxID_REDO; break;
        case DO_CUT:            cmdid = wxID_CUT; break;
        case DO_COPY:           cmdid = wxID_COPY; break;
        case DO_PASTE:          cmdid = wxID_PASTE; break;
        case DO_CLEAR:          cmdid = wxID_CLEAR; break;
        case DO_SELALL:         cmdid = wxID_SELECTALL; break;
        case DO_POINTER:        cmdid = ID::Pointer; break;
        case DO_PENCIL:         cmdid = ID::Pencil; break;
        case DO_BRUSH:          cmdid = ID::Brush; break;
        case DO_PICKER:         cmdid = ID::Picker; break;
        
        // View menu
        case DO_FULLSCREEN:     cmdid = ID::FullScreen; break;
        case DO_FIT:            cmdid = ID::FitPattern; break;
        case DO_WIREFRAME:      cmdid = ID::Wireframe; break;
        case DO_PATTERNS:       cmdid = ID::PatternsPane; break;
        case DO_INFO:           cmdid = ID::InfoPane; break;
        case DO_HELP:           cmdid = ID::HelpPane; break;
        case DO_FILETOOLBAR:    cmdid = ID::FileToolbar; break;
        case DO_ACTIONTOOLBAR:  cmdid = ID::ActionToolbar; break;
        case DO_PAINTTOOLBAR:   cmdid = ID::PaintToolbar; break;
        case DO_RESTORE:        cmdid = ID::RestoreDefaultPerspective; break;
        case DO_CHEMICAL:       cmdid = ID::ChangeActiveChemical; break;
        
        // Action menu
        case DO_STEP1:          cmdid = ID::Step1; break;
        case DO_STEPN:          cmdid = ID::StepN; break;
        case DO_RUNSTOP:        cmdid = ID::RunStop; break;
        case DO_FASTER:         cmdid = ID::Faster; break;
        case DO_SLOWER:         cmdid = ID::Slower; break;
        case DO_CHANGESPEED:    cmdid = ID::ChangeRunningSpeed; break;
        case DO_RESET:          cmdid = ID::Reset; break;
        case DO_GENPATT:        cmdid = ID::GenerateInitialPattern; break;
        case DO_BLANK:          cmdid = ID::Blank; break;
        case DO_ADDPARAM:       cmdid = ID::AddParameter; break;
        case DO_DELPARAM:       cmdid = ID::DeleteParameter; break;
        case DO_VIEWKERNEL:     cmdid = ID::ViewFullKernel; break;
        case DO_DEVICE:         cmdid = ID::SelectOpenCLDevice; break;
        case DO_OPENCL:         cmdid = ID::OpenCLDiagnostics; break;
        
        // Help menu
        case DO_ABOUT:          cmdid = wxID_ABOUT; break;
        
        default:                Warning(_("Bug detected in ProcessKey!"));
    }
   
    if (cmdid != 0) {
        wxCommandEvent cmdevent(wxEVT_COMMAND_MENU_SELECTED, cmdid);
        cmdevent.SetEventObject(this);
        this->GetEventHandler()->ProcessEvent(cmdevent);
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnKeyDown(wxKeyEvent& event)
{
    #ifdef __WXMAC__
        // close any open tool tip window (fixes wxMac bug?)
        wxToolTip::RemoveToolTips();
    #endif

    realkey = event.GetKeyCode();
    int mods = event.GetModifiers();

    // WARNING: logic must match that in KeyComboCtrl::OnKeyDown in prefs.cpp
    if (mods == wxMOD_NONE || realkey == WXK_ESCAPE || realkey > 127) {
        // tell OnChar handler to ignore realkey
        realkey = 0;
    }

    #ifdef __WXOSX__
        // pass ctrl/cmd-key combos directly to OnChar
        if (realkey > 0 && ((mods & wxMOD_CONTROL) || (mods & wxMOD_CMD))) {
            this->OnChar(event);
            return;
        }
    #endif
    
    #ifdef __WXMSW__
        // on Windows, OnChar is NOT called for some ctrl-key combos like
        // ctrl-0..9 or ctrl-alt-key, so we call OnChar ourselves
        if (realkey > 0 && (mods & wxMOD_CONTROL)) {
            this->OnChar(event);
            return;
        }
    #endif

    #ifdef __WXGTK__
        if (realkey == ' ' && mods == wxMOD_SHIFT) {
            // fix wxGTK bug (curiously, the bug isn't seen in the prefs dialog);
            // OnChar won't see the shift modifier, so set realkey to a special
            // value to tell OnChar that shift-space was pressed
            realkey = -666;
        }
    #endif

    event.Skip();
}

// ---------------------------------------------------------------------

void MyFrame::OnChar(wxKeyEvent& event)
{
    int key = event.GetKeyCode();
    int mods = event.GetModifiers();

    // WARNING: logic here must match that in KeyComboCtrl::OnChar in prefs.cpp
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
            if (key >= 'A' && key <= 'Z') key += 32;  // convert A..Z to a..z
        }
    }
    
    #ifdef __WXGTK__
        if (realkey == -666) {
            // OnKeyDown saw that shift-space was pressed but for some reason
            // OnChar doesn't see the modifier (ie. mods is wxMOD_NONE)
            key = ' ';
            mods = wxMOD_SHIFT;
        }
    #endif
    
    if (this->pVTKWindow->HasFocus()) {
        ProcessKey(key, mods);
        // don't call default handler (wxVTKRenderWindowInteractor::OnChar)
        return;
    }
    
    if (this->patterns_panel->TreeHasFocus()) {
        // process keyboard shortcut for patterns panel
        if (this->patterns_panel->DoKey(key, mods)) return;
        // else call default handler
        event.Skip();
        return;
    }
    
    if (this->info_panel->HtmlHasFocus()) {
        // process keyboard shortcut for info panel
        if (this->info_panel->DoKey(key, mods)) return;
        // else call default handler
        event.Skip();
        return;
    }
    
    if (this->help_panel->HtmlHasFocus()) {
        // process keyboard shortcut for help panel
        if (this->help_panel->DoKey(key, mods)) return;
        // else call default handler
        event.Skip();
        return;
    }
}

// ---------------------------------------------------------------------

void MyFrame::InitializeDefaultRenderSettings()
{
    this->render_settings.DeleteAllProperties();
    this->render_settings.AddProperty(Property("surface_color","color",1.0f,1.0f,1.0f)); // RGB [0,1]
    this->render_settings.AddProperty(Property("color_low","color",0.0f,0.0f,1.0f));
    this->render_settings.AddProperty(Property("color_high","color",1.0f,0.0f,0.0f));
    this->render_settings.AddProperty(Property("show_color_scale",true));
    this->render_settings.AddProperty(Property("show_multiple_chemicals",true));
    this->render_settings.AddProperty(Property("active_chemical","chemical","a"));
    this->render_settings.AddProperty(Property("low",0.0f));
    this->render_settings.AddProperty(Property("high",1.0f));
    this->render_settings.AddProperty(Property("vertical_scale_1D",30.0f));
    this->render_settings.AddProperty(Property("vertical_scale_2D",15.0f));
    this->render_settings.AddProperty(Property("contour_level",0.25f));
    this->render_settings.AddProperty(Property("use_wireframe",false));
    this->render_settings.AddProperty(Property("show_cell_edges",false));
    this->render_settings.AddProperty(Property("show_bounding_box",true));
    this->render_settings.AddProperty(Property("slice_3D",true));
    this->render_settings.AddProperty(Property("slice_3D_axis","axis","z"));
    this->render_settings.AddProperty(Property("slice_3D_position",0.5f)); // [0,1]
    this->render_settings.AddProperty(Property("show_displacement_mapped_surface",true));
    this->render_settings.AddProperty(Property("color_displacement_mapped_surface",true));
    this->render_settings.AddProperty(Property("use_image_interpolation",true));
    this->render_settings.AddProperty(Property("timesteps_per_render",100));
    // TODO: allow user to change defaults
}

// ---------------------------------------------------------------------

void MyFrame::SetNumberOfChemicals(int n)
{
    bool had_error = true;
    try 
    {
        this->system->SetNumberOfChemicals(n);
        had_error = false;
    }
    catch(const exception& e)
    {
        MonospaceMessageBox(_("Changing the number of chemicals caused an error:\n\n")+wxString(e.what(),wxConvUTF8),_("Error"),wxART_ERROR);
    }
    catch(...)
    {
        wxMessageBox(_("Changing the number of chemicals caused an unknown error"));
    }
    if(!had_error) // don't want to plague the user with error messages, one will do for now
    {
        try
        {
            this->system->GenerateInitialPattern();
        }
        catch(const exception& e)
        {
            MonospaceMessageBox(_("Generating an initial pattern caused an error:\n\n")+wxString(e.what(),wxConvUTF8),_("Error"),wxART_ERROR);
        }
        catch(...)
        {
            wxMessageBox(_("Generating an initial pattern caused an unknown error"));
        }
    }
    // (we allow the user to proceed because they might now want to change other things to match)
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->render_settings,false);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

bool MyFrame::SetDimensions(int x,int y,int z)
{
    try 
    {
        if(x<1 || y<1 || z<1) throw runtime_error("Dimensions must be at least 1");
        if( x%this->system->GetBlockSizeX() || y%this->system->GetBlockSizeY() || z%this->system->GetBlockSizeZ() )
        {
            ostringstream oss;
            oss << "Dimensions must be a multiple of the current block size (" << this->system->GetBlockSizeX() << 
                "x" << this->system->GetBlockSizeY() << "x" << this->system->GetBlockSizeZ() << ")";
            throw runtime_error(oss.str().c_str());
        }
        // rearrange the dimensions (for visualization we need the z to be 1 for 2D images, and both y and z to be 1 for 1D images)
        if( (x==1 && (y>1 || z>1)) || (y==1 && z>1) )
        {
            float d[3]={x,y,z};
            sort(d,d+3);
            if(d[2]!=x || d[1]!=y || d[0]!=z) {
                x=d[2]; y=d[1]; z=d[0];
                wxString msg = _("We've rearranged the order of the dimensions for visualization. New dimensions: ");
                msg << x << _T(" x ") << y << _T(" x ") << z;
                wxMessageBox(msg);
            }
        }
        // attempt the size change
        this->system->SetDimensions(x,y,z);
    }
    catch(const exception& e)
    {
        MonospaceMessageBox(_("Dimensions not permitted:\n\n")+wxString(e.what(),wxConvUTF8),_("Error"),wxART_ERROR);
        return false;
    }
    catch(...)
    {
        wxMessageBox(_("Dimensions not permitted"));
        return false;
    }
    this->system->GenerateInitialPattern();
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->render_settings,true);
    this->UpdateWindows();
    return true;
}

// ---------------------------------------------------------------------

void MyFrame::SetBlockSize(int x,int y,int z)
{
    this->system->SetBlockSizeX(x);
    this->system->SetBlockSizeY(y);
    this->system->SetBlockSizeZ(z);
    this->system->GenerateInitialPattern();
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->render_settings,false);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::RenderSettingsChanged()
{
    // first do some range checking (not done in InfoPanel::ChangeRenderSetting)
    Property& prop = this->render_settings.GetProperty("timesteps_per_render");
    if (prop.GetInt() < 1) prop.SetInt(1);
    if (prop.GetInt() > MAX_TIMESTEPS_PER_RENDER) prop.SetInt(MAX_TIMESTEPS_PER_RENDER);
    
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->render_settings,false);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnAddParameter(wxCommandEvent& event)
{
    StringDialog dlg(this,_("Add a parameter"),_("Name:"),wxEmptyString,wxDefaultPosition,wxDefaultSize);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->GetCurrentRDSystem()->AddParameter(string(dlg.GetValue().mb_str()),0.0f);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnDeleteParameter(wxCommandEvent& event)
{
    wxArrayString as;
    for(int i=0;i<this->GetCurrentRDSystem()->GetNumberOfParameters();i++)
        as.Add(wxString(this->GetCurrentRDSystem()->GetParameterName(i).c_str(),wxConvUTF8));
    wxSingleChoiceDialog dlg(this,_("Select a parameter to delete:"),_("Delete a parameter"),as);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->GetCurrentRDSystem()->DeleteParameter(dlg.GetSelection());
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateAddParameter(wxUpdateUIEvent& event)
{
    event.Enable(this->GetCurrentRDSystem()->HasEditableFormula());
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateDeleteParameter(wxUpdateUIEvent& event)
{
    event.Enable(this->GetCurrentRDSystem()->HasEditableFormula() &&
                 this->GetCurrentRDSystem()->GetNumberOfParameters() > 0);
}

// ---------------------------------------------------------------------

void MyFrame::OnRunFaster(wxCommandEvent& event)
{
    Property& prop = this->render_settings.GetProperty("timesteps_per_render");
    prop.SetInt(prop.GetInt() * 2);
    // check for overflow, or if beyond limit used in OnChangeRunningSpeed
    if (prop.GetInt() <= 0 || prop.GetInt() > MAX_TIMESTEPS_PER_RENDER) prop.SetInt(MAX_TIMESTEPS_PER_RENDER);
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

void MyFrame::OnRunSlower(wxCommandEvent& event)
{
    Property& prop = this->render_settings.GetProperty("timesteps_per_render");
    prop.SetInt(prop.GetInt() / 2);
    // don't let timesteps_per_render get to 0 otherwise OnRunFaster can't double it
    if (prop.GetInt() < 1) prop.SetInt(1);
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

void MyFrame::OnChangeRunningSpeed(wxCommandEvent& event)
{
    IntegerDialog dlg(this, _("Running speed"), _("New value (timesteps per render):"),
                      this->render_settings.GetProperty("timesteps_per_render").GetInt(),
                      1, MAX_TIMESTEPS_PER_RENDER, wxDefaultPosition, wxDefaultSize);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->render_settings.GetProperty("timesteps_per_render").SetInt(dlg.GetValue());
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

void MyFrame::OnImportMesh(wxCommandEvent& event)
{
    // possible uses:
    // 1. as MeshRD (to run RD on surface, or internally between 3d cells)
    //    - will need to ask user for other pattern to duplicate formula etc. from, or whether to define this later
    // 2. as representation of a binary image for ImageRD (e.g. import a 3D logo, then run tip-splitting from that seed)
    //    - will need to ask for an existing pattern to load the image into, and whether to clear that image first
    //    - will need to ask which chemical(s) to affect, and at what level (overlays engine)

    wxString mesh_filename = wxFileSelector(_("Import a mesh:"),wxEmptyString,wxEmptyString,wxEmptyString,
        _("Supported mesh formats (*.obj;*.vtu;*.vtp)|*.obj;*.vtu;*.vtp"),wxFD_OPEN);
    if(mesh_filename.empty()) return; // user cancelled

    /*
    wxArrayString choices;
    choices.Add(_("Run a pattern on the surface of this mesh"));
    choices.Add(_("Paint this pattern into a 3D volume image"));
    int ret = wxGetSingleChoiceIndex(_("What would you like to do with the mesh?"),_("Select one of these options:"),choices);
    if(ret==-1) return; // user cancelled

    if(ret!=0) { wxMessageBox(_("Not yet implemented.")); return; } // TODO
    */

    // for now we give the mesh an inbuilt rule (Gray-Scott) but this should be different

	MeshRD *mesh_sys;
    if(mesh_filename.EndsWith(_T("vtp")))
    {
        if(UserWantsToCancelWhenAskedIfWantsToSave()) return;

        wxBusyCursor busy;

        this->InitializeDefaultRenderSettings();
        this->render_settings.GetProperty("slice_3D").SetBool(false);
        this->render_settings.GetProperty("active_chemical").SetChemical("b");

        vtkSmartPointer<vtkXMLPolyDataReader> vtp_reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        vtp_reader->SetFileName(mesh_filename.mb_str());
        vtp_reader->Update();
        if(this->is_opencl_available)
			mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
		else
			mesh_sys = new GrayScottMeshRD();
        vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        ug->SetPoints(vtp_reader->GetOutput()->GetPoints());
        ug->SetCells(VTK_POLYGON,vtp_reader->GetOutput()->GetPolys());
        mesh_sys->CopyFromMesh(ug);
    }
    else if(mesh_filename.EndsWith(_T("vtu")))
    {
        if(UserWantsToCancelWhenAskedIfWantsToSave()) return;

        wxBusyCursor busy;

        this->InitializeDefaultRenderSettings();
        this->render_settings.GetProperty("slice_3D").SetBool(false);
        this->render_settings.GetProperty("active_chemical").SetChemical("b");

        vtkSmartPointer<vtkXMLUnstructuredGridReader> vtu_reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        vtu_reader->SetFileName(mesh_filename.mb_str());
        vtu_reader->Update();
        if(this->is_opencl_available)
			mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
		else
			mesh_sys = new GrayScottMeshRD();
        mesh_sys->CopyFromMesh(vtu_reader->GetOutput());
    }
    else if(mesh_filename.EndsWith(_T("obj")))
    {
        if(UserWantsToCancelWhenAskedIfWantsToSave()) return;
    
        wxBusyCursor busy;

        this->InitializeDefaultRenderSettings();
        this->render_settings.GetProperty("slice_3D").SetBool(false);
        this->render_settings.GetProperty("active_chemical").SetChemical("b");

        vtkSmartPointer<vtkOBJReader> obj_reader = vtkSmartPointer<vtkOBJReader>::New();
        obj_reader->SetFileName(mesh_filename.mb_str());
        obj_reader->Update();
        if(this->is_opencl_available)
			mesh_sys = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
		else
			mesh_sys = new GrayScottMeshRD();
        vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        ug->SetPoints(obj_reader->GetOutput()->GetPoints());
        ug->SetCells(VTK_POLYGON,obj_reader->GetOutput()->GetPolys());
        mesh_sys->CopyFromMesh(ug);
    }
    else
    {
        wxMessageBox(_("Unsupported file type")); 
        return; 
    }

	mesh_sys->SetNumberOfChemicals(2);
	mesh_sys->CreateDefaultInitialPatternGenerator();
	mesh_sys->GenerateInitialPattern();
	this->SetCurrentRDSystem(mesh_sys);
}

// ---------------------------------------------------------------------

void MyFrame::OnExportMesh(wxCommandEvent& event)
{
    // possible uses: (context dependent)
    // 1. output MeshRD surface (although can already use Paraview to convert VTU to OBJ etc.)
    // 2. output ImageRD 3d-image contour for active chemical
    // 3. output ImageRD 2d-image displacement-mapped surface for active chemical

    wxString mesh_filename = wxFileSelector(_("Export a mesh:"),wxEmptyString,wxEmptyString,wxEmptyString,
        _("Supported mesh formats (*.obj;*.vtp)|*.obj;*.vtp"),wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if(mesh_filename.empty()) return; // user cancelled

    vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
    this->system->GetAsMesh(pd,this->render_settings);

    if(mesh_filename.EndsWith(_T("obj")))
    {
        wxBusyCursor busy;
        ofstream out(mesh_filename.mb_str());
        out << "# Output from Ready - http://code.google.com/p/reaction-diffusion\n";
        pd->BuildCells();
        for(vtkIdType iPt=0;iPt<pd->GetNumberOfPoints();iPt++)
            out << "v " << pd->GetPoint(iPt)[0] << " " << pd->GetPoint(iPt)[1] << " " << pd->GetPoint(iPt)[2] << "\n";
        if(pd->GetPointData()->GetNormals())
        {
            for(vtkIdType iPt=0;iPt<pd->GetNumberOfPoints();iPt++)
                out << "vn " << pd->GetPointData()->GetNormals()->GetTuple3(iPt)[0] << " " 
                    << pd->GetPointData()->GetNormals()->GetTuple3(iPt)[1] << " " 
                    << pd->GetPointData()->GetNormals()->GetTuple3(iPt)[2] << "\n";
        }
        vtkIdType npts,*pts;
        for(vtkIdType iCell=0;iCell<pd->GetPolys()->GetNumberOfCells();iCell++)
        {
            pd->GetCellPoints(iCell,npts,pts);
            out << "f";
            if(pd->GetPointData()->GetNormals())
            {
                for(vtkIdType iPt=0;iPt<npts;iPt++)
                    out << " " << pts[iPt]+1 << "//" << pts[iPt]+1; // (OBJ indices are 1-based)
            }
            else
            {
                for(vtkIdType iPt=0;iPt<npts;iPt++)
                    out << " " << pts[iPt]+1; // (OBJ indices are 1-based)
            }
            out << "\n";
        }
    }
    else if(mesh_filename.EndsWith(_T("vtp")))
    {
        wxBusyCursor busy;
        vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
        writer->SetFileName(mesh_filename.mb_str());
        writer->SetInput(pd);
        writer->Write();
    }
    else
    {
        wxMessageBox(_("Unsupported file type")); 
        return; 
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnReloadFromDisk(wxCommandEvent &event)
{
    this->OpenFile(this->system->GetFilename());
}

// ---------------------------------------------------------------------

void MyFrame::OnExportImage(wxCommandEvent &event)
{
    // find an unused filename
    const wxString default_filename_root = _("Ready_image_");
    const wxString default_filename_ext = _T("png");
    int unused_value = 0;
    wxString filename;
    wxString extension,folder;
    folder = screenshotdir;
    do {
        filename = default_filename_root;
        filename << wxString::Format(_("%04d."),unused_value) << default_filename_ext;
        unused_value++;
    } while(::wxFileExists(folder+_T("/")+filename));

    // ask the user for confirmation
    bool accepted = true;
    do {
        filename = wxFileSelector(_("Specify the image filename"),folder,filename,default_filename_ext,
            _("PNG files (*.png)|*.png|JPG files (*.jpg)|*.jpg"),
            wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
        if(filename.empty()) return; // user cancelled
        // validate
        wxFileName::SplitPath(filename,&folder,NULL,&extension);
        if(extension!=_T("png") && extension!=_T("jpg"))
        {
            wxMessageBox(_("Unsupported format"));
            accepted = false;
        }
    } while(!accepted);

    screenshotdir = folder;

    vtkSmartPointer<vtkImageWriter> writer;
    if(extension==_T("png")) writer = vtkSmartPointer<vtkPNGWriter>::New();
    else if(extension==_T("jpg")) writer = vtkSmartPointer<vtkJPEGWriter>::New();
    writer->SetFileName(filename.mb_str());
    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    system->GetAs2DImage(image,this->render_settings);
    writer->SetInput(image);
    writer->Write();

    // TODO: merge with OnSaveScreenshot
}

// ---------------------------------------------------------------------

void MyFrame::RecordFrame()
{
    ostringstream oss; 
    oss << this->recording_prefix << setfill('0') << setw(6) << this->iRecordingFrame << this->recording_extension;
    vtkSmartPointer<vtkImageWriter> writer;
    if(this->recording_extension==_T(".png")) writer = vtkSmartPointer<vtkPNGWriter>::New();
    else if(this->recording_extension==_T(".jpg")) writer = vtkSmartPointer<vtkJPEGWriter>::New();
    if(this->record_data_image) // take the 2D data (2D system or 2D slice)
    {
        vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
        this->system->GetAs2DImage(image,this->render_settings);
        writer->SetInput(image);
    }
    else // take a screenshot of the current view
    {
        vtkSmartPointer<vtkWindowToImageFilter> screenshot = vtkSmartPointer<vtkWindowToImageFilter>::New();
        screenshot->SetInput(this->pVTKWindow->GetRenderWindow());
        writer->SetInputConnection(screenshot->GetOutputPort());
    }
    writer->SetFileName(oss.str().c_str());
    writer->Write();
    this->iRecordingFrame++;
}

// ---------------------------------------------------------------------

void MyFrame::OnRecordFrames(wxCommandEvent &event)
{
    if(!this->is_recording)
    {
        bool default_to_2D_data = (this->system->GetArenaDimensionality()==2);

        RecordingDialog dlg(this,default_to_2D_data);
        if(dlg.ShowModal()!=wxID_OK) return;
        this->recording_prefix = dlg.recording_prefix;
        this->recording_extension = dlg.recording_extension;
        this->record_data_image = dlg.record_data_image;
        this->iRecordingFrame = 0;
        this->is_recording = true;
    }
    else
        this->is_recording = false;
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateRecordFrames(wxUpdateUIEvent &event)
{
    event.Check(this->is_recording);
}

// ---------------------------------------------------------------------

void MyFrame::OnBlank(wxCommandEvent& event)
{
    this->system->BlankImage();
    this->is_running = false;
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnViewFullKernel(wxCommandEvent& event)
{
    MonospaceMessageBox(wxString(this->system->GetKernel().c_str(),wxConvUTF8),
        _("The full OpenCL kernel for this formula rule:"),wxART_INFORMATION);
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateViewFullKernel(wxUpdateUIEvent& event)
{
    event.Enable(this->system->GetRuleType()=="formula");
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectPointerTool(wxCommandEvent& event)
{
    this->CurrentCursor = POINTER;
    this->pVTKWindow->SetCursor(wxCursor(wxCURSOR_ARROW));
    this->left_mouse_is_down = false;
    this->right_mouse_is_down = false;
    this->erasing = false;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> is = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    this->pVTKWindow->SetInteractorStyle(is);
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateSelectPointerTool(wxUpdateUIEvent& event)
{
    event.Check(this->CurrentCursor==POINTER);
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectPencilTool(wxCommandEvent& event)
{
    this->CurrentCursor = PENCIL;
    this->pVTKWindow->SetCursor(*this->pencil_cursor);
    this->left_mouse_is_down = false;
    this->right_mouse_is_down = false;
    this->erasing = false;
    vtkSmartPointer<InteractorStylePainter> is = vtkSmartPointer<InteractorStylePainter>::New();
    is->SetPaintHandler(this);
    this->pVTKWindow->SetInteractorStyle(is);
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateSelectPencilTool(wxUpdateUIEvent& event)
{
    event.Check(this->CurrentCursor==PENCIL);
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectBrushTool(wxCommandEvent& event)
{
    this->CurrentCursor = BRUSH;
    this->pVTKWindow->SetCursor(*this->brush_cursor);
    this->left_mouse_is_down = false;
    this->right_mouse_is_down = false;
    this->erasing = false;
    vtkSmartPointer<InteractorStylePainter> is = vtkSmartPointer<InteractorStylePainter>::New();
    is->SetPaintHandler(this);
    this->pVTKWindow->SetInteractorStyle(is);
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateSelectBrushTool(wxUpdateUIEvent& event)
{
    event.Check(this->CurrentCursor==BRUSH);
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectPickerTool(wxCommandEvent& event)
{
    this->CurrentCursor = PICKER;
    this->pVTKWindow->SetCursor(*this->picker_cursor);
    this->left_mouse_is_down = false;
    this->right_mouse_is_down = false;
    this->erasing = false;
    vtkSmartPointer<InteractorStylePainter> is = vtkSmartPointer<InteractorStylePainter>::New();
    is->SetPaintHandler(this);
    this->pVTKWindow->SetInteractorStyle(is);
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateSelectPickerTool(wxUpdateUIEvent& event)
{
    event.Check(this->CurrentCursor==PICKER);
}

// ---------------------------------------------------------------------

void MyFrame::LeftMouseDown(int x, int y)
{
    this->left_mouse_is_down = true;

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.000001);
    int ret = picker->Pick(x,y,0,this->pVTKWindow->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
    if(!ret) return;
    double *p = picker->GetPickPosition();

    if(!this->pVTKWindow->GetShiftKey())
    {
        switch(this->CurrentCursor)
        {
            case PENCIL:
            {
                if (repaint_to_erase && this->current_paint_value == this->system->GetValue(p[0],p[1],p[2],this->render_settings)) {
                    // erase cell by using low value
                    this->system->SetValue(p[0],p[1],p[2],this->render_settings.GetProperty("low").GetFloat(),this->render_settings);
                    // set flag in case mouse is moved (see MouseMove)
                    this->erasing = true;
                } else {
                    this->system->SetValue(p[0],p[1],p[2],this->current_paint_value,this->render_settings);
                }
                this->pVTKWindow->Refresh();
            }
            break;
            case BRUSH:
            {
                float brush_size = 0.02f; // proportion of the diagonal of the bounding box TODO: make a user option
                this->system->SetValuesInRadius(p[0],p[1],p[2],brush_size,this->current_paint_value,this->render_settings);
                this->pVTKWindow->Refresh();
            }
            break;
            case PICKER:
            {
                this->current_paint_value = this->system->GetValue(p[0],p[1],p[2],this->render_settings);
                this->UpdateToolbars();
            }
            break;
        }
    }
    else
    {
        // pick
        this->current_paint_value = this->system->GetValue(p[0],p[1],p[2],this->render_settings);
        this->UpdateToolbars();
    }
}

// ---------------------------------------------------------------------

void MyFrame::LeftMouseUp(int x, int y)
{
    this->left_mouse_is_down = false;
    this->erasing = false;
    this->system->SetUndoPoint();
}

// ---------------------------------------------------------------------

void MyFrame::RightMouseDown(int x, int y)
{
    this->right_mouse_is_down = true;

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.000001);
    int ret = picker->Pick(x,y,0,this->pVTKWindow->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
    if(!ret) return;
    double *p = picker->GetPickPosition();

    // color pick
    this->pVTKWindow->SetCursor(*this->picker_cursor);
    this->current_paint_value = this->system->GetValue(p[0],p[1],p[2],this->render_settings);
    this->UpdateToolbars();
}

// ---------------------------------------------------------------------

void MyFrame::RightMouseUp(int x, int y)
{
    this->right_mouse_is_down = false;
    if(!this->pVTKWindow->GetShiftKey())
    {
        if(this->CurrentCursor == PENCIL)
            this->pVTKWindow->SetCursor(*this->pencil_cursor);
        else if(this->CurrentCursor == BRUSH )
            this->pVTKWindow->SetCursor(*this->brush_cursor);
    }
}

// ---------------------------------------------------------------------

void MyFrame::MouseMove(int x, int y)
{
    if(!this->left_mouse_is_down && !this->right_mouse_is_down) return;

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.000001);
    int ret = picker->Pick(x,y,0,this->pVTKWindow->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
    if(!ret) return;
    double *p = picker->GetPickPosition();

    if(this->left_mouse_is_down && !this->pVTKWindow->GetShiftKey())
    {
        switch(this->CurrentCursor)
        {
            case PENCIL:
            {
                if (erasing) {
                    this->system->SetValue(p[0],p[1],p[2],this->render_settings.GetProperty("low").GetFloat(),this->render_settings);
                } else {
                    this->system->SetValue(p[0],p[1],p[2],this->current_paint_value,this->render_settings);
                }
                this->pVTKWindow->Refresh();
            }
            break;
            case BRUSH:
            {
                float brush_size = 0.02f; // proportion of the diagonal of the bounding box TODO: make a user option
                this->system->SetValuesInRadius(p[0],p[1],p[2],brush_size,this->current_paint_value,this->render_settings);
                this->pVTKWindow->Refresh();
            }
            break;
            case PICKER:
            {
                this->current_paint_value = this->system->GetValue(p[0],p[1],p[2],this->render_settings);
                this->UpdateToolbars();
            }
            break;
        }
    }
    else
    {
        // color pick
        this->current_paint_value = this->system->GetValue(p[0],p[1],p[2],this->render_settings);
        this->UpdateToolbars();
    }
}

// ---------------------------------------------------------------------

void MyFrame::KeyDown()
{
    if(this->pVTKWindow->GetShiftKey() && ( this->CurrentCursor == PENCIL || this->CurrentCursor == BRUSH ) )
        this->pVTKWindow->SetCursor(*this->picker_cursor);
}

// ---------------------------------------------------------------------

void MyFrame::KeyUp()
{
    if(!this->pVTKWindow->GetShiftKey())
    {
        if(this->CurrentCursor == PENCIL)
            this->pVTKWindow->SetCursor(*this->pencil_cursor);
        else if(this->CurrentCursor == BRUSH )
            this->pVTKWindow->SetCursor(*this->brush_cursor);
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnUndo(wxCommandEvent& event)
{
    if (this->system->CanUndo()) {
        this->system->Undo();
        this->pVTKWindow->Refresh();
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable(this->system->CanUndo());
}

// ---------------------------------------------------------------------

void MyFrame::OnRedo(wxCommandEvent& event)
{
    if (this->system->CanRedo()) {
        this->system->Redo();
        this->pVTKWindow->Refresh();
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable(this->system->CanRedo());
}

// ---------------------------------------------------------------------

void MyFrame::OnChangeCurrentColor(wxCommandEvent& event)
{
    if(GetFloat(_("Enter a new value to paint with:"),_("Value:"),
        this->current_paint_value,&this->current_paint_value))
        this->UpdateToolbars();}

// ---------------------------------------------------------------------
