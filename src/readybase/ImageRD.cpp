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
#include "ImageRD.hpp"
#include "IO_XML.hpp"
#include "overlays.hpp"
#include "Properties.hpp"
#include "scene_items.hpp"
#include "utils.hpp"

// STL:
#include <algorithm>
#include <cassert>
#include <stdexcept>

// VTK:
#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkAssignAttribute.h>
#include <vtkBox.h>
#include <vtkCamera.h>
#include <vtkCaptionActor2D.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkContourFilter.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkCubeSource.h>
#include <vtkCutter.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractEdges.h>
#include <vtkExtractPolyDataGeometry.h>
#include <vtkGeometryFilter.h>
#include <vtkImageActor.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageConstantPad.h>
#include <vtkImageData.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkImageExtractComponents.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper.h>
#include <vtkImageMirrorPad.h>
#include <vtkImageReslice.h>
#include <vtkImageStencil.h>
#include <vtkImageThreshold.h>
#include <vtkImageToStructuredPoints.h>
#include <vtkImageWrapPad.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkMergeFilter.h>
#include <vtkPlane.h>
#include <vtkPlaneSource.h>
#include <vtkPointData.h>
#include <vtkPointDataToCellData.h>
#include <vtkPointSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRearrangeFields.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarsToColors.h>
#include <vtkSmartPointer.h>
#include <vtkStripper.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTexture.h>
#include <vtkTextureMapToPlane.h>
#include <vtkThreshold.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTubeFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkWarpScalar.h>
#include <vtkWarpVector.h>
#include <vtkXMLUtilities.h>

using namespace std;

// -------------------------------------------------------------------

ImageRD::ImageRD(int data_type)
    : AbstractRD(data_type)
    , image_top1D(2.0)
    , image_ratio1D(30.0)
{
    this->starting_pattern = vtkSmartPointer<vtkImageData>::New();
    this->assign_attribute_filter = NULL;
    this->rearrange_fields_filter = NULL;
}

// ---------------------------------------------------------------------

ImageRD::~ImageRD()
{
    this->DeallocateImages();
}

// ---------------------------------------------------------------------

void ImageRD::DeallocateImages()
{
    this->images.clear();
    this->n_chemicals = 0;
}

// ---------------------------------------------------------------------

int ImageRD::GetArenaDimensionality() const
{
    assert(this->images.front());
    int dimensionality=0;
    for(int iDim=0;iDim<3;iDim++)
        if(this->images.front()->GetDimensions()[iDim]>1)
            dimensionality++;
    return dimensionality;
}

// ---------------------------------------------------------------------

float ImageRD::GetX() const
{
    return this->images.front()->GetDimensions()[0];
}

// ---------------------------------------------------------------------

float ImageRD::GetY() const
{
    return this->images.front()->GetDimensions()[1];
}

// ---------------------------------------------------------------------

float ImageRD::GetZ() const
{
    return this->images.front()->GetDimensions()[2];
}

// ---------------------------------------------------------------------

vtkImageData* ImageRD::GetImage(int iChemical) const
{
    return this->images[iChemical];
}

// ---------------------------------------------------------------------

void ImageRD::GetImage(vtkImageData *im) const
{
    vtkSmartPointer<vtkImageAppendComponents> iac = vtkSmartPointer<vtkImageAppendComponents>::New();
    for(int i=0;i<this->GetNumberOfChemicals();i++)
    {
        iac->AddInputData(this->GetImage(i));
    }
    iac->Update();
    im->DeepCopy(iac->GetOutput());
}

// ---------------------------------------------------------------------

void ImageRD::CopyFromImage(vtkImageData* im)
{
    int n_arrays = im->GetPointData()->GetNumberOfArrays();
    int n_components = im->GetNumberOfScalarComponents();

    bool has_named_arrays = true;
    for(int iChem=0;iChem<this->GetNumberOfChemicals();iChem++)
    {
        if(!im->GetPointData()->HasArray(GetChemicalName(iChem).c_str()))
        {
            has_named_arrays = false;
            break;
        }
    }

    if(has_named_arrays && n_components==1 && n_arrays==this->GetNumberOfChemicals())
    {
        // convert named array data to single-component data in multiple images
        for(int iChem=0;iChem<this->GetNumberOfChemicals();iChem++)
        {
            this->images[iChem]->SetExtent(im->GetExtent());
            this->images[iChem]->GetPointData()->SetScalars(im->GetPointData()->GetArray(GetChemicalName(iChem).c_str()));
        }
    }
    else if(n_arrays==1 && n_components==this->GetNumberOfChemicals())
    {
        // convert multi-component data to single-component data in multiple images
        vtkSmartPointer<vtkImageExtractComponents> iec = vtkSmartPointer<vtkImageExtractComponents>::New();
        iec->SetInputData(im);
        for(int i=0;i<this->GetNumberOfChemicals();i++)
        {
            iec->SetComponents(i);
            iec->Update();
            this->images[i]->DeepCopy(iec->GetOutput());
        }
    }
    else
        throw runtime_error("ImageRD::CopyFromImage : chemical count mismatch");

    this->undo_stack.clear();
}

// ---------------------------------------------------------------------

void ImageRD::CopyFromMesh(
    vtkUnstructuredGrid* mesh,
    const int num_chemicals,
    const size_t target_chemical,
    const size_t largest_dimension,
    const float value_inside,
    const float value_outside)
{
    // decide the size of the image
    mesh->ComputeBounds();
    double bounds[6];
    mesh->GetBounds(bounds);
    const double mesh_size[3] = { bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4] };
    const float max_mesh_size = std::max(mesh_size[0], std::max(mesh_size[1], mesh_size[2]));
    const float scale = (largest_dimension-1) / max_mesh_size;
    int image_size[3];
    for(size_t xyz = 0; xyz < 3; ++xyz)
    {
        image_size[xyz] = 1;
        while( image_size[xyz] < mesh_size[xyz] * scale )
        {
            image_size[xyz] <<= 1;
        }
    }
    AllocateImages(image_size[0], image_size[1], image_size[2], num_chemicals, this->GetDataType());
    BlankImage(value_outside);

    // write data into the image
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->PostMultiply();
    transform->Translate(-(bounds[0] + bounds[1]) / 2.0, -(bounds[2] + bounds[3]) / 2.0, -(bounds[4] + bounds[5]) / 2.0); // center at origin
    transform->Scale(scale, scale, scale);
    transform->Translate(image_size[0] / 2, image_size[1] / 2, image_size[2] / 2); // center in volume
    vtkSmartPointer<vtkTransformFilter> transform_filter = vtkSmartPointer<vtkTransformFilter>::New();
    transform_filter->SetTransform(transform);
    transform_filter->SetInputData(mesh);
    vtkSmartPointer<vtkGeometryFilter> get_surface = vtkSmartPointer<vtkGeometryFilter>::New();
    get_surface->SetInputConnection(transform_filter->GetOutputPort());
    vtkSmartPointer<vtkPolyDataToImageStencil> pol2stenc = vtkSmartPointer<vtkPolyDataToImageStencil>::New();
    pol2stenc->SetInputConnection(get_surface->GetOutputPort());
    vtkSmartPointer<vtkImageStencil> imgstenc = vtkSmartPointer<vtkImageStencil>::New();
    imgstenc->SetInputData(this->images[target_chemical]);
    imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
    imgstenc->ReverseStencilOn();
    imgstenc->SetBackgroundValue(value_inside);
    imgstenc->Update();
    this->images[target_chemical]->DeepCopy(imgstenc->GetOutput());
}

// ---------------------------------------------------------------------

void ImageRD::AllocateImages(int x,int y,int z,int nc,int data_type)
{
    this->DeallocateImages();
    this->n_chemicals = nc;
    this->images.resize(nc);
    for(int i=0;i<nc;i++)
        this->images[i] = AllocateVTKImage(x,y,z,data_type);
    this->is_modified = true;
    this->undo_stack.clear();
}

// ---------------------------------------------------------------------

/* static */ vtkSmartPointer<vtkImageData> ImageRD::AllocateVTKImage(int x,int y,int z,int data_type)
{
    vtkSmartPointer<vtkImageData> im = vtkSmartPointer<vtkImageData>::New();
    im->SetDimensions(x,y,z);
    im->AllocateScalars(data_type,1);
    if(im->GetDimensions()[0]!=x || im->GetDimensions()[1]!=y || im->GetDimensions()[2]!=z)
        throw runtime_error("ImageRD::AllocateVTKImage : Failed to allocate image data - dimensions too big?");
    return im;
}

// ---------------------------------------------------------------------

void ImageRD::GenerateInitialPattern()
{
    if (this->initial_pattern_generator.ShouldZeroFirst()) {
        this->BlankImage();
    }

    const int X = this->images.front()->GetDimensions()[0];
    const int Y = this->images.front()->GetDimensions()[1];
    const int Z = this->images.front()->GetDimensions()[2];

    for (size_t iOverlay = 0; iOverlay < this->initial_pattern_generator.GetNumberOfOverlays(); iOverlay++)
    {
        this->initial_pattern_generator.GetOverlay(iOverlay).Reseed();
    }

    for(int z=0;z<Z;z++)
    {
        for(int y=0;y<Y;y++)
        {
            for(int x=0;x<X;x++)
            {
                for(size_t iOverlay=0; iOverlay < this->initial_pattern_generator.GetNumberOfOverlays(); iOverlay++)
                {
                    const Overlay& overlay = this->initial_pattern_generator.GetOverlay(iOverlay);

                    int iC = overlay.GetTargetChemical();
                    if(iC<0 || iC>=this->GetNumberOfChemicals())
                        continue; // best for now to silently ignore this overlay, because the user has no way of editing the overlays (short of editing the file)
                        //throw runtime_error("Overlay: chemical out of range: "+GetChemicalName(iC));

                    vector<double> vals(this->GetNumberOfChemicals());
                    for(int i=0;i<this->GetNumberOfChemicals();i++)
                        vals[i] = this->GetImage(i)->GetScalarComponentAsDouble(x,y,z,0);
                    this->GetImage(iC)->SetScalarComponentFromDouble(x, y, z, 0, overlay.Apply(vals, *this, x, y, z));
                }
            }
        }
    }
    for(int i=0;i<(int)this->images.size();i++)
        this->images[i]->Modified();
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

void ImageRD::BlankImage(float value)
{
    for(int iImage=0;iImage<(int)this->images.size();iImage++)
    {
        this->images[iImage]->GetPointData()->GetScalars()->FillComponent(0, value);
        this->images[iImage]->Modified();
    }
    this->timesteps_taken = 0;
    this->undo_stack.clear();
}

// ---------------------------------------------------------------------

void ImageRD::Update(int n_steps)
{
    this->undo_stack.clear();
    this->InternalUpdate(n_steps);

    this->timesteps_taken += n_steps;

    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
        this->images[ic]->Modified();

    if(this->rearrange_fields_filter && this->assign_attribute_filter)
    {
        // manually update the rendering pipeline to allow for changing array names
        // TODO: this step shouldn't be necessary
        this->rearrange_fields_filter->Update();
        assign_attribute_filter->Assign(this->rearrange_fields_filter->GetOutput()->GetCellData()->GetArray(0)->GetName(),
            vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
    }
}

// ---------------------------------------------------------------------

void ImageRD::InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings)
{
    this->rearrange_fields_filter = NULL;
    this->assign_attribute_filter = NULL;

    switch(this->GetArenaDimensionality())
    {
        // TODO: merge the dimensionalities (often want one/more slices from lower dimensionalities)
        case 0:
        case 1: this->InitializeVTKPipeline_1D(pRenderer,render_settings); break;
        case 2: this->InitializeVTKPipeline_2D(pRenderer,render_settings); break;
        case 3: this->InitializeVTKPipeline_3D(pRenderer,render_settings); break;
        default:
            throw runtime_error("ImageRD::InitializeRenderPipeline : Unsupported dimensionality");
    }
}

// ---------------------------------------------------------------------

void ImageRD::InitializeVTKPipeline_1D(vtkRenderer* pRenderer,const Properties& render_settings)
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float vertical_scale_1D = render_settings.GetProperty("vertical_scale_1D").GetFloat();
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();
    bool show_chemical_label = render_settings.GetProperty("show_chemical_label").GetBool();
    bool show_phase_plot = render_settings.GetProperty("show_phase_plot").GetBool();
    int iPhasePlotX = IndexFromChemicalName(render_settings.GetProperty("phase_plot_x_axis").GetChemical());
    int iPhasePlotY = IndexFromChemicalName(render_settings.GetProperty("phase_plot_y_axis").GetChemical());
    int iPhasePlotZ = IndexFromChemicalName(render_settings.GetProperty("phase_plot_z_axis").GetChemical());
    bool plot_ab_orthogonally = render_settings.GetProperty("plot_ab_orthogonally").GetBool();
    if (plot_ab_orthogonally && this->GetNumberOfChemicals() <= 1)
    {
        plot_ab_orthogonally = false;
    }

    int iFirstChem = 0;
    int iLastChem = this->GetNumberOfChemicals() - 1;
    if(!show_multiple_chemicals)
    {
        if (plot_ab_orthogonally && iActiveChemical < 2)
        {
            iFirstChem = 0;
            iLastChem = 1;
        }
        else
        {
            iFirstChem = iActiveChemical;
            iLastChem = iFirstChem;
        }
    }

    float scaling = vertical_scale_1D / (high-low); // vertical_scale gives the height of the graph in worldspace units
    const float image_height = this->GetX() / this->image_ratio1D; // we thicken it
    const float y_gap = image_height;

    vtkSmartPointer<vtkScalarsToColors> lut = GetColorMap(render_settings);

    for(int iChem = iFirstChem; iChem <= iLastChem; ++iChem)
    {
        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        image_mapper->SetInputData(this->GetImage(iChem));

        // will convert the x*y 2D image to a x*y grid of quads
        const float image_offset = this->image_top1D - image_height * 2.0f * (iChem - iFirstChem);
        vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
        plane->SetXResolution(this->GetX());
        plane->SetYResolution(this->GetY());
        plane->SetOrigin(0,image_offset-image_height,0);
        plane->SetPoint1(this->GetX(),image_offset-image_height,0);
        plane->SetPoint2(0,image_offset,0);

        vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
        texture->SetInputConnection(image_mapper->GetOutputPort());
        if(use_image_interpolation)
            texture->InterpolateOn();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(plane->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->SetTexture(texture);
        if(show_cell_edges)
            actor->GetProperty()->EdgeVisibilityOn();
        actor->GetProperty()->SetEdgeColor(0,0,0);
        actor->GetProperty()->LightingOff();
        pRenderer->AddActor(actor);

        // add a text label
        if(show_chemical_label && this->GetNumberOfChemicals()>1)
        {
            vtkSmartPointer<vtkCaptionActor2D> captionActor = vtkSmartPointer<vtkCaptionActor2D>::New();
            captionActor->SetAttachmentPoint(-image_height, image_offset - image_height/2, 0);
            captionActor->SetPosition(0, 0);
            captionActor->SetCaption(GetChemicalName(iChem).c_str());
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
    }

    // also add a scalar bar to show how the colors correspond to values
    if(show_color_scale)
    {
        AddScalarBar(pRenderer,lut);
    }

    // add a line graph for all the chemicals (active one highlighted)
    const float graph_bottom = this->image_top1D + y_gap;
    const float graph_top = graph_bottom + (high-low) * scaling;
    for(int iChemical = iFirstChem; iChemical <= iLastChem; iChemical++)
    {
        if (plot_ab_orthogonally && iChemical == 1)
        {
            continue;
        }
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        if (plot_ab_orthogonally && iChemical == 0)
        {
            // plot the merged ab pair here
            vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
            plane->SetInputData(this->GetImage(0));
            vtkSmartPointer<vtkWarpScalar> warp_y = vtkSmartPointer<vtkWarpScalar>::New();
            warp_y->SetInputConnection(plane->GetOutputPort());
            warp_y->SetScaleFactor(scaling);
            warp_y->SetNormal(0, 1, 0);
            vtkSmartPointer<vtkImageDataGeometryFilter> plane2 = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
            plane2->SetInputData(this->GetImage(1));
            vtkSmartPointer<vtkMergeFilter> merge = vtkSmartPointer<vtkMergeFilter>::New();
            merge->SetGeometryConnection(warp_y->GetOutputPort());
            merge->SetScalarsConnection(plane2->GetOutputPort());
            vtkSmartPointer<vtkWarpScalar> warp_z = vtkSmartPointer<vtkWarpScalar>::New();
            warp_z->SetInputConnection(merge->GetOutputPort());
            warp_z->SetScaleFactor(scaling);
            warp_z->SetNormal(0, 0, -1);
            vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
            stripper->SetInputConnection(warp_z->GetOutputPort());
            vtkSmartPointer<vtkTubeFilter> tube = vtkSmartPointer<vtkTubeFilter>::New();
            tube->SetInputConnection(stripper->GetOutputPort());
            tube->SetRadius(this->GetX() / 512.0);
            tube->SetNumberOfSides(6);
            mapper->SetInputConnection(tube->GetOutputPort());
            actor->GetProperty()->SetColor(1, 1, 1);
            actor->GetProperty()->SetAmbient(0.3);
        }
        else
        {
            // plot this chemical in the normal way
            vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
            plane->SetInputData(this->GetImage(iChemical));
            vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
            warp->SetInputConnection(plane->GetOutputPort());
            warp->SetScaleFactor(scaling);
            warp->SetNormal(0, 1, 0);
            mapper->SetInputConnection(warp->GetOutputPort());
            actor->GetProperty()->SetAmbient(1);
            if (iChemical == iActiveChemical)
                actor->GetProperty()->SetColor(1, 1, 1);
            else
                actor->GetProperty()->SetColor(0.5, 0.5, 0.5);
        }
        mapper->ScalarVisibilityOff();
        actor->SetMapper(mapper);
        actor->PickableOff();
        actor->SetPosition(0, graph_bottom - low * scaling,0);
        pRenderer->AddActor(actor);
    }

    // add an axis
    vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
    axis->SetCamera(pRenderer->GetActiveCamera());
    axis->SetBounds(0,0,graph_bottom,graph_top,0,0);
    axis->SetRanges(0,0,low,high,0,0);
    axis->UseRangesOn();
    axis->XAxisVisibilityOff();
    axis->ZAxisVisibilityOff();
    axis->SetYLabel("");
    axis->SetLabelFormat("%.2f");
    axis->SetInertia(10000);
    axis->SetCornerOffset(0);
    axis->SetNumberOfLabels(5);
    axis->PickableOff();
    pRenderer->AddActor(axis);
    if (plot_ab_orthogonally)
    {
        axis->SetYLabel("a");
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(0, 0, graph_top, graph_top, -low * scaling, -high * scaling);
        axis->SetRanges(0, 0, 0, 0, low, high);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->YAxisVisibilityOff();
        axis->SetZLabel("b");
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }

    // add a phase plot
    const float phase_plot_bottom = graph_top + y_gap*2;
    if(show_phase_plot && this->GetNumberOfChemicals()>=2)
    {
        this->AddPhasePlot(pRenderer,scaling,low,high,0.0f, phase_plot_bottom,0.0f,iPhasePlotX,iPhasePlotY,iPhasePlotZ);
    }
}

// ---------------------------------------------------------------------

void ImageRD::InitializeVTKPipeline_2D(vtkRenderer* pRenderer,const Properties& render_settings)
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float vertical_scale_2D = render_settings.GetProperty("vertical_scale_2D").GetFloat();
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    float surface_r,surface_g,surface_b;
    render_settings.GetProperty("surface_color").GetColor(surface_r,surface_g,surface_b);
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    bool show_displacement_mapped_surface = render_settings.GetProperty("show_displacement_mapped_surface").GetBool();
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool color_displacement_mapped_surface = render_settings.GetProperty("color_displacement_mapped_surface").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();
    bool show_bounding_box = render_settings.GetProperty("show_bounding_box").GetBool();
    bool show_chemical_label = render_settings.GetProperty("show_chemical_label").GetBool();
    bool show_phase_plot = render_settings.GetProperty("show_phase_plot").GetBool();
    int iPhasePlotX = IndexFromChemicalName(render_settings.GetProperty("phase_plot_x_axis").GetChemical());
    int iPhasePlotY = IndexFromChemicalName(render_settings.GetProperty("phase_plot_y_axis").GetChemical());
    int iPhasePlotZ = IndexFromChemicalName(render_settings.GetProperty("phase_plot_z_axis").GetChemical());

    const float scaling = vertical_scale_2D / (high-low); // vertical_scale gives the height of the graph in worldspace units
    const float x_gap = this->x_spacing_proportion * this->GetX();
    const float y_gap = this->y_spacing_proportion * this->GetY();

    double offset[3] = {0,0,0};
    const float surface_bottom = offset[1] + 0.5 + this->GetY() + y_gap;
    const float surface_top = surface_bottom + this->GetY();

    int iFirstChem = 0;
    int iLastChem = this->GetNumberOfChemicals() - 1;
    if(!show_multiple_chemicals) { iFirstChem = iActiveChemical; iLastChem = iFirstChem; }

    vtkSmartPointer<vtkScalarsToColors> lut = GetColorMap(render_settings);

    for(int iChem = iFirstChem; iChem <= iLastChem; iChem++)
    {
        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        image_mapper->SetInputData(this->GetImage(iChem));

        // will convert the x*y 2D image to a x*y grid of quads
        vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
        plane->SetXResolution(this->GetX());
        plane->SetYResolution(this->GetY());
        plane->SetOrigin(0,0,0);
        plane->SetPoint1(this->GetX(),0,0);
        plane->SetPoint2(0,this->GetY(),0);

        vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
        texture->SetInputConnection(image_mapper->GetOutputPort());
        if(use_image_interpolation)
            texture->InterpolateOn();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(plane->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->SetPosition(offset);
        actor->SetTexture(texture);
        if(show_cell_edges)
            actor->GetProperty()->EdgeVisibilityOn();
        actor->GetProperty()->SetEdgeColor(0,0,0);
        actor->GetProperty()->LightingOff();
        pRenderer->AddActor(actor);

        if(show_displacement_mapped_surface)
        {
            vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
            plane->SetInputData(this->GetImage(iChem));
            vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
            warp->SetInputConnection(plane->GetOutputPort());
            warp->SetScaleFactor(scaling);
            vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
            normals->SetInputConnection(warp->GetOutputPort());
            normals->SplittingOff();
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            if(color_displacement_mapped_surface)
            {
                vtkSmartPointer<vtkTextureMapToPlane> tmap = vtkSmartPointer<vtkTextureMapToPlane>::New();
                tmap->SetOrigin(0,0,0);
                tmap->SetPoint1(this->GetX(),0,0);
                tmap->SetPoint2(0,this->GetY(),0);
                tmap->SetInputConnection(normals->GetOutputPort());
                mapper->SetInputConnection(tmap->GetOutputPort());
            }
            else
                mapper->SetInputConnection(normals->GetOutputPort());
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
            actor->SetPosition(offset[0]+0.5, surface_bottom, offset[2] - low*scaling);
            actor->PickableOff();
            pRenderer->AddActor(actor);

            // add the color image as the texture of the displacement-mapped surface
            if(color_displacement_mapped_surface)
            {
                vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
                texture->SetInputConnection(image_mapper->GetOutputPort());
                if(use_image_interpolation)
                    texture->InterpolateOn();
                else
                    texture->InterpolateOn();
                actor->SetTexture(texture);
            }

            // add the bounding box
            if(show_bounding_box)
            {
                vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
                box->SetBounds(0,this->GetX()-1,0,this->GetY()-1,0,(high-low)*scaling);

                vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
                edges->SetInputConnection(box->GetOutputPort());

                vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                mapper->SetInputConnection(edges->GetOutputPort());

                vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
                actor->SetMapper(mapper);
                actor->GetProperty()->SetColor(0,0,0);
                actor->GetProperty()->SetAmbient(1);
                actor->SetPosition(offset[0]+0.5,surface_bottom,offset[2]);
                actor->PickableOff();

                pRenderer->AddActor(actor);
            }
        }

        // add a text label
        if(show_chemical_label && this->GetNumberOfChemicals()>1)
        {
            const float text_label_offset = 5.0 + max(this->GetX(), this->GetY()) / 20.0f;
            vtkSmartPointer<vtkCaptionActor2D> captionActor = vtkSmartPointer<vtkCaptionActor2D>::New();
            captionActor->SetAttachmentPoint(offset[0] + this->GetX() / 2, offset[1] - text_label_offset, offset[2]);
            captionActor->SetPosition(0, 0);
            captionActor->SetCaption(GetChemicalName(iChem).c_str());
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

    // add an axis to the first diplacement-mapped surface
    if(show_displacement_mapped_surface)
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(0.5,0.5,surface_top,surface_top,0,(high-low)*scaling);
        axis->SetRanges(0,0,0,0,low,high);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->YAxisVisibilityOff();
        axis->SetZLabel("");
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }

    // add a scalar bar to show how the colors correspond to values
    if(show_color_scale)
    {
        AddScalarBar(pRenderer,lut);
    }

    // add a phase plot
    if(show_phase_plot && this->GetNumberOfChemicals()>=2)
    {
        float phase_plot_bottom = surface_bottom;
        if(show_displacement_mapped_surface)
            phase_plot_bottom = surface_top + y_gap * 2;

        this->AddPhasePlot(pRenderer,this->GetX()/(high-low),low,high,0.0f, phase_plot_bottom,0.0f,iPhasePlotX,iPhasePlotY,iPhasePlotZ);
    }
}

// ---------------------------------------------------------------------

void ImageRD::InitializeVTKPipeline_3D(vtkRenderer* pRenderer,const Properties& render_settings)
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();
    bool cap_contour = render_settings.GetProperty("cap_contour").GetBool();
    bool invert_contour_cap = render_settings.GetProperty("invert_contour_cap").GetBool();
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    bool slice_3D = render_settings.GetProperty("slice_3D").GetBool();
    string slice_3D_axis = render_settings.GetProperty("slice_3D_axis").GetAxis();
    float slice_3D_position = render_settings.GetProperty("slice_3D_position").GetFloat();
    float surface_r,surface_g,surface_b;
    render_settings.GetProperty("surface_color").GetColor(surface_r,surface_g,surface_b);
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();
    bool show_bounding_box = render_settings.GetProperty("show_bounding_box").GetBool();
    bool show_chemical_label = render_settings.GetProperty("show_chemical_label").GetBool();
    bool show_phase_plot = render_settings.GetProperty("show_phase_plot").GetBool();
    int iPhasePlotX = IndexFromChemicalName(render_settings.GetProperty("phase_plot_x_axis").GetChemical());
    int iPhasePlotY = IndexFromChemicalName(render_settings.GetProperty("phase_plot_y_axis").GetChemical());
    int iPhasePlotZ = IndexFromChemicalName(render_settings.GetProperty("phase_plot_z_axis").GetChemical());

    vtkSmartPointer<vtkScalarsToColors> lut = GetColorMap(render_settings);

    int iFirstChem = 0;
    int iLastChem = this->GetNumberOfChemicals() - 1;
    if(!show_multiple_chemicals) { iFirstChem = iActiveChemical; iLastChem = iFirstChem; }

    const float x_gap = this->x_spacing_proportion * this->GetX();
    const float y_gap = this->y_spacing_proportion * this->GetY();
    double offset[3] = {0,0,0};

    for(int iChem = iFirstChem; iChem <= iLastChem; ++iChem)
    {
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

        vtkImageData *image = this->GetImage(iChem);
        int *extent = image->GetExtent();

        // we first convert the image from point data to cell data, to match the users expectations

        vtkSmartPointer<vtkImageWrapPad> pad = vtkSmartPointer<vtkImageWrapPad>::New();
        pad->SetInputData(image);
        pad->SetOutputWholeExtent(extent[0],extent[1]+1,extent[2],extent[3]+1,extent[4],extent[5]+1);

        // move the pixel values (stored in the point data) to cell data
        vtkSmartPointer<vtkRearrangeFields> prearrange_fields = vtkSmartPointer<vtkRearrangeFields>::New();
        prearrange_fields->SetInputData(image);
        prearrange_fields->AddOperation(vtkRearrangeFields::MOVE,vtkDataSetAttributes::SCALARS,
            vtkRearrangeFields::POINT_DATA,vtkRearrangeFields::CELL_DATA);

        // get the image scalars name from the first array
        prearrange_fields->Update();
        const char *scalars_array_name = prearrange_fields->GetOutput()->GetCellData()->GetArray(0)->GetName();

        // mark the new cell data array as the active attribute
        vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
        assign_attribute->SetInputConnection(prearrange_fields->GetOutputPort());
        assign_attribute->Assign(scalars_array_name, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);

        // save the filters so we can perform a manual update step on the pipeline in Update() (TODO: work out how to do this properly)
        this->rearrange_fields_filter = prearrange_fields;
        this->assign_attribute_filter = assign_attribute;

        vtkSmartPointer<vtkMergeFilter> merge_datasets = vtkSmartPointer<vtkMergeFilter>::New();
        merge_datasets->SetGeometryConnection(pad->GetOutputPort());
        merge_datasets->SetScalarsConnection(assign_attribute->GetOutputPort());

        vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New(); // (only used if needed)
        to_point_data->SetInputConnection(merge_datasets->GetOutputPort());

        if(use_image_interpolation)
        {
            // turns the 3d grid of sampled values into a polygon mesh for rendering,
            // by making a surface that contours the volume at a specified level
            vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
            surface->SetInputConnection(to_point_data->GetOutputPort());
            surface->SetValue(0, contour_level);

            vtkSmartPointer<vtkAppendPolyData> append = vtkSmartPointer<vtkAppendPolyData>::New();
            append->AddInputConnection(surface->GetOutputPort());

            if (cap_contour)
            {
                // pad outside the volume with zero so that the contour caps the ends instead of leaving holes
                vtkSmartPointer<vtkImageConstantPad> cap_pad = vtkSmartPointer<vtkImageConstantPad>::New();
                cap_pad->SetInputConnection(to_point_data->GetOutputPort());
                cap_pad->SetOutputWholeExtent(extent[0] - 1, extent[1] + 2, extent[2] - 1, extent[3] + 2, extent[4] - 1, extent[5] + 2);
                if (invert_contour_cap)
                {
                    cap_pad->SetConstant(high + (high - low));
                }
                else
                {
                    cap_pad->SetConstant(low + (low - high));
                }

                // again make the same contour surface
                vtkSmartPointer<vtkContourFilter> cap_surface = vtkSmartPointer<vtkContourFilter>::New();
                cap_surface->SetInputConnection(cap_pad->GetOutputPort());
                cap_surface->SetValue(0, contour_level);

                // clip away the internal parts of the volume, to leave only the caps
                vtkSmartPointer<vtkBox> box = vtkSmartPointer<vtkBox>::New();
                double* bounds = image->GetBounds();
                box->SetBounds(bounds[0], bounds[1] + 1, bounds[2], bounds[3] + 1, bounds[4], bounds[5] + 1);
                vtkSmartPointer<vtkExtractPolyDataGeometry> gf = vtkSmartPointer<vtkExtractPolyDataGeometry>::New();
                gf->SetInputConnection(cap_surface->GetOutputPort());
                gf->SetImplicitFunction(box);
                gf->SetExtractInside(0);

                // throw away the normals from the volume, to leave flat sharp normals on the caps
                vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
                normals->SetInputConnection(gf->GetOutputPort());
                normals->SetFeatureAngle(5);

                // add the caps to the scene with their own normals
                append->AddInputConnection(normals->GetOutputPort());
            }

            mapper->SetInputConnection(append->GetOutputPort());
            mapper->ScalarVisibilityOff();
        }
        else
        {
            // render as cubes, Minecraft-style
            vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
            threshold->SetInputConnection(merge_datasets->GetOutputPort());
            threshold->SetInputArrayToProcess(0, 0, 0,
                vtkDataObject::FIELD_ASSOCIATION_CELLS,
                vtkDataSetAttributes::SCALARS);
#if VTK_MAJOR_VERSION > 9 || (VTK_MAJOR_VERSION == 9 && VTK_MINOR_VERSION >= 1)
            threshold->SetUpperThreshold(contour_level);
            threshold->SetThresholdFunction(vtkThreshold::THRESHOLD_UPPER);
#else
            threshold->ThresholdByUpper(contour_level);
#endif

            vtkSmartPointer<vtkGeometryFilter> geometry = vtkSmartPointer<vtkGeometryFilter>::New();
            geometry->SetInputConnection(threshold->GetOutputPort());

            mapper->SetInputConnection(geometry->GetOutputPort());
        }

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(surface_r,surface_g,surface_b);
        actor->GetProperty()->SetAmbient(0.1);
        actor->GetProperty()->SetDiffuse(0.5);
        actor->GetProperty()->SetSpecular(0.4);
        actor->GetProperty()->SetSpecularPower(10);
        if(use_wireframe)
            actor->GetProperty()->SetRepresentationToWireframe();
        if(show_cell_edges && !use_image_interpolation)
        {
            actor->GetProperty()->EdgeVisibilityOn();
            actor->GetProperty()->SetEdgeColor(0,0,0);
        }
        vtkSmartPointer<vtkProperty> bfprop = vtkSmartPointer<vtkProperty>::New();
        actor->SetBackfaceProperty(bfprop);
        bfprop->SetColor(0.7,0.6,0.55);
        bfprop->SetAmbient(0.1);
        bfprop->SetDiffuse(0.5);
        bfprop->SetSpecular(0.4);
        bfprop->SetSpecularPower(10);
        actor->SetPosition(offset);

        // add the actor to the renderer's scene
        actor->PickableOff(); // not sure about this - sometimes it is nice to paint on the contoured surface too, for 3d sculpting
        pRenderer->AddActor(actor);

        // add the bounding box
        if(show_bounding_box)
        {
            vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
            box->SetBounds(extent[0],extent[1]+1,extent[2],extent[3]+1,extent[4],extent[5]+1);

            vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
            edges->SetInputConnection(box->GetOutputPort());

            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(edges->GetOutputPort());

            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(0,0,0);
            actor->GetProperty()->SetAmbient(1);
            actor->SetPosition(offset);
            actor->PickableOff();

            pRenderer->AddActor(actor);
        }

        // add a 2D slice too
        if(slice_3D)
        {
            vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
            double *bounds = image->GetBounds();
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
            if(use_image_interpolation)
                cutter->SetInputConnection(to_point_data->GetOutputPort());
            else
                cutter->SetInputConnection(merge_datasets->GetOutputPort());
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(cutter->GetOutputPort());
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

        // add a text label
        if(show_chemical_label && this->GetNumberOfChemicals()>1)
        {
            const float text_label_offset = 5.0 + max(this->GetX(), this->GetY()) / 20.0f;
            vtkSmartPointer<vtkCaptionActor2D> captionActor = vtkSmartPointer<vtkCaptionActor2D>::New();
            captionActor->SetAttachmentPoint(offset[0] + this->GetX() / 2, offset[1] - text_label_offset, offset[2]);
            captionActor->SetPosition(0, 0);
            captionActor->SetCaption(GetChemicalName(iChem).c_str());
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
    if (show_color_scale)
    {
        AddScalarBar(pRenderer, lut);
    }

    // add a phase plot
    if(show_phase_plot && this->GetNumberOfChemicals()>=2)
    {
        this->AddPhasePlot( pRenderer,this->GetX()/(high-low),low,high,0.0f,this->GetY()+y_gap,0.0f,
                            iPhasePlotX,iPhasePlotY,iPhasePlotZ);
    }
}

// ---------------------------------------------------------------------

void ImageRD::AddPhasePlot(vtkRenderer* pRenderer,float scaling,float low,float high,float posX,float posY,float posZ,
    int iChemX,int iChemY,int iChemZ)
{
    iChemX = max( 0, min( iChemX, this->GetNumberOfChemicals()-1 ) );
    iChemY = max( 0, min( iChemY, this->GetNumberOfChemicals()-1 ) );
    iChemZ = max( 0, min( iChemZ, this->GetNumberOfChemicals()-1 ) );

    // ensure plot points remain within a reasonable range (else get view clipping issues)
    double minVal = low-(high-low)*100.0;
    double maxVal = high+(high-low)*100.0;

    vtkSmartPointer<vtkPointSource> points = vtkSmartPointer<vtkPointSource>::New();
    points->SetNumberOfPoints(this->GetNumberOfCells());
    points->SetRadius(0);

    vtkSmartPointer<vtkImageThreshold> thresholdXmin = vtkSmartPointer<vtkImageThreshold>::New();
    thresholdXmin->SetInputData(this->GetImage(iChemX));
    thresholdXmin->ThresholdByLower(minVal);
    thresholdXmin->ReplaceInOn();
    thresholdXmin->SetInValue(minVal);
    thresholdXmin->ReplaceOutOff();
    vtkSmartPointer<vtkImageThreshold> thresholdXmax = vtkSmartPointer<vtkImageThreshold>::New();
    thresholdXmax->SetInputConnection(thresholdXmin->GetOutputPort());
    thresholdXmax->ThresholdByUpper(maxVal);
    thresholdXmax->ReplaceInOn();
    thresholdXmax->SetInValue(maxVal);
    thresholdXmax->ReplaceOutOff();

    vtkSmartPointer<vtkMergeFilter> mergeX = vtkSmartPointer<vtkMergeFilter>::New();
    mergeX->SetGeometryConnection(points->GetOutputPort());
    mergeX->SetScalarsConnection(thresholdXmax->GetOutputPort());

    vtkSmartPointer<vtkWarpScalar> warpX = vtkSmartPointer<vtkWarpScalar>::New();
    warpX->UseNormalOn();
    warpX->SetNormal(1,0,0);
    warpX->SetInputConnection(mergeX->GetOutputPort());
    warpX->SetScaleFactor(scaling);

    vtkSmartPointer<vtkImageThreshold> thresholdYmin = vtkSmartPointer<vtkImageThreshold>::New();
    thresholdYmin->SetInputData(this->GetImage(iChemY));
    thresholdYmin->ThresholdByLower(minVal);
    thresholdYmin->ReplaceInOn();
    thresholdYmin->SetInValue(minVal);
    thresholdYmin->ReplaceOutOff();
    vtkSmartPointer<vtkImageThreshold> thresholdYmax = vtkSmartPointer<vtkImageThreshold>::New();
    thresholdYmax->SetInputConnection(thresholdYmin->GetOutputPort());
    thresholdYmax->ThresholdByUpper(maxVal);
    thresholdYmax->ReplaceInOn();
    thresholdYmax->SetInValue(maxVal);
    thresholdYmax->ReplaceOutOff();

    vtkSmartPointer<vtkMergeFilter> mergeY = vtkSmartPointer<vtkMergeFilter>::New();
    mergeY->SetGeometryConnection(warpX->GetOutputPort());
    mergeY->SetScalarsConnection(thresholdYmax->GetOutputPort());

    vtkSmartPointer<vtkWarpScalar> warpY = vtkSmartPointer<vtkWarpScalar>::New();
    warpY->UseNormalOn();
    warpY->SetNormal(0,1,0);
    warpY->SetInputConnection(mergeY->GetOutputPort());
    warpY->SetScaleFactor(scaling);

    vtkSmartPointer<vtkVertexGlyphFilter> glyph = vtkSmartPointer<vtkVertexGlyphFilter>::New();

    float offsetZ = 0.0f;
    if(this->GetNumberOfChemicals()>2)
    {
        vtkSmartPointer<vtkImageThreshold> thresholdZmin = vtkSmartPointer<vtkImageThreshold>::New();
        thresholdZmin->SetInputData(this->GetImage(iChemZ));
        thresholdZmin->ThresholdByLower(minVal);
        thresholdZmin->ReplaceInOn();
        thresholdZmin->SetInValue(minVal);
        thresholdZmin->ReplaceOutOff();
        vtkSmartPointer<vtkImageThreshold> thresholdZmax = vtkSmartPointer<vtkImageThreshold>::New();
        thresholdZmax->SetInputConnection(thresholdZmin->GetOutputPort());
        thresholdZmax->ThresholdByUpper(maxVal);
        thresholdZmax->ReplaceInOn();
        thresholdZmax->SetInValue(maxVal);
        thresholdZmax->ReplaceOutOff();

        vtkSmartPointer<vtkMergeFilter> mergeZ = vtkSmartPointer<vtkMergeFilter>::New();
        mergeZ->SetGeometryConnection(warpY->GetOutputPort());
        mergeZ->SetScalarsConnection(thresholdZmax->GetOutputPort());

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

void ImageRD::SaveStartingPattern()
{
    this->GetImage(this->starting_pattern);
}

// ---------------------------------------------------------------------

void ImageRD::RestoreStartingPattern()
{
    this->CopyFromImage(this->starting_pattern);
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

void ImageRD::SetDimensions(int x, int y, int z)
{
    this->AllocateImages(x,y,z,this->GetNumberOfChemicals(),this->data_type);
}

// ---------------------------------------------------------------------

void ImageRD::SetNumberOfChemicals(int n, bool reallocate_storage)
{
    const int X = this->GetX();
    const int Y = this->GetY();
    const int Z = this->GetZ();
    if (reallocate_storage)
    {
        this->DeallocateImages();
    }
    if (n == this->n_chemicals) {
        return;
    }
    if (n > this->n_chemicals)
    {
        while (static_cast<int>(this->images.size()) < n) {
            this->images.push_back( AllocateVTKImage(X, Y, Z, this->data_type) );
            this->images.back()->GetPointData()->GetScalars()->FillComponent(0, 0.0);
        }
    }
    else {
        this->images.resize(n);
    }
    this->n_chemicals = n;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void ImageRD::SetDimensionsAndNumberOfChemicals(int x,int y,int z,int nc)
{
    this->AllocateImages(x,y,z,nc,this->data_type);
}

// ---------------------------------------------------------------------

int ImageRD::GetNumberOfCells() const
{
    return this->GetX() * this->GetY() * this->GetZ();
}

// ---------------------------------------------------------------------

void ImageRD::GetAsMesh(vtkPolyData *out, const Properties &render_settings) const
{
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();

    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float vertical_scale_1D = render_settings.GetProperty("vertical_scale_1D").GetFloat();
    float vertical_scale_2D = render_settings.GetProperty("vertical_scale_2D").GetFloat();

    switch(this->GetArenaDimensionality())
    {
        case 1:
            {
                float scaling = vertical_scale_1D / (high-low); // vertical_scale gives the height of the graph in worldspace units

                vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
                plane->SetInputData(this->GetImage(iActiveChemical));
                vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
                warp->SetInputConnection(plane->GetOutputPort());
                warp->SetScaleFactor(-scaling);
                warp->Update();
                out->DeepCopy(warp->GetOutput());
            }
            break;
        case 2:
            {
                float scaling = vertical_scale_2D / (high-low); // vertical_scale gives the height of the graph in worldspace units

                vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
                plane->SetInputData(this->GetImage(iActiveChemical));
                vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
                warp->SetInputConnection(plane->GetOutputPort());
                warp->SetScaleFactor(scaling);
                vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
                normals->SetInputConnection(warp->GetOutputPort());
                normals->SplittingOff();
                normals->Update();
                out->DeepCopy(normals->GetOutput());
            }
            break;
        case 3:
            if(use_image_interpolation)
            {
                // turns the 3d grid of sampled values into a polygon mesh for rendering,
                // by making a surface that contours the volume at a specified level
                vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
                surface->SetInputData(this->GetImage(iActiveChemical));
                surface->SetValue(0, contour_level);
                surface->Update();
                out->DeepCopy(surface->GetOutput());
            }
            else
            {
                // render as cubes, Minecraft-style
                vtkImageData *image = this->GetImage(iActiveChemical);
                int *extent = image->GetExtent();

                vtkSmartPointer<vtkImageWrapPad> pad = vtkSmartPointer<vtkImageWrapPad>::New();
                pad->SetInputData(image);
                pad->SetOutputWholeExtent(extent[0],extent[1]+1,extent[2],extent[3]+1,extent[4],extent[5]+1);
                pad->Update();
                pad->GetOutput()->GetCellData()->SetScalars(image->GetPointData()->GetScalars()); // a non-pipelined operation

                vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
                threshold->SetInputConnection(pad->GetOutputPort());
                threshold->SetInputArrayToProcess(0, 0, 0,
                    vtkDataObject::FIELD_ASSOCIATION_CELLS,
                    vtkDataSetAttributes::SCALARS);
#if VTK_MAJOR_VERSION > 9 || (VTK_MAJOR_VERSION == 9 && VTK_MINOR_VERSION >= 1)
                threshold->SetUpperThreshold(contour_level);
                threshold->SetThresholdFunction(vtkThreshold::THRESHOLD_UPPER);
#else
                threshold->ThresholdByUpper(contour_level);
#endif
                vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
                transform->Translate (-.5, -.5, -.5);
                vtkSmartPointer<vtkTransformFilter> transformModel = vtkSmartPointer<vtkTransformFilter>::New();
                transformModel->SetTransform(transform);
                transformModel->SetInputConnection(threshold->GetOutputPort());

                vtkSmartPointer<vtkGeometryFilter> geometry = vtkSmartPointer<vtkGeometryFilter>::New();
                geometry->SetInputConnection(transformModel->GetOutputPort());
                geometry->Update();

                out->DeepCopy(geometry->GetOutput());
            }
            break;
    }
}

// ---------------------------------------------------------------------

void ImageRD::SaveFile(const char* filename,const Properties& render_settings,bool generate_initial_pattern_when_loading) const
{
    // convert the image to named arrays
    vtkSmartPointer<vtkImageData> im = vtkSmartPointer<vtkImageData>::New();
    im->DeepCopy(this->images.front());
    im->GetPointData()->SetScalars(NULL);
    for(int iChem=0;iChem<this->GetNumberOfChemicals();iChem++)
    {
        vtkSmartPointer<vtkDataArray> da = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( this->data_type ) );
        da->DeepCopy(this->images[iChem]->GetPointData()->GetScalars());
        da->SetName(GetChemicalName(iChem).c_str());
        im->GetPointData()->AddArray(da);
    }

    vtkSmartPointer<RD_XMLImageWriter> iw = vtkSmartPointer<RD_XMLImageWriter>::New();
    iw->SetSystem(this);
    iw->SetRenderSettings(&render_settings);
    if(generate_initial_pattern_when_loading)
        iw->GenerateInitialPatternWhenLoading();
    iw->SetFileName(filename);
    iw->SetDataModeToBinary(); // (to match MeshRD::SaveFile())
    iw->SetInputData(im);
    iw->Write();
}

// --------------------------------------------------------------------------------

void ImageRD::GetAs2DImage(vtkImageData *out,const Properties& render_settings) const
{
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());

    // create a lookup table for mapping values to colors
    vtkSmartPointer<vtkScalarsToColors> lut = GetColorMap(render_settings);

    // pass the image through the lookup table
    vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
    image_mapper->SetLookupTable(lut);
    image_mapper->SetOutputFormatToRGB(); // without this, vtkJPEGWriter writes JPEGs that some software struggles with
    switch(this->GetArenaDimensionality())
    {
        case 1:
        case 2:
            image_mapper->SetInputData(this->GetImage(iActiveChemical));
            break;
        case 3:
            {
                string slice_3D_axis = render_settings.GetProperty("slice_3D_axis").GetAxis();
                float slice_3D_position = render_settings.GetProperty("slice_3D_position").GetFloat();

                // Matrices for axial, coronal, sagittal, oblique view orientations
                static double sagittalElements[16] = { // x
                       0, 0,-1, 0,
                       1, 0, 0, 0,
                       0,-1, 0, 0,
                       0, 0, 0, 1 };
                static double coronalElements[16] = { // y
                         1, 0, 0, 0,
                         0, 0, 1, 0,
                         0,-1, 0, 0,
                         0, 0, 0, 1 };
                static double axialElements[16] = { // z
                         1, 0, 0, 0,
                         0, 1, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1 };
                /*static double obliqueElements[16] = { // could get fancy and have slanting slices
                         1, 0, 0, 0,
                         0, 0.866025, -0.5, 0,
                         0, 0.5, 0.866025, 0,
                         0, 0, 0, 1 };*/
                // Set the slice orientation
                vtkSmartPointer<vtkMatrix4x4> resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
                if(slice_3D_axis=="x")
                    resliceAxes->DeepCopy(sagittalElements);
                else if(slice_3D_axis=="y")
                    resliceAxes->DeepCopy(coronalElements);
                else if(slice_3D_axis=="z")
                    resliceAxes->DeepCopy(axialElements);
                resliceAxes->SetElement(0, 3, slice_3D_position * this->GetX());
                resliceAxes->SetElement(1, 3, slice_3D_position * this->GetY());
                resliceAxes->SetElement(2, 3, slice_3D_position * this->GetZ());

                vtkSmartPointer<vtkImageReslice> voi = vtkSmartPointer<vtkImageReslice>::New();
                voi->SetInputData(this->GetImage(iActiveChemical));
                voi->SetOutputDimensionality(2);
                voi->SetResliceAxes(resliceAxes);
                image_mapper->SetInputConnection(voi->GetOutputPort());
            };
    }
    image_mapper->Update();

    out->DeepCopy(image_mapper->GetOutput());
}

// --------------------------------------------------------------------------------

void ImageRD::SetFrom2DImage(int iChemical, vtkImageData *im)
{
    if (this->images.front()->GetDimensions()[0] != im->GetDimensions()[0] ||
        this->images.front()->GetDimensions()[1] != im->GetDimensions()[1] ||
        this->images.front()->GetDimensions()[2] != im->GetDimensions()[2] ||
        im->GetPointData()->GetNumberOfComponents() != 1 ||
        im->GetPointData()->GetNumberOfArrays() != 1)
    {
           throw runtime_error("ImageRD::SetFrom2DImage : size mismatch");
    }
    this->images[iChemical]->GetPointData()->DeepCopy(im->GetPointData());
    this->images[iChemical]->Modified();
    this->undo_stack.clear();
}

// --------------------------------------------------------------------------------

float ImageRD::GetValue(float x,float y,float z,const Properties& render_settings)
{
    const int X = this->GetX();
    const int Y = this->GetY();
    const int Z = this->GetZ();

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals && this->GetArenaDimensionality()==1)
    {
        // detect which chemical was drawn on from the click position
        const double image_height = X / this->image_ratio1D;
        iChemical = int(floor((- y + this->image_top1D + image_height)/(image_height*2)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
    }
    else if(show_multiple_chemicals && this->GetArenaDimensionality()>=2)
    {
        // detect which chemical was drawn on from the click position
        const float x_gap = this->x_spacing_proportion * this->GetX();
        iChemical = int(floor((x + x_gap / 2) / (X + x_gap)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X + x_gap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    int ix,iy,iz;
    ix = int(floor(x-offset_x));
    iy = int(floor(y));
    iz = int(floor(z));
    ix = min(X-1,max(0,ix));
    iy = min(Y-1,max(0,iy));
    iz = min(Z-1,max(0,iz));

    return this->GetImage(iChemical)->GetScalarComponentAsFloat(ix,iy,iz,0);
}

// --------------------------------------------------------------------------------

void ImageRD::SetValue(float x,float y,float z,float val,const Properties& render_settings)
{
    const int X = this->GetX();
    const int Y = this->GetY();
    const int Z = this->GetZ();

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals && this->GetArenaDimensionality()==1)
    {
        // detect which chemical was drawn on from the click position
        const double image_height = X / this->image_ratio1D;
        iChemical = int(floor((- y + this->image_top1D + image_height)/(image_height*2)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
    }
    else if(show_multiple_chemicals && this->GetArenaDimensionality()>=2)
    {
        // detect which chemical was drawn on from the click position
        const float x_gap = this->x_spacing_proportion * this->GetX();
        iChemical = int(floor((x + x_gap / 2) / (X + x_gap)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X + x_gap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    int ix,iy,iz;
    ix = int(floor(x-offset_x));
    iy = int(floor(y));
    iz = int(floor(z));
    ix = min(X-1,max(0,ix));
    iy = min(Y-1,max(0,iy));
    iz = min(Z-1,max(0,iz));

    float old_val = this->GetImage(iChemical)->GetScalarComponentAsFloat(ix,iy,iz,0);
    int ijk[3] = { ix, iy, iz };
    vtkIdType iCell = this->GetImage(iChemical)->ComputeCellId(ijk);
    this->StorePaintAction(iChemical,iCell,old_val);
    this->GetImage(iChemical)->SetScalarComponentFromFloat(ix,iy,iz,0,val);
    this->images[iChemical]->Modified();
    this->is_modified = true;
}

// --------------------------------------------------------------------------------

void ImageRD::SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings)
{
    const int X = this->GetX();
    const int Y = this->GetY();
    const int Z = this->GetZ();

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals && this->GetArenaDimensionality()==1)
    {
        // detect which chemical was drawn on from the click position
        const double image_height = X / this->image_ratio1D;
        iChemical = int(floor((- y + this->image_top1D + image_height)/(image_height*2)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
    }
    else if(show_multiple_chemicals && this->GetArenaDimensionality()>=2)
    {
        // detect which chemical was drawn on from the click position
        const float x_gap = this->x_spacing_proportion * this->GetX();
        iChemical = int(floor((x + x_gap  / 2) / (X + x_gap)));
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X + x_gap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    double *dataset_bbox = this->images.front()->GetBounds();
    r *= hypot3(dataset_bbox[1]-dataset_bbox[0],dataset_bbox[3]-dataset_bbox[2],dataset_bbox[5]-dataset_bbox[4]);

    int ix,iy,iz;
    ix = int(floor(x-offset_x));
    iy = int(floor(y));
    iz = int(floor(z));
    ix = min(X-1,max(0,ix));
    iy = min(Y-1,max(0,iy));
    iz = min(Z-1,max(0,iz));

    for(int tz=max(0,int(iz-r));tz<=min(Z-1,int(iz+r));tz++)
    {
        for(int ty=max(0,int(iy-r));ty<=min(Y-1,int(iy+r));ty++)
        {
            for(int tx=max(0,int(ix-r));tx<=min(X-1,int(ix+r));tx++)
            {
                if(hypot3(ix-tx,iy-ty,iz-tz)<r)
                {
                    float old_val = this->GetImage(iChemical)->GetScalarComponentAsFloat(tx,ty,tz,0);
                    int ijk[3] = { tx, ty, tz };
                    vtkIdType iCell = this->GetImage(iChemical)->ComputeCellId(ijk);
                    this->StorePaintAction(iChemical,iCell,old_val);
                    this->GetImage(iChemical)->SetScalarComponentFromFloat(tx,ty,tz,0,val);
                }
            }
        }
    }
    this->images[iChemical]->Modified();
    this->is_modified = true;
}

// --------------------------------------------------------------------------------

void ImageRD::FlipPaintAction(PaintAction& cca)
{
    float *pCell = static_cast<float*>(this->GetImage(cca.iChemical)->GetScalarPointer()) + cca.iCell;
    float old_val = *pCell;
    *pCell = cca.val;
    cca.val = old_val;
    cca.done = !cca.done;
    this->images[cca.iChemical]->Modified();
}

// --------------------------------------------------------------------------------

size_t ImageRD::GetMemorySize() const
{
    return this->n_chemicals * this->data_type_size * this->GetX() * this->GetY() * this->GetZ();
}

// --------------------------------------------------------------------------------

vector<float> ImageRD::GetData(int i_chemical) const
{
    vector<float> values(this->GetX() * this->GetY() * this->GetZ());
    size_t i = 0;
    for(int z = 0; z < this->GetZ(); z++)
    {
        for (int y = 0; y < this->GetY(); y++)
        {
            for (int x = 0; x < this->GetX(); x++)
            {
                values[i++] = this->images[i_chemical]->GetScalarComponentAsFloat(x, y, z, 0);
            }
        }
    }
    return values;
}

// --------------------------------------------------------------------------------
