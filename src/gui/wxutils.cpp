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

// local
#include "wxutils.hpp"
#include "app.hpp"          // for wxGetApp
#include "prefs.hpp"        // for allowbeep
#include "dialogs.hpp"

// wxWidgets:
#include <wx/clipbrd.h>
#include <wx/datstrm.h>
#include <wx/ffile.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

// VTK:
#include <vtkBMPReader.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkImageReader2.h>
#include <vtkJPEGReader.h>
#include <vtkJPEGWriter.h>
#include <vtkPNGReader.h>
#include <vtkPNGWriter.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>

// -----------------------------------------------------------------------------

void Note(const wxString& msg)
{
    wxString title = wxGetApp().GetAppName() + _(" note:");
    #ifdef __WXMAC__
        wxSetCursor(*wxSTANDARD_CURSOR);
    #endif
    wxMessageBox(msg, title, wxOK | wxICON_INFORMATION, wxGetActiveWindow());
}

// -----------------------------------------------------------------------------

void Warning(const wxString& msg)
{
    Beep();
    wxString title = wxGetApp().GetAppName() + _(" warning:");
    #ifdef __WXMAC__
        wxSetCursor(*wxSTANDARD_CURSOR);
    #endif
    wxMessageBox(msg, title, wxOK | wxICON_EXCLAMATION, wxGetActiveWindow());
}

// -----------------------------------------------------------------------------

void Fatal(const wxString& msg)
{
    Beep();
    wxString title = wxGetApp().GetAppName() + _(" error:");
    #ifdef __WXMAC__
        wxSetCursor(*wxSTANDARD_CURSOR);
    #endif
    wxMessageBox(msg, title, wxOK | wxICON_ERROR, wxGetActiveWindow());

    exit(1);     // safer than calling wxExit()
}

// -----------------------------------------------------------------------------

void Beep()
{
    if (allowbeep) wxBell();
}

// -----------------------------------------------------------------------------

bool GetString(const wxString& title, const wxString& prompt,
               const wxString& instring, wxString& outstring,
               const wxPoint& pos, const wxSize& size)
{
    StringDialog dialog(wxGetApp().GetTopWindow(), title, prompt,
                        instring, pos, size);
    if ( dialog.ShowModal() == wxID_OK ) {
        outstring = dialog.GetValue();
        return true;
    } else {
        // user hit Cancel button
        return false;
    }
}

// -----------------------------------------------------------------------------

bool GetInteger(const wxString& title, const wxString& prompt,
                int inval, int minval, int maxval, int* outval,
                const wxPoint& pos, const wxSize& size)
{
    IntegerDialog dialog(wxGetApp().GetTopWindow(), title, prompt,
                         inval, minval, maxval, pos, size);
    if ( dialog.ShowModal() == wxID_OK ) {
        *outval = dialog.GetValue();
        return true;
    } else {
        // user hit Cancel button
        return false;
    }
}

// -----------------------------------------------------------------------------

bool GetFloat(const wxString& title, const wxString& prompt,
              float inval, float* outval,
              const wxPoint& pos, const wxSize& size)
{
    FloatDialog dialog(wxGetApp().GetTopWindow(), title, prompt,
                         inval, pos, size);
    if ( dialog.ShowModal() == wxID_OK ) {
        *outval = dialog.GetValue();
        return true;
    } else {
        // user hit Cancel button
        return false;
    }
}

// =============================================================================

bool IsHTMLFile(const wxString& filename)
{
    wxString ext = filename.AfterLast('.');
    // if filename has no extension then ext == filename
    if (ext == filename) return false;
    return ( ext.IsSameAs(wxT("htm"),false) ||
             ext.IsSameAs(wxT("html"),false) );
}

// -----------------------------------------------------------------------------

bool IsTextFile(const wxString& filename)
{
    if (!IsHTMLFile(filename)) {
        // if non-html file name contains "readme" then assume it's a text file
        wxString name = filename.AfterLast(wxFILE_SEP_PATH).MakeLower();
        if (name.Contains(wxT("readme"))) return true;
    }
    wxString ext = filename.AfterLast('.');
    // if filename has no extension then ext == filename
    if (ext == filename) return false;
    return ( ext.IsSameAs(wxT("txt"),false) ||
             ext.IsSameAs(wxT("doc"),false) );
}

// -----------------------------------------------------------------------------

int SaveChanges(const wxString& query, const wxString& msg)
{
    #ifdef __WXMAC__
        // create a standard looking Mac dialog
        wxMessageDialog dialog(wxGetActiveWindow(), msg, query,
                               wxCENTER | wxNO_DEFAULT | wxYES_NO | wxCANCEL |
                               wxICON_INFORMATION);

        // change button order to what Mac users expect to see
        dialog.SetYesNoCancelLabels("Cancel", "Save", "Don't Save");

        switch ( dialog.ShowModal() )
        {
            case wxID_YES:    return wxCANCEL;  // Cancel
            case wxID_NO:     return wxYES;     // Save
            case wxID_CANCEL: return wxNO;      // Don't Save
            default:          return wxCANCEL;  // should never happen
        }
    #else
        // Windows/Linux
        return wxMessageBox(msg, query, wxICON_QUESTION | wxYES_NO | wxCANCEL,
                            wxGetActiveWindow());
    #endif
}

// -----------------------------------------------------------------------------

bool ClipboardHasText()
{
    bool hastext = false;
    #ifdef __WXGTK__
        // avoid re-entrancy bug in wxGTK 2.9.x
        if (wxTheClipboard->IsOpened()) return false;
    #endif
    if (wxTheClipboard->Open()) {
        hastext = wxTheClipboard->IsSupported(wxDF_TEXT);
        wxTheClipboard->Close();
    }
    return hastext;
}

// -----------------------------------------------------------------------------

bool CopyTextToClipboard(const wxString& text)
{
    bool result = true;
    if (wxTheClipboard->Open()) {
        if ( !wxTheClipboard->SetData(new wxTextDataObject(text)) ) {
            Warning(_("Could not copy text to clipboard!"));
            result = false;
        }
        wxTheClipboard->Close();
    } else {
        Warning(_("Could not open clipboard!"));
        result = false;
    }
    return result;
}

// -----------------------------------------------------------------------------

wxString FormatFloat(float f,int mdp)
{
    wxString result = wxString::Format(wxT("%.*f"),mdp,f);
    // strip any trailing zeros
    while (result.GetChar(result.Length()-1) == wxChar('0')) {
        result.Truncate(result.Length()-1);
    }
    // strip any trailing decimal separator
    if (result.GetChar(result.Length()-1) == wxChar('.') || result.GetChar(result.Length()-1) == wxChar(',')) {
        result.Truncate(result.Length()-1);
    }
    return result;
}

// -----------------------------------------------------------------------------

wxString FileNameToString(const wxFileName& filename) // TODO: this only helps on Windows when reading from a file
{
    // if unicode characters in path, use the short form
    wxString path = filename.GetFullPath();
    if (strlen(path.mb_str()) > 0) {
        return path;
    }
    return filename.GetShortPath();
}

// -----------------------------------------------------------------------------

wxString ReadEntireFile(const wxFileName& filename)
{
    wxFFile file(filename.GetFullPath());
    if (!file.IsOpened()) {
        throw std::runtime_error("Failed to open file for reading");
    }
    wxString file_content;
    file.ReadAll(&file_content);
    return file_content;
}

// -----------------------------------------------------------------------------

wxFileName FindUnusedFilename(const wxString& folder, const wxString& format)
{
    int suffix_value = 0;
    while(true)
    {
        const wxFileName filename(folder, wxString::Format(format, suffix_value++));
        if (!filename.FileExists())
            return filename;
    }
}

// -----------------------------------------------------------------------------

bool AskUserWhereToSaveImage(wxFileName& filename)
{
    while (true)
    {
        filename = wxFileSelector(
            _("Specify the image filename"),
            filename.GetPath(),
            filename.GetName(),
            filename.GetExt(),
            _("PNG files (*.png)|*.png|JPG files (*.jpg)|*.jpg"),
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (filename.GetFullPath().empty())
            return false; // user cancelled
        if (filename.GetExt().Lower() == _("png") || filename.GetExt().Lower() == _("jpg"))
            return true;
        wxMessageBox(_("Unsupported format"), _("Error"), wxOK | wxCENTER | wxICON_ERROR);
    }
}

// -----------------------------------------------------------------------------

void WriteImageToFile(vtkAlgorithmOutput* image, const wxFileName& filename)
{
    // write the image into memory
    // (this is a workaround because VTK doesn't yet support unicode in filenames)
    vtkSmartPointer<vtkUnsignedCharArray> bytes;
    if (filename.GetExt().Lower() == _("png")) {
        vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
        writer->SetInputConnection(image);
        writer->WriteToMemoryOn();
        writer->Write();
        bytes = writer->GetResult();
    }
    else if (filename.GetExt().Lower() == _("jpg")) {
        vtkSmartPointer<vtkJPEGWriter> writer = vtkSmartPointer<vtkJPEGWriter>::New();
        writer->SetInputConnection(image);
        writer->WriteToMemoryOn();
        writer->Write();
        bytes = writer->GetResult();
    }
    else {
        wxMessageBox(_("Internal error: Unsupported format"), _("Error"), wxOK | wxCENTER | wxICON_ERROR);
        return;
    }

    WriteBytesToFile(bytes, filename);
}

// -----------------------------------------------------------------------------

void WriteBytesToFile(vtkUnsignedCharArray* bytes, const wxFileName& filename)
{
    wxFileOutputStream to_file(filename.GetFullPath());
    wxDataOutputStream out(to_file);
    for (vtkIdType i = 0; i < bytes->GetNumberOfTuples(); i++) {
        out.Write8(bytes->GetValue(i));
    }
}

// -----------------------------------------------------------------------------

void GetScalarRangeFromImage(wxFileName filename, double range[2])
{
    wxBusyCursor busy;

    // load the image just to retrieve the input range
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
    reader->Update();
    reader->GetOutput()->GetPointData()->GetScalars()->GetRange(range);
}

// -----------------------------------------------------------------------------
