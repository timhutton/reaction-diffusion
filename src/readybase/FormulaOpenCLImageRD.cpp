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
#include "FormulaOpenCLImageRD.hpp"
#include "stencils.hpp"
#include "utils.hpp"

// STL:
#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <string>

// VTK:
#include <vtkXMLUtilities.h>

using namespace std;

// -------------------------------------------------------------------------

FormulaOpenCLImageRD::FormulaOpenCLImageRD(int opencl_platform,int opencl_device,int data_type)
    : OpenCLImageRD(opencl_platform,opencl_device,data_type)
    , block_size{4, 1, 1}
{
    // these settings are used in File > New Pattern
    this->SetRuleName("Gray-Scott");
    this->AddParameter("timestep",1.0f);
    this->AddParameter("D_a",0.082f);
    this->AddParameter("D_b",0.041f);
    this->AddParameter("K",0.06f);
    this->AddParameter("F",0.035f);
    this->SetFormula("\
delta_a = D_a * laplacian_a - a*b*b + F*(1.0"+this->data_type_suffix+"-a);\n\
delta_b = D_b * laplacian_b + a*b*b - (F+K)*b;");
}

// -------------------------------------------------------------------------

struct InputsNeeded {
    vector<string> chemicals_needed;
    vector<AppliedStencil> stencils_needed;
    set<InputPoint> cells_needed;
    map<string, int> gradient_mag_squared;
    bool using_x_pos;
    bool using_y_pos;
    bool using_z_pos;
    vector<string> deltas_needed;
    vector<string> local_memory_needed;
    int stencil_radii[3];
};

// -------------------------------------------------------------------------

InputsNeeded DetectInputsNeeded(const string& formula, int num_chemicals, int dimensionality, const int block_size[3],
                                const AbstractRD::Accuracy& accuracy)
{
    InputsNeeded inputs_needed;

    const vector<string> formula_tokens = tokenize_for_keywords(formula);
    const vector<Stencil> known_stencils = GetKnownStencils(dimensionality, accuracy);
    for (int i = 0; i < num_chemicals; i++)
    {
        const string chem = GetChemicalName(i);
        inputs_needed.chemicals_needed.push_back(chem);
        // assume we will need the central cell
        inputs_needed.cells_needed.insert({ { { 0, 0, 0 } }, chem });
        // assume we need delta_<chem> for the forward Euler step
        inputs_needed.deltas_needed.push_back(chem);
        // assume we need local memory for every chemical
        inputs_needed.local_memory_needed.push_back(chem);
        // search for keywords that make use of stencils
        set<string> dependent_stencils;
        if (UsingKeyword(formula_tokens, "gradient_mag_squared_" + chem))
        {
            inputs_needed.gradient_mag_squared[chem] = dimensionality;
            switch (dimensionality)
            {
            default:
            case 3:
                dependent_stencils.insert("z_gradient_" + chem);
            case 2:
                dependent_stencils.insert("y_gradient_" + chem);
            case 1:
                dependent_stencils.insert("x_gradient_" + chem); // (N.B. no breaks)
            }
        }
        // search for keywords that are stencils
        for (const Stencil& stencil : known_stencils)
        {
            const string keyword = stencil.label + "_" + chem;
            if (UsingKeyword(formula_tokens, keyword) || dependent_stencils.find(keyword) != dependent_stencils.end())
            {
                const AppliedStencil applied_stencil{ stencil, chem };
                inputs_needed.stencils_needed.push_back(applied_stencil);
                // add the cell inputs needed for this stencil
                const set<InputPoint> input_points = applied_stencil.GetInputPoints();
                inputs_needed.cells_needed.insert(input_points.begin(), input_points.end());
            }
        }
        // search for direct access to neighbors, e.g. "a_nw"
        const int MAX_RADIUS = 10; // surely if the user wants something this big they should use a kernel?
        for (int x = -MAX_RADIUS; x <= MAX_RADIUS; x++)
        {
            for (int y = -MAX_RADIUS; y <= MAX_RADIUS; y++)
            {
                for (int z = -MAX_RADIUS; z <= MAX_RADIUS; z++)
                {
                    const InputPoint input_point{ { {x, y, z} }, chem };
                    if (UsingKeyword(formula_tokens, input_point.GetName()))
                    {
                        inputs_needed.cells_needed.insert(input_point);
                    }
                }
            }
        }
    }
    if (block_size[0] == 4)
    {
        // non-block-aligned inputs need other inputs: the two blocks that supply them
        vector<InputPoint> blocks_needed;
        for (const InputPoint& input_point : inputs_needed.cells_needed)
        {
            if (input_point.point.x % 4 != 0)
            {
                const pair<InputPoint, InputPoint> blocks = input_point.GetAlignedBlocks_Block411();
                blocks_needed.push_back(blocks.first);
                blocks_needed.push_back(blocks.second);
            }
        }
        inputs_needed.cells_needed.insert(blocks_needed.begin(), blocks_needed.end());
    }
    // detect if using x_pos, y_pos or z_pos
    inputs_needed.using_x_pos = UsingKeyword(formula_tokens, "x_pos");
    inputs_needed.using_y_pos = UsingKeyword(formula_tokens, "y_pos");
    inputs_needed.using_z_pos = UsingKeyword(formula_tokens, "z_pos");
    // compute the overall stencil radius in each direction
    inputs_needed.stencil_radii[0] = 0;
    inputs_needed.stencil_radii[1] = 0;
    inputs_needed.stencil_radii[2] = 0;
    for (const InputPoint& input_point : inputs_needed.cells_needed)
    {
        inputs_needed.stencil_radii[0] = max(inputs_needed.stencil_radii[0], abs(input_point.point.x) / block_size[0]);
        inputs_needed.stencil_radii[1] = max(inputs_needed.stencil_radii[1], abs(input_point.point.y) / block_size[1]);
        inputs_needed.stencil_radii[2] = max(inputs_needed.stencil_radii[2], abs(input_point.point.z) / block_size[2]);
    }

    return inputs_needed;
}

// -------------------------------------------------------------------------

struct KernelOptions {
    KernelOptions(bool wrap, const string& indent, int data_type, const string& data_type_string,
                  const string& data_type_suffix, const int block_size[3],
                  bool use_local_memory, const size_t local_work_size[3])
        : wrap(wrap)
        , indent(indent)
        , data_type(data_type)
        , data_type_string(data_type_string)
        , data_type_suffix(data_type_suffix)
        , block_size{ block_size[0], block_size[1], block_size[2] }
        , use_local_memory(use_local_memory)
        , local_work_size{ local_work_size[0], local_work_size[1], local_work_size[2] }
    {}
    bool wrap;
    string indent;
    int data_type;
    string data_type_string;
    string data_type_suffix;
    const int block_size[3];
    bool use_local_memory;
    const size_t local_work_size[3];
};

// -------------------------------------------------------------------------

void WriteHeader(ostringstream& kernel_source, const InputsNeeded& inputs_needed, const KernelOptions& options)
{
    if (options.data_type == VTK_DOUBLE)
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
    if (options.use_local_memory)
    {
        kernel_source << "// work group size, in blocks:\n";
        kernel_source << "#define LX " << options.local_work_size[0] << "\n";
        kernel_source << "#define LY " << options.local_work_size[1] << "\n";
        kernel_source << "#define LZ " << options.local_work_size[2] << "\n\n";
        kernel_source << "// neighborhood size in each direction, in blocks:\n";
        kernel_source << "#define XR " << inputs_needed.stencil_radii[0] << "\n";
        kernel_source << "#define YR " << inputs_needed.stencil_radii[1] << "\n";
        kernel_source << "#define ZR " << inputs_needed.stencil_radii[2] << "\n\n";
    }
    // output the function declaration
    kernel_source << "kernel void rd_compute(";
    for (const string& chem : inputs_needed.chemicals_needed)
    {
        kernel_source << "global " << options.data_type_string << " *" << chem << "_in";
        kernel_source << ",";
    }
    for (size_t i = 0; i < inputs_needed.chemicals_needed.size(); i++)
    {
        kernel_source << "global " << options.data_type_string << " *" << inputs_needed.chemicals_needed[i] << "_out";
        if (i < inputs_needed.chemicals_needed.size() - 1)
        {
            kernel_source << ",";
        }
    }
    kernel_source << ")\n{\n";
}

// -------------------------------------------------------------------------

void WriteParameters(ostringstream& kernel_source, const vector<AbstractRD::Parameter>& parameters,
                     const InputsNeeded& inputs_needed, const KernelOptions& options)
{
    kernel_source << options.indent << "// parameters:\n";
    for (const AbstractRD::Parameter& parameter : parameters)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " " << parameter.name
            << " = " << setprecision(8) << parameter.value << options.data_type_suffix << ";\n";
    }
    // add a dx parameter for grid spacing if one is not already supplied
    const bool has_dx_parameter = find_if(parameters.begin(), parameters.end(),
        [](const AbstractRD::Parameter& param) { return param.name == "dx"; }) != parameters.end();
    if (!inputs_needed.stencils_needed.empty() && !has_dx_parameter)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " dx = 1.0" << options.data_type_suffix << "; // grid spacing\n";
        // TODO: only need this if using a stencil that uses dx
    }
    kernel_source << "\n";
}

// -------------------------------------------------------------------------

void WriteIndices(ostringstream& kernel_source, const InputsNeeded& inputs_needed, const KernelOptions& options)
{
    kernel_source << options.indent << "// indices:\n";
    kernel_source << options.indent << "const int index_x = get_global_id(0);\n";
    kernel_source << options.indent << "const int index_y = get_global_id(1);\n";
    kernel_source << options.indent << "const int index_z = get_global_id(2);\n";
    if (options.use_local_memory)
    {
        kernel_source << options.indent << "const int local_x = get_local_id(0);\n";
        kernel_source << options.indent << "const int local_y = get_local_id(1);\n";
        kernel_source << options.indent << "const int local_z = get_local_id(2);\n";
    }
    kernel_source << options.indent << "const int X = get_global_size(0);\n";
    kernel_source << options.indent << "const int Y = get_global_size(1);\n";
    kernel_source << options.indent << "const int Z = get_global_size(2);\n";
    kernel_source << options.indent << "const int index_here = X*(Y*index_z + index_y) + index_x;\n";
    for (const string& chem : inputs_needed.chemicals_needed)
    {
        kernel_source << options.indent << options.data_type_string << " " << chem << " = " << chem << "_in[index_here];\n";
        // (non-const to allow the user to assign directly to it if needed)
    }
    kernel_source << "\n";
}

// -------------------------------------------------------------------------

void WriteLocalMemoryCopyBlocksUnrolled(ostringstream& kernel_source, const InputsNeeded& inputs_needed, const KernelOptions& options)
{
    // unroll the copy blocks, with if-statements to check if local index is for a cell that should be copied
    int copy_size[3];
    for (int i = 0; i < 3; i++)
    {
        copy_size[i] = ceil((options.local_work_size[i] + inputs_needed.stencil_radii[i] * 2) / (float)options.local_work_size[i]);
    }
    for (int cz = 0; cz < copy_size[2]; cz++)
    {
        for (int cy = 0; cy < copy_size[1]; cy++)
        {
            for (int cx = 0; cx < copy_size[0]; cx++)
            {
                const bool first_block = (cx == 0 && cy == 0 && cz == 0);
                if (!first_block) // (the first block is always full, so no need for the if-statement)
                {
                    kernel_source << options.indent << "if(";
                    if (cx > 0)
                    {
                        kernel_source << cx << " * LX + local_x < LX + XR * 2";
                        if (cy > 0 || cz > 0)
                        {
                            kernel_source << " && ";
                        }
                    }
                    if (cy > 0)
                    {
                        kernel_source << cy << " * LY + local_y < LY + YR * 2";
                        if (cz > 0)
                        {
                            kernel_source << " && ";
                        }
                    }
                    if (cz > 0)
                    {
                        kernel_source << cz << " * LZ + local_z < LZ + ZR * 2";
                    }
                    kernel_source << ") {\n";
                }
                for (const string& chem : inputs_needed.local_memory_needed)
                {
                    ostringstream ix;
                    ix << "index_x - XR";
                    if (cx > 0)
                    {
                        ix << " + " << cx << " * LX";
                    }
                    ostringstream iy;
                    iy << "index_y - YR";
                    if (cy > 0)
                    {
                        iy << " + " << cy << " * LY";
                    }
                    ostringstream iz;
                    iz << "index_z - ZR";
                    if (cz > 0)
                    {
                        iz << " + " << cz << " * LZ";
                    }
                    if (!first_block)
                    {
                        kernel_source << options.indent;
                    }
                    kernel_source << options.indent << "local_" << chem << "[";
                    if (cz > 0)
                    {
                        kernel_source << cz << " * LZ + ";
                    }
                    kernel_source << "local_z][";
                    if (cy > 0)
                    {
                        kernel_source << cy << " * LY + ";
                    }
                    kernel_source << "local_y][";
                    if (cx > 0)
                    {
                        kernel_source << cx << " * LX + ";
                    }
                    kernel_source << "local_x]";
                    kernel_source << "= " << chem << "_in[" << GetIndexString(ix.str(), iy.str(), iz.str(), options.wrap) << "]; \n";
                }
                if (!first_block)
                {
                    kernel_source << options.indent << "}\n";
                }
            }
        }
    }
}

// -------------------------------------------------------------------------

void WriteLocalMemoryCopyBlocksWithLoops(ostringstream& kernel_source, const InputsNeeded& inputs_needed, const KernelOptions& options)
{
    // include the for-loops in the kernel code
    kernel_source << options.indent << "const int x_start = index_x - local_x - XR;\n";
    kernel_source << options.indent << "const int x_end = index_x - local_x + LX + XR;\n";
    kernel_source << options.indent << "const int y_start = index_y - local_y - YR;\n";
    kernel_source << options.indent << "const int y_end = index_y - local_y + LY + YR;\n";
    kernel_source << options.indent << "const int z_start = index_z - local_z - ZR;\n";
    kernel_source << options.indent << "const int z_end = index_z - local_z + LZ + ZR;\n";
    kernel_source << options.indent << "for (int z = z_start + local_z; z < z_end; z += LZ) {\n";
    kernel_source << options.indent << options.indent << "for (int y = y_start + local_y; y < y_end; y += LY) {\n";
    kernel_source << options.indent << options.indent << options.indent << "for (int x = x_start + local_x; x < x_end; x += LX) {\n";
    for (const string& chem : inputs_needed.local_memory_needed)
    {
        kernel_source << options.indent << options.indent << options.indent << options.indent << "local_" << chem
            << "[z - z_start][y - y_start][x - x_start] = " << chem << "_in["
            << GetIndexString("x", "y", "z", options.wrap) << "];\n";
    }
    kernel_source << options.indent << options.indent << options.indent << "}\n";
    kernel_source << options.indent << options.indent << "}\n";
    kernel_source << options.indent << "}\n";
}

// -------------------------------------------------------------------------

void WriteLocalMemorySection(ostringstream& kernel_source, const InputsNeeded& inputs_needed, const KernelOptions& options)
{
    kernel_source << options.indent << "// copy into local memory:\n";
    // declare the local memory arrays
    for (const string& chem : inputs_needed.local_memory_needed)
    {
        kernel_source << options.indent << "local " << options.data_type_string << " local_" << chem
            << "[LZ + ZR * 2][LY + YR * 2][LX + XR * 2];\n";
    }
    WriteLocalMemoryCopyBlocksUnrolled(kernel_source, inputs_needed, options);
    //WriteLocalMemoryCopyBlocksWithLoops(kernel_source, inputs_needed, options); // possibly a bit slower, may depend on stencils
    kernel_source << options.indent << "barrier(CLK_LOCAL_MEM_FENCE);\n";
    kernel_source << options.indent << "const int lx = local_x + XR;\n";
    kernel_source << options.indent << "const int ly = local_y + YR;\n";
    kernel_source << options.indent << "const int lz = local_z + ZR;\n";
    kernel_source << "\n";
}

// -------------------------------------------------------------------------

void WriteCellsNeeded(ostringstream& kernel_source, const set<InputPoint>& cells_needed, const KernelOptions& options)
{
    kernel_source << options.indent << "// cells needed:\n";
    // write code to retrieve the block-aligned inputs from global memory
    for (const InputPoint& input_point : cells_needed)
    {
        // block-aligned points (but not the central cell)
        if (!(input_point.point.x == 0 && input_point.point.y == 0 && input_point.point.z == 0)
            && input_point.point.x % options.block_size[0] == 0)
        {
            kernel_source << options.indent << "const " << options.data_type_string << " "
                          << input_point.GetDirectAccessCode(options.wrap, options.block_size, options.use_local_memory) << ";\n";
        }
    }
    if (options.block_size[0] == 4)
    {
        // write code to compute the non-block-aligned float4's from the block-aligned ones we have retrieved
        for (const InputPoint& input_point : cells_needed)
        {
            if (input_point.point.x % options.block_size[0] != 0)
            {
                // swizzle from the retrieved blocks
                kernel_source << options.indent << "const " << options.data_type_string << " " << input_point.GetName()
                    << " = (" << options.data_type_string << ")(" << input_point.GetSwizzled_Block411() << ");\n";
            }
        }
    }
    kernel_source << "\n";
}

// -------------------------------------------------------------------------

void WriteKeywords(ostringstream& kernel_source, const InputsNeeded& inputs_needed, const KernelOptions& options)
{
    kernel_source << options.indent << "// keywords needed:\n";
    // write code for the stencils we need
    for (const AppliedStencil& applied_stencil : inputs_needed.stencils_needed)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " " << applied_stencil.GetCode() << ";\n";
    }
    // write code for x_pos, y_pos, z_pos if needed
    if (inputs_needed.using_x_pos)
    {
        if (options.block_size[0] == 4)
        {
            kernel_source << options.indent << "const " << options.data_type_string << " x_pos = (index_x + (" << options.data_type_string
                << ")(0.0" << options.data_type_suffix << ", 0.25" << options.data_type_suffix << ", 0.5" << options.data_type_suffix
                << ", 0.75" << options.data_type_suffix << ")) / X;\n";
        }
        else
        {
            kernel_source << options.indent << "const " << options.data_type_string << " x_pos = index_x / (" << options.data_type_string << ")(X); \n";
        }
    }
    if (inputs_needed.using_y_pos)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " y_pos = index_y / (" << options.data_type_string << ")(Y); \n";
    }
    if (inputs_needed.using_z_pos)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " z_pos = index_z / (" << options.data_type_string << ")(Z);\n";
    }
    // write code for gradient_mag_squared if needed
    for (const auto& pair : inputs_needed.gradient_mag_squared)
    {
        const string& chem = pair.first;
        const int dimensionality = pair.second;
        kernel_source << options.indent << "const " << options.data_type_string << " gradient_mag_squared_" << chem
            << " = pow(x_gradient_" << chem << ", 2.0" << options.data_type_suffix << ")";
        if (dimensionality > 1)
        {
            kernel_source << " + pow(y_gradient_" << chem << ", 2.0" << options.data_type_suffix << ")";
            if (dimensionality > 2)
            {
                kernel_source << " + pow(z_gradient_" << chem << ", 2.0" << options.data_type_suffix << ")";
            }
        }
        kernel_source << ";\n";
    }
    // declare delta_a, etc. and initialize to zero
    for (const string& chem : inputs_needed.deltas_needed)
    {
        kernel_source << options.indent << options.data_type_string << " delta_" << chem << " = 0.0" << options.data_type_suffix << ";\n";
    }
    kernel_source << "\n";
}

// -------------------------------------------------------------------------

string AssembleKernelSource(const InputsNeeded& inputs_needed,
    const vector<AbstractRD::Parameter>& parameters,
    const string& formula,
    const KernelOptions& options)
{
    ostringstream kernel_source;
    kernel_source << fixed << setprecision(6);
    // add the #defines and the kernel definition header
    WriteHeader(kernel_source, inputs_needed, options);
    // add the parameters
    WriteParameters(kernel_source, parameters, inputs_needed, options);
    // add the bit that retrieves the global indices etc.
    WriteIndices(kernel_source, inputs_needed, options);
    // add the bit that declares local memory and copies into it
    if (options.use_local_memory)
    {
        WriteLocalMemorySection(kernel_source, inputs_needed, options);
    }
    // add the cells we need
    WriteCellsNeeded(kernel_source, inputs_needed.cells_needed, options);
    // add the keywords we need
    WriteKeywords(kernel_source, inputs_needed, options);
    // add the formula
    kernel_source << options.indent << "// the formula:\n";
    istringstream iss(formula);
    string s;
    while (iss.good())
    {
        getline(iss, s);
        kernel_source << options.indent << s << "\n";
    }
    kernel_source << "\n";
    // add the forward-Euler step
    // TODO: only add this when delta_<chem> appears in the formula
    kernel_source << options.indent << "// forward-Euler update step:\n";
    for (const string& chem : inputs_needed.chemicals_needed)
    {
        kernel_source << options.indent << chem << "_out[index_here] = " << chem << " + timestep * delta_" << chem << ";\n";
    }
    // TODO: timestep only needed if it appears in the formula or if we are doing forward-Euler for at least one chemical
    // finish up
    kernel_source << "}\n";

    return kernel_source.str();
}

// -------------------------------------------------------------------------

string FormulaOpenCLImageRD::AssembleKernelSourceFromFormula(const string& formula) const
{
    string full_data_type_string = this->data_type_string;
    if (this->block_size[0] == 4 && this->block_size[1] == 1 && this->block_size[2] == 1)
    {
        full_data_type_string += "4";
    }
    else if(this->block_size[0] == 1 && this->block_size[1] == 1 && this->block_size[2] == 1)
    {
    }
    else
    {
        throw runtime_error("unsupported block size in AssembleKernelSourceFromFormula");
    }

    const InputsNeeded inputs_needed = DetectInputsNeeded(formula, this->GetNumberOfChemicals(),
        this->GetArenaDimensionality(), this->block_size, this->GetAccuracy());

    const string indent = "    ";
    const KernelOptions options(this->wrap, indent, this->data_type, full_data_type_string, this->data_type_suffix, this->block_size,
        this->use_local_memory, this->local_work_size);

    string amended_formula = formula;
    if (this->data_type == VTK_DOUBLE)
    {
        // float4 doesn't auto-convert to double4 or double
        amended_formula = ReplaceAllSubstrings(amended_formula, "float4", full_data_type_string);
    }
    else if (this->data_type == VTK_FLOAT)
    {
        // float4 doesn't auto-convert to float
        amended_formula = ReplaceAllSubstrings(amended_formula, "float4", full_data_type_string);
        // double4 doesn't auto-convert to float4 or float
        amended_formula = ReplaceAllSubstrings(amended_formula, "double4", full_data_type_string);
        // double doesn't auto-convert to float4 or float
        amended_formula = ReplaceAllSubstrings(amended_formula, "double", full_data_type_string);
    }

    const string kernel_source = AssembleKernelSource(inputs_needed, this->parameters, amended_formula, options);

    return kernel_source;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    OpenCLImageRD::InitializeFromXML(rd,warn_to_update);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // formula:
    vtkSmartPointer<vtkXMLDataElement> xml_formula = rule->FindNestedElementWithName("formula");
    if(!xml_formula) throw runtime_error("formula node not found in file");
    read_optional_attribute(xml_formula, "block_size_x", this->block_size[0]);
    read_optional_attribute(xml_formula, "block_size_y", this->block_size[1]);
    read_optional_attribute(xml_formula, "block_size_z", this->block_size[2]);

    // number_of_chemicals:
    read_required_attribute(xml_formula,"number_of_chemicals",this->n_chemicals);

    // accuracy
    string accuracy_string;
    read_optional_attribute(xml_formula, "accuracy", accuracy_string);
    if (accuracy_string.size() > 0)
    {
        const char* accuracy_labels[3] = { "low", "medium", "high" };
        auto it = find(accuracy_labels, accuracy_labels + 3, accuracy_string);
        if (it == accuracy_labels + 3)
        {
            throw std::runtime_error("unknown accuracy attribute: " + accuracy_string);
        }
        this->SetAccuracy(static_cast<AbstractRD::Accuracy>(it - accuracy_labels));
    }

    string formula = trim_multiline_string(xml_formula->GetCharacterData());
    //this->TestFormula(formula); // will throw on error
    this->SetFormula(formula); // (won't throw yet)
}

// -------------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> FormulaOpenCLImageRD::GetAsXML(bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<vtkXMLDataElement> rd = OpenCLImageRD::GetAsXML(generate_initial_pattern_when_loading);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found");

    // formula
    vtkSmartPointer<vtkXMLDataElement> formula = vtkSmartPointer<vtkXMLDataElement>::New();
    formula->SetName("formula");
    formula->SetIntAttribute("number_of_chemicals",this->GetNumberOfChemicals());
    formula->SetIntAttribute("block_size_x", this->block_size[0]);
    formula->SetIntAttribute("block_size_y", this->block_size[1]);
    formula->SetIntAttribute("block_size_z", this->block_size[2]);
    const char* accuracy_labels[3] = { "low", "medium", "high" };
    formula->SetAttribute("accuracy", accuracy_labels[static_cast<int>(this->accuracy)]);
    string f = this->GetFormula();
    f = ReplaceAllSubstrings(f, "\n", "\n        "); // indent the lines
    formula->SetCharacterData(f.c_str(), (int)f.length());
    rule->AddNestedElement(formula);

    return rd;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::SetParameterValue(int iParam,float val)
{
    AbstractRD::SetParameterValue(iParam,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::SetParameterName(int iParam,const string& s)
{
    AbstractRD::SetParameterName(iParam,s);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::AddParameter(const std::string& name,float val)
{
    AbstractRD::AddParameter(name,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::DeleteParameter(int iParam)
{
    AbstractRD::DeleteParameter(iParam);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::DeleteAllParameters()
{
    AbstractRD::DeleteAllParameters();
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::SetWrap(bool w)
{
    AbstractRD::SetWrap(w);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------
