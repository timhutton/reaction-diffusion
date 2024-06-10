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
#include "OpenCLImageRD.hpp"

/// An RD system that uses an OpenCL formula snippet.
/** An N-dimensional (1D,2D,3D) OpenCL RD implementations with n chemicals
 *  specified as a short formula involving delta_a, laplacian_a, etc.
 *  implemented with Euler integration, a basic finite difference stencil
 *  and float4 blocks for speed */
class FormulaOpenCLImageRD : public OpenCLImageRD
{
    public:

        FormulaOpenCLImageRD(int opencl_platform,int opencl_device,int data_type);

        void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update) override;
        vtkSmartPointer<vtkXMLDataElement> GetAsXML(bool generate_initial_pattern_when_loading) const override;

        std::string GetRuleType() const override { return "formula"; }

        bool HasEditableBlockSize() const override { return true; }
        int GetBlockSizeX() const override { return this->block_size[0]; }
        int GetBlockSizeY() const override { return this->block_size[1]; }
        int GetBlockSizeZ() const override { return this->block_size[2]; }
        void SetBlockSizeX(int n) override { this->block_size[0] = n; this->need_reload_formula = true; }
        void SetBlockSizeY(int n) override { this->block_size[1] = n; this->need_reload_formula = true; }
        void SetBlockSizeZ(int n) override { this->block_size[2] = n; this->need_reload_formula = true; }

        bool HasEditableAccuracyOption() const override { return true; }
        void SetAccuracy(Accuracy acc) override { this->accuracy = acc; this->need_reload_formula = true; }

        std::string AssembleKernelSourceFromFormula(const std::string& formula) const override;

        // we override the parameter access functions because changing the parameters requires rewriting the kernel
        void AddParameter(const std::string& name,float val) override;
        void DeleteParameter(int iParam) override;
        void DeleteAllParameters() override;
        void SetParameterName(int iParam,const std::string& s) override;
        void SetParameterValue(int iParam,float val) override;

        bool HasEditableWrapOption() const override { return true; }
        void SetWrap(bool w) override;
        bool HasEditableDataType() const override { return true; }

    private:

        int block_size[3];
};
