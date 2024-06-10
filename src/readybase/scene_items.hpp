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

// Local:
class Properties;

// STL:
#include <string>

// VTK:
class vtkRenderer;
class vtkScalarsToColors;
#include <vtkSmartPointer.h>

void AddScalarBar(vtkRenderer* pRenderer,vtkScalarsToColors* lut);

vtkSmartPointer<vtkScalarsToColors> GetColorMap(const Properties& render_settings);

void SetDefaultRenderSettings(Properties& render_settings);

bool RenderSettingAppliesToDimensionality(const std::string& render_setting, int dimensionality);
bool RenderSettingDoesntApplyToMesh(const std::string& render_setting);

static const std::string SupportedColorMaps[] = {
    "HSV blend", "spectral", "spectral reversed", "inferno", "inferno reversed", "terrain", "terrain reversed",
    "orange-purple", "purple-orange", "brown-teal", "teal-brown" };
