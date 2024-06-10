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
#include "vtk_pipeline.hpp"
#include "wxVTKRenderWindowInteractor.h"

// readybase:
#include "ImageRD.hpp"
#include "utils.hpp"
#include "Properties.hpp"

// VTK:
#include <vtkCamera.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>

// STL:
#include <stdexcept>

using namespace std;

// ------------------------------------------------------------------------------------------------

void InitializeVTKPipeline(
    wxVTKRenderWindowInteractor* pVTKWindow,
    AbstractRD& system,
    const Properties& render_settings,
    bool reset_camera)
{
    assert(pVTKWindow);

    // the VTK renderer is responsible for drawing the scene onto the screen
    vtkSmartPointer<vtkRenderer> pRenderer;
    if(pVTKWindow->GetRenderWindow()->GetRenderers()->GetNumberOfItems()>0)
    {
        pRenderer = pVTKWindow->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
        pRenderer->RemoveAllViewProps();
    }
    else
    {
        pRenderer = vtkSmartPointer<vtkRenderer>::New();
        pVTKWindow->GetRenderWindow()->AddRenderer(pRenderer); // connect it to the window

        // set the background color
        pRenderer->GradientBackgroundOn();
        pRenderer->SetBackground(0,0.4,0.6);
        pRenderer->SetBackground2(0,0.2,0.3);

        // change the interactor style to a trackball
        vtkSmartPointer<vtkInteractorStyleTrackballCamera> is = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
        pVTKWindow->SetInteractorStyle(is);
    }

    system.InitializeRenderPipeline(pRenderer,render_settings);

    if(reset_camera)
    {
        vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
        if(system.GetArenaDimensionality()>2)
        {
            camera->SetPosition(-3,3,10); // move off-axis to look more natural
        }
        else if( system.GetArenaDimensionality()==2
                && render_settings.GetProperty("show_displacement_mapped_surface").GetBool()
                && system.GetFileExtension()!="vtu" )
        {
            camera->SetPosition(0,-3,10); // move slightly down, to give perspective on the displacement plots
        }
        pRenderer->SetActiveCamera(camera);
        pRenderer->ResetCamera();
    }
}

// ------------------------------------------------------------------------------------------------
