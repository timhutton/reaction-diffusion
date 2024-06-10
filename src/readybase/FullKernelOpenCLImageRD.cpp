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
#include "FullKernelOpenCLImageRD.hpp"
#include "utils.hpp"

// STL:
#include <string>

// VTK:
#include <vtkXMLUtilities.h>
#include <vtkImageData.h>

using namespace std;

// ---------------------------------------------------------------------------------------------------------

FullKernelOpenCLImageRD::FullKernelOpenCLImageRD(int opencl_platform,int opencl_device,int data_type)
    : OpenCLImageRD(opencl_platform,opencl_device,data_type)
{
    this->SetRuleName("Full kernel example");
    this->SetFormula("kernel void rd_compute() {}");
    this->block_size[0]=1;
    this->block_size[1]=1;
    this->block_size[2]=1;
}

// ---------------------------------------------------------------------------------------------------------

FullKernelOpenCLImageRD::FullKernelOpenCLImageRD(const OpenCLImageRD& source)
    : OpenCLImageRD(source.GetPlatform(),source.GetDevice(),source.GetDataType())
{
    this->block_size[0] = source.GetBlockSizeX();
    this->block_size[1] = source.GetBlockSizeY();
    this->block_size[2] = source.GetBlockSizeZ();

    this->SetFormula(source.GetKernel());

    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    source.GetImage(image);
    this->SetDimensionsAndNumberOfChemicals(image->GetDimensions()[0],image->GetDimensions()[1],
        image->GetDimensions()[2],source.GetNumberOfChemicals());
    this->CopyFromImage(image);

    this->SetRuleName(source.GetRuleName());
    this->SetDescription(source.GetDescription());

    this->initial_pattern_generator.ReadFromXML(source.GetAsXML(false)->FindNestedElementWithName("initial_pattern_generator"));

    // TODO: copy starting pattern?
}

// ---------------------------------------------------------------------------------------------------------

string FullKernelOpenCLImageRD::AssembleKernelSourceFromFormula(const string& formula) const
{
    ostringstream kernel_source;
    if (this->use_local_memory)
    {
        kernel_source << "#define LX " << this->local_work_size[0] << "\n";
        kernel_source << "#define LY " << this->local_work_size[1] << "\n";
        kernel_source << "#define LZ " << this->local_work_size[2] << "\n";
    }
    kernel_source << formula;
    return kernel_source.str();
}

// ---------------------------------------------------------------------------------------------------------

void FullKernelOpenCLImageRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    OpenCLImageRD::InitializeFromXML(rd,warn_to_update);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // kernel:
    vtkSmartPointer<vtkXMLDataElement> xml_kernel = rule->FindNestedElementWithName("kernel");
    if(!xml_kernel) throw runtime_error("kernel node not found in file");
    string formula = trim_multiline_string(xml_kernel->GetCharacterData());
    read_required_attribute(xml_kernel,"block_size_x",this->block_size[0]);
    read_required_attribute(xml_kernel,"block_size_y",this->block_size[1]);
    read_required_attribute(xml_kernel,"block_size_z",this->block_size[2]);

    // number_of_chemicals:
    read_required_attribute(xml_kernel,"number_of_chemicals",this->n_chemicals);

    // do this last, because it requires everything else to be set up first
    this->TestFormula(formula); // will throw on error but won't set
    this->SetFormula(formula); // will set but won't throw
}

// ---------------------------------------------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> FullKernelOpenCLImageRD::GetAsXML(bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<vtkXMLDataElement> rd = OpenCLImageRD::GetAsXML(generate_initial_pattern_when_loading);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found");

    vtkSmartPointer<vtkXMLDataElement> kernel = vtkSmartPointer<vtkXMLDataElement>::New();
    kernel->SetName("kernel");
    kernel->SetIntAttribute("number_of_chemicals",this->GetNumberOfChemicals());
    kernel->SetIntAttribute("block_size_x",this->block_size[0]);
    kernel->SetIntAttribute("block_size_y",this->block_size[1]);
    kernel->SetIntAttribute("block_size_z",this->block_size[2]);
    string f = this->GetFormula();
    f = ReplaceAllSubstrings(f, "\n", "\n        "); // indent the lines
    kernel->SetCharacterData(f.c_str(), (int)f.length());
    rule->AddNestedElement(kernel);

    return rd;
}

// ---------------------------------------------------------------------------------------------------------
