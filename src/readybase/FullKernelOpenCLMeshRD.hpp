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
#include "OpenCLMeshRD.hpp"

/// An RD system that uses an OpenCL program.
class FullKernelOpenCLMeshRD : public OpenCLMeshRD
{
    public:

        FullKernelOpenCLMeshRD(int opencl_platform,int opencl_device,int data_type);
        FullKernelOpenCLMeshRD(const OpenCLMeshRD& source); // copy construct from source

        void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update) override;
        vtkSmartPointer<vtkXMLDataElement> GetAsXML(bool generate_initial_pattern_when_loading) const override;

        std::string GetRuleType() const override { return "kernel"; }

        std::string AssembleKernelSourceFromFormula(const std::string& formula) const override;

        bool HasEditableDataType() const override { return false; }
};
