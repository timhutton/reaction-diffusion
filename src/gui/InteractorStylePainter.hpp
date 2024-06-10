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

// VTK:
#include <vtkInteractorStyleTrackballCamera.h>

/// Interface for handling callbacks from InteractorStylePainter. MyFrame implements this interface.
class IPaintHandler
{
    public:
        virtual void LeftMouseDown(int x,int y) =0;
        virtual void LeftMouseUp(int x,int y) =0;
        virtual void RightMouseDown(int x,int y) =0;
        virtual void RightMouseUp(int x,int y) =0;
        virtual void MouseMove(int x,int y) =0;
        virtual void KeyDown() =0;
        virtual void KeyUp() =0;

        virtual ~IPaintHandler() {}
};

/// Interactor style where mouse events are passed to an external handler.
class InteractorStylePainter : public vtkInteractorStyleTrackballCamera
{
    public:

        InteractorStylePainter() : paint_handler(NULL) {}
        static InteractorStylePainter* New();

        void SetPaintHandler(IPaintHandler* ph);

        // handle mouse events:
        virtual void OnLeftButtonDown();
        virtual void OnLeftButtonUp();
        virtual void OnRightButtonDown();
        virtual void OnRightButtonUp();
        virtual void OnMouseMove();
        virtual void OnKeyDown();
        virtual void OnKeyUp();

    private:

        IPaintHandler *paint_handler;
};
