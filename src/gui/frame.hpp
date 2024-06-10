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
#include <wx/aui/aui.h>
#include <wx/filename.h>

// local:
class PatternsPanel;
class InfoPanel;
class HelpPanel;
class wxVTKRenderWindowInteractor;
#include "InteractorStylePainter.hpp"

// readybase
#include "AbstractRD.hpp"
#include "Properties.hpp"

// VTK:
class vtkUnstructuredGrid;

/// The wxFrame-derived top-level window for the Ready GUI.
class MyFrame : public wxFrame, public IPaintHandler
{
    public:

        MyFrame(const wxString& title);
        ~MyFrame();

        // interface with PatternsPanel
        void OpenFile(const wxString& path, bool remember = true);
        void EditFile(const wxString& path);
        bool UserWantsToCancelWhenAskedIfWantsToSave();

        // interface with InfoPanel
        AbstractRD& GetCurrentRDSystem() { return *this->system; }
        void SetRuleName(std::string s);
        void SetDescription(std::string s);
        void SetParameter(int iParam,float val);
        void SetParameterName(int iParam,std::string s);
        void SetFormula(std::string s);
        void SetNumberOfChemicals(int n);
        bool SetDimensions(int x,int y,int z);
        void SetBlockSize(int x,int y,int z);
        void SetDataType(int data_type);
        Properties& GetRenderSettings() { return this->render_settings; }
        void RenderSettingsChanged();

        // interface with Preferences dialog
        void ShowPrefsDialog(const wxString& page = wxEmptyString);
        void UpdateMenuAccelerators();

        // handle keyboard shortcuts not appearing in menu items
        void OnKeyDown(wxKeyEvent& event);
        void OnChar(wxKeyEvent& event);
        void ProcessKey(int key, int modifiers);

        bool IsFullScreen() const { return this->fullscreen; }

        // implementation of IPaintHandler interface
        virtual void LeftMouseDown(int x,int y);
        virtual void LeftMouseUp(int x,int y);
        virtual void RightMouseDown(int x,int y);
        virtual void RightMouseUp(int x,int y);
        virtual void MouseMove(int x,int y);
        virtual void KeyDown();
        virtual void KeyUp();

    private:

        // File menu
        void OnNewPattern(wxCommandEvent& event);
        void OnOpenPattern(wxCommandEvent& event);
        void OnReloadFromDisk(wxCommandEvent& event);
        void OnSavePattern(wxCommandEvent& event);
        void OnSaveCompact(wxCommandEvent& event);
        void OnScreenshot(wxCommandEvent& event);
        void OnRecordFrames(wxCommandEvent& event);
        void OnUpdateRecordFrames(wxUpdateUIEvent& event);
        void OnAddMyPatterns(wxCommandEvent& event);
        void OnPreferences(wxCommandEvent& event);
        void OnImportMesh(wxCommandEvent& event);
        void OnExportMesh(wxCommandEvent& event);
        void OnImportImage(wxCommandEvent& event);
        void OnUpdateImportImage(wxUpdateUIEvent& event);
        void OnExportImage(wxCommandEvent& event);
        void OnUpdateExportImage(wxUpdateUIEvent& event);
        void OnQuit(wxCommandEvent& event);

        // Open Recent submenu
        void OnOpenRecent(wxCommandEvent& event);
        void AddRecentPattern(const wxString& path);
        void OpenRecentPattern(int id);
        void ClearMissingPatterns();
        void ClearAllPatterns();

        // Edit menu
        void OnUndo(wxCommandEvent& event);
        void OnUpdateUndo(wxUpdateUIEvent& event);
        void OnRedo(wxCommandEvent& event);
        void OnUpdateRedo(wxUpdateUIEvent& event);
        void OnCut(wxCommandEvent& event);
        void OnCopy(wxCommandEvent& event);
        void OnPaste(wxCommandEvent& event);
        void OnUpdatePaste(wxUpdateUIEvent& event);
        void OnClear(wxCommandEvent& event);
        void OnSelectAll(wxCommandEvent& event);
        void OnSelectPointerTool(wxCommandEvent& event);
        void OnUpdateSelectPointerTool(wxUpdateUIEvent& event);
        void OnSelectPencilTool(wxCommandEvent& event);
        void OnUpdateSelectPencilTool(wxUpdateUIEvent& event);
        void OnSelectBrushTool(wxCommandEvent& event);
        void OnUpdateSelectBrushTool(wxUpdateUIEvent& event);
        void OnSelectPickerTool(wxCommandEvent& event);
        void OnUpdateSelectPickerTool(wxUpdateUIEvent& event);

        void OnBrushSizeExtraSmall(wxCommandEvent& event);
        void OnUpdateBrushSizeExtraSmall(wxUpdateUIEvent& event);
        void OnBrushSizeSmall(wxCommandEvent& event);
        void OnUpdateBrushSizeSmall(wxUpdateUIEvent& event);
        void OnBrushSizeMedium(wxCommandEvent& event);
        void OnUpdateBrushSizeMedium(wxUpdateUIEvent& event);
        void OnBrushSizeLarge(wxCommandEvent& event);
        void OnUpdateBrushSizeLarge(wxUpdateUIEvent& event);
        void OnBrushSizeExtraLarge(wxCommandEvent& event);
        void OnUpdateBrushSizeExtraLarge(wxUpdateUIEvent& event);

        // View menu
        void OnFullScreen(wxCommandEvent& event);
        void OnFitPattern(wxCommandEvent& event);
        void OnWireframe(wxCommandEvent& event);
        void OnUpdateWireframe(wxUpdateUIEvent& event);
        void OnToggleViewPane(wxCommandEvent& event);
        void OnUpdateViewPane(wxUpdateUIEvent& event);
        void OnRestoreDefaultPerspective(wxCommandEvent& event);
        void OnChangeActiveChemical(wxCommandEvent& event);
        void OnViewFullKernel(wxCommandEvent& event);
        void OnUpdateViewFullKernel(wxUpdateUIEvent& event);
        void OnOpenCLDiagnostics(wxCommandEvent& event);

        // Action menu
        void OnStep(wxCommandEvent& event);
        void OnUpdateStep(wxUpdateUIEvent& event);
        void OnRunStop(wxCommandEvent& event);
        void OnUpdateRunStop(wxUpdateUIEvent& event);
        void OnRunFaster(wxCommandEvent& event);
        void OnRunSlower(wxCommandEvent& event);
        void OnChangeRunningSpeed(wxCommandEvent& event);
        void OnReset(wxCommandEvent& event);
        void OnUpdateReset(wxUpdateUIEvent& event);
        void OnGenerateInitialPattern(wxCommandEvent& event);
        void OnBlank(wxCommandEvent& event);
        void OnAddParameter(wxCommandEvent& event);
        void OnUpdateAddParameter(wxUpdateUIEvent& event);
        void OnDeleteParameter(wxCommandEvent& event);
        void OnUpdateDeleteParameter(wxUpdateUIEvent& event);
        void OnConvertToFullKernel(wxCommandEvent& event);
        void OnUpdateConvertToFullKernel(wxUpdateUIEvent& event);
        void OnSelectOpenCLDevice(wxCommandEvent& event);

        // Help menu
        void OnAbout(wxCommandEvent& event);
        void OnHelp(wxCommandEvent& event);

        // Paint toolbar:
        void OnChangeCurrentColor(wxCommandEvent& event);

        // other event handlers
        void OnActivate(wxActivateEvent& event);
        void OnIdle(wxIdleEvent& event);
        void OnSize(wxSizeEvent& event);
        void OnClose(wxCloseEvent& event);

        // internal functions

        void InitializeMenus();
        void InitializeToolbars();
        void InitializeCursors();
        void InitializePatternsPane();
        void InitializeInfoPane();
        void UpdateInfoPane();
        void InitializeHelpPane();
        void InitializeRenderPane();
        void LoadSettings();
        void SaveSettings();
        void CheckFocus();
        void EnableAllMenus(bool enable);

        void SetCurrentRDSystem(std::unique_ptr<AbstractRD> system);
        void UpdateWindows();
        void UpdateWindowTitle();
        void UpdateToolbars();
        void SetStatusBarText();
        void RecordFrame();

        bool LoadMesh(const wxFileName& filename, vtkUnstructuredGrid* ug);
        void MakeDefaultImageSystemFromMesh(vtkUnstructuredGrid* ug);
        void MakeDefaultMeshSystemFromMesh(vtkUnstructuredGrid* ug);

        wxString SavePatternDialog();   // return empty path if user cancels
        void SaveFile(const wxString& path);
        void SaveCurrentMesh(const wxFileName& mesh_filename, bool should_decimate, double targetReduction);

    private:

        // using wxAUI for window management
        wxAuiManager aui_mgr;
        wxString default_perspective;

        // VTK does the rendering
        vtkSmartPointer<wxVTKRenderWindowInteractor> pVTKWindow;

        // current system being simulated (in future we might want more than one)
        std::unique_ptr<AbstractRD> system;

        // panes:
        PatternsPanel *patterns_panel;
        InfoPanel *info_panel;
        HelpPanel *help_panel;
        // toolbars:
        wxAuiToolBar *file_toolbar,*action_toolbar,*paint_toolbar;

        // settings:
        Properties render_settings;

        // following are used when running a simulation:
        bool is_running;
        int num_steps;
        bool do_one_render;

        // used for reporting speed:
        int steps_since_last_render;
        double computation_time_since_last_render, computed_frames_per_second_buffer[10];
        double time_at_last_render, percentage_spent_rendering;
        double smoothed_timesteps_per_second,timesteps_per_second_buffer[10];
        int i_timesteps_per_second_buffer;
        bool speed_data_available;

        // used when recording frames to disk
        bool is_recording,record_data_image,record_all_chemicals,record_3D_surface,recording_should_decimate;
        std::string recording_prefix,recording_extension;
        int iRecordingFrame;
        float recording_target_reduction;

        static const int MAX_TIMESTEPS_PER_RENDER = 1e8;

        int realkey;  // used to pass info from OnKeyDown to OnChar

        bool fullscreen;    // in full screen mode?
        bool is_opencl_available;

        // toolbar things
        enum class TCursorType { POINTER, PENCIL, BRUSH, PICKER } CurrentCursor;
        std::unique_ptr<wxCursor> pencil_cursor, brush_cursor, picker_cursor;
        float current_paint_value;
        bool left_mouse_is_down,right_mouse_is_down;
        wxString icons_folder;
        bool erasing;
        static const float brush_sizes[5];

        DECLARE_EVENT_TABLE()
};
