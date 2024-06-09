/*  Copyright 2011-2021 The Ready Bunch

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
#include "ImportImageDialog.hpp"
#include "MakeNewSystem.hpp"

// readybase:
#include <FormulaOpenCLImageRD.hpp>
#include <FormulaOpenCLMeshRD.hpp>
#include <FullKernelOpenCLImageRD.hpp>
#include <FullKernelOpenCLMeshRD.hpp>
#include <GrayScottImageRD.hpp>
#include <GrayScottMeshRD.hpp>
#include <IO_XML.hpp>
#include <OpenCL_utils.hpp>
#include <scene_items.hpp>
#include <SystemFactory.hpp>
#include <utils.hpp>

// local resources:
#include "appicon16.xpm"

// wxWidgets:
#include <wx/dir.h>
#include <wx/dnd.h>
#include <wx/filename.h>
#include <wx/filesys.h>
#if wxUSE_TOOLTIPS
   #include <wx/tooltip.h>
#endif
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

// wxVTK: (local copy)
#include "wxVTKRenderWindowInteractor.h"

// STL:
#include <string>
#include <algorithm>

// VTK:
#include <vtkBMPReader.h>
#include <vtkCellArray.h>
#include <vtkCellDataToPointData.h>
#include <vtkCellPicker.h>
#include <vtkDoubleArray.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageLuminance.h>
#include <vtkImageReader2.h>
#include <vtkImageResize.h>
#include <vtkImageShiftScale.h>
#include <vtkJPEGReader.h>
#include <vtkJPEGWriter.h>
#include <vtkOBJReader.h>
#include <vtkPNGReader.h>
#include <vtkPNGWriter.h>
#include <vtkPLYWriter.h>
#include <vtkPointData.h>
#include <vtkPolyDataNormals.h>
#include <vtkQuadricDecimation.h>
#include <vtkRendererCollection.h>
#include <vtkScalarsToColors.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkWindowToImageFilter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>

#ifdef __WXMAC__
    #if wxCHECK_VERSION(3,1,3)
        // don't include Carbon.h (causes problems with Xcode 11.x)
    #else
        #include <Carbon/Carbon.h>  // for GetCurrentProcess, etc
    #endif
#endif

#if wxCHECK_VERSION(2,9,0)
    // some wxMenuItem method names have changed in wx 2.9
    #define GetText GetItemLabel
    #define SetText SetItemLabel
#endif

using namespace std;

// ---------------------------------------------------------------------

wxString PaneName(int id)
{
    switch(id)
    {
        case ID::FileToolbar:   return _T("FileToolbar");
        case ID::ActionToolbar: return _T("ActionToolbar");
        case ID::PaintToolbar:  return _T("PaintToolbar");
        case ID::PatternsPane:  return _T("PatternsPane");
        case ID::InfoPane:      return _T("InfoPane");
        case ID::HelpPane:      return _T("HelpPane");
        case ID::CanvasPane:    return _T("CanvasPane");
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
    EVT_MENU(ID::SaveCompact, MyFrame::OnSaveCompact)
    EVT_MENU(ID::ImportMesh, MyFrame::OnImportMesh)
    EVT_MENU(ID::ExportMesh, MyFrame::OnExportMesh)
    EVT_MENU(ID::ImportImage, MyFrame::OnImportImage)
    EVT_UPDATE_UI(ID::ImportImage, MyFrame::OnUpdateImportImage)
    EVT_MENU(ID::ExportImage, MyFrame::OnExportImage)
    EVT_UPDATE_UI(ID::ExportImage, MyFrame::OnUpdateExportImage)
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
    EVT_MENU(ID::BrushSizeExtraSmall, MyFrame::OnBrushSizeExtraSmall)
    EVT_UPDATE_UI(ID::BrushSizeExtraSmall, MyFrame::OnUpdateBrushSizeExtraSmall)
    EVT_MENU(ID::BrushSizeSmall, MyFrame::OnBrushSizeSmall)
    EVT_UPDATE_UI(ID::BrushSizeSmall, MyFrame::OnUpdateBrushSizeSmall)
    EVT_MENU(ID::BrushSizeMedium, MyFrame::OnBrushSizeMedium)
    EVT_UPDATE_UI(ID::BrushSizeMedium, MyFrame::OnUpdateBrushSizeMedium)
    EVT_MENU(ID::BrushSizeLarge, MyFrame::OnBrushSizeLarge)
    EVT_UPDATE_UI(ID::BrushSizeLarge, MyFrame::OnUpdateBrushSizeLarge)
    EVT_MENU(ID::BrushSizeExtraLarge, MyFrame::OnBrushSizeExtraLarge)
    EVT_UPDATE_UI(ID::BrushSizeExtraLarge, MyFrame::OnUpdateBrushSizeExtraLarge)
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
    EVT_MENU(ID::ViewFullKernel, MyFrame::OnViewFullKernel)
    EVT_UPDATE_UI(ID::ViewFullKernel, MyFrame::OnUpdateViewFullKernel)
    EVT_MENU(ID::OpenCLDiagnostics, MyFrame::OnOpenCLDiagnostics)
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
    EVT_MENU(ID::ConvertToFullKernel,MyFrame::OnConvertToFullKernel)
    EVT_UPDATE_UI(ID::ConvertToFullKernel, MyFrame::OnUpdateConvertToFullKernel)
    EVT_MENU(ID::SelectOpenCLDevice, MyFrame::OnSelectOpenCLDevice)
    // help menu
    EVT_MENU(wxID_HELP, MyFrame::OnHelp)
    EVT_MENU(ID::HelpQuick, MyFrame::OnHelp)
    EVT_MENU(ID::HelpIntro, MyFrame::OnHelp)
    EVT_MENU(ID::HelpWriting, MyFrame::OnHelp)
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

// ---------------------------------------------------------------------

const float MyFrame::brush_sizes[] = {0.002f, 0.005f, 0.01f, 0.02f, 0.05f};

// ---------------------------------------------------------------------

// constructor
MyFrame::MyFrame(const wxString& title)
   : wxFrame(NULL, wxID_ANY, title),
    render_settings("render_settings"),
    is_running(false),
    time_at_last_render(0),
    i_timesteps_per_second_buffer(0),
    speed_data_available(false),
    is_recording(false),
    fullscreen(false),
    CurrentCursor(TCursorType::POINTER),
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
        // don't add wxAUI_MGR_ALLOW_ACTIVE_PANE below as it doesn't work correctly
        this->aui_mgr.SetFlags( wxAUI_MGR_ALLOW_FLOATING | wxAUI_MGR_TRANSPARENT_HINT );
        this->icons_folder = _T("resources/Icons/32px/");
        // following avoids black background in pane captions on macOS 10.11
        wxAuiDockArt* dockart = this->aui_mgr.GetArtProvider();
        dockart->SetMetric(wxAUI_DOCKART_GRADIENT_TYPE, wxAUI_GRADIENT_NONE);
        dockart->SetColour(wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR, wxColor(210,210,210));
        dockart->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR, wxColor(210,210,210));
    #else
        this->icons_folder = _T("resources/Icons/22px/");
    #endif
    #ifdef __WXMSW__
        this->aui_mgr.SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_LIVE_RESIZE);
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
        SetDefaultRenderSettings(this->render_settings);
        unique_ptr<GrayScottImageRD> s = make_unique<GrayScottImageRD>();
        s->SetDimensionsAndNumberOfChemicals(30,25,20,2);
        s->SetModified(false);
        s->SetFilename("untitled");
        s->GenerateInitialPattern();
        this->SetCurrentRDSystem(std::move(s));
    }
}

// ---------------------------------------------------------------------

MyFrame::~MyFrame()
{
    this->SaveSettings(); // save the current settings so it starts up the same next time
    this->aui_mgr.UnInit();
}

// ---------------------------------------------------------------------

void MyFrame::InitializeMenus()
{
    wxMenuBar *menuBar = new wxMenuBar();
    {   // file menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_NEW, _("New Pattern...") + GetAccelerator(DO_NEWPATT), _("Create a new pattern"));
        menu->AppendSeparator();
        menu->Append(wxID_OPEN, _("Open Pattern...") + GetAccelerator(DO_OPENPATT), _("Choose a pattern file to open"));
        menu->Append(ID::OpenRecent, _("Open Recent"), patternSubMenu);
        menu->Append(ID::ReloadFromDisk, _("Reload from Disk") + GetAccelerator(DO_RELOAD), _("Reload the pattern file from disk"));
        menu->AppendSeparator();
        menu->Append(ID::ImportMesh, _("Import Mesh...") + GetAccelerator(DO_IMPORTMESH), _("Import a mesh"));
        menu->Append(ID::ExportMesh, _("Export Mesh...") + GetAccelerator(DO_EXPORTMESH), _("Export the mesh"));
        menu->AppendSeparator();
        menu->Append(ID::ImportImage, _("Import Image...") + GetAccelerator(DO_IMPORTIMAGE), _("Import the image"));
        menu->Append(ID::ExportImage, _("Export Image...") + GetAccelerator(DO_EXPORTIMAGE), _("Export the image"));
        menu->AppendSeparator();
        menu->Append(wxID_SAVE, _("Save Pattern...") + GetAccelerator(DO_SAVE), _("Save the current pattern"));
        menu->Append(ID::SaveCompact, _("Save Compact...") + GetAccelerator(DO_SAVECOMPACT), _("Blank the image and save with the initial pattern generator set to apply when the file is loaded"));
        menu->AppendSeparator();
        menu->Append(ID::Screenshot, _("Save Screenshot...") + GetAccelerator(DO_SCREENSHOT), _("Save a screenshot of the current view"));
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
        menu->AppendSeparator();
        wxMenu* brush_size_menu = new wxMenu;
        brush_size_menu->AppendCheckItem(ID::BrushSizeExtraSmall,_("Extra small") + GetAccelerator(DO_BRUSHEXTRASMALL),_("Select the extra small brush"));
        brush_size_menu->AppendCheckItem(ID::BrushSizeSmall,_("Small") + GetAccelerator(DO_BRUSHSMALL),_("Select the small brush"));
        brush_size_menu->AppendCheckItem(ID::BrushSizeMedium,_("Medium") + GetAccelerator(DO_BRUSHMEDIUM),_("Select the medium brush"));
        brush_size_menu->AppendCheckItem(ID::BrushSizeLarge,_("Large") + GetAccelerator(DO_BRUSHLARGE),_("Select the large brush"));
        brush_size_menu->AppendCheckItem(ID::BrushSizeExtraLarge,_("Extra large") + GetAccelerator(DO_BRUSHEXTRALARGE),_("Select the extra large brush"));
        menu->AppendSubMenu(brush_size_menu,_("Brush Size"),_("Choose the size of brush"));
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
        menu->AppendSeparator();
        menu->Append(ID::ViewFullKernel, _("View Full Kernel") + GetAccelerator(DO_VIEWKERNEL), _("Shows the full OpenCL kernel as expanded from the formula"));
        menu->Append(ID::OpenCLDiagnostics, _("Show Open&CL Diagnostics...") + GetAccelerator(DO_OPENCL), _("Show the available OpenCL devices and their attributes"));
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
        menu->AppendSeparator();
        menu->Append(ID::ConvertToFullKernel, _("Convert to Full Kernel") + GetAccelerator(DO_CONVERTTOKERNEL),_("Converts the formula to a full kernel rule"));
        menu->AppendSeparator();
        menu->Append(ID::SelectOpenCLDevice, _("Select &OpenCL Device...") + GetAccelerator(DO_DEVICE), _("Choose which OpenCL device to run on"));
        menuBar->Append(menu, _("&Action"));
    }
    {   // help menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_HELP,        _("Contents"));
        menu->Append(ID::HelpQuick,    _("Quick Start"));
        menu->Append(ID::HelpIntro,    _("Introduction to RD"));
        menu->Append(ID::HelpWriting,  _("Writing new rules"));
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
        #ifdef __WXMAC__
            // avoid black background on macOS 10.11
            this->file_toolbar = new wxAuiToolBar(this,ID::FileToolbar, wxDefaultPosition, wxDefaultSize, wxAUI_TB_PLAIN_BACKGROUND);
        #else
            this->file_toolbar = new wxAuiToolBar(this,ID::FileToolbar);
        #endif
        this->file_toolbar->AddTool(wxID_NEW,_("New Pattern..."),wxBitmap(this->icons_folder + _T("document-new.png"),wxBITMAP_TYPE_PNG),
            _("New Pattern..."));
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
        #ifdef __WXMAC__
            // avoid black background on macOS 10.11
            this->action_toolbar = new wxAuiToolBar(this,ID::ActionToolbar, wxDefaultPosition, wxDefaultSize, wxAUI_TB_PLAIN_BACKGROUND);
        #else
            this->action_toolbar = new wxAuiToolBar(this,ID::ActionToolbar);
        #endif
        this->action_toolbar->AddTool(ID::Step1, _("Step by 1"),wxBitmap(this->icons_folder + _T("list-add_gray.png"),wxBITMAP_TYPE_PNG),
            _("Step by 1"));
        this->action_toolbar->AddTool(ID::RunStop,_("Run"),wxBitmap(this->icons_folder + _T("media-playback-start_green.png"),wxBITMAP_TYPE_PNG),
            _("Run"));
        //this->action_toolbar->AddTool(ID::RecordFrames,_("Start Recording..."),wxBitmap(this->icons_folder + _T("media-record.png"),wxBITMAP_TYPE_PNG),
        //    _("Start Recording..."),wxITEM_CHECK);
        this->action_toolbar->AddTool(ID::Slower,_("Run Slower"),wxBitmap(this->icons_folder + _T("media-seek-backward.png"),wxBITMAP_TYPE_PNG),
            _("Run Slower (render more often)"));
        wxStaticText *st = new wxStaticText(this->action_toolbar,ID::TimestepsPerRender,
            wxString::Format(_(" %d "),MAX_TIMESTEPS_PER_RENDER),wxDefaultPosition,wxDefaultSize,wxALIGN_CENTRE_HORIZONTAL);
        st->SetToolTip(_("Timesteps per render"));
        this->action_toolbar->AddControl(st,_("Timesteps per render"));
        this->action_toolbar->AddTool(ID::Faster,_("Run Faster"),wxBitmap(this->icons_folder + _T("media-seek-forward.png"),wxBITMAP_TYPE_PNG),
            _("Run Faster (render less often)"));
        this->action_toolbar->AddTool(ID::Reset, _("Reset"),wxBitmap(this->icons_folder + _T("media-skip-backward_modified.png"),wxBITMAP_TYPE_PNG),
            _("Reset"));
        this->action_toolbar->AddTool(ID::GenerateInitialPattern,_("Generate Initial Pattern"),wxBitmap(this->icons_folder + _T("system-run.png"),wxBITMAP_TYPE_PNG),
            _("Generate Initial Pattern"));

        this->action_toolbar->SetToolBorderPadding(toolbar_padding);
        this->aui_mgr.AddPane(this->action_toolbar,wxAuiPaneInfo().ToolbarPane().Top()
            .Name(PaneName(ID::ActionToolbar)).Position(1).Caption(_("Action tools")));
    }
    {   // paint items
        #ifdef __WXMAC__
            // avoid black background on macOS 10.11
            this->paint_toolbar = new wxAuiToolBar(this,ID::PaintToolbar, wxDefaultPosition, wxDefaultSize, wxAUI_TB_PLAIN_BACKGROUND);
        #else
            this->paint_toolbar = new wxAuiToolBar(this,ID::PaintToolbar);
        #endif
        this->paint_toolbar->AddTool(ID::Pointer,_("Pointer"),wxBitmap(this->icons_folder + _T("icon-pointer.png"),wxBITMAP_TYPE_PNG),
            _("Pointer"),wxITEM_RADIO);
        this->paint_toolbar->AddTool(ID::Pencil,_("Pencil"),wxBitmap(this->icons_folder + _T("draw-freehand.png"),wxBITMAP_TYPE_PNG),
            _("Pencil (right-click to pick color)"),wxITEM_RADIO);
        this->paint_toolbar->AddTool(ID::Brush,_("Brush"),wxBitmap(this->icons_folder + _T("draw-brush.png"),wxBITMAP_TYPE_PNG),
            _("Brush (right-click to pick color)"),wxITEM_RADIO);
        this->paint_toolbar->AddTool(ID::Picker,_("Color picker"),wxBitmap(this->icons_folder + _T("color-picker.png"),wxBITMAP_TYPE_PNG),
            _("Color picker"),wxITEM_RADIO);
        wxImage im(22,22);
        im.SetRGB(wxRect(0,0,22,22),255,0,0);
        wxBitmapButton *cb = new wxBitmapButton(this->paint_toolbar,ID::CurrentValueColor,wxBitmap(im));
        cb->SetToolTip(_("Color of the current paint value. Click to change the value."));
        this->paint_toolbar->AddControl(cb,_("Color"));
        wxStaticText *st = new wxStaticText(this->paint_toolbar,ID::CurrentValueText,_(" 0.00000E+000 "), // allow enough space for %6g
            wxDefaultPosition,wxDefaultSize,wxALIGN_CENTRE_HORIZONTAL);
        st->SetToolTip(_("Current value to paint with"));
        this->paint_toolbar->AddControl(st,_("Color"));

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
    this->pencil_cursor = make_unique<wxCursor>(im1);

    wxImage im2(cursors_folder + _T("brush-cursor.png"),wxBITMAP_TYPE_PNG);
    im2.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 3);
    im2.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 21);
    this->brush_cursor = make_unique<wxCursor>(im2);

    wxImage im3(cursors_folder + _T("picker-cursor.png"),wxBITMAP_TYPE_PNG);
    im3.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 4);
    im3.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 14);
    this->picker_cursor = make_unique<wxCursor>(im3);
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
    this->info_panel->UpdatePanel(*this->system);
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
        #if wxCHECK_VERSION(3,1,3)
            // use wxExecute to avoid deprecated calls
            wxString app = readydir + wxT("Ready.app");
            wxString cmd = wxString::Format(wxT("\"%s\""), app.c_str());
            wxExecute(cmd, wxEXEC_ASYNC);
        #else
            ProcessSerialNumber process;
            if ( GetCurrentProcess(&process) == noErr ) SetFrontProcess(&process);
        #endif
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
    this->pVTKWindow = vtkSmartPointer<wxVTKRenderWindowInteractor>::Take(new wxVTKRenderWindowInteractor(this,wxID_ANY));
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

        // hide all currently shown panes (except the canvas pane)
        wxAuiPaneInfoArray panes = this->aui_mgr.GetAllPanes();
        for(size_t i=0;i<panes.Count();i++)
            if(panes[i].name != PaneName(ID::CanvasPane))
                this->aui_mgr.GetPane(panes[i].name).Hide();

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
    while((ren = renWin->GetRenderers()->GetNextItem()))
        ren->ResetCamera();
    this->Refresh(false);
}

// ---------------------------------------------------------------------

void MyFrame::OnWireframe(wxCommandEvent& event)
{
    bool wireframe = this->render_settings.GetProperty("use_wireframe").GetBool();
    wireframe = !wireframe;
    this->render_settings.GetProperty("use_wireframe").SetBool(wireframe);
    InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, false);
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
    wxFileName filename = FindUnusedFilename(screenshotdir, _("Ready_screenshot_%04d.png"));

    if (!AskUserWhereToSaveImage(filename))
        return; // user cancelled

    screenshotdir = filename.GetPath(); // default to this folder next time

    vtkSmartPointer<vtkWindowToImageFilter> screenshot = vtkSmartPointer<vtkWindowToImageFilter>::New();
    screenshot->SetInput(this->pVTKWindow->GetRenderWindow());
    screenshot->ReadFrontBufferOff();

    WriteImageToFile(screenshot->GetOutputPort(), filename);
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

void MyFrame::SetCurrentRDSystem(unique_ptr<AbstractRD> sys)
{
    this->system = std::move(sys);
    int iChem = IndexFromChemicalName(this->render_settings.GetProperty("active_chemical").GetChemical());
    iChem = min(iChem,this->system->GetNumberOfChemicals()-1); // ensure is in valid range
    this->render_settings.GetProperty("active_chemical").SetChemical(GetChemicalName(iChem));
    InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, true);
    this->is_running = false;
    this->i_timesteps_per_second_buffer = 0;
    this->speed_data_available = false;
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
        if (event.GetId() == ID::Step1)
        {
            this->system->Update(1);
            this->pVTKWindow->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCameraClippingRange();
        }
        else if (event.GetId() == ID::StepN)
        {
            // timesteps_per_render might be huge, so don't do this:
            // this->system->Update(this->render_settings.GetProperty("timesteps_per_render").GetInt());
            // instead we let OnIdle do the stepping, but stop at next render
            this->is_running = true;
            steps_since_last_render = 0;
            this->computation_time_since_last_render = 0.0;
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
        this->computation_time_since_last_render = 0.0;
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
        this->is_running ? wxBitmap(this->icons_folder + _T("media-playback-pause_red.png"),wxBITMAP_TYPE_PNG)
                         : wxBitmap(this->icons_folder + _T("media-playback-start_green.png"),wxBITMAP_TYPE_PNG) );

    this->action_toolbar->FindTool(ID::RunStop)->SetShortHelp(
        this->is_running ? _("Stop running the simulation")
                         : _("Start running the simulation") );
    this->action_toolbar->FindTool(ID::RunStop)->SetLabel(
        this->is_running ? _("Stop")
                         : _("Run") );
    this->action_toolbar->FindControl(ID::TimestepsPerRender)->SetLabel( wxString::Format(_T("%d"),
        this->render_settings.GetProperty("timesteps_per_render").GetInt()) );
    this->paint_toolbar->FindControl(ID::CurrentValueText)->SetLabel( wxString::Format(_T("%.6g"),
        this->current_paint_value) );
    // update the color swatch with the current color
    wxImage im(22,22);
    vtkSmartPointer<vtkScalarsToColors> lut = GetColorMap(this->render_settings);
    const unsigned char* rgba = lut->MapValue(this->current_paint_value);
    im.SetRGB(wxRect(0,0,22,22), rgba[0], rgba[1], rgba[2]);
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
            this->pVTKWindow->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCameraClippingRange();
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

        this->computation_time_since_last_render += time_diff;
        steps_since_last_render += temp_steps;

        if (steps_since_last_render >= timesteps_per_render) {
            // it's time to render what we've computed so far
            if (this->computation_time_since_last_render == 0.0)
                this->computation_time_since_last_render = 0.000001;  // unlikely, but play safe
            double time_since_last_render = time_before - this->time_at_last_render;
            this->time_at_last_render = time_before;
            this->timesteps_per_second_buffer[this->i_timesteps_per_second_buffer] = steps_since_last_render / time_since_last_render;
            this->computed_frames_per_second_buffer[this->i_timesteps_per_second_buffer] = steps_since_last_render / this->computation_time_since_last_render;
            this->i_timesteps_per_second_buffer++;
            if(this->i_timesteps_per_second_buffer==10)
            {
                this->smoothed_timesteps_per_second = 0.0;
                double smoothed_cfps = 0.0;
                for(int i=0;i<10;i++) {
                    this->smoothed_timesteps_per_second += this->timesteps_per_second_buffer[i]/10.0;
                    smoothed_cfps += this->computed_frames_per_second_buffer[i]/10.0;
                }
                if(smoothed_cfps > this->smoothed_timesteps_per_second)
                    this->percentage_spent_rendering = 100.0 - 100.0 * this->smoothed_timesteps_per_second / smoothed_cfps;
                this->i_timesteps_per_second_buffer = 0;
                this->speed_data_available = true;
            }

            if(this->is_recording)
                this->RecordFrame();

            this->pVTKWindow->Refresh(false);
            this->SetStatusBarText();

            if (do_one_render) {
                // user selected Step by N so stop now
                this->is_running = false;
                this->speed_data_available = false;
                this->SetStatusBarText();
                this->UpdateToolbars();
            } else {
                // keep simulating
                steps_since_last_render = 0;
                this->computation_time_since_last_render = 0.0;
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
    if(this->speed_data_available)
    {
        txt << wxString::Format(_T("  -   %.0f"),this->smoothed_timesteps_per_second)
            << _(" timesteps per second");
        txt << _T("   ( ")
            << wxString::Format(_T("%.1f"),this->percentage_spent_rendering)
            << _("% of time spent rendering )");
    }
    //txt << " GPU mem: " << this->system->GetMemorySize()/(1024*1024) << " MB";
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
        MonospaceMessageBox((_("OpenCL not available: ")+
            wxString(e.what(),wxConvUTF8)+
            _("\n\n")+
            wxString(OpenCL_utils::GetOpenCLInstallationHints(),wxConvUTF8)),_("Error"), wxART_ERROR);
        return;
    }
    catch(...)
    {
        MonospaceMessageBox((_("OpenCL not available:\n\n")+
            wxString(OpenCL_utils::GetOpenCLInstallationHints(), wxConvUTF8)), _("Error"), wxART_ERROR);
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
        case ID::HelpWriting:   this->help_panel->ShowHelp(_("Help/writing_new_rules.html")); break;
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
    wxBeginBusyCursor();

    try
    {
        this->system->SaveFile(path.mb_str(), this->render_settings, false);
    }
    catch (const exception& e)
    {
        wxEndBusyCursor();
        string message;
        message += _("Failed to save file. Error:\n\n");
        message += wxString(e.what(), wxConvUTF8);
        MonospaceMessageBox(message, _("Error saving file"), wxART_ERROR);
        return;
    }
    catch (...)
    {
        wxEndBusyCursor();
        string message;
        message += _("Failed to save file.");
        MonospaceMessageBox(message, _("Error saving file"), wxART_ERROR);
        return;
    }

    AddRecentPattern(path);
    this->system->SetFilename(string(path.mb_str()));
    this->system->SetModified(false);
    this->UpdateWindowTitle();

    wxEndBusyCursor();
}

// ---------------------------------------------------------------------

void MyFrame::OnNewPattern(wxCommandEvent& event)
{
    if(UserWantsToCancelWhenAskedIfWantsToSave()) return;

    // ask user what type of dataset to generate:
    enum class GenType { Image1D, Image2D, Image3D, GeoSphere, Torus, TriMesh, HexMesh, Rhombille, PenroseP3, PenroseP2, Del2D, Vor2D, Del3D,
        BodyCentredCubic, FaceCentredCubic, Diamond, HyperbolicPlane, HyperbolicSpace }; // TODO: tetrahedral grid (different kinds)
    GenType sel;
    {
        const int N_CHOICES = 18;
        wxString dataset_types[N_CHOICES] = { _("1D image strip"), _("2D image"), _("3D image volume"),
            _("Geodesic sphere"), _("Torus"), _("Triangular mesh"), _("Hexagonal mesh"),
            _("Rhombille tiling"), _("Penrose tiling (rhombi)"), _("Penrose tiling (darts and kites)"),
            _("Random 2D Delaunay mesh (triangles)"), _("Random 2D Voronoi mesh"), _("Random 3D Delaunay honeycomb (tetrahedra)"),
            _("Body-centred cubic honeycomb (truncated octahedra)"), _("Face-centred cubic honeycomb (rhombic dodecahedra)"),
            _("Diamond honeycomb (triakis truncated tetrahedra)"), _("Hyperbolic plane tiling"), _("Hyperbolic space tiling") };
        wxSingleChoiceDialog dlg(this, _("Select a pattern type:"), _("New Pattern"), N_CHOICES, dataset_types);
        dlg.SetSelection(1); // default selection
        dlg.SetSize(wxDefaultCoord, 130+N_CHOICES*20); // increase dlg height so we see all choices without having to scroll
        if(dlg.ShowModal()!=wxID_OK) return;
        sel = static_cast<GenType>( dlg.GetSelection() );
    }

    this->SetStatusText(_("Generating..."));

    unique_ptr<AbstractRD> sys;
    Properties new_render_settings("render_settings");
    SetDefaultRenderSettings(new_render_settings);
    try
    {
        switch(sel)
        {
            case GenType::Image1D:          sys = MakeNewImage1D(this->is_opencl_available,opencl_platform,opencl_device,new_render_settings); break;
            case GenType::Image2D:          sys = MakeNewImage2D(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::Image3D:          sys = MakeNewImage3D(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::GeoSphere:        sys = MakeNewGeodesicSphere(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::Torus:            sys = MakeNewTorus(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::TriMesh:          sys = MakeNewTriangularMesh(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::HexMesh:          sys = MakeNewHexagonalMesh(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::Rhombille:        sys = MakeNewRhombilleTiling(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::PenroseP3:        sys = MakeNewPenroseP3Tiling(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::PenroseP2:        sys = MakeNewPenroseP2Tiling(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::Del2D:            sys = MakeNewDelaunay2D(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::Vor2D:            sys = MakeNewVoronoi2D(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::Del3D:            sys = MakeNewDelaunay3D(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::BodyCentredCubic: sys = MakeNewBodyCentredCubicHoneycomb(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::FaceCentredCubic: sys = MakeNewFaceCentredCubicHoneycomb(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::Diamond:          sys = MakeNewDiamondHoneycomb(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::HyperbolicPlane:  sys = MakeNewHyperbolicPlane(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
            case GenType::HyperbolicSpace:  sys = MakeNewHyperbolicSpace(this->is_opencl_available, opencl_platform, opencl_device, new_render_settings); break;
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

    if (!sys)
    {
        return; // user cancelled
    }

    this->render_settings = new_render_settings;

    wxBusyCursor busy;
    this->SetStatusText(_("Generating data values..."));
    sys->CreateDefaultInitialPatternGenerator(sys->GetNumberOfChemicals());
    sys->GenerateInitialPattern();
    this->SetCurrentRDSystem(std::move(sys));

    this->system->SetFilename("untitled");
    this->system->SetModified(false);
    this->UpdateWindows();
    this->SetStatusText(_("Ready"));
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
        OpenFile(opendlg.GetPath());
    }
}

// ---------------------------------------------------------------------

void MyFrame::OpenFile(const wxString& raw_path, bool remember)
{
    wxString path = FileNameToString(raw_path);

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
        wxMessageBox(_("File doesn't exist: ")+path, _("Error"), wxOK | wxCENTER | wxICON_ERROR);
        return;
    }

    if(UserWantsToCancelWhenAskedIfWantsToSave()) return;

    if(remember) AddRecentPattern(path);

    wxBeginBusyCursor();

    // load pattern file
    bool warn_to_update = false;
    unique_ptr<AbstractRD> target_system;
    Properties previous_render_settings = this->render_settings;
    try
    {
        SetDefaultRenderSettings(this->render_settings);
        target_system = SystemFactory::CreateFromFile(path.mb_str(),this->is_opencl_available,opencl_platform,opencl_device,this->render_settings,warn_to_update);
        this->patterns_panel->SelectPath(path);
        this->SetCurrentRDSystem(std::move(target_system));
    }
    catch(const exception& e)
    {
        wxEndBusyCursor();
        wxString message = warn_to_update ? _("This file is from a more recent version of Ready. You should download a newer version.\n\n") : _("");
        message += _("Failed to open file. Error:\n\n");
        message += wxString(e.what(),wxConvUTF8);
        MonospaceMessageBox(message,_("Error reading file"),wxART_ERROR);
        this->render_settings = previous_render_settings;
        return;
    }
    catch(...)
    {
        wxEndBusyCursor();
        wxString message = warn_to_update ? _("This file is from a more recent version of Ready. You should download a newer version.\n\n") : _("");
        message += _("Failed to open file.");
        MonospaceMessageBox(message,_("Error reading file"),wxART_ERROR);
        this->render_settings = previous_render_settings;
        return;
    }
    wxEndBusyCursor();
    if(warn_to_update)
    {
        wxMessageBox("This file is from a more recent version of Ready. For best results you should download a newer version.");
        // TODO: allow user to stop this message from appearing every time
    }
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
    #if wxCHECK_VERSION(3,1,3)
        // in 3.1.3+ wxExecute returns -1 if cmd fails (pid if success)
        if (result == -1)
    #else
        // pre 3.1.3, wxExecute returns -1 if cmd succeeds (bug, or wx docs are wrong)
        if (result != -1)
    #endif
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
    InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, false);
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
        SetAccelerator(mbar, ID::ImportImage,               DO_IMPORTIMAGE);
        SetAccelerator(mbar, ID::ExportImage,               DO_EXPORTIMAGE);
        SetAccelerator(mbar, wxID_SAVE,                     DO_SAVE);
        SetAccelerator(mbar, ID::SaveCompact,               DO_SAVECOMPACT);
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
        SetAccelerator(mbar, ID::BrushSizeExtraSmall,       DO_BRUSHEXTRASMALL);
        SetAccelerator(mbar, ID::BrushSizeSmall,            DO_BRUSHSMALL);
        SetAccelerator(mbar, ID::BrushSizeMedium,           DO_BRUSHMEDIUM);
        SetAccelerator(mbar, ID::BrushSizeLarge,            DO_BRUSHLARGE);
        SetAccelerator(mbar, ID::BrushSizeExtraLarge,       DO_BRUSHEXTRALARGE);

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
        SetAccelerator(mbar, ID::ConvertToFullKernel,       DO_CONVERTTOKERNEL);
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
                                if (fullscreen) { cmdid = ID::FullScreen; }
                                break;

        case DO_OPENFILE:       OpenFile(action.file);
                                return;

        // File menu
        case DO_NEWPATT:        cmdid = wxID_NEW; break;
        case DO_OPENPATT:       cmdid = wxID_OPEN; break;
        case DO_RELOAD:         cmdid = ID::ReloadFromDisk; break;
        case DO_IMPORTMESH:     cmdid = ID::ImportMesh; break;
        case DO_EXPORTMESH:     cmdid = ID::ExportMesh; break;
        case DO_IMPORTIMAGE:    cmdid = ID::ImportImage; break;
        case DO_EXPORTIMAGE:    cmdid = ID::ExportImage; break;
        case DO_SAVE:           cmdid = wxID_SAVE; break;
        case DO_SAVECOMPACT:    cmdid = ID::SaveCompact; break;
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
        case DO_BRUSHEXTRASMALL:cmdid = ID::BrushSizeExtraSmall; break;
        case DO_BRUSHSMALL:     cmdid = ID::BrushSizeSmall; break;
        case DO_BRUSHMEDIUM:    cmdid = ID::BrushSizeMedium; break;
        case DO_BRUSHLARGE:     cmdid = ID::BrushSizeLarge; break;
        case DO_BRUSHEXTRALARGE:cmdid = ID::BrushSizeExtraLarge; break;

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
        case DO_CONVERTTOKERNEL:cmdid = ID::ConvertToFullKernel; break;
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

void MyFrame::SetNumberOfChemicals(int n)
{
    try
    {
        this->system->SetNumberOfChemicals(n);
    }
    catch(const exception& e)
    {
        MonospaceMessageBox(_("Changing the number of chemicals caused an error:\n\n")+wxString(e.what(),wxConvUTF8),_("Error"),wxART_ERROR);
    }
    catch(...)
    {
        wxMessageBox(_("Changing the number of chemicals caused an unknown error"));
    }
    // (we allow the user to proceed because they might now want to change the formula/kernel to match)

    // limit active_chemical to legal range
    int ic = IndexFromChemicalName(this->render_settings.GetProperty("active_chemical").GetChemical());
    if(ic>=n)
        this->render_settings.GetProperty("active_chemical").SetChemical(GetChemicalName(n-1));
    InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, false);
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
        if( this->system->GetRuleType()=="formula" && ( x&(x - 1) || y&(y - 1) || z&(z - 1) ) )
        {
            return false; // for wrap-around in OpenCL we require all the dimensions to be powers of 2
        }
        // rearrange the dimensions (for visualization we need the z to be 1 for 2D images, and both y and z to be 1 for 1D images)
        if( (x==1 && (y>1 || z>1)) || (y==1 && z>1) )
        {
            float d[3]={ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
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
    this->system->BlankImage();
    this->system->GenerateInitialPattern();
    InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, true);
    this->UpdateWindows();
    return true;
}

// ---------------------------------------------------------------------

void MyFrame::SetBlockSize(int x,int y,int z)
{
    this->system->SetBlockSizeX(x);
    this->system->SetBlockSizeY(y);
    this->system->SetBlockSizeZ(z);
    this->UpdateInfoPane();
}

// ---------------------------------------------------------------------

void MyFrame::SetDataType(int data_type)
{
    const int old_data_type = this->system->GetDataType();
    try
    {
        this->system->SetDataType(data_type);
        InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, false);
        this->UpdateWindows();
    }
    catch (const exception& e)
    {
        MonospaceMessageBox(_("Failed to set data type:\n\n") + wxString(e.what(), wxConvUTF8), _("Error"), wxART_ERROR);
        this->system->SetDataType(old_data_type);
        InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, false);
        this->UpdateWindows();
    }
    catch (...)
    {
        wxMessageBox(_("Failed to set data type"));
        this->system->SetDataType(old_data_type);
        InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, false);
        this->UpdateWindows();
    }
}

// ---------------------------------------------------------------------

void MyFrame::RenderSettingsChanged()
{
    // first do some range checking (not done in InfoPanel::ChangeRenderSetting)
    Property& prop = this->render_settings.GetProperty("timesteps_per_render");
    if (prop.GetInt() < 1) prop.SetInt(1);
    if (prop.GetInt() > MAX_TIMESTEPS_PER_RENDER) prop.SetInt(MAX_TIMESTEPS_PER_RENDER);

    InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, false);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnAddParameter(wxCommandEvent& event)
{
    StringDialog dlg(this,_("Add a parameter"),_("Name:"),wxEmptyString,wxDefaultPosition,wxDefaultSize);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->GetCurrentRDSystem().AddParameter(string(dlg.GetValue().mb_str()),0.0f);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnDeleteParameter(wxCommandEvent& event)
{
    wxArrayString as;
    for(int i=0;i<this->GetCurrentRDSystem().GetNumberOfParameters();i++)
        as.Add(wxString(this->GetCurrentRDSystem().GetParameterName(i).c_str(),wxConvUTF8));
    wxSingleChoiceDialog dlg(this,_("Select a parameter to delete:"),_("Delete a parameter"),as);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->GetCurrentRDSystem().DeleteParameter(dlg.GetSelection());
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateAddParameter(wxUpdateUIEvent& event)
{
    event.Enable(this->GetCurrentRDSystem().GetRuleType()=="formula");
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateDeleteParameter(wxUpdateUIEvent& event)
{
    event.Enable(this->GetCurrentRDSystem().GetRuleType()=="formula" &&
                 this->GetCurrentRDSystem().GetNumberOfParameters() > 0);
}

// ---------------------------------------------------------------------

void MyFrame::OnRunFaster(wxCommandEvent& event)
{
    Property& prop = this->render_settings.GetProperty("timesteps_per_render");
    prop.SetInt(prop.GetInt() * 2);
    // check for overflow, or if beyond limit used in OnChangeRunningSpeed
    if (prop.GetInt() <= 0 || prop.GetInt() > MAX_TIMESTEPS_PER_RENDER) prop.SetInt(MAX_TIMESTEPS_PER_RENDER);
    this->UpdateInfoPane();
    this->UpdateToolbars();
}

// ---------------------------------------------------------------------

void MyFrame::OnRunSlower(wxCommandEvent& event)
{
    Property& prop = this->render_settings.GetProperty("timesteps_per_render");
    prop.SetInt(prop.GetInt() / 2);
    // don't let timesteps_per_render get to 0 otherwise OnRunFaster can't double it
    if (prop.GetInt() < 1) prop.SetInt(1);
    this->UpdateInfoPane();
    this->UpdateToolbars();
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
    this->UpdateToolbars(); // show the new value
}

// ---------------------------------------------------------------------

bool MyFrame::LoadMesh(const wxFileName& mesh_filename, vtkUnstructuredGrid* ug)
{
    bool all_ok = true;
    vtkObject::GlobalWarningDisplayOn(); // handling errors in this section
    try
    {
        if (mesh_filename.GetExt().Lower()==_T("vtp"))
        {
            vtkSmartPointer<vtkXMLPolyDataReader> vtp_reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
            vtp_reader->ReadFromInputStringOn();
            vtp_reader->SetInputString(ReadEntireFile(mesh_filename).ToStdString());
            vtp_reader->AddObserver(vtkCommand::ErrorEvent, vtkSmartPointer<ThrowOnErrorObserver>::New());
            vtp_reader->Update();
            ug->SetPoints(vtp_reader->GetOutput()->GetPoints());
            ug->SetCells(VTK_POLYGON, vtp_reader->GetOutput()->GetPolys());
        }
        else if (mesh_filename.GetExt().Lower()==_T("vtu"))
        {
            vtkSmartPointer<vtkXMLUnstructuredGridReader> vtu_reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
            vtu_reader->ReadFromInputStringOn();
            vtu_reader->SetInputString(ReadEntireFile(mesh_filename).ToStdString());
            vtu_reader->AddObserver(vtkCommand::ErrorEvent, vtkSmartPointer<ThrowOnErrorObserver>::New());
            vtu_reader->Update();
            ug->DeepCopy(vtu_reader->GetOutput());
        }
        else if (mesh_filename.GetExt().Lower() ==_T("obj"))
        {
            wxString source_filename(mesh_filename.GetFullPath());
            bool made_temp_file = false;
            if(strlen(mesh_filename.GetFullPath().mb_str())==0) {
                // unicode characters in path, copy to temp file
                const wxString temp_name = wxFileName::CreateTempFileName("");
                if (wxCopyFile(mesh_filename.GetFullPath(), temp_name)) {
                    source_filename = temp_name;
                    made_temp_file = true;
                }
            }
            {
                // temporarily turn off internationalisation, to avoid string-to-float conversion issues
                char* old_locale = setlocale(LC_NUMERIC, "C");

                vtkSmartPointer<vtkOBJReader> obj_reader = vtkSmartPointer<vtkOBJReader>::New();
                obj_reader->SetFileName(source_filename.mb_str());
                obj_reader->AddObserver(vtkCommand::ErrorEvent, vtkSmartPointer<ThrowOnErrorObserver>::New());
                obj_reader->Update();

                ug->SetPoints(obj_reader->GetOutput()->GetPoints());
                ug->SetCells(VTK_POLYGON, obj_reader->GetOutput()->GetPolys());

                // restore the old locale
                setlocale(LC_NUMERIC, old_locale);
            }
            if (made_temp_file) {
                wxRemoveFile(source_filename);
            }
        }
        else
        {
            throw runtime_error("Unsupported file type");
        }
    }
    catch (const exception& e)
    {
        wxMessageBox(_("Error importing mesh: ") + e.what(), _("Error"), wxOK | wxICON_ERROR);
        all_ok = false;
    }
    catch (...)
    {
        wxMessageBox(_("Unknown problem importing mesh"), _("Error"), wxOK | wxICON_ERROR);
        all_ok = false;
    }
    vtkObject::GlobalWarningDisplayOff(); // (turn off again to avoid message window appearing)
    return all_ok;
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

    wxString mesh_filename = wxFileSelector(_("Import a mesh:"), wxEmptyString, _("bunny.vtu"), wxEmptyString,
        _("Supported mesh formats (*.obj;*.vtu;*.vtp)|*.obj;*.vtu;*.vtp"), wxFD_OPEN);
    if (mesh_filename.empty()) return; // user cancelled

    wxArrayString choices;
    choices.Add(_("Run a pattern on the surface of this mesh"));
    choices.Add(_("Paint this pattern into a 3D volume image"));
    int choice = wxGetSingleChoiceIndex(_("What would you like to do with the mesh?"), _("Select one of these options:"), choices);
    if (choice == -1) return; // user cancelled

    if (UserWantsToCancelWhenAskedIfWantsToSave()) return;

    wxBusyCursor busy;

    vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
    bool ok = LoadMesh(mesh_filename, ug);
    if (!ok) return;

    SetDefaultRenderSettings(this->render_settings);
    this->render_settings.GetProperty("slice_3D").SetBool(false);
    this->render_settings.GetProperty("active_chemical").SetChemical("b");

    switch (choice)
    {
        default:
        case 0: MakeDefaultMeshSystemFromMesh(ug); break;
        case 1: MakeDefaultImageSystemFromMesh(ug); break;
    }
}

// ---------------------------------------------------------------------

void MyFrame::MakeDefaultImageSystemFromMesh(vtkUnstructuredGrid* ug)
{
    size_t num_chemicals = 2;
    size_t target_chemical = 1;
    size_t largest_dimension = 64;
    float value_inside = 0.0f;
    float value_outside = 1.0f;

    unique_ptr<ImageRD> image_sys;
    if (this->is_opencl_available)
    {
        IntegerDialog nc_dlg(this, _("Number of chemicals in new volume image"), _("Number of chemicals:"),
            3, 1, 20, wxDefaultPosition, wxDefaultSize);
        if (nc_dlg.ShowModal() != wxID_OK) return;
        num_chemicals = nc_dlg.GetValue();

        wxArrayString choices;
        for (size_t i = 0; i<num_chemicals; i++)
            choices.Add(GetChemicalName(i));
        wxSingleChoiceDialog tc_dlg(this, _("Select the chemical to write the volume into:"), _("Select target chemical"),
            choices);
        tc_dlg.SetSelection(2);
        if (tc_dlg.ShowModal() != wxID_OK) return;
        target_chemical = tc_dlg.GetSelection();

        IntegerDialog ld_dlg(this, _("Largest dimension of the new volume image"), _("Largest dimension:"),
            32, 1, 1024, wxDefaultPosition, wxDefaultSize);
        if (ld_dlg.ShowModal() != wxID_OK) return;
        largest_dimension = ld_dlg.GetValue();

        FloatDialog inval_dlg(this, _("Value to set inside the mesh"), _("Value inside:"),
            0.1f, wxDefaultPosition, wxDefaultSize);
        if (inval_dlg.ShowModal() != wxID_OK) return;
        value_inside = inval_dlg.GetValue();

        FloatDialog outval_dlg(this, _("Value to set outside the mesh"), _("Value outside:"),
            0.5f, wxDefaultPosition, wxDefaultSize);
        if (outval_dlg.ShowModal() != wxID_OK) return;
        value_outside = outval_dlg.GetValue();

        // at some point we would want the user to decide what data type to use in the image
        const int data_type = VTK_FLOAT;

        image_sys = make_unique<FormulaOpenCLImageRD>(opencl_platform, opencl_device, data_type);
        image_sys->SetFormula("delta_a = D_a * laplacian_a - a*b*b + F*(1.0f-a);\ndelta_b = D_b * laplacian_b + a*b*b - (F+K+c*0.035f)*b;");
    }
    else {
        image_sys = make_unique<GrayScottImageRD>();
    }

    image_sys->CopyFromMesh(ug, num_chemicals, target_chemical, largest_dimension, value_inside, value_outside);
    image_sys->CreateDefaultInitialPatternGenerator(2);
    image_sys->GenerateInitialPattern();
    this->render_settings.GetProperty("timesteps_per_render").SetInt(16);
    this->SetCurrentRDSystem(std::move(image_sys));
}

// ---------------------------------------------------------------------

void MyFrame::MakeDefaultMeshSystemFromMesh(vtkUnstructuredGrid* ug)
{
    // at some point we would want the user to decide what data type to use on the imported mesh
    const int data_type = VTK_FLOAT;

    unique_ptr<MeshRD> mesh_sys;
    if (this->is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(ug);
    mesh_sys->SetNumberOfChemicals(2);
    mesh_sys->CreateDefaultInitialPatternGenerator(2);
    mesh_sys->GenerateInitialPattern();
    this->SetCurrentRDSystem(std::move(mesh_sys));
}

// ---------------------------------------------------------------------

void MyFrame::OnExportMesh(wxCommandEvent& event)
{
    // possible uses: (context dependent)
    // 1. output MeshRD surface (although can already use Paraview to convert VTU to OBJ etc.)
    // 2. output ImageRD 3d-image contour for active chemical
    // 3. output ImageRD 2d-image displacement-mapped surface for active chemical

    wxString mesh_filename = wxFileSelector(_("Export a mesh:"), wxEmptyString, wxEmptyString, wxEmptyString,
        _("Supported mesh formats (*.obj;*.ply;*.vtp)|*.obj;*.ply;*.vtp"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (mesh_filename.empty()) return; // user cancelled

    SaveCurrentMesh(mesh_filename,false,0.0);
}

// ---------------------------------------------------------------------

void MyFrame::SaveCurrentMesh(const wxFileName& mesh_filename, bool should_decimate, double targetReduction)
{
    vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
    this->system->GetAsMesh(mesh,this->render_settings);

    if (should_decimate)
    {
        vtkSmartPointer<vtkQuadricDecimation> dec = vtkSmartPointer<vtkQuadricDecimation>::New();
        dec->SetInputData(mesh);
        dec->SetTargetReduction(targetReduction);
        dec->Update();
        mesh->DeepCopy(dec->GetOutput());
    }

    vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
    normals->SetInputData(mesh);
    normals->SplittingOff();
    normals->Update();
    vtkPolyData* pd = normals->GetOutput();

    if(mesh_filename.GetExt().Lower() == _T("obj"))
    {
        wxBusyCursor busy;
        wxFileOutputStream to_file(mesh_filename.GetFullPath());
        wxTextOutputStream out(to_file);
        out << "# Output from Ready - https://github.com/GollyGang/ready\n";
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
        for(vtkIdType iCell=0;iCell<pd->GetPolys()->GetNumberOfCells();iCell++)
        {
            vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
            pd->GetCellPoints(iCell, ids);
            out << "f";
            if(pd->GetPointData()->GetNormals())
            {
                for(vtkIdType iPt = 0; iPt < ids->GetNumberOfIds(); iPt++)
                    out << " " << wxInt32(ids->GetId(iPt) + 1) << "//" << wxInt32(ids->GetId(iPt) + 1); // (OBJ indices are 1-based)
            }
            else
            {
                for(vtkIdType iPt = 0; iPt < ids->GetNumberOfIds(); iPt++)
                    out << " " << wxInt32(ids->GetId(iPt) + 1); // (OBJ indices are 1-based)
            }
            out << "\n";
        }
    }
    else if(mesh_filename.GetExt().Lower() == _T("vtp"))
    {
        wxBusyCursor busy;
        vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
        writer->SetInputData(pd);
        wxFileOutputStream to_file(mesh_filename.GetFullPath());
        wxTextOutputStream out(to_file);
        writer->SetWriteToOutputString(true); // workaround because VTK doesn't yet allow unicode filepaths
        writer->Write();
        out << writer->GetOutputString();
    }
    else if(mesh_filename.GetExt().Lower() == _T("ply"))
    {
        wxBusyCursor busy;
        vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
        to_point_data->SetInputData(pd);
        vtkSmartPointer<vtkPLYWriter> writer = vtkSmartPointer<vtkPLYWriter>::New();
        writer->SetInputConnection(to_point_data->GetOutputPort());
        writer->SetFileName(mesh_filename.GetFullPath());
        vtkSmartPointer<vtkScalarsToColors> lut = GetColorMap(this->render_settings);
        writer->SetLookupTable(lut);
        writer->SetArrayName(this->render_settings.GetProperty("active_chemical").GetChemical().c_str());
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

void MyFrame::OnImportImage(wxCommandEvent &event)
{
    wxFileDialog opendlg(this,
        _("Choose an image file to import"),
        wxEmptyString,
        wxEmptyString,
        _("Image files (*.jpg;*.png;*.bmp)|*.jpg;*.jpeg;*.png;*.bmp"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (opendlg.ShowModal() != wxID_OK)
        return;

    double scalar_range[2];
    GetScalarRangeFromImage(opendlg.GetPath(), scalar_range);

    ImportImageDialog dlg(this,
                          opendlg.GetPath(),
                          this->system->GetNumberOfChemicals(),
                          IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical()),
                          scalar_range[0],
                          scalar_range[1],
                          this->render_settings.GetProperty("low").GetFloat(),
                          this->render_settings.GetProperty("high").GetFloat());
    if (dlg.ShowModal() != wxID_OK) return;
    wxFileName filename = dlg.image_filename;

    vtkSmartPointer<vtkImageReader2> reader;
    wxString ext = filename.GetExt();
    if (ext.IsSameAs("png", false)) reader = vtkSmartPointer<vtkPNGReader>::New();
    else if (ext.IsSameAs("jpg", false) || ext.IsSameAs("jpeg", false)) reader = vtkSmartPointer<vtkJPEGReader>::New();
    else if (ext.IsSameAs("bmp", false)) reader = vtkSmartPointer<vtkBMPReader>::New();
    else {
        wxMessageBox(_("Unsupported extension: ") + ext, _("Error reading image file"), wxICON_ERROR);
        return;
    }
    reader->SetFileName(FileNameToString(filename).mb_str());

    // for now we convert the image to grayscale
    vtkSmartPointer<vtkImageLuminance> to_gray = vtkSmartPointer<vtkImageLuminance>::New();
    to_gray->SetInputConnection(reader->GetOutputPort());

    // apply the maths requested
    vtkSmartPointer<vtkImageShiftScale> to_range = vtkSmartPointer<vtkImageShiftScale>::New();
    to_range->SetInputConnection(to_gray->GetOutputPort());
    float scale = (dlg.out_high - dlg.out_low) / (dlg.in_high - dlg.in_low);
    float shift = dlg.out_low / scale - dlg.in_low;
    to_range->SetShift(shift);
    to_range->SetScale(scale);
    to_range->SetOutputScalarType(this->system->GetDataType());

    // scale to the correct size
    vtkSmartPointer<vtkImageResize> resize = vtkSmartPointer<vtkImageResize>::New();
    resize->SetInputConnection(to_range->GetOutputPort());
    resize->SetOutputDimensions(this->system->GetX(), this->system->GetY(), this->system->GetZ());
    // TODO: obey 3D slice direction when resizing a 2D image for a 3D volume?
    resize->Update();

    // overwrite the requested chemical
    this->system->SetFrom2DImage(dlg.iTargetChemical, resize->GetOutput());

    this->is_running = false;
    InitializeVTKPipeline(this->pVTKWindow, *this->system, this->render_settings, false);
    this->UpdateWindows();
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateImportImage(wxUpdateUIEvent& event)
{
    event.Enable(this->system->Is2DImageAvailable());
}

// ---------------------------------------------------------------------

void MyFrame::OnExportImage(wxCommandEvent &event)
{
    wxFileName filename = FindUnusedFilename(screenshotdir, _("Ready_image_%04d.png"));

    if (!AskUserWhereToSaveImage(filename))
        return; // user cancelled

    screenshotdir = filename.GetPath(); // default to this location next time

    vtkSmartPointer<vtkImageChangeInformation> passthrough = vtkSmartPointer<vtkImageChangeInformation>::New();
    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    system->GetAs2DImage(image, this->render_settings);
    passthrough->SetInputData(image);

    WriteImageToFile(passthrough->GetOutputPort(), filename);
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateExportImage(wxUpdateUIEvent& event)
{
    event.Enable(this->system->Is2DImageAvailable());
}

// ---------------------------------------------------------------------

void MyFrame::RecordFrame()
{
    ostringstream oss;

    if (this->record_3D_surface)
    {
        // save the 3D mesh
        oss << this->recording_prefix << setfill('0') << setw(6) << this->iRecordingFrame << this->recording_extension;
        SaveCurrentMesh(wxString(oss.str().c_str()), this->recording_should_decimate, this->recording_target_reduction);
    }
    else
    {
        vtkSmartPointer<vtkImageWriter> writer;
        if (this->recording_extension == _T(".png")) writer = vtkSmartPointer<vtkPNGWriter>::New();
        else if (this->recording_extension == _T(".jpg")) writer = vtkSmartPointer<vtkJPEGWriter>::New();

        // store the currently active chemical, in the case of multi-reagent output it needs to be restored later.
        std::string remember_chemical = this->render_settings.GetProperty("active_chemical").GetChemical();

        if (this->record_data_image) // take the 2D data (2D system or 2D slice)
        {
            if (this->record_all_chemicals)
            {
                int num_chems = this->system->GetNumberOfChemicals();
                for (int chemical_number = 0; chemical_number < num_chems; chemical_number++)
                {
                    // make modified name for chemicals.
                    oss.str("");
                    oss.clear();
                    std::string chemical_name = GetChemicalName(chemical_number);
                    oss << this->recording_prefix << chemical_name << "_" << setfill('0') << setw(6) << this->iRecordingFrame << this->recording_extension;

                    this->render_settings.GetProperty("active_chemical").SetChemical(GetChemicalName(chemical_number));

                    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
                    this->system->GetAs2DImage(image, this->render_settings);
                    writer->SetInputData(image);
                    if (chemical_number < num_chems - 1) //write all but the last one as it will get written by the code below.
                    {
                        writer->SetFileName(oss.str().c_str());
                        writer->Write();
                        if (this->recording_extension == _T(".png")) writer = vtkSmartPointer<vtkPNGWriter>::New();
                        else if (this->recording_extension == _T(".jpg")) writer = vtkSmartPointer<vtkJPEGWriter>::New();
                    }
                }
            }
            else
            {
                oss << this->recording_prefix << setfill('0') << setw(6) << this->iRecordingFrame << this->recording_extension;
                vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
                this->system->GetAs2DImage(image, this->render_settings);
                writer->SetInputData(image);
            }
        }
        else // take a screenshot of the current view
        {
            oss << this->recording_prefix << setfill('0') << setw(6) << this->iRecordingFrame << this->recording_extension;
            vtkSmartPointer<vtkWindowToImageFilter> screenshot = vtkSmartPointer<vtkWindowToImageFilter>::New();
            screenshot->SetInput(this->pVTKWindow->GetRenderWindow());
            writer->SetInputConnection(screenshot->GetOutputPort());
        }

        writer->SetFileName(oss.str().c_str());
        writer->Write();

        if (this->record_all_chemicals)
        {
            // restore the stored active chemical so that the user still sees what they usually see in the viewport.
            // only do this in the case of record_all_chemicals (ie when it has potentially changed), ere it may cause wasted cycles?.
            this->render_settings.GetProperty("active_chemical").SetChemical(remember_chemical);
        }
    }

    this->iRecordingFrame++;
}

// ---------------------------------------------------------------------

void MyFrame::OnRecordFrames(wxCommandEvent &event)
{
    if (this->is_recording)
    {
        this->is_recording = false;
        return;
    }

    bool is_2D_data_available = this->system->Is2DImageAvailable();
    bool are_multiple_chemicals_available = is_2D_data_available && (this->system->GetNumberOfChemicals() > 1);
    bool default_to_2D_data = ( is_2D_data_available && this->system->GetArenaDimensionality()==2
                                && !this->render_settings.GetProperty("show_displacement_mapped_surface").GetBool()
                                && !this->render_settings.GetProperty("show_phase_plot").GetBool() );
    bool is_3D_surface_available = this->system->GetArenaDimensionality() > 1;

    RecordingDialog dlg(this,is_2D_data_available,are_multiple_chemicals_available,default_to_2D_data,is_3D_surface_available);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->recording_prefix = dlg.recording_prefix;
    this->recording_extension = dlg.recording_extension;
    this->record_data_image = dlg.record_data_image;
    this->record_all_chemicals = dlg.record_all_chemicals;
    this->record_3D_surface = dlg.record_3D_surface;
    this->recording_should_decimate = dlg.should_decimate;
    this->recording_target_reduction = 1.0 - dlg.target_reduction / 100.0; // convert from target percentage to proportion reduction

    this->iRecordingFrame = 0;
    this->is_recording = true;
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
    event.Enable(true);// this->system->GetRuleType() == "formula");
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectPointerTool(wxCommandEvent& event)
{
    this->CurrentCursor = TCursorType::POINTER;
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
    event.Check(this->CurrentCursor == TCursorType::POINTER);
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectPencilTool(wxCommandEvent& event)
{
    this->CurrentCursor = TCursorType::PENCIL;
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
    event.Check(this->CurrentCursor == TCursorType::PENCIL);
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectBrushTool(wxCommandEvent& event)
{
    this->CurrentCursor = TCursorType::BRUSH;
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
    event.Check(this->CurrentCursor == TCursorType::BRUSH);
}

// ---------------------------------------------------------------------

void MyFrame::OnSelectPickerTool(wxCommandEvent& event)
{
    this->CurrentCursor = TCursorType::PICKER;
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
    event.Check(this->CurrentCursor == TCursorType::PICKER);
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
            case TCursorType::POINTER:
                break; // (VTK will handle the control of the viewpoint)
            case TCursorType::PENCIL:
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
            case TCursorType::BRUSH:
            {
                this->system->SetValuesInRadius(p[0],p[1],p[2],this->brush_sizes[current_brush_size],
                    this->current_paint_value,this->render_settings);
                this->pVTKWindow->Refresh();
            }
            break;
            case TCursorType::PICKER:
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
        if(this->CurrentCursor == TCursorType::PENCIL)
            this->pVTKWindow->SetCursor(*this->pencil_cursor);
        else if(this->CurrentCursor == TCursorType::BRUSH )
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
            case TCursorType::POINTER:
                break; // (VTK will handle the control of the viewpoint)
            case TCursorType::PENCIL:
            {
                if (erasing) {
                    this->system->SetValue(p[0],p[1],p[2],this->render_settings.GetProperty("low").GetFloat(),this->render_settings);
                } else {
                    this->system->SetValue(p[0],p[1],p[2],this->current_paint_value,this->render_settings);
                }
                this->pVTKWindow->Refresh();
            }
            break;
            case TCursorType::BRUSH:
            {
                this->system->SetValuesInRadius(p[0],p[1],p[2],this->brush_sizes[current_brush_size],
                    this->current_paint_value,this->render_settings);
                this->pVTKWindow->Refresh();
            }
            break;
            case TCursorType::PICKER:
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
    if(this->pVTKWindow->GetShiftKey() && ( this->CurrentCursor == TCursorType::PENCIL || this->CurrentCursor == TCursorType::BRUSH ) )
        this->pVTKWindow->SetCursor(*this->picker_cursor);
}

// ---------------------------------------------------------------------

void MyFrame::KeyUp()
{
    if(!this->pVTKWindow->GetShiftKey())
    {
        if(this->CurrentCursor == TCursorType::PENCIL)
            this->pVTKWindow->SetCursor(*this->pencil_cursor);
        else if(this->CurrentCursor == TCursorType::BRUSH )
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
        this->current_paint_value,&this->current_paint_value,wxDefaultPosition,wxSize(300,wxDefaultCoord)))
    {
        this->UpdateToolbars();
    }
}

// ---------------------------------------------------------------------

void MyFrame::OnBrushSizeExtraSmall(wxCommandEvent& event)
{
    current_brush_size = 0;
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateBrushSizeExtraSmall(wxUpdateUIEvent& event)
{
    event.Check(current_brush_size == 0);
}

// ---------------------------------------------------------------------

void MyFrame::OnBrushSizeSmall(wxCommandEvent& event)
{
    current_brush_size = 1;
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateBrushSizeSmall(wxUpdateUIEvent& event)
{
    event.Check(current_brush_size == 1);
}

// ---------------------------------------------------------------------

void MyFrame::OnBrushSizeMedium(wxCommandEvent& event)
{
    current_brush_size = 2;
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateBrushSizeMedium(wxUpdateUIEvent& event)
{
    event.Check(current_brush_size==2);
}

// ---------------------------------------------------------------------

void MyFrame::OnBrushSizeLarge(wxCommandEvent& event)
{
    current_brush_size = 3;
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateBrushSizeLarge(wxUpdateUIEvent& event)
{
    event.Check(current_brush_size==3);
}

// ---------------------------------------------------------------------

void MyFrame::OnBrushSizeExtraLarge(wxCommandEvent& event)
{
    current_brush_size = 4;
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateBrushSizeExtraLarge(wxUpdateUIEvent& event)
{
    event.Check(current_brush_size==4);
}

// ---------------------------------------------------------------------

void MyFrame::OnSaveCompact(wxCommandEvent& event)
{
    wxString filename = SavePatternDialog();
    if(filename.empty()) return;

    if (strlen(filename.mb_str()) == 0)
    {
        wxMessageBox(_("Unsupported characters in path"), _("Error"), wxOK | wxCENTER | wxICON_ERROR);
        return;
    }

    wxBeginBusyCursor();

    // TODO: might be preferable to not adopt the saved file
    this->system->BlankImage();
    try
    {
        this->system->SaveFile(filename.mb_str(),this->render_settings,true);
    }
    catch (const exception& e)
    {
        wxEndBusyCursor();
        string message;
        message += _("Failed to save file. Error:\n\n");
        message += wxString(e.what(), wxConvUTF8);
        MonospaceMessageBox(message, _("Error saving file"), wxART_ERROR);
    }
    catch (...)
    {
        wxEndBusyCursor();
        string message;
        message += _("Failed to save file.");
        MonospaceMessageBox(message, _("Error saving file"), wxART_ERROR);
    }
    this->is_running = false;
    this->UpdateWindows();

    wxEndBusyCursor();
}

// ---------------------------------------------------------------------

void MyFrame::OnConvertToFullKernel(wxCommandEvent& event)
{
    unique_ptr<AbstractRD> sys;
    try
    {
        if(this->system->GetFileExtension()=="vti")
        {
            sys = make_unique<FullKernelOpenCLImageRD>(dynamic_cast<const OpenCLImageRD&>(*this->system));
        }
        else if(this->system->GetFileExtension()=="vtu")
        {
            sys = make_unique<FullKernelOpenCLMeshRD>(dynamic_cast<const OpenCLMeshRD&>(*this->system));
        }
        else wxMessageBox(_T("Internal error: unrecognised file extension"));
    }
    catch(const exception& e)
    {
        wxMessageBox(wxString::Format(_T("Error converting rule: %s"),e.what()));
        return;
    }
    this->SetCurrentRDSystem(std::move(sys));
}

// ---------------------------------------------------------------------

void MyFrame::OnUpdateConvertToFullKernel(wxUpdateUIEvent& event)
{
    event.Enable(this->system->GetRuleType()=="formula");
}

// ---------------------------------------------------------------------
