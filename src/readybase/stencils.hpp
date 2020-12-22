/*  Copyright 2011-2020 The Ready Bunch

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

// Stdlib:
#include <set>
#include <string>
#include <vector>

union Point
{
    struct { int x, y, z; };
    int xyz[3];

    std::string GetName() const;

    bool operator<(const Point& b) const
    {
        if (x < b.x) return true;
        else if (!(b.x < x) && y < b.y) return true;
        else if(!(b.y < y) && z < b.z) return true;
        return false;
    }
};

struct StencilPoint
{
    Point point;
    float weight;

    std::string GetCode(int iSlot, const std::string& chem) const;
};

struct Stencil
{
    std::string label; // e.g. "laplacian"
    std::vector<StencilPoint> points;
    int divisor;
    int dx_power;

    std::string GetDivisorCode() const;
};

struct InputPoint
{
    Point point;
    std::string chem;

    std::string GetName() const;

    bool operator<(const InputPoint& b) const
    {
        if (point < b.point) return true;
        else if (!(b.point < point) && chem < b.chem) return true;
        return false;
    }
};

struct AppliedStencil
{
    Stencil stencil;
    std::string chem; // e.g. "a"

    std::string GetName() const { return stencil.label + "_" + chem; }
    std::set<InputPoint> GetInputPoints_Block411() const;
    static Point CellPointToBlockPoint(const Point& point);
};

std::vector<Stencil> GetKnownStencils();