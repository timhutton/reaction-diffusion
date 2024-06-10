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

/// Base class for all the inbuilt implementations.
// (TODO: put in separate files when we have more than one derived class)
class InbuiltImageRD : public ImageRD
{
    public:
        InbuiltImageRD(int data_type) : ImageRD(data_type) {}

        std::string GetRuleType() const override { return "inbuilt"; }

        bool HasEditableFormula() const override { return false; }
        bool HasEditableNumberOfChemicals() const override { return false; }

        bool HasEditableWrapOption() const override { return true; }
        bool HasEditableDataType() const override { return false; }
};

/// An inbuilt implementation: n-dimensional Gray-Scott.
class GrayScottImageRD : public InbuiltImageRD
{
    public:

        GrayScottImageRD();
        ~GrayScottImageRD();

    protected:

        std::vector<vtkSmartPointer<vtkImageData>> buffer_images; // one for each chemical

    protected:

        void AllocateImages(int x,int y,int z,int nc,int data_type) override;

        void InternalUpdate(int n_steps) override;

        void DeleteBuffers();
};
