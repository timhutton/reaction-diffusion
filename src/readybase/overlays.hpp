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

#ifndef __OVERLAYS__
#define __OVERLAYS__

// STL:
#include <memory>
#include <string>
#include <vector>

// VTK:
#include <vtkSmartPointer.h>
#include <vtkXMLDataElement.h>

// local:
#include "utils.hpp"
class AbstractRD;

// ------------------------------------------------------------------------------------------------

/// Base class for a mathematical operation to be carried out at a particular location in the RD system.
class BaseOperation : public XML_Object
{
public:

    virtual ~BaseOperation() {}

    /// construct when we don't know the derived type (returns empty pointer if name is unknown)
    static std::unique_ptr<BaseOperation> New(vtkXMLDataElement* node);

    /// apply the operation to target, with parameter value
    virtual void Apply(double& target, double value) const = 0;

protected:

    /// can construct from an XML node
    BaseOperation(vtkXMLDataElement* node) : XML_Object(node) {}
};

// ------------------------------------------------------------------------------------------------

/// Base class for different ways of specifying values at a particular location in the RD system.
class BaseFill : public XML_Object
{
public:

    virtual ~BaseFill() {}

    /// construct when we don't know the derived type (returns empty pointer if name is unknown)
    static std::unique_ptr<BaseFill> New(vtkXMLDataElement* node);

    /// what value would this fill type be at the given location, given the existing data
    virtual double GetValue(const AbstractRD& system, const std::vector<double>& vals, float x, float y, float z) const = 0;

    /// cause the fill to give different results next time, for those fills that use randomness
    virtual void Reseed() {}

protected:

    /// can construct from an XML node
    BaseFill(vtkXMLDataElement* node) : XML_Object(node) {}
};

// ------------------------------------------------------------------------------------------------

/// Base class for different shapes that we can draw onto the RD system.
class BaseShape : public XML_Object
{
public:

    virtual ~BaseShape() {}

    /// construct when we don't know the derived type (returns empty pointer if name is unknown)
    static std::unique_ptr<BaseShape> New(vtkXMLDataElement* node);

    /// returns whether the x, y, z location is inside this shape
    virtual bool IsInside(float x, float y, float z, float X, float Y, float Z, int dimensionality) const = 0;

protected:

    /// can construct from an XML node
    BaseShape(vtkXMLDataElement* node) : XML_Object(node) {}
};

// ------------------------------------------------------------------------------------------------

/// An overlay is a filled shape to be drawn on top of an image (think: stacked transparencies).
class Overlay : public XML_Object
{
    public:

        /// can construct from an XML node
        Overlay(vtkXMLDataElement* node);

        /// for saving to file, get the overlay as an XML element
        vtkSmartPointer<vtkXMLDataElement> GetAsXML() const override;

        static const char* GetTypeName() { return "overlay"; }

        int GetTargetChemical() const { return this->iTargetChemical; }

        /// given a vector of values (one for each chemical) at a location in a system,
        /// apply all the operations and return the new value
        double Apply(const std::vector<double>& vals, const AbstractRD& system,float x,float y,float z) const;

        /// cause the overlay to give different results next time, for those overlays that use randomness
        void Reseed() { this->fill->Reseed(); }

    protected:

        int iTargetChemical;             ///< each overlay applies to a single chemical

        std::unique_ptr<BaseOperation> op;               ///< e.g. overwrite, add, multiply, etc.
        std::unique_ptr<BaseFill> fill;                  ///< e.g. constant value, white noise, named parameter, other chemical, etc.
        std::vector<std::unique_ptr<BaseShape>> shapes;  ///< e.g. rectangle, sphere, scattered shapes, etc.

    private:

        Overlay();          ///< not implemented
        Overlay(Overlay&);  ///< not implemented
};

// ------------------------------------------------------------------------------------------------

#endif
