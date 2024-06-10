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

/// Options for importing an image.
class ImportImageDialog : public wxDialog
{
    public:

        ImportImageDialog(wxWindow *parent, wxString filename, int num_chemicals, int i_target_chemical, float in_low, float in_high, float out_low, float out_high);

        bool TransferDataFromWindow();  /// called when user hits OK
    
        wxString image_filename;
        int iTargetChemical;
        double in_low;
        double in_high;
        double out_low;
        double out_high;

    protected:
      
        void OnChangeFilename(wxCommandEvent& event);

    protected:

        wxTextCtrl *filename_edit;
        wxComboBox *chemical_combo;
        wxTextCtrl *in_low_edit;
        wxTextCtrl *in_high_edit;
        wxTextCtrl *out_low_edit;
        wxTextCtrl *out_high_edit;

    private:

        DECLARE_EVENT_TABLE()
};
