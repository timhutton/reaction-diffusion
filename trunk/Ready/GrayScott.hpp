/*  Copyright 2011, 2012 The Ready Bunch

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
#include "BaseRD.hpp"

// a base class for all the inbuilt implementations (TODO: put in separate files when we have more than one derived class)
class BaseInbuilt_RD : public BaseRD
{
    public:

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        virtual bool HasEditableFormula() const { return false; }
        virtual bool HasEditableNumberOfChemicals() const { return false; }
};

// inbuilt implementation: n-dimensional Gray-Scott
class GrayScott : public BaseInbuilt_RD
{
    public:

        GrayScott();
        ~GrayScott();

        virtual void Allocate(int x,int y,int z,int nc);
        virtual void Update(int n_steps);

    protected:

        std::vector<vtkImageData*> buffer_images; // one for each chemical

    protected:

        void DeleteBuffers();
};