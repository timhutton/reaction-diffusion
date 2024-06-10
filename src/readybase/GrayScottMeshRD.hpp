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
#include "MeshRD.hpp"

/// Base class for all the inbuilt mesh implementations.
// TODO: put in its own file (when there is more than one derived class)
class InbuiltMeshRD : public MeshRD
{
    public:

        InbuiltMeshRD(int data_type) : MeshRD(data_type) {}

        std::string GetRuleType() const override { return "inbuilt"; }

        bool HasEditableFormula() const override { return false; }
        bool HasEditableNumberOfChemicals() const override { return false; }
        bool HasEditableDataType() const override { return false; }
};

/// A non-OpenCL mesh implementation, just as an example.
class GrayScottMeshRD : public InbuiltMeshRD
{
    public:

        GrayScottMeshRD();

        void SetNumberOfChemicals(int n, bool reallocate_storage = false) override;
        void CopyFromMesh(vtkUnstructuredGrid *mesh2) override;

    protected:

        void InternalUpdate(int n_steps) override;

    protected:

        vtkSmartPointer<vtkUnstructuredGrid> buffer;           ///< temporary storage used during computation
};
