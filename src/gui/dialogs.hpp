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
#include <wx/artprov.h>
#include <wx/string.h>
#include <wx/spinctrl.h>    // for wxSpinCtrl

// readybase:
class ImageRD;

//----------------------------------------------------------------------------------

/// Displays a block of text in a monospace font.
class MonospaceMessageBox: public wxDialog
{
    public: 
        MonospaceMessageBox(const wxString& message, const wxString& title, const wxArtID& icon);
};

//----------------------------------------------------------------------------------

/// A modal dialog for editing integer X,Y,Z values.
class XYZIntDialog : public wxDialog
{
    public:
        XYZIntDialog(wxWindow* parent, const wxString& title,
                  int inx, int iny, int inz,
                  const wxPoint& pos, const wxSize& size);
    
        bool ValidNumber(wxTextCtrl* box, int* val);
    
        virtual bool TransferDataFromWindow();  // called when user hits OK
    
        int GetX() { return xval; }
        int GetY() { return yval; }
        int GetZ() { return zval; }
    
    private:
        wxTextCtrl* xbox;           // for entering X value
        wxTextCtrl* ybox;           // for entering Y value
        wxTextCtrl* zbox;           // for entering Z value
        int xval, yval, zval;       // the given X,Y,Z values
};

//----------------------------------------------------------------------------------

/// A modal dialog for editing float X,Y,Z values.
class XYZFloatDialog : public wxDialog
{
    public:
        XYZFloatDialog(wxWindow* parent, const wxString& title,
                  float inx, float iny, float inz,
                  const wxPoint& pos, const wxSize& size);
    
        virtual bool TransferDataFromWindow();  // called when user hits OK
    
        float GetX() { return xval; }
        float GetY() { return yval; }
        float GetZ() { return zval; }
    
    private:
        wxTextCtrl* xbox;           // for entering X value
        wxTextCtrl* ybox;           // for entering Y value
        wxTextCtrl* zbox;           // for entering Z value
        float xval, yval, zval;     // the given X,Y,Z values
};

//----------------------------------------------------------------------------------

/// A modal dialog for editing multi-line text.
// (essentially wxTextEntryDialog but with wxRESIZE_BORDER style)
class MultiLineDialog : public wxDialog
{
    public:
        MultiLineDialog(wxWindow* parent,
                        const wxString& caption,
                        const wxString& message,
                        const wxString& value);
    
        wxString GetValue() const { return m_value; }
        void OnOK(wxCommandEvent& event);
    
    private:
        wxTextCtrl* m_textctrl;
        wxString m_value;
    
        DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------------

/// A modal dialog for editing a parameter name and/or value.
class ParameterDialog : public wxDialog
{
    public:
        ParameterDialog(wxWindow* parent, bool can_edit_name,
                        const wxString& inname, float inval,
                        const wxPoint& pos, const wxSize& size);
    
        #ifdef __WXOSX__
            ~ParameterDialog() { delete onetimer; }
            void OnOneTimer(wxTimerEvent& event);
        #endif
    
        bool TransferDataFromWindow() override;  // called when user hits OK
    
        wxString GetName() const override { return name; }
        float GetValue() { return value; }
    
    private:
        wxTextCtrl* namebox;    // text box for entering name
        wxTextCtrl* valuebox;   // text box for entering value
        wxString name;          // the given name
        float value;            // the given value
    
        #ifdef __WXOSX__
            wxTimer* onetimer;  // one shot timer (see OnOneTimer)
            DECLARE_EVENT_TABLE()
        #endif
};

//----------------------------------------------------------------------------------

/// A modal dialog for getting a string.
class StringDialog : public wxDialog
{
    public:
        StringDialog(wxWindow* parent, const wxString& title,
                     const wxString& prompt, const wxString& instring,
                     const wxPoint& pos, const wxSize& size);
    
        virtual bool TransferDataFromWindow();     // called when user hits OK
    
        wxString GetValue() { return result; }
    
    private:
        wxTextCtrl* textbox;       // text box for entering the string
        wxString result;           // the resulting string
};

//----------------------------------------------------------------------------------

/// A modal dialog for getting an integer.
class IntegerDialog : public wxDialog
{
    public:
        IntegerDialog(wxWindow* parent,
                      const wxString& title,
                      const wxString& prompt,
                      int inval, int minval, int maxval,
                      const wxPoint& pos, const wxSize& size);
    
        #ifdef __WXOSX__
            ~IntegerDialog() { delete onetimer; }
            void OnOneTimer(wxTimerEvent& event);
        #endif
        
        virtual bool TransferDataFromWindow();     // called when user hits OK
    
        #ifdef __WXMAC__
            void OnSpinCtrlChar(wxKeyEvent& event);
        #endif
    
        int GetValue() { return result; }
    
    private:
        enum {
            ID_SPIN_CTRL = wxID_HIGHEST + 1
        };
        wxSpinCtrl* spinctrl;    // for entering the integer
        int minint;              // minimum value
        int maxint;              // maximum value
        int result;              // the resulting integer
    
        #ifdef __WXOSX__
            wxTimer* onetimer;   // one shot timer (see OnOneTimer)
            DECLARE_EVENT_TABLE()
        #endif
};

/// A modal dialog for getting a float.
class FloatDialog : public wxDialog
{
    public:
        FloatDialog(wxWindow* parent,
                    const wxString& title,
                    const wxString& prompt,
                    float inval, 
                    const wxPoint& pos, const wxSize& size);
    
        virtual bool TransferDataFromWindow();     // called when user hits OK
    
        float GetValue() { return result; }
    
    private:
        wxTextCtrl* textbox;       // text box for entering the float
        float result;              // the resulting float
};

// -----------------------------------------------------------------------------

// need platform-specific gap after OK/Cancel buttons
#ifdef __WXMAC__
    const int STDHGAP = 0;
#elif defined(__WXMSW__)
    const int STDHGAP = 6;
#else
    const int STDHGAP = 10;
#endif

// -----------------------------------------------------------------------------
