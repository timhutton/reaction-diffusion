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

// Local:
#include "MakeNewSystem.hpp"

// readybase:
#include <FormulaOpenCLImageRD.hpp>
#include <FormulaOpenCLMeshRD.hpp>
#include <GrayScottImageRD.hpp>
#include <GrayScottMeshRD.hpp>
#include <MeshGenerators.hpp>
#include <Properties.hpp>

// wxWidgets:
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>

// VTK:
#include <vtkUnstructuredGrid.h>

// STL:
#include <vector>

using namespace std;

unique_ptr<AbstractRD> MakeNewImage1D(const bool is_opencl_available,const int opencl_platform,const int opencl_device,Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    wxBusyCursor busy;
    unique_ptr<ImageRD> image_sys;
    if (is_opencl_available)
        image_sys = make_unique<FormulaOpenCLImageRD>(opencl_platform, opencl_device, data_type);
    else
        image_sys = make_unique<GrayScottImageRD>();
    image_sys->SetDimensionsAndNumberOfChemicals(128, 1, 1, 2);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    wxMessageBox(_("Created a 128x1x1 image. The dimensions can be edited in the Info Pane."));
    return image_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewImage2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    wxBusyCursor busy;
    unique_ptr<ImageRD> image_sys;
    if (is_opencl_available)
        image_sys = make_unique<FormulaOpenCLImageRD>(opencl_platform, opencl_device, data_type);
    else
        image_sys = make_unique<GrayScottImageRD>();
    image_sys->SetDimensionsAndNumberOfChemicals(128, 128, 1, 2);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    wxMessageBox(_("Created a 128x128x1 image. The dimensions can be edited in the Info Pane."));
    return image_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewImage3D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    wxBusyCursor busy;
    unique_ptr<ImageRD> image_sys;
    if (is_opencl_available)
        image_sys = make_unique<FormulaOpenCLImageRD>(opencl_platform, opencl_device, data_type);
    else
        image_sys = make_unique<GrayScottImageRD>();
    image_sys->SetDimensionsAndNumberOfChemicals(32, 32, 32, 2);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    wxMessageBox(_("Created a 32x32x32 image. The dimensions can be edited in the Info Pane."));
    return image_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewGeodesicSphere(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int divs;
    {
        const int N_CHOICES = 6;
        int div_choices[N_CHOICES] = { 2,3,4,5,6,7 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d subdivisions - %d cells", div_choices[i], 20 << (div_choices[i] * 2));
        wxSingleChoiceDialog dlg(NULL, _("Select the number of subdivisions:"), _("Geodesic sphere options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(0); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        divs = div_choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetGeodesicSphere(divs, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewTorus(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int nx, ny;
    {
        const int N_CHOICES = 4;
        int x_choices[N_CHOICES] = { 100,160,200,500 };
        int y_choices[N_CHOICES] = { 125,200,250,625 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%dx%d - %d cells", x_choices[i], y_choices[i], x_choices[i] * y_choices[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the resolution:"), _("Torus tiling options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(2); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        nx = x_choices[dlg.GetSelection()];
        ny = y_choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetTorus(nx, ny, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewTriangularMesh(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int n;
    {
        const int N_CHOICES = 5;
        int choices[N_CHOICES] = { 30,50,100,200,500 };
        int cells[N_CHOICES] = { 1682,4802,19602,79202,498002 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d cells", cells[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the grid size:"), _("Triangular mesh options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(1); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        n = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetTriangularMesh(n, n, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("show_cell_edges").SetBool(n<100);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewHexagonalMesh(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int n;
    {
        const int N_CHOICES = 4;
        int choices[N_CHOICES] = { 100,150,200,500 };
        int cells[N_CHOICES] = { 3185,7326,13068,82668 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d cells", cells[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the grid size:"), _("Hexagonal mesh options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(0); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        n = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetHexagonalMesh(n, n, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("show_cell_edges").SetBool(n<200);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewRhombilleTiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int n;
    {
        const int N_CHOICES = 5;
        int choices[N_CHOICES] = { 50,75,100,150,200 };
        int cells[N_CHOICES] = { 2304,5367,9555,21978,39204 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d cells", cells[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the grid size:"), _("Rhombille mesh options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(0); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        n = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetRhombilleTiling(n, n, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("show_cell_edges").SetBool(n<100);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewPenroseP3Tiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int divs;
    {
        const int N_CHOICES = 8;
        int div_choices[N_CHOICES] = { 5,7,8,9,10,11,12,13 };
        int cells[N_CHOICES] = { 430,3010,7920,20800,54560,143010,374680,981370 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d subdivisions - %d cells", div_choices[i], cells[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the number of subdivisions:"), _("Penrose tiling options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(1); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        divs = div_choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetPenroseTiling(divs, 0, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("show_cell_edges").SetBool(divs<9);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewPenroseP2Tiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int divs;
    {
        const int N_CHOICES = 8;
        int div_choices[N_CHOICES] = { 5,6,7,8,9,10,11,12 };
        int cells[N_CHOICES] = { 705,1855,4885,12845,33705,88355,231515,606445 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d subdivisions - %d cells", div_choices[i], cells[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the number of subdivisions:"), _("Penrose tiling options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(2); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        divs = div_choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetPenroseTiling(divs, 1, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("show_cell_edges").SetBool(divs<8);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    wxMessageBox(_("There's a problem with rendering concave polygons in OpenGL, so the display might be slightly corrupted."));
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewDelaunay2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int npts;
    {
        const int N_CHOICES = 5;
        int choices[N_CHOICES] = { 1000,2000,5000,10000,20000 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d cells", choices[i] * 2); // (rough approximation)
        wxSingleChoiceDialog dlg(NULL, _("Select the number of cells:"), _("Delaunay 2D mesh options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(1); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        npts = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetRandomDelaunay2D(npts, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("show_cell_edges").SetBool(npts<5000);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewVoronoi2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int npts;
    {
        const int N_CHOICES = 5;
        int choices[N_CHOICES] = { 1000,2000,10000,20000,50000 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d cells", choices[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the number of cells:"), _("Voronoi 2D mesh options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(1); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        npts = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetRandomVoronoi2D(npts, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("show_cell_edges").SetBool(npts<10000);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewDelaunay3D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int npts;
    {
        const int N_CHOICES = 5;
        int choices[N_CHOICES] = { 500,1000,1500,2000,5000 };
        wxString div_descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            div_descriptions[i] = wxString::Format("%d points - approximately %d cells", choices[i], choices[i] * 6);
        wxSingleChoiceDialog dlg(NULL, _("Select the mesh size:"), _("Delaunay 3D mesh options"), N_CHOICES, div_descriptions);
        dlg.SetSelection(1); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        npts = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetRandomDelaunay3D(npts, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D_axis").SetAxis("y");
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewBodyCentredCubicHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int side;
    {
        const int N_CHOICES = 3;
        int choices[N_CHOICES] = { 5,10,20 };
        int cells[N_CHOICES] = { 189,1729,14859 };
        wxString descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            descriptions[i] = wxString::Format("%d cells", cells[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the grid size:"), _("Body-centred cubic honeycomb options"), N_CHOICES, descriptions);
        dlg.SetSelection(0); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        side = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetBodyCentredCubicHoneycomb(side, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(true);
    render_settings.GetProperty("slice_3D_axis").SetAxis("y");
    render_settings.GetProperty("show_cell_edges").SetBool(true);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewFaceCentredCubicHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int side;
    {
        const int N_CHOICES = 4;
        int choices[N_CHOICES] = { 5,10,15,20 };
        int cells[N_CHOICES] = { 500,4000,13500,32000 };
        wxString descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            descriptions[i] = wxString::Format("%d cells", cells[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the grid size:"), _("Face-centred cubic honeycomb options"), N_CHOICES, descriptions);
        dlg.SetSelection(0); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        side = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetFaceCentredCubicHoneycomb(side, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(true);
    render_settings.GetProperty("slice_3D_axis").SetAxis("y");
    render_settings.GetProperty("show_cell_edges").SetBool(true);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewDiamondHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    int side;
    {
        const int N_CHOICES = 3;
        int choices[N_CHOICES] = { 5,10,20 };
        int cells[N_CHOICES] = { 250,2000,16000 };
        wxString descriptions[N_CHOICES];
        for (int i = 0; i<N_CHOICES; i++)
            descriptions[i] = wxString::Format("%d cells", cells[i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the grid size:"), _("Diamond honeycomb options"), N_CHOICES, descriptions);
        dlg.SetSelection(0); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + N_CHOICES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        side = choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetDiamondCells(side, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(true);
    render_settings.GetProperty("slice_3D_axis").SetAxis("y");
    render_settings.GetProperty("show_cell_edges").SetBool(true);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewHyperbolicPlane(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    // choose the tessellation
    int schlafli1, schlafli2;
    {
        // sensible choices: {3,7+},{4,5+},{5,4+},{6,4+},{7+,3+}
        const int s1_min = 3;
        const int s_max = 8;
        const int s2_min[s_max + 1] = { 0, 0, 0, 7, 5, 4, 4, 3, 3 };
        const wxString polygons[s_max + 1] = { _(""), _(""), _(""), _("triangles"), _("squares"), _("pentagons"), _("hexagons"), _("heptagons"), _("octagons") };
        wxArrayString descriptions;
        vector<pair<int, int> > choices;
        for (int s1 = s1_min; s1 <= s_max; ++s1) {
            for (int s2 = s2_min[s1]; s2 <= s_max; ++s2) {
                descriptions.Add(wxString::Format(_("{%d,%d} - %d %s around each vertex"), s1, s2, s2, polygons[s1]));
                choices.push_back(make_pair(s1, s2));
            }
        }
        wxSingleChoiceDialog dlg(NULL, _("Select the tiling:"), _("Hyperbolic plane options"), descriptions);
        dlg.SetSelection(0); // default selection
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        schlafli1 = choices[dlg.GetSelection()].first;
        schlafli2 = choices[dlg.GetSelection()].second;
    }
    int levels = 30 / schlafli1; // make this a user option?
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetHyperbolicPlaneTiling(schlafli1, schlafli2, levels, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(false);
    render_settings.GetProperty("show_bounding_box").SetBool(true);
    render_settings.GetProperty("show_cell_edges").SetBool(true);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    render_settings.GetProperty("timesteps_per_render").SetInt(1);
    return mesh_sys;
}

// ---------------------------------------------------------------------

unique_ptr<AbstractRD> MakeNewHyperbolicSpace(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings)
{
    // perhaps at some point we will want this to be determined by the user
    const int data_type = VTK_FLOAT;

    const int NUM_TESSELLATION_TYPES = 4;
    // choose the tessellation
    int tessellationType, schlafli1=0, schlafli2=0, schlafli3=0;
    {
        wxString descriptions[NUM_TESSELLATION_TYPES] = { "{4,3,5} : order-5 cubic honeycomb", "{5,3,4} : order-4 dodecahedral honeycomb",
            "{5,3,5} : order-5 dodecahedral honeycomb",  "{3,5,3} : icosahedral honeycomb" };
        wxSingleChoiceDialog dlg(NULL, _("Select the tessellation required:"), _("Hyperbolic space tessellation options"), NUM_TESSELLATION_TYPES, descriptions);
        dlg.SetSelection(0); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + NUM_TESSELLATION_TYPES * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        tessellationType = dlg.GetSelection();
        switch (tessellationType) {
            case 0: schlafli1 = 4; schlafli2 = 3; schlafli3 = 5; break;
            case 1: schlafli1 = 5; schlafli2 = 3; schlafli3 = 4; break;
            case 2: schlafli1 = 5; schlafli2 = 3; schlafli3 = 5; break;
            case 3: schlafli1 = 3; schlafli2 = 5; schlafli3 = 3; break;
        }
    }
    // choose the recursion depth
    int levels;
    {
        const int MAX_CHOICES = 7;
        int level_choices[MAX_CHOICES] = { 1,2,3,4,5,6,7 };
        int cells[NUM_TESSELLATION_TYPES][MAX_CHOICES] = {
            { 7,37,163,661,2643,10497,41505 }, // {4,3,5} : https://oeis.org/A247308
            { 13,115,927,7329,57741,0,0 },     // {5,3,4}
            { 13,145,1537,16129,0,0,0 },       // {5,3,5}
            { 21,281,3493,42963,0,0,0 }        // {3,5,3}
        };
        int num_choices[NUM_TESSELLATION_TYPES] = { 7, 5, 4, 4 };
        wxArrayString level_descriptions;
        level_descriptions.resize(num_choices[tessellationType]);
        for (int i = 0; i < num_choices[tessellationType]; ++i)
            level_descriptions[i] = wxString::Format("%d levels - %d cells", level_choices[i], cells[tessellationType][i]);
        wxSingleChoiceDialog dlg(NULL, _("Select the number of levels:"), _("Hyperbolic space tessellation options"),
            level_descriptions);
        dlg.SetSelection(2); // default selection
        dlg.SetSize(wxDefaultCoord, 130 + num_choices[tessellationType] * 20); // increase dlg height so we see all choices without having to scroll
        if (dlg.ShowModal() != wxID_OK) {
            return NULL;
        }
        levels = level_choices[dlg.GetSelection()];
    }
    wxBusyCursor busy;
    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    MeshGenerators::GetHyperbolicSpaceTessellation(schlafli1, schlafli2, schlafli3, levels, mesh, 2, data_type);
    unique_ptr<MeshRD> mesh_sys;
    if (is_opencl_available)
        mesh_sys = make_unique<FormulaOpenCLMeshRD>(opencl_platform, opencl_device, data_type);
    else
        mesh_sys = make_unique<GrayScottMeshRD>();
    mesh_sys->CopyFromMesh(mesh);
    render_settings.GetProperty("active_chemical").SetChemical("b");
    render_settings.GetProperty("slice_3D").SetBool(true);
    render_settings.GetProperty("show_bounding_box").SetBool(true);
    render_settings.GetProperty("show_cell_edges").SetBool(true);
    render_settings.GetProperty("use_image_interpolation").SetBool(false);
    render_settings.GetProperty("timesteps_per_render").SetInt(1);
    return mesh_sys;
}

// ---------------------------------------------------------------------
