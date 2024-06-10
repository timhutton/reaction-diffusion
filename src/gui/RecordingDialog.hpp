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

/// Options for recording the frames of a simulation as images to disk.
class RecordingDialog : public wxDialog
{
    public:

        RecordingDialog(wxWindow *parent,
                        bool is_2D_data_available,
                        bool are_multiple_chemicals_available,
                        bool default_is_2D_data,
                        bool is_3D_surface_available);

        bool Validate();                /// checks for value correctness
        bool TransferDataFromWindow();  /// called when user hits OK
    
        std::string recording_prefix;
        std::string recording_extension;
        bool record_data_image;
        bool record_all_chemicals;
        bool record_3D_surface;
        bool should_decimate;
        double target_reduction;

    protected:

        void OnSourceSelectionChange(wxCommandEvent& event);
        void OnChangeFolder(wxCommandEvent& event);

    protected:

        const wxString source_current_view;
        const wxString source_2D_data;
        const wxString source_2D_data_all_chemicals;
        const wxString source_3D_surface;

        wxComboBox *source_combo;
        wxComboBox *extension_combo;
        wxTextCtrl *folder_edit;
        wxTextCtrl *filename_prefix_edit;
        wxCheckBox *should_decimate_check;
        wxTextCtrl *target_reduction_edit;

    private:

        DECLARE_EVENT_TABLE()
};
