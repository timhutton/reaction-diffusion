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
#include <SystemFactory.hpp>
#include <IO_XML.hpp>
#include <GrayScottImageRD.hpp>
#include <FormulaOpenCLImageRD.hpp>
#include <FullKernelOpenCLImageRD.hpp>
#include <GrayScottMeshRD.hpp>
#include <FormulaOpenCLMeshRD.hpp>
#include <FullKernelOpenCLMeshRD.hpp>
#include <Properties.hpp>
#include <OpenCL_utils.hpp>

// VTK:
#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLGenericDataObjectReader.h>

// STL:
#include <stdexcept>

using namespace std;

// -------------------------------------------------------------------------------------------------------------

unique_ptr<AbstractRD> CreateFromImageDataFile(
    const char *filename,
    bool is_opencl_available,
    int opencl_platform,
    int opencl_device,
    Properties &render_settings,
    bool &warn_to_update);

unique_ptr<AbstractRD> CreateFromUnstructuredGridFile(
    const char *filename,
    bool is_opencl_available,
    int opencl_platform,
    int opencl_device,
    Properties &render_settings,
    bool &warn_to_update);

// -------------------------------------------------------------------------------------------------------------

unique_ptr<AbstractRD> SystemFactory::CreateFromFile(
    const char *filename,
    bool is_opencl_available,
    int opencl_platform,
    int opencl_device,
    Properties &render_settings,
    bool &warn_to_update)
{
    // temporarily turn off internationalisation, to avoid string-to-float conversion issues
    char *old_locale = setlocale(LC_NUMERIC,"C");

    vtkSmartPointer<vtkXMLGenericDataObjectReader> generic_reader = vtkSmartPointer<vtkXMLGenericDataObjectReader>::New();
    bool parallel;
    int data_structure_type = generic_reader->ReadOutputType(filename,parallel);
    unique_ptr<AbstractRD> system;
    switch(data_structure_type)
    {
        case VTK_IMAGE_DATA:
            system = CreateFromImageDataFile(filename,is_opencl_available,opencl_platform,opencl_device,
                render_settings,warn_to_update);
            break;
        case VTK_UNSTRUCTURED_GRID:
            system = CreateFromUnstructuredGridFile(filename,is_opencl_available,opencl_platform,opencl_device,
                render_settings,warn_to_update);
            break;
        default:
            throw runtime_error("Unsupported data type or file read error");
    }

    // restore the old locale
    setlocale(LC_NUMERIC,old_locale);

    system->SetFilename(filename);
    system->SetModified(false);
    return system;
}

// -------------------------------------------------------------------------------------------------------------

unique_ptr<AbstractRD> CreateFromImageDataFile(
    const char *filename,
    bool is_opencl_available,
    int opencl_platform,
    int opencl_device,
    Properties &render_settings,
    bool &warn_to_update)
{
    vtkSmartPointer<RD_XMLImageReader> reader = vtkSmartPointer<RD_XMLImageReader>::New();
    reader->SetFileName(filename);
    reader->Update();
    vtkImageData *image = reader->GetOutput();

    if( image == NULL )
        throw runtime_error("Failed to read image.");
    if (image->GetPointData() == NULL)
        throw runtime_error("Image has no point data.");
    if (image->GetPointData()->GetArray(0) == NULL)
        throw runtime_error("No arrays in image point data.");

    int data_type = image->GetPointData()->GetArray(0)->GetDataType();
    string type = reader->GetType();
    string name = reader->GetName();

    unique_ptr<ImageRD> image_system;
    if(type=="inbuilt")
    {
        if(name=="Gray-Scott")
            image_system = make_unique<GrayScottImageRD>();
        else
            throw runtime_error("Unsupported inbuilt implementation: "+name);
    }
    else if(type=="formula")
    {
        if(!is_opencl_available)
            throw runtime_error(OpenCL_utils::GetOpenCLInstallationHints());
        image_system = make_unique<FormulaOpenCLImageRD>(opencl_platform,opencl_device,data_type);
    }
    else if(type=="kernel")
    {
        if(!is_opencl_available)
            throw runtime_error(OpenCL_utils::GetOpenCLInstallationHints());
        image_system = make_unique<FullKernelOpenCLImageRD>(opencl_platform,opencl_device,data_type);
    }
    else throw runtime_error("Unsupported rule type: "+type);
    image_system->InitializeFromXML(reader->GetRDElement(),warn_to_update);

    // render settings
    vtkSmartPointer<vtkXMLDataElement> xml_render_settings =
        reader->GetRDElement()->FindNestedElementWithName("render_settings");
    if(xml_render_settings) // optional
        render_settings.OverwriteFromXML(xml_render_settings);

    int dim[3];
    image->GetDimensions(dim);
    int nc = image->GetNumberOfScalarComponents() * image->GetPointData()->GetNumberOfArrays();
    image_system->SetDimensions(dim[0],dim[1],dim[2]);
    image_system->SetNumberOfChemicals(nc);
    image_system->CopyFromImage(image);
    if (reader->ShouldGenerateInitialPatternWhenLoading())
    {
        image_system->GenerateInitialPattern();
    }

    return image_system;
}

// -------------------------------------------------------------------------------------------------------------

unique_ptr<AbstractRD> CreateFromUnstructuredGridFile(
    const char *filename,
    bool is_opencl_available,
    int opencl_platform,
    int opencl_device,
    Properties &render_settings,
    bool &warn_to_update)
{
    vtkSmartPointer<RD_XMLUnstructuredGridReader> reader = vtkSmartPointer<RD_XMLUnstructuredGridReader>::New();
    reader->SetFileName(filename);
    reader->Update();
    vtkUnstructuredGrid *ugrid = reader->GetOutput();

    if (ugrid == NULL)
        throw runtime_error("Failed to read unstructured grid.");
    if (ugrid->GetCellData() == NULL)
        throw runtime_error("Unstructured grid has no cell data.");
    if (ugrid->GetCellData()->GetArray(0) == NULL)
        throw runtime_error("No arrays in unstructured grid cell data.");

    int data_type = ugrid->GetCellData()->GetArray(0)->GetDataType();
    string type = reader->GetType();
    string name = reader->GetName();

    unique_ptr<MeshRD> mesh_system;
    if(type=="inbuilt")
    {
        if(name=="Gray-Scott")
            mesh_system = make_unique<GrayScottMeshRD>();
        else
            throw runtime_error("Unsupported inbuilt implementation: "+name);
    }
    else if(type=="formula")
    {
        if(!is_opencl_available)
            throw runtime_error(OpenCL_utils::GetOpenCLInstallationHints());
        mesh_system = make_unique<FormulaOpenCLMeshRD>(opencl_platform,opencl_device,data_type);
    }
    else if(type=="kernel")
    {
        if(!is_opencl_available)
            throw runtime_error(OpenCL_utils::GetOpenCLInstallationHints());
        mesh_system = make_unique<FullKernelOpenCLMeshRD>(opencl_platform,opencl_device,data_type);
    }
    else throw runtime_error("Unsupported rule type: "+type);

    mesh_system->InitializeFromXML(reader->GetRDElement(),warn_to_update);

    mesh_system->CopyFromMesh(ugrid);
    // render settings
    vtkSmartPointer<vtkXMLDataElement> xml_render_settings =
        reader->GetRDElement()->FindNestedElementWithName("render_settings");
    if(xml_render_settings) // optional
        render_settings.OverwriteFromXML(xml_render_settings);

    if(reader->ShouldGenerateInitialPatternWhenLoading())
        mesh_system->GenerateInitialPattern();

    return mesh_system;
}

// -------------------------------------------------------------------------------------------------------------
