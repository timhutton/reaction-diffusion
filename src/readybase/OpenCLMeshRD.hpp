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

#ifndef __OPENCLMESHRD__
#define __OPENCLMESHRD__

// local:
#include "MeshRD.hpp"
#include "OpenCL_MixIn.hpp"

/// Base class for mesh implementations that use OpenCL.
class OpenCLMeshRD : public MeshRD, public OpenCL_MixIn
{
    public:

        OpenCLMeshRD(int opencl_platform,int opencl_device,int data_type);
        ~OpenCLMeshRD();

        void SetNumberOfChemicals(int n, bool reallocate_storage = false) override;

        bool HasEditableFormula() const override { return true; }

        void CopyFromMesh(vtkUnstructuredGrid* mesh2) override;

        // we override the parameter access functions because changing the parameters requires rewriting the kernel
        void AddParameter(const std::string& name,float val) override;
        void DeleteParameter(int iParam) override;
        void DeleteAllParameters() override;
        void SetParameterName(int iParam,const std::string& s) override;
        void SetParameterValue(int iParam,float val) override;

        void GenerateInitialPattern() override;
        void BlankImage(float value = 0.0f) override;

        void TestFormula(std::string program_string) override;
        std::string GetKernel() const override { return this->AssembleKernelSourceFromFormula(this->formula); }

        void SetValue(float x,float y,float z,float val,const Properties& render_settings) override;
        void SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings) override;

        void Undo() override;
        void Redo() override;

    protected:

        void InternalUpdate(int n_steps) override;

        void ReloadKernelIfNeeded() override;

        void CreateOpenCLBuffers() override;
        void WriteToOpenCLBuffersIfNeeded() override;
        void ReadFromOpenCLBuffers() override;
        void ReleaseOpenCLBuffers() override;

    private:

        cl_mem clBuffer_cell_neighbor_indices;
        cl_mem clBuffer_cell_neighbor_weights;
};

#endif
