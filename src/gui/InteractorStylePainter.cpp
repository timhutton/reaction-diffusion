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
#include "InteractorStylePainter.hpp"

// VTK:
#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>

// wxWidgets:
#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

// ------------------------------------------------------------------------------------------------

vtkStandardNewMacro(InteractorStylePainter);

// ------------------------------------------------------------------------------------------------

void InteractorStylePainter::OnLeftButtonDown()
{
    if(!this->paint_handler) return; // can't do anything if not connected to an event handler
    int *pos = this->GetInteractor()->GetEventPosition();
    this->paint_handler->LeftMouseDown(pos[0],pos[1]);
}

// ------------------------------------------------------------------------------------------------

void InteractorStylePainter::OnLeftButtonUp()
{
    if(!this->paint_handler) return; // can't do anything if not connected to an event handler
    int *pos = this->GetInteractor()->GetEventPosition();
    this->paint_handler->LeftMouseUp(pos[0],pos[1]);
}

// ------------------------------------------------------------------------------------------------

void InteractorStylePainter::OnRightButtonDown()
{
    if(!this->paint_handler) return; // can't do anything if not connected to an event handler
    int *pos = this->GetInteractor()->GetEventPosition();
    this->paint_handler->RightMouseDown(pos[0],pos[1]);
}

// ------------------------------------------------------------------------------------------------

void InteractorStylePainter::OnRightButtonUp()
{
    if(!this->paint_handler) return; // can't do anything if not connected to an event handler
    int *pos = this->GetInteractor()->GetEventPosition();
    this->paint_handler->RightMouseUp(pos[0],pos[1]);
}

// ------------------------------------------------------------------------------------------------

void InteractorStylePainter::OnMouseMove()
{
    if(!this->paint_handler) return; // can't do anything if not connected to an event handler
    int *pos = this->GetInteractor()->GetEventPosition();
    this->paint_handler->MouseMove(pos[0],pos[1]);
}

// ------------------------------------------------------------------------------------------------

void InteractorStylePainter::SetPaintHandler(IPaintHandler* ph)
{
    this->paint_handler = ph;
}

// ------------------------------------------------------------------------------------------------

void InteractorStylePainter::OnKeyDown()
{
    if(!this->paint_handler) return; // can't do anything if not connected to an event handler
    this->paint_handler->KeyDown();
}

// ------------------------------------------------------------------------------------------------

void InteractorStylePainter::OnKeyUp()
{
    if(!this->paint_handler) return; // can't do anything if not connected to an event handler
    this->paint_handler->KeyUp();
}

// ------------------------------------------------------------------------------------------------
