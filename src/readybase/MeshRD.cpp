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
#include "IO_XML.hpp"
#include "MeshRD.hpp"
#include "overlays.hpp"
#include "Properties.hpp"
#include "scene_items.hpp"
#include "utils.hpp"

// VTK:
#include <vtkActor.h>
#include <vtkAssignAttribute.h>
#include <vtkCaptionActor2D.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkCellLocator.h>
#include <vtkContourFilter.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkCubeSource.h>
#include <vtkCutter.h>
#include <vtkDataSetMapper.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkExtractEdges.h>
#include <vtkGenericCell.h>
#include <vtkGeometryFilter.h>
#include <vtkIdList.h>
#include <vtkMath.h>
#include <vtkMergeFilter.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPointSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRearrangeFields.h>
#include <vtkRenderer.h>
#include <vtkReverseSense.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarsToColors.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkThreshold.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkWarpScalar.h>
#include <vtkXMLDataElement.h>

// STL:
#include <stdexcept>
#include <algorithm>

using namespace std;

// ---------------------------------------------------------------------

MeshRD::MeshRD(int data_type)
    : AbstractRD(data_type)
{
    this->starting_pattern = vtkSmartPointer<vtkUnstructuredGrid>::New();
    this->mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
}

// ---------------------------------------------------------------------

void MeshRD::Update(int n_steps)
{
    this->undo_stack.clear();
    this->InternalUpdate(n_steps);

    this->timesteps_taken += n_steps;

    this->mesh->Modified();
}

// ---------------------------------------------------------------------

void MeshRD::SetNumberOfChemicals(int n, bool reallocate_storage)
{
    if (reallocate_storage)
    {
        this->mesh->GetCellData()->Initialize();
        this->n_chemicals = 0;
    }
    if (n == this->n_chemicals) {
        return;
    }
    if (n > this->n_chemicals) {
        while (this->mesh->GetCellData()->GetNumberOfArrays() < n) {
            vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take(vtkDataArray::CreateDataArray(this->data_type));
            scalars->SetNumberOfComponents(1);
            scalars->SetNumberOfTuples(this->mesh->GetNumberOfCells());
            std::string cn = GetChemicalName(this->mesh->GetCellData()->GetNumberOfArrays());
            scalars->SetName(cn.c_str());
            scalars->FillComponent(0, 0.0f);
            this->mesh->GetCellData()->AddArray(scalars);
        }
    }
    else {
        while (this->mesh->GetCellData()->GetNumberOfArrays() > n) {
            std::string cn = GetChemicalName(this->mesh->GetCellData()->GetNumberOfArrays()-1);
            this->mesh->GetCellData()->RemoveArray(cn.c_str());
        }
    }
    this->n_chemicals = n;
    this->mesh->Modified();
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void MeshRD::SaveFile(const char* filename,const Properties& render_settings,bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<RD_XMLUnstructuredGridWriter> iw = vtkSmartPointer<RD_XMLUnstructuredGridWriter>::New();
    iw->SetSystem(this);
    iw->SetRenderSettings(&render_settings);
    if(generate_initial_pattern_when_loading)
        iw->GenerateInitialPatternWhenLoading();
    iw->SetFileName(filename);
    iw->SetDataModeToBinary(); // workaround for http://www.vtk.org/Bug/view.php?id=13382
    iw->SetInputData(this->mesh);
    iw->Write();
}

// ---------------------------------------------------------------------

void MeshRD::GenerateInitialPattern()
{
    if (this->initial_pattern_generator.ShouldZeroFirst()) {
        this->BlankImage();
    }

    for (size_t iOverlay = 0; iOverlay < this->initial_pattern_generator.GetNumberOfOverlays(); iOverlay++)
    {
        this->initial_pattern_generator.GetOverlay(iOverlay).Reseed();
    }

    float cp[3];
    double *bounds = this->mesh->GetBounds();
    for(vtkIdType iCell=0;iCell<this->mesh->GetNumberOfCells();iCell++)
    {
        vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
        this->mesh->GetCellPoints(iCell, ids);
        // get a point at the centre of the cell (need a location to sample the overlays)
        cp[0]=cp[1]=cp[2]=0.0f;
        for(vtkIdType iPt=0;iPt<ids->GetNumberOfIds();iPt++)
            for(int xyz=0;xyz<3;xyz++)
                cp[xyz] += this->mesh->GetPoint(ids->GetId(iPt))[xyz]-bounds[xyz*2+0];
        for(int xyz=0;xyz<3;xyz++)
            cp[xyz] /= ids->GetNumberOfIds();
        for(size_t iOverlay=0; iOverlay < this->initial_pattern_generator.GetNumberOfOverlays(); iOverlay++)
        {
            const Overlay& overlay = this->initial_pattern_generator.GetOverlay(iOverlay);

            int iC = overlay.GetTargetChemical();
            if(iC<0 || iC>=this->GetNumberOfChemicals())
                continue; // best for now to silently ignore this overlay, because the user has no way of editing the overlays (short of editing the file)
                //throw runtime_error("Overlay: chemical out of range: "+GetChemicalName(iC));

            vector<double> vals(this->GetNumberOfChemicals());
            for(int i=0;i<this->GetNumberOfChemicals();i++)
            {
                vals[i] = this->mesh->GetCellData()->GetArray(GetChemicalName(i).c_str())->GetComponent( iCell, 0 );
            }
            this->mesh->GetCellData()->GetArray(GetChemicalName(iC).c_str())->SetComponent(iCell, 0, overlay.Apply(vals, *this, cp[0], cp[1], cp[2]));
        }
    }
    this->mesh->Modified();
    this->is_modified = true;
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

void MeshRD::BlankImage(float value)
{
    for(int iChem=0;iChem<this->n_chemicals;iChem++)
    {
        this->mesh->GetCellData()->GetArray(GetChemicalName(iChem).c_str())->FillComponent(0, value);
    }
    this->mesh->Modified();
    this->is_modified = true;
    this->undo_stack.clear();
}

// ---------------------------------------------------------------------

float MeshRD::GetX() const
{
    return this->mesh->GetBounds()[1]-this->mesh->GetBounds()[0];
}

// ---------------------------------------------------------------------

float MeshRD::GetY() const
{
    return this->mesh->GetBounds()[3]-this->mesh->GetBounds()[2];
}

// ---------------------------------------------------------------------

float MeshRD::GetZ() const
{
    return this->mesh->GetBounds()[5]-this->mesh->GetBounds()[4];
}

// ---------------------------------------------------------------------

void MeshRD::CopyFromMesh(vtkUnstructuredGrid* mesh2)
{
    this->undo_stack.clear();
    this->mesh->DeepCopy(mesh2);
    this->is_modified = true;
    this->n_chemicals = this->mesh->GetCellData()->GetNumberOfArrays();

    this->cell_locator = NULL;

    this->ComputeCellNeighbors(this->neighborhood_type);
}

// ---------------------------------------------------------------------

void MeshRD::InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings)
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();
    bool show_bounding_box = render_settings.GetProperty("show_bounding_box").GetBool();
    bool show_chemical_label = render_settings.GetProperty("show_chemical_label").GetBool();
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();
    float surface_r,surface_g,surface_b;
    render_settings.GetProperty("surface_color").GetColor(surface_r,surface_g,surface_b);
    bool slice_3D = render_settings.GetProperty("slice_3D").GetBool();
    string slice_3D_axis = render_settings.GetProperty("slice_3D_axis").GetAxis();
    float slice_3D_position = render_settings.GetProperty("slice_3D_position").GetFloat();
    bool show_phase_plot = render_settings.GetProperty("show_phase_plot").GetBool();
    int iPhasePlotX = IndexFromChemicalName(render_settings.GetProperty("phase_plot_x_axis").GetChemical());
    int iPhasePlotY = IndexFromChemicalName(render_settings.GetProperty("phase_plot_y_axis").GetChemical());
    int iPhasePlotZ = IndexFromChemicalName(render_settings.GetProperty("phase_plot_z_axis").GetChemical());

    vtkSmartPointer<vtkScalarsToColors> lut = GetColorMap(render_settings);

    int iFirstChem=0,iLastChem=this->GetNumberOfChemicals();
    if(!show_multiple_chemicals) { iFirstChem = iActiveChemical; iLastChem = iFirstChem+1; }

    double offset[3] = {0,0,0};
    const float x_gap = this->x_spacing_proportion * this->GetX();

    for(int iChem = iFirstChem; iChem < iLastChem; ++iChem)
    {
        string chem = GetChemicalName(iChem);
        if(this->mesh->GetCellType(0)==VTK_POLYGON)
        {
            // add the mesh actor
            vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            if(use_wireframe && !slice_3D) // full wireframe mode: all internal edges
            {
                // explicitly extract the edges - the default mapper only shows the outside surface
                vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
                edges->SetInputData(this->mesh);
                mapper->SetInputConnection(edges->GetOutputPort());
                mapper->SetScalarModeToUseCellFieldData();
            }
            else if(slice_3D) // partial wireframe mode: only external surface edges
            {
                vtkSmartPointer<vtkGeometryFilter> geom = vtkSmartPointer<vtkGeometryFilter>::New();
                geom->SetInputData(this->mesh);
                vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
                edges->SetInputConnection(geom->GetOutputPort());
                mapper->SetInputConnection(edges->GetOutputPort());
                mapper->SetScalarModeToUseCellFieldData();
            }
            else // non-wireframe mode: shows filled external surface
            {
                if(use_image_interpolation)
                {
                    vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
                    to_point_data->SetInputData(this->mesh);
                    mapper->SetInputConnection(to_point_data->GetOutputPort());
                    mapper->SetScalarModeToUsePointFieldData();
                }
                else
                {
                    mapper->SetInputData(this->mesh);
                    mapper->SetScalarModeToUseCellFieldData();
                }
                if(show_cell_edges)
                {
                    actor->GetProperty()->EdgeVisibilityOn();
                    actor->GetProperty()->SetEdgeColor(0,0,0); // could be a user option
                }
            }
            mapper->SelectColorArray(chem.c_str());
            mapper->SetLookupTable(lut);
            mapper->UseLookupTableScalarRangeOn();

            actor->SetPosition(offset);
            pRenderer->AddActor(actor);
        }
        else if(use_image_interpolation)
        {
            // show a contour
            vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
            assign_attribute->SetInputData(this->mesh);
            assign_attribute->Assign(chem.c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
            vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
            to_point_data->SetInputConnection(assign_attribute->GetOutputPort());
            vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
            surface->SetInputConnection(to_point_data->GetOutputPort());
            surface->SetValue(0,contour_level);
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(surface->GetOutputPort());
            mapper->ScalarVisibilityOff();
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(surface_r,surface_g,surface_b);
            actor->GetProperty()->SetAmbient(0.1);
            actor->GetProperty()->SetDiffuse(0.7);
            actor->GetProperty()->SetSpecular(0.2);
            actor->GetProperty()->SetSpecularPower(3);
            if(use_wireframe)
                actor->GetProperty()->SetRepresentationToWireframe();
            /*vtkSmartPointer<vtkProperty> bfprop = vtkSmartPointer<vtkProperty>::New();
            actor->SetBackfaceProperty(bfprop);
            bfprop->SetColor(0.3,0.3,0.3);
            bfprop->SetAmbient(0.3);
            bfprop->SetDiffuse(0.6);
            bfprop->SetSpecular(0.1);*/ // TODO: re-enable this if can get correct normals
            actor->PickableOff();
            actor->SetPosition(offset);
            pRenderer->AddActor(actor);
        }
        else // visualise the cells
        {
            vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
            assign_attribute->SetInputData(this->mesh);
            assign_attribute->Assign(chem.c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
            vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
            threshold->SetInputConnection(assign_attribute->GetOutputPort());
#if VTK_MAJOR_VERSION > 9 || (VTK_MAJOR_VERSION == 9 && VTK_MINOR_VERSION >= 1)
            threshold->SetUpperThreshold(contour_level);
            threshold->SetThresholdFunction(vtkThreshold::THRESHOLD_UPPER);
#else
            threshold->ThresholdByUpper(contour_level);
#endif
            vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
            mapper->SetInputConnection(threshold->GetOutputPort());
            mapper->SetLookupTable(lut);
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            if(show_cell_edges)
            {
                actor->GetProperty()->EdgeVisibilityOn();
                actor->GetProperty()->SetEdgeColor(0,0,0); // could be a user option
            }
            if(use_wireframe)
                actor->GetProperty()->SetRepresentationToWireframe();
            actor->PickableOff();
            actor->SetPosition(offset);
            pRenderer->AddActor(actor);
        }

        // add a slice
        if(slice_3D)
        {
            vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
            double *bounds = this->mesh->GetBounds();
            plane->SetOrigin(slice_3D_position*(bounds[1]-bounds[0])+bounds[0],
                             slice_3D_position*(bounds[3]-bounds[2])+bounds[2],
                             slice_3D_position*(bounds[5]-bounds[4])+bounds[4]);
            if(slice_3D_axis=="x")
                plane->SetNormal(1,0,0);
            else if(slice_3D_axis=="y")
                plane->SetNormal(0,1,0);
            else
                plane->SetNormal(0,0,1);
            vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
            cutter->SetCutFunction(plane);
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(cutter->GetOutputPort());
            if(use_image_interpolation)
            {
                vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
                to_point_data->SetInputData(this->mesh);
                cutter->SetInputConnection(to_point_data->GetOutputPort());
                mapper->SetScalarModeToUsePointFieldData();
            }
            else
            {
                cutter->SetInputData(this->mesh);
                mapper->SetScalarModeToUseCellFieldData();
            }
            mapper->SelectColorArray(chem.c_str());
            mapper->SetLookupTable(lut);
            mapper->UseLookupTableScalarRangeOn();
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->LightingOff();
            if(show_cell_edges)
            {
                actor->GetProperty()->EdgeVisibilityOn();
                actor->GetProperty()->SetEdgeColor(0,0,0); // could be a user option
            }
            actor->SetPosition(offset);
            pRenderer->AddActor(actor);
        }

        // add the bounding box
        if(show_bounding_box)
        {
            vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
            box->SetBounds(this->mesh->GetBounds());

            vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
            edges->SetInputConnection(box->GetOutputPort());

            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(edges->GetOutputPort());

            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(0,0,0);
            actor->GetProperty()->SetAmbient(1);

            actor->PickableOff();
            actor->SetPosition(offset);
            pRenderer->AddActor(actor);
        }

        // add a text label
        if(show_chemical_label && this->GetNumberOfChemicals()>1)
        {
            const float text_label_offset = this->GetX()*0.05 + max(this->GetX(), this->GetY()) / 20.0f;
            vtkSmartPointer<vtkCaptionActor2D> captionActor = vtkSmartPointer<vtkCaptionActor2D>::New();
            captionActor->SetAttachmentPoint(this->mesh->GetBounds()[0] + offset[0] + this->GetX() / 2, this->mesh->GetBounds()[2] + offset[1] - text_label_offset, this->mesh->GetBounds()[4] + offset[2]);
            captionActor->SetPosition(0, 0);
            captionActor->SetCaption(chem.c_str());
            captionActor->BorderOff();
            captionActor->LeaderOff();
            captionActor->SetPadding(0);
            captionActor->GetCaptionTextProperty()->SetJustificationToLeft();
            captionActor->GetCaptionTextProperty()->BoldOff();
            captionActor->GetCaptionTextProperty()->ShadowOff();
            captionActor->GetCaptionTextProperty()->ItalicOff();
            captionActor->GetCaptionTextProperty()->SetFontFamilyToArial();
            captionActor->GetCaptionTextProperty()->SetFontSize(16);
            captionActor->GetCaptionTextProperty()->SetVerticalJustificationToCentered();
            captionActor->GetTextActor()->SetTextScaleModeToNone();
            pRenderer->AddActor(captionActor);
        }

        offset[0] += this->GetX() + x_gap; // the next chemical should appear further to the right
    }

    // also add a scalar bar to show how the colors correspond to values
    if(show_color_scale)
    {
        AddScalarBar(pRenderer,lut);
    }

    // add a phase plot
    if(show_phase_plot && this->GetNumberOfChemicals()>=2)
    {
        this->AddPhasePlot( pRenderer,GetX()/(high-low),low,high,
                            this->mesh->GetBounds()[0],
                            this->mesh->GetBounds()[3]+GetY()*0.1f,
                            this->mesh->GetBounds()[4],
                            iPhasePlotX,iPhasePlotY,iPhasePlotZ);
    }
}

// ---------------------------------------------------------------------

void MeshRD::AddPhasePlot(vtkRenderer* pRenderer,float scaling,float low,float high,float posX,float posY,float posZ,
    int iChemX,int iChemY,int iChemZ)
{
    iChemX = max( 0, min( iChemX, this->GetNumberOfChemicals()-1 ) );
    iChemY = max( 0, min( iChemY, this->GetNumberOfChemicals()-1 ) );
    iChemZ = max( 0, min( iChemZ, this->GetNumberOfChemicals()-1 ) );

    vtkSmartPointer<vtkPointSource> points = vtkSmartPointer<vtkPointSource>::New();
    points->SetNumberOfPoints(this->GetNumberOfCells());
    points->SetRadius(0);

    vtkSmartPointer<vtkRearrangeFields> rearrange_fieldsX = vtkSmartPointer<vtkRearrangeFields>::New();
    rearrange_fieldsX->SetInputData(this->mesh);
    rearrange_fieldsX->AddOperation(vtkRearrangeFields::MOVE,GetChemicalName(iChemX).c_str(),vtkRearrangeFields::CELL_DATA,vtkRearrangeFields::POINT_DATA);
    vtkSmartPointer<vtkAssignAttribute> assign_attributeX = vtkSmartPointer<vtkAssignAttribute>::New();
    assign_attributeX->SetInputConnection(rearrange_fieldsX->GetOutputPort());
    assign_attributeX->Assign(GetChemicalName(iChemX).c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
    vtkSmartPointer<vtkMergeFilter> mergeX = vtkSmartPointer<vtkMergeFilter>::New();
    mergeX->SetGeometryConnection(points->GetOutputPort());
    mergeX->SetScalarsConnection(assign_attributeX->GetOutputPort());
    vtkSmartPointer<vtkWarpScalar> warpX = vtkSmartPointer<vtkWarpScalar>::New();
    warpX->UseNormalOn();
    warpX->SetNormal(1,0,0);
    warpX->SetInputConnection(mergeX->GetOutputPort());
    warpX->SetScaleFactor(scaling);

    vtkSmartPointer<vtkRearrangeFields> rearrange_fieldsY = vtkSmartPointer<vtkRearrangeFields>::New();
    rearrange_fieldsY->SetInputData(this->mesh);
    rearrange_fieldsY->AddOperation(vtkRearrangeFields::MOVE,GetChemicalName(iChemY).c_str(),vtkRearrangeFields::CELL_DATA,vtkRearrangeFields::POINT_DATA);
    vtkSmartPointer<vtkAssignAttribute> assign_attributeY = vtkSmartPointer<vtkAssignAttribute>::New();
    assign_attributeY->SetInputConnection(rearrange_fieldsY->GetOutputPort());
    assign_attributeY->Assign(GetChemicalName(iChemY).c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
    vtkSmartPointer<vtkMergeFilter> mergeY = vtkSmartPointer<vtkMergeFilter>::New();
    mergeY->SetGeometryConnection(warpX->GetOutputPort());
    mergeY->SetScalarsConnection(assign_attributeY->GetOutputPort());
    vtkSmartPointer<vtkWarpScalar> warpY = vtkSmartPointer<vtkWarpScalar>::New();
    warpY->UseNormalOn();
    warpY->SetNormal(0,1,0);
    warpY->SetInputConnection(mergeY->GetOutputPort());
    warpY->SetScaleFactor(scaling);

    vtkSmartPointer<vtkVertexGlyphFilter> glyph = vtkSmartPointer<vtkVertexGlyphFilter>::New();

    float offsetZ = 0.0f;
    if(this->GetNumberOfChemicals()>2)
    {
        vtkSmartPointer<vtkRearrangeFields> rearrange_fieldsZ = vtkSmartPointer<vtkRearrangeFields>::New();
        rearrange_fieldsZ->SetInputData(this->mesh);
        rearrange_fieldsZ->AddOperation(vtkRearrangeFields::MOVE,GetChemicalName(iChemZ).c_str(),vtkRearrangeFields::CELL_DATA,vtkRearrangeFields::POINT_DATA);
        vtkSmartPointer<vtkAssignAttribute> assign_attributeZ = vtkSmartPointer<vtkAssignAttribute>::New();
        assign_attributeZ->SetInputConnection(rearrange_fieldsZ->GetOutputPort());
        assign_attributeZ->Assign(GetChemicalName(iChemZ).c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
        vtkSmartPointer<vtkMergeFilter> mergeZ = vtkSmartPointer<vtkMergeFilter>::New();
        mergeZ->SetGeometryConnection(warpY->GetOutputPort());
        mergeZ->SetScalarsConnection(assign_attributeZ->GetOutputPort());
        vtkSmartPointer<vtkWarpScalar> warpZ = vtkSmartPointer<vtkWarpScalar>::New();
        warpZ->UseNormalOn();
        warpZ->SetNormal(0,0,1);
        warpZ->SetInputConnection(mergeZ->GetOutputPort());
        warpZ->SetScaleFactor(scaling);

        glyph->SetInputConnection(warpZ->GetOutputPort());

        offsetZ = low*scaling;
    }
    else
    {
        glyph->SetInputConnection(warpY->GetOutputPort());
    }

    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
    trans->Scale(1,1,-1);
    vtkSmartPointer<vtkTransformFilter> transFilter = vtkSmartPointer<vtkTransformFilter>::New();
    transFilter->SetTransform(trans);
    transFilter->SetInputConnection(glyph->GetOutputPort());

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transFilter->GetOutputPort());
    mapper->ScalarVisibilityOff();
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbient(1);
    actor->GetProperty()->SetPointSize(1);
    actor->PickableOff();
    actor->SetPosition(posX-low*scaling,posY-low*scaling,posZ+offsetZ);
    pRenderer->AddActor(actor);

    // also add the axes
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX+scaling*(high-low),posY,posY,posZ,posZ);
        axis->SetRanges(low,high,0,0,0,0);
        axis->UseRangesOn();
        axis->YAxisVisibilityOff();
        axis->ZAxisVisibilityOff();
        axis->SetXLabel(GetChemicalName(iChemX).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX,posY,posY+(high-low)*scaling,posZ,posZ);
        axis->SetRanges(0,0,low,high,0,0);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->ZAxisVisibilityOff();
        axis->SetYLabel(GetChemicalName(iChemY).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }
    if(this->GetNumberOfChemicals()>2)
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX,posY,posY,posZ,posZ-scaling*(high-low));
        axis->SetRanges(0,0,0,0,low,high);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->YAxisVisibilityOff();
        axis->SetZLabel(GetChemicalName(iChemZ).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }
}

// ---------------------------------------------------------------------

void MeshRD::SaveStartingPattern()
{
    this->starting_pattern->DeepCopy(this->mesh);
}

// ---------------------------------------------------------------------

void MeshRD::RestoreStartingPattern()
{
    this->CopyFromMesh(this->starting_pattern);
    this->is_modified = true;
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

struct TNeighbor { vtkIdType iNeighbor; float weight; };

void add_if_new(vector<TNeighbor>& neighbors,TNeighbor neighbor)
{
    for(vector<TNeighbor>::const_iterator it=neighbors.begin();it!=neighbors.end();it++)
        if(it->iNeighbor==neighbor.iNeighbor)
            return;
    neighbors.push_back(neighbor);
}

bool IsEdgeNeighbor(vtkUnstructuredGrid *grid,vtkIdType iCell1,vtkIdType iCell2)
{
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    vtkCell* pCell = grid->GetCell(iCell1);
    for(int iEdge=0;iEdge<pCell->GetNumberOfEdges();iEdge++)
    {
        vtkIdList *vertIds = pCell->GetEdge(iEdge)->GetPointIds();
        grid->GetCellNeighbors(iCell1,vertIds,cellIds);
        if(cellIds->IsId(iCell2)>=0)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------

void MeshRD::ComputeCellNeighbors(TNeighborhood neighborhood_type)
{
    if(!this->mesh->IsHomogeneous())
        throw runtime_error("MeshRD::ComputeCellNeighbors : mixed cell types not supported");

    vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    TNeighbor nbor;

    vector<vector<TNeighbor> > cell_neighbors; // the connectivity between cells; for each cell, what cells are its neighbors?
    this->max_neighbors = 0;
    for(vtkIdType iCell=0;iCell<this->mesh->GetNumberOfCells();iCell++)
    {
        vector<TNeighbor> neighbors;
        this->mesh->GetCellPoints(iCell,ptIds);
        vtkIdType npts = ptIds->GetNumberOfIds();
        switch(neighborhood_type)
        {
            case TNeighborhood::VERTEX_NEIGHBORS: // neighbors share a vertex
            {
                vtkSmartPointer<vtkIdList> vertIds = vtkSmartPointer<vtkIdList>::New();
                vertIds->SetNumberOfIds(1);
                // first try to add neighbors that are also edge-neighbors of the previously added cell
                size_t n_previously;
                do {
                    n_previously = neighbors.size();
                    for(vtkIdType iPt=0;iPt<npts;iPt++)
                    {
                        vertIds->SetId(0,ptIds->GetId(iPt));
                        this->mesh->GetCellNeighbors(iCell,vertIds,cellIds);
                        for(vtkIdType iNeighbor=0;iNeighbor<cellIds->GetNumberOfIds();iNeighbor++)
                        {
                            nbor.iNeighbor = cellIds->GetId(iNeighbor);
                            nbor.weight = 1.0f;
                            if(neighbors.empty() || IsEdgeNeighbor(this->mesh,neighbors.back().iNeighbor,nbor.iNeighbor))
                                add_if_new(neighbors,nbor);
                        }
                    }
                } while(neighbors.size() > n_previously);
                // add any remaining neighbors (in case mesh is non-manifold)
                for(vtkIdType iPt=0;iPt<npts;iPt++)
                {
                    vertIds->SetId(0,ptIds->GetId(iPt));
                    this->mesh->GetCellNeighbors(iCell,vertIds,cellIds);
                    for(vtkIdType iNeighbor=0;iNeighbor<cellIds->GetNumberOfIds();iNeighbor++)
                    {
                        nbor.iNeighbor = cellIds->GetId(iNeighbor);
                        nbor.weight = 1.0f;
                        add_if_new(neighbors,nbor);
                    }
                }
            }
            break;
            case TNeighborhood::EDGE_NEIGHBORS: // neighbors share an edge
            {
                vtkCell* pCell = this->mesh->GetCell(iCell);
                for(int iEdge=0;iEdge<pCell->GetNumberOfEdges();iEdge++)
                {
                    vtkIdList *vertIds = pCell->GetEdge(iEdge)->GetPointIds();
                    this->mesh->GetCellNeighbors(iCell,vertIds,cellIds);
                    for(vtkIdType iNeighbor=0;iNeighbor<cellIds->GetNumberOfIds();iNeighbor++)
                    {
                        nbor.iNeighbor = cellIds->GetId(iNeighbor);
                        nbor.weight = 1.0f;
                        add_if_new(neighbors,nbor);
                    }
                }
            }
            break;
            case TNeighborhood::FACE_NEIGHBORS:
            {
                vtkCell* pCell = this->mesh->GetCell(iCell);
                for(int iEdge=0;iEdge<pCell->GetNumberOfFaces();iEdge++)
                {
                    vtkIdList *vertIds = pCell->GetFace(iEdge)->GetPointIds();
                    this->mesh->GetCellNeighbors(iCell,vertIds,cellIds);
                    for(vtkIdType iNeighbor=0;iNeighbor<cellIds->GetNumberOfIds();iNeighbor++)
                    {
                        nbor.iNeighbor = cellIds->GetId(iNeighbor);
                        nbor.weight = 1.0f;
                        add_if_new(neighbors,nbor);
                    }
                }
            }
            break;
            default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported neighborhood type");
        }
        // normalize the weights for this cell
        float weight_sum=0.0f;
        for(int iN=0;iN<(int)neighbors.size();iN++)
            weight_sum += neighbors[iN].weight;
        weight_sum = max(weight_sum,1e-5f); // avoid div0
        for(int iN=0;iN<(int)neighbors.size();iN++)
            neighbors[iN].weight /= weight_sum;
        // store this list of neighbors
        cell_neighbors.push_back(neighbors);
        if((int)neighbors.size()>this->max_neighbors)
            this->max_neighbors = (int)neighbors.size();
        this->max_neighbors = max(1,this->max_neighbors); // avoid error in case of unconnected cells or single cell
    }

    // copy data to plain arrays
    this->cell_neighbor_indices.resize(this->mesh->GetNumberOfCells() * this->max_neighbors);
    this->cell_neighbor_weights.resize(this->mesh->GetNumberOfCells() * this->max_neighbors);
    for(int i=0;i<this->mesh->GetNumberOfCells();i++)
    {
        for(int j=0;j<(int)cell_neighbors[i].size();j++)
        {
            int k = i*this->max_neighbors + j;
            this->cell_neighbor_indices[k] = cell_neighbors[i][j].iNeighbor;
            this->cell_neighbor_weights[k] = cell_neighbors[i][j].weight;
        }
        // fill any remaining slots with iCell,0.0
        for(int j=(int)cell_neighbors[i].size();j<max_neighbors;j++)
        {
            int k = i*this->max_neighbors + j;
            this->cell_neighbor_indices[k] = i;
            this->cell_neighbor_weights[k] = 0.0f;
        }
    }
}

// ---------------------------------------------------------------------

int MeshRD::GetNumberOfCells() const
{
    return this->mesh->GetNumberOfCells();
}

// ---------------------------------------------------------------------

void MeshRD::GetAsMesh(vtkPolyData *out, const Properties &render_settings) const
{
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    string activeChemical = render_settings.GetProperty("active_chemical").GetChemical();
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();

    // 2D meshes will get returned unchanged, meshes with 3D cells will have their contour returned
    if(this->mesh->GetCellType(0)==VTK_POLYGON)
    {
        vtkSmartPointer<vtkDataSetSurfaceFilter> geom = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        geom->SetInputData(this->mesh);
        geom->Update();
        out->DeepCopy(geom->GetOutput());
    }
    else if(use_image_interpolation)
    {
        vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
        assign_attribute->SetInputData(this->mesh);
        assign_attribute->Assign(activeChemical.c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
        vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
        to_point_data->SetInputConnection(assign_attribute->GetOutputPort());
        vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
        surface->SetInputConnection(to_point_data->GetOutputPort());
        surface->SetValue(0,contour_level);
        surface->Update();
        out->DeepCopy(surface->GetOutput());
    }
    else
    {
        vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
        assign_attribute->SetInputData(this->mesh);
        assign_attribute->Assign(activeChemical.c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        threshold->SetInputConnection(assign_attribute->GetOutputPort());
#if VTK_MAJOR_VERSION > 9 || (VTK_MAJOR_VERSION == 9 && VTK_MINOR_VERSION >= 1)
        threshold->SetUpperThreshold(contour_level);
        threshold->SetThresholdFunction(vtkThreshold::THRESHOLD_UPPER);
#else
        threshold->ThresholdByUpper(contour_level);
#endif
        vtkSmartPointer<vtkDataSetSurfaceFilter> geom = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        geom->SetInputConnection(threshold->GetOutputPort());
        geom->Update();
        out->DeepCopy(geom->GetOutput());
    }
}

// ---------------------------------------------------------------------

int MeshRD::GetArenaDimensionality() const
{
    double epsilon = 1e-4;
    double *bounds = this->mesh->GetBounds();
    int dimensionality = 0;
    for(int xyz=0;xyz<3;xyz++)
        if(bounds[xyz*2+1]-bounds[xyz*2+0] > epsilon)
            dimensionality++;
    return dimensionality;
    // TODO: rotate datasets on input such that if dimensionality=2 then all z=constant, and if dimensionality=1 then all y=constant and all z=constant
}

// ---------------------------------------------------------------------

void MeshRD::GetAs2DImage(vtkImageData *out,const Properties& render_settings) const
{
    throw runtime_error("MeshRD::GetAs2DImage() : no 2D image available");
}

// ---------------------------------------------------------------------

void MeshRD::SetFrom2DImage(int iChemical, vtkImageData *im)
{
    throw runtime_error("MeshRD::SetFrom2DImage() : no 2D image available");
}

// ---------------------------------------------------------------------

float MeshRD::GetValue(float x, float y, float z, const Properties& render_settings)
{
    const double X = this->GetX();

    this->CreateCellLocatorIfNeeded();

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals)
    {
        // detect which chemical was drawn on from the click position
        const float x_gap = this->x_spacing_proportion * this->GetX();
        iChemical = int(floor((x-this->mesh->GetBounds()[0] + x_gap / 2) / (X + x_gap)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X + x_gap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    double p[3]={x-offset_x,y,z},cp[3],dist2;
    vtkIdType iCell;
    int subId;
    this->cell_locator->FindClosestPoint(p,cp,iCell,subId,dist2);

    if(iCell<0)
        return 0.0f;

    return this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str())->GetComponent( iCell, 0 );
}

// --------------------------------------------------------------------------------

void MeshRD::SetValue(float x,float y,float z,float val,const Properties& render_settings)
{
    const double X = this->GetX();

    this->CreateCellLocatorIfNeeded();

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals)
    {
        // detect which chemical was drawn on from the click position
        const float x_gap = this->x_spacing_proportion * this->GetX();
        iChemical = int(floor((x-this->mesh->GetBounds()[0] + x_gap / 2) / (X + x_gap)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X + x_gap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    double p[3]={x-offset_x,y,z},cp[3],dist2;
    vtkIdType iCell;
    int subId;
    this->cell_locator->FindClosestPoint(p,cp,iCell,subId,dist2);

    if(iCell<0)
        return;

    float old_val = this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str())->GetComponent( iCell, 0 );
    this->StorePaintAction(iChemical,iCell,old_val);
    this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str())->SetComponent( iCell, 0, val );
    this->mesh->Modified();
    this->is_modified = true;
}

// --------------------------------------------------------------------------------

void MeshRD::SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings)
{
    const double X = this->GetX();
    const double Y = this->GetY();
    const double Z = this->GetZ();

    this->CreateCellLocatorIfNeeded();

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals)
    {
        // detect which chemical was drawn on from the click position
        const float x_gap = this->x_spacing_proportion * this->GetX();
        iChemical = int(floor((x-this->mesh->GetBounds()[0] + x_gap / 2) / (X + x_gap)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X + x_gap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    r *= hypot3(X,Y,Z);

    double bbox[6]={x-offset_x-r,x-offset_x+r,y-r,y+r,z-r,z+r};
    vtkSmartPointer<vtkIdList> cells = vtkSmartPointer<vtkIdList>::New();
    this->cell_locator->FindCellsWithinBounds(bbox,cells);

    double p[3] = {x-offset_x,y,z};

    for(vtkIdType i=0;i<cells->GetNumberOfIds();i++)
    {
        int iCell = cells->GetId(i);
        vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
        this->mesh->GetCellPoints(iCell, ids);
        // set this cell if any of its points are inside
        for(vtkIdType iPt=0;iPt<ids->GetNumberOfIds();iPt++)
        {
            if(vtkMath::Distance2BetweenPoints(this->mesh->GetPoint(ids->GetId(iPt)),p)<r*r)
            {
                float old_val = this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str())->GetComponent( iCell, 0 );
                this->StorePaintAction(iChemical,iCell,old_val);
                this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str())->SetComponent( iCell, 0, val );
                break;
            }
        }
    }
    this->mesh->Modified();
    this->is_modified = true;
}

// --------------------------------------------------------------------------------

void MeshRD::CreateCellLocatorIfNeeded()
{
    if(this->cell_locator) return;

    this->cell_locator = vtkSmartPointer<vtkCellLocator>::New();
    this->cell_locator->SetDataSet(this->mesh);
    this->cell_locator->SetTolerance(0.0001);
    this->cell_locator->BuildLocator();
}

// --------------------------------------------------------------------------------

void MeshRD::FlipPaintAction(PaintAction& cca)
{
    float old_val = this->mesh->GetCellData()->GetArray(GetChemicalName(cca.iChemical).c_str())->GetComponent( cca.iCell, 0 );
    this->mesh->GetCellData()->GetArray(GetChemicalName(cca.iChemical).c_str())->SetComponent( cca.iCell, 0, cca.val );
    cca.val = old_val;
    cca.done = !cca.done;
    this->mesh->Modified();
    this->is_modified = true;
}

// --------------------------------------------------------------------------------

void MeshRD::GetMesh(vtkUnstructuredGrid* mesh) const
{
    mesh->DeepCopy(this->mesh);
}

// --------------------------------------------------------------------------------

size_t MeshRD::GetMemorySize() const
{
    const size_t DATA_SIZE = this->n_chemicals * this->data_type_size * this->mesh->GetNumberOfCells();
    const size_t NBORS_INDICES_SIZE = sizeof(int) * this->mesh->GetNumberOfCells() * this->max_neighbors;
    const size_t NBORS_WEIGHTS_SIZE = sizeof(float) * this->mesh->GetNumberOfCells() * this->max_neighbors;
    return DATA_SIZE + NBORS_INDICES_SIZE + NBORS_WEIGHTS_SIZE;
}

// --------------------------------------------------------------------------------

vector<float> MeshRD::GetData(int i_chemical) const
{
    vtkDataArray* data = this->mesh->GetCellData()->GetArray(GetChemicalName(i_chemical).c_str());
    vector<float> values(this->mesh->GetNumberOfCells());
    for (int i = 0; i < this->mesh->GetNumberOfCells(); i++)
    {
        values[i] = data->GetComponent(i, 0);
    }
    return values;
}

// --------------------------------------------------------------------------------
