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

#include "scene_items.hpp"

// local:
#include "colormaps.hpp"
#include "Properties.hpp"

// VTK:
#include <vtkColorSeries.h>
#include <vtkColorTransferFunction.h>
#include <vtkRenderer.h>
#include <vtkScalarsToColors.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>

// STL:
#include <set>

using namespace std;

void AddScalarBar(vtkRenderer* pRenderer,vtkScalarsToColors* lut)
{
    vtkSmartPointer<vtkScalarBarActor> scalar_bar = vtkSmartPointer<vtkScalarBarActor>::New();
    scalar_bar->SetLookupTable(lut);
    scalar_bar->SetOrientationToHorizontal();
    scalar_bar->SetWidth(0.5);
    scalar_bar->SetHeight(0.15);
    scalar_bar->SetPosition(0.25,0.01);
    scalar_bar->SetMaximumHeightInPixels(80);
    // workaround for http://www.paraview.org/Bug/view.php?id=14561
    scalar_bar->SetTitle("M");
    scalar_bar->GetTitleTextProperty()->SetOpacity(0);
    scalar_bar->SetTitleRatio(0.7);
    pRenderer->AddActor2D(scalar_bar);
}

template<int N>
void ColorMapFromList(vtkColorSeries* color_series, const float values[N][3])
{
    color_series->SetNumberOfColors(N);
    for (int i = 0; i < N; i++)
    {
        vtkColor3f color(values[i][0], values[i][1], values[i][2]);
        vtkColor3ub color_ub(color[0] * 255, color[1] * 255, color[2] * 255);
        color_series->SetColor(i, color_ub);
    }
}

void ColorTransferFunctionFromLinearColorSeries(vtkColorTransferFunction* lut, vtkColorSeries* color_series, float min_val, float max_val)
{
    for (int i = 0; i < color_series->GetNumberOfColors(); i++)
    {
        const vtkColor3ub color = color_series->GetColor(i);
        const float u = min_val + i * (max_val - min_val) / (color_series->GetNumberOfColors() - 1);
        float rgb[3] = { color.GetRed() / 255.0f, color.GetGreen() / 255.0f, color.GetBlue() / 255.0f };
        lut->AddRGBPoint(u, rgb[0], rgb[1], rgb[2]);
    }
}

vtkSmartPointer<vtkScalarsToColors> GetColorMap(const Properties& render_settings)
{
    const string colormap_label = render_settings.GetProperty("colormap").GetColorMap();
    const float low = render_settings.GetProperty("low").GetFloat();
    const float high = render_settings.GetProperty("high").GetFloat();

    vtkSmartPointer<vtkColorTransferFunction> lut = vtkSmartPointer<vtkColorTransferFunction>::New();
    if (colormap_label == "HSV blend")
    {
        lut->SetColorSpaceToHSV();
        lut->HSVWrapOff();
        float r, g, b;
        render_settings.GetProperty("color_low").GetColor(r, g, b);
        lut->AddRGBPoint(low, r, g, b);
        render_settings.GetProperty("color_high").GetColor(r, g, b);
        lut->AddRGBPoint(high, r, g, b);
    }
    else
    {
        vtkSmartPointer<vtkColorSeries> color_series = vtkSmartPointer<vtkColorSeries>::New();
        bool reverse = false;
        if (colormap_label == "spectral")
        {
            color_series->SetColorScheme(vtkColorSeries::BREWER_DIVERGING_SPECTRAL_9);
            reverse = true;
        }
        else if (colormap_label == "spectral reversed")
        {
            color_series->SetColorScheme(vtkColorSeries::BREWER_DIVERGING_SPECTRAL_9);
        }
        else if (colormap_label == "orange-purple")
        {
            color_series->SetColorScheme(vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_11);
        }
        else if (colormap_label == "purple-orange")
        {
            color_series->SetColorScheme(vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_11);
            reverse = true;
        }
        else if (colormap_label == "brown-teal")
        {
            color_series->SetColorScheme(vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_11);
        }
        else if (colormap_label == "teal-brown")
        {
            color_series->SetColorScheme(vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_11);
            reverse = true;
        }
        else if (colormap_label == "inferno")
        {
            ColorMapFromList<256>(color_series, colormaps::inferno);
        }
        else if (colormap_label == "inferno reversed")
        {
            ColorMapFromList<256>(color_series, colormaps::inferno);
            reverse = true;
        }
        else if (colormap_label == "terrain")
        {
            ColorMapFromList<6>(color_series, colormaps::terrain);
        }
        else if (colormap_label == "terrain reversed")
        {
            ColorMapFromList<6>(color_series, colormaps::terrain);
            reverse = true;
        }
        else
        {
            throw runtime_error("unsupported colormap: " + colormap_label);
        }
        const float min_val = reverse ? high : low;
        const float max_val = reverse ? low : high;
        ColorTransferFunctionFromLinearColorSeries(lut, color_series, min_val, max_val);
    }
    lut->SetNanColor(0.1, 0.1, 0.1);
    return lut;
}

void SetDefaultRenderSettings(Properties& render_settings)
{
    render_settings.DeleteAllProperties();
    render_settings.AddProperty(Property("surface_color", "color", 1.0f, 1.0f, 1.0f)); // RGB [0,1]
    render_settings.AddProperty(Property("colormap", "colormap", "spectral"));
    render_settings.AddProperty(Property("color_low", "color", 0.0f, 0.0f, 1.0f));
    render_settings.AddProperty(Property("color_high", "color", 1.0f, 0.0f, 0.0f));
    render_settings.AddProperty(Property("show_color_scale", true));
    render_settings.AddProperty(Property("show_multiple_chemicals", true));
    render_settings.AddProperty(Property("active_chemical", "chemical", "a"));
    render_settings.AddProperty(Property("low", 0.0f));
    render_settings.AddProperty(Property("high", 1.0f));
    render_settings.AddProperty(Property("vertical_scale_1D", 30.0f));
    render_settings.AddProperty(Property("vertical_scale_2D", 15.0f));
    render_settings.AddProperty(Property("contour_level", 0.25f));
    render_settings.AddProperty(Property("cap_contour", true));
    render_settings.AddProperty(Property("invert_contour_cap", false));
    render_settings.AddProperty(Property("use_wireframe", false));
    render_settings.AddProperty(Property("show_cell_edges", false));
    render_settings.AddProperty(Property("show_bounding_box", true));
    render_settings.AddProperty(Property("show_chemical_label", true));
    render_settings.AddProperty(Property("slice_3D", true));
    render_settings.AddProperty(Property("slice_3D_axis", "axis", "z"));
    render_settings.AddProperty(Property("slice_3D_position", 0.5f)); // [0,1]
    render_settings.AddProperty(Property("show_displacement_mapped_surface", true));
    render_settings.AddProperty(Property("color_displacement_mapped_surface", false));
    render_settings.AddProperty(Property("use_image_interpolation", true));
    render_settings.AddProperty(Property("timesteps_per_render", 100));
    render_settings.AddProperty(Property("show_phase_plot", false));
    render_settings.AddProperty(Property("phase_plot_x_axis", "chemical", "a"));
    render_settings.AddProperty(Property("phase_plot_y_axis", "chemical", "b"));
    render_settings.AddProperty(Property("phase_plot_z_axis", "chemical", "c"));
    render_settings.AddProperty(Property("plot_ab_orthogonally", false));
}

bool RenderSettingAppliesToDimensionality(const string& render_setting, int dimensionality)
{
    map<string, set<int>> applies; // whether the string applies to dimensionalities 1, 2 and 3
    // default is true if string is not listed
    // default for a dimensionality is false if the integer is not inserted
    applies["surface_color"].insert(2);
    applies["surface_color"].insert(3);
    applies["vertical_scale_1D"].insert(1);
    applies["vertical_scale_2D"].insert(2);
    applies["contour_level"].insert(3);
    applies["cap_contour"].insert(3);
    applies["invert_contour_cap"].insert(3);
    applies["use_wireframe"].insert(2);
    applies["use_wireframe"].insert(3);
    applies["show_bounding_box"].insert(2);
    applies["show_bounding_box"].insert(3);
    applies["slice_3D"].insert(3);
    applies["slice_3D_axis"].insert(3);
    applies["slice_3D_position"].insert(3);
    applies["show_displacement_mapped_surface"].insert(2);
    applies["color_displacement_mapped_surface"].insert(2);
    applies["plot_ab_orthogonally"].insert(1);
    if (applies.count(render_setting))
    {
        return applies[render_setting].count(dimensionality);
    }
    return true; // default to true if we don't know about this render setting
}

bool RenderSettingDoesntApplyToMesh(const string& render_setting)
{
    set<string> doesnt_apply;
    doesnt_apply.insert("vertical_scale_1D");
    doesnt_apply.insert("vertical_scale_2D");
    doesnt_apply.insert("show_displacement_mapped_surface");
    doesnt_apply.insert("color_displacement_mapped_surface");
    doesnt_apply.insert("plot_ab_orthogonally");
    return doesnt_apply.count(render_setting);
}
