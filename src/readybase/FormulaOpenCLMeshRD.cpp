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
#include "FormulaOpenCLMeshRD.hpp"
#include "utils.hpp"

// STL:
#include <string>
#include <sstream>

// VTK:
#include <vtkXMLUtilities.h>

using namespace std;

// -------------------------------------------------------------------------

FormulaOpenCLMeshRD::FormulaOpenCLMeshRD(int opencl_platform,int opencl_device,int data_type)
    : OpenCLMeshRD(opencl_platform,opencl_device,data_type)
{
    // these settings are used in File > New Pattern
    this->SetRuleName("Gray-Scott");
    this->AddParameter("timestep",1.0f);
    this->AddParameter("D_a",0.082f);
    this->AddParameter("D_b",0.041f);
    this->AddParameter("K",0.06f);
    this->AddParameter("F",0.035f);
    this->SetFormula("\
delta_a = D_a * laplacian_a - a*b*b + F*(1.0" + this->data_type_suffix + "-a);\n\
delta_b = D_b * laplacian_b + a*b*b - (F+K)*b;");
}

// -------------------------------------------------------------------------

std::string FormulaOpenCLMeshRD::AssembleKernelSourceFromFormula(const std::string& f) const
{
    const string indent = "    ";
    const int NC = this->GetNumberOfChemicals();

    ostringstream kernel_source;
    kernel_source << fixed << setprecision(6);
    if( this->data_type == VTK_DOUBLE )
    {
        kernel_source << "\
#ifdef cl_khr_fp64\n\
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable\n\
#elif defined(cl_amd_fp64)\n\
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable\n\
#else\n\
    #error \"Double precision floating point not supported on this OpenCL device. Choose another or contact the Ready team.\"\n\
#endif\n\n";
    }
    // output the function definition
    kernel_source << "kernel void rd_compute(";
    for(int i=0;i<NC;i++)
        kernel_source << "global " << this->data_type_string << " *" << GetChemicalName(i) << "_in,";
    for(int i=0;i<NC;i++)
        kernel_source << "global " << this->data_type_string << " *" << GetChemicalName(i) << "_out,";
    kernel_source << "global int* neighbor_indices,global float* neighbor_weights,const int max_neighbors)\n";
    // output the body
    kernel_source << "{\n";
    kernel_source << indent << "const int index_x = get_global_id(0);\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << this->data_type_string << " " << GetChemicalName(i) << " = " << GetChemicalName(i) << "_in[index_x];\n";
    kernel_source << "\n";
    // compute the laplacians
    kernel_source << indent << "// compute the Laplacians\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << this->data_type_string << " laplacian_" << GetChemicalName(i) << " = -" << GetChemicalName(i) << ";\n";
    kernel_source << indent << "int _offset = index_x * max_neighbors;\n";
    kernel_source << indent << "for(int _i=0;_i<max_neighbors;_i++)\n" << indent << "{\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << indent << "laplacian_" << GetChemicalName(i) << " += " << GetChemicalName(i)
                      << "_in[neighbor_indices[_offset+_i]] * neighbor_weights[_offset+_i];\n";
    kernel_source << indent << "}\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "laplacian_" << GetChemicalName(i) << " *= 4.0" << this->data_type_suffix << ";\n"; // TODO: not sure about 3D meshes
    kernel_source << "\n";
    // the parameters (assume all float for now)
    kernel_source << indent << "// parameters:\n";
    for (const Parameter& parameter : this->parameters)
    {
        kernel_source << indent << this->data_type_string << " " << parameter.name << " = " << parameter.value << this->data_type_suffix << ";\n";
    }
    // the update step
    for(int i=0;i<NC;i++)
        kernel_source << indent << this->data_type_string << " delta_" << GetChemicalName(i) << " = 0.0" << this->data_type_suffix << ";\n";
    kernel_source << "\n" << indent << "// the formula:\n";
    kernel_source << f << "\n";
    // the forward-Euler step
    kernel_source << indent << "// forward-Euler update step:\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << GetChemicalName(i) << "_out[index_x] = " << GetChemicalName(i) << " + timestep * delta_" << GetChemicalName(i) << ";\n";
    // finish up
    kernel_source << "}\n";

    return kernel_source.str();
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    OpenCLMeshRD::InitializeFromXML(rd,warn_to_update);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // formula:
    vtkSmartPointer<vtkXMLDataElement> xml_formula = rule->FindNestedElementWithName("formula");
    if(!xml_formula) throw runtime_error("formula node not found in file");

    // number_of_chemicals:
    read_required_attribute(xml_formula,"number_of_chemicals",this->n_chemicals);

    string formula = trim_multiline_string(xml_formula->GetCharacterData());
    this->TestFormula(formula); // will throw on error
    this->SetFormula(formula); // (won't throw yet)
}

// -------------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> FormulaOpenCLMeshRD::GetAsXML(bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<vtkXMLDataElement> rd = OpenCLMeshRD::GetAsXML(generate_initial_pattern_when_loading);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found");

    // formula
    vtkSmartPointer<vtkXMLDataElement> formula = vtkSmartPointer<vtkXMLDataElement>::New();
    formula->SetName("formula");
    formula->SetIntAttribute("number_of_chemicals",this->GetNumberOfChemicals());
    string f = this->GetFormula();
    f = ReplaceAllSubstrings(f, "\n", "\n        "); // indent the lines
    formula->SetCharacterData(f.c_str(), (int)f.length());
    rule->AddNestedElement(formula);

    return rd;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::SetParameterValue(int iParam,float val)
{
    AbstractRD::SetParameterValue(iParam,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::SetParameterName(int iParam,const string& s)
{
    AbstractRD::SetParameterName(iParam,s);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::AddParameter(const std::string& name,float val)
{
    AbstractRD::AddParameter(name,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::DeleteParameter(int iParam)
{
    AbstractRD::DeleteParameter(iParam);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::DeleteAllParameters()
{
    AbstractRD::DeleteAllParameters();
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------
