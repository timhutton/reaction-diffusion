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

#define _USE_MATH_DEFINES

// local:
#include "MeshGenerators.hpp"
#include "utils.hpp"

// VTK:
#include <vtkAppendFilter.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCleanPolyData.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkGenericCell.h>
#include <vtkLinearSubdivisionFilter.h>
#include <vtkMath.h>
#include <vtkPlatonicSolidSource.h>
#include <vtkPointLocator.h>
#include <vtkPointSource.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangle.h>
#include <vtkUnstructuredGrid.h>

// STL:
#include <cmath>
#include <vector>

using namespace std;

// ---------------------------------------------------------------------

void MeshGenerators::GetGeodesicSphere(int n_subdivisions,vtkUnstructuredGrid *mesh,int n_chems,int data_type)
{
    vtkSmartPointer<vtkPlatonicSolidSource> icosahedron = vtkSmartPointer<vtkPlatonicSolidSource>::New();
    icosahedron->SetSolidTypeToIcosahedron();
    vtkSmartPointer<vtkLinearSubdivisionFilter> subdivider = vtkSmartPointer<vtkLinearSubdivisionFilter>::New();
    subdivider->SetInputConnection(icosahedron->GetOutputPort());
    subdivider->SetNumberOfSubdivisions(n_subdivisions);
    subdivider->Update();
    mesh->SetPoints(subdivider->GetOutput()->GetPoints());
    mesh->SetCells(VTK_POLYGON,subdivider->GetOutput()->GetPolys());

    // push the vertices out into the shape of a sphere
    const float scale = 100.0f; // we make the sphere larger to make <pixel> access more useful
    double p[3];
    for(int i=0;i<mesh->GetNumberOfPoints();i++)
    {
        mesh->GetPoint(i,p);
        vtkMath::Normalize(p);
        mesh->GetPoints()->SetPoint(i,p[0]*scale,p[1]*scale,p[2]*scale);
    }

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetTorus(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems,int data_type)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    float radius1 = nx; // we scale the torus to give <pixel> access a better chance of being useful
    float radius2 = radius1 * 1.5f; // could allow user to change the proportions
    vtkSmartPointer<vtkTransform> r1 = vtkSmartPointer<vtkTransform>::New();
    vtkSmartPointer<vtkTransform> r2 = vtkSmartPointer<vtkTransform>::New();
    double p[3],p2[3];
    for(int x=0;x<nx;x++)
    {
        p[0]=p[1]=p[2]=0;
        // translate, rotate
        p[1] += radius1;
        r1->TransformPoint(p,p);
        // rotate the transform further for next time
        r1->RotateX(360.0/nx);
        for(int y=0;y<ny;y++)
        {
            // translate
            p2[0] = p[0];
            p2[1] = p[1] + radius2;
            p2[2] = p[2];
            // rotate
            r2->TransformPoint(p2,p2);
            pts->InsertNextPoint(p2);
            // rotate the transform further for next time
            r2->RotateZ(360.0/ny);
            // make a quad
            cells->InsertNextCell(4);
            cells->InsertCellPoint(x*ny+y);
            cells->InsertCellPoint(x*ny+(y+1)%ny);
            cells->InsertCellPoint(((x+1)%nx)*ny+(y+1)%ny);
            cells->InsertCellPoint(((x+1)%nx)*ny+y);
        }
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetTriangularMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems,int data_type)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    const double scale = 2.0;
    const double th = sqrt(3.0)/2.0; // height of an equilateral triangle with edge length 1

    double p[3]={0,0,0};
    for(int y=0;y<ny;y++)
    {
        p[1] = th*y * scale;
        for(int x=0;x<nx;x++)
        {
            p[0] = ( ((y%2)?0.5:0) + x) * scale;
            pts->InsertNextPoint(p);
            if(y%2 && x<nx-1)
            {
                cells->InsertNextCell(3);
                cells->InsertCellPoint(y*nx+x);
                cells->InsertCellPoint((y-1)*nx+x);
                cells->InsertCellPoint((y-1)*nx+x+1);
                cells->InsertNextCell(3);
                cells->InsertCellPoint(y*nx+x);
                cells->InsertCellPoint((y-1)*nx+x+1);
                cells->InsertCellPoint(y*nx+x+1);
                if(y<ny-1)
                {
                    cells->InsertNextCell(3);
                    cells->InsertCellPoint(y*nx+x);
                    cells->InsertCellPoint((y+1)*nx+x+1);
                    cells->InsertCellPoint((y+1)*nx+x);
                    cells->InsertNextCell(3);
                    cells->InsertCellPoint(y*nx+x);
                    cells->InsertCellPoint(y*nx+x+1);
                    cells->InsertCellPoint((y+1)*nx+x+1);
                }
            }
        }
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetRhombilleTiling(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems,int data_type)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    const double scale = 2.0;
    const double th = sqrt(3.0)/2.0; // height of an equilateral triangle with edge length 1

    double p[3]={0,0,0};
    for(int y=0;y<ny;y++)
    {
        p[1] = th*y * scale;
        for(int x=0;x<nx;x++)
        {
            p[0] = (((y%2)?0.5:0) + x) * scale;
            pts->InsertNextPoint(p);
            if(y%2 && x%3==2 && y<ny-1)
            {
                cells->InsertNextCell(4);
                cells->InsertCellPoint(y*nx+x);       // a
                cells->InsertCellPoint((y-1)*nx+x);   // b
                cells->InsertCellPoint((y-1)*nx+x-1); // c
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertNextCell(4);
                cells->InsertCellPoint((y-1)*nx+x-1); // c
                cells->InsertCellPoint(y*nx+x-2);     // d
                cells->InsertCellPoint((y+1)*nx+x-1); // e
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertNextCell(4);
                cells->InsertCellPoint((y+1)*nx+x-1); // e
                cells->InsertCellPoint((y+1)*nx+x);   // f
                cells->InsertCellPoint(y*nx+x);       // a
                cells->InsertCellPoint(y*nx+x-1);     // center
            }
            else if(y%2==0 && x%3==1 && y>0 && y<ny-1 && x>1)
            {
                cells->InsertNextCell(4);
                cells->InsertCellPoint(y*nx+x);       // a
                cells->InsertCellPoint((y-1)*nx+x-1); // b
                cells->InsertCellPoint((y-1)*nx+x-2); // c
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertNextCell(4);
                cells->InsertCellPoint((y-1)*nx+x-2); // c
                cells->InsertCellPoint(y*nx+x-2);     // d
                cells->InsertCellPoint((y+1)*nx+x-2); // e
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertNextCell(4);
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertCellPoint((y+1)*nx+x-2); // e
                cells->InsertCellPoint((y+1)*nx+x-1); // f
                cells->InsertCellPoint(y*nx+x);       // a
            }
        }
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetHexagonalMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems,int data_type)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    const double scale = 2.0;
    const double th = sqrt(3.0)/2.0; // height of an equilateral triangle with edge length 1

    double p[3]={0,0,0};
    for(int y=0;y<ny;y++)
    {
        p[1] = th*y * scale;
        for(int x=0;x<nx;x++)
        {
            p[0] = (((y%2)?0.5:0) + x) * scale;
            pts->InsertNextPoint(p);
            if(y%2 && x%3==2 && y<ny-1)
            {
                cells->InsertNextCell(6);
                cells->InsertCellPoint(y*nx+x);
                cells->InsertCellPoint((y-1)*nx+x);
                cells->InsertCellPoint((y-1)*nx+x-1);
                cells->InsertCellPoint(y*nx+x-2);
                cells->InsertCellPoint((y+1)*nx+x-1);
                cells->InsertCellPoint((y+1)*nx+x);
            }
            else if(y%2==0 && x%3==1 && y>0 && y<ny-1 && x>1)
            {
                cells->InsertNextCell(6);
                cells->InsertCellPoint(y*nx+x);
                cells->InsertCellPoint((y-1)*nx+x-1);
                cells->InsertCellPoint((y-1)*nx+x-2);
                cells->InsertCellPoint(y*nx+x-2);
                cells->InsertCellPoint((y+1)*nx+x-2);
                cells->InsertCellPoint((y+1)*nx+x-1);
            }
        }
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

/// A two-dimensional triangle, used in MeshRD::GetPenroseTiling().
struct Tri {
    double p[3][2];  /// Coordinates of corners A, B and C.
    int index[3];    /// Index of corners A, B and C in the points structure.
    Tri(double ax,double ay,int iA,double bx,double by,int iB,double cx,double cy,int iC) {
        p[0][0] = ax; p[0][1] = ay; index[0] = iA;
        p[1][0] = bx; p[1][1] = by; index[1] = iB;
        p[2][0] = cx; p[2][1] = cy; index[2] = iC;
    }
};

typedef map<pair<int,int>,int> TPairIndex; /// For accessing an int by an ordered pair of ints.

/// Insert a new point between the points, unless one already exists.
int SplitEdge(const Tri &tri,int i1,int i2,double &x, double &y,TPairIndex &edge_splits,vtkPoints* pts)
{
    const double goldenRatio = (1.0 + sqrt(5.0)) / 2.0;

    x = tri.p[i1][0] + (tri.p[i2][0] - tri.p[i1][0]) / goldenRatio;
    y = tri.p[i1][1] + (tri.p[i2][1] - tri.p[i1][1]) / goldenRatio;
    // (x,y is closer to point 2 than point 1)

    pair<int,int> edge(tri.index[i1],tri.index[i2]);
    TPairIndex::const_iterator found = edge_splits.find(edge);
    if(found!=edge_splits.end())
    {
        return found->second;
    }
    else
    {
        int iP = pts->InsertNextPoint(x,y,0);
        edge_splits[edge] = iP;
        return iP;
    }
}

// workaround for LLVM/Clang issue: lld-link : error : undefined symbol: __powidf2
void MeshGenerators::GetPenroseTiling(/*int*/double n_subdivisions,int type,vtkUnstructuredGrid* mesh,int n_chems,int data_type)
{
    // Many thanks to Jeff Preshing: http://preshing.com/20110831/penrose-tiling-explained

    const int RHOMBI = 0;
    const int DARTS_AND_KITES = 1;

    // we keep a list of the 'red' and 'blue' Robinson triangles and use 'deflation' (decomposition)
    vector<Tri> red_tris[2],blue_tris[2]; // each list has two buffers
    int iCurrentBuffer = 0;

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();

    TPairIndex edge_splits; // given a pair of point indices, what is the index of the point made by splitting that edge?

    // start with 10 red triangles in a wheel, to get a nice circular shape (with 5-fold rotational symmetry)
    // (any correctly-tiled starting pattern will work too)
    const int NT = 10;
    const double angle_step = 2.0 * 3.1415926535 / NT;
    const double goldenRatio = (1.0 + sqrt(5.0)) / 2.0;
    const double scale = pow( goldenRatio, n_subdivisions );
    pts->InsertNextPoint(0,0,0);
    for(int i=0;i<NT;i++)
    {
        pts->InsertNextPoint(scale*cos(angle_step*i),scale*sin(angle_step*i),0);
        int i1 = (i + i%2) % NT;
        int i2 = (i + 1 - i%2) % NT;
        double angle1 = angle_step * i1;
        double angle2 = angle_step * i2;
        double p1[3] = { scale*cos(angle1), scale*sin(angle1), 0 };
        double p2[3] = { scale*cos(angle2), scale*sin(angle2), 0 };
        switch(type) {
            default:
            case RHOMBI:
                red_tris[iCurrentBuffer].push_back(Tri(0,0,0,p1[0],p1[1],1+i1,p2[0],p2[1],1+i2));
                break;
            case DARTS_AND_KITES:
                red_tris[iCurrentBuffer].push_back(Tri(p1[0],p1[1],1+i1,0,0,0,p2[0],p2[1],1+i2));
                break;
        }
    }

    // subdivide
    double px,py,qx,qy,rx,ry;
    for(int i=0;i<n_subdivisions;i++)
    {
        int iTargetBuffer = 1-iCurrentBuffer;
        red_tris[iTargetBuffer].clear();
        blue_tris[iTargetBuffer].clear();
        // subdivide the red triangles
        for(vector<Tri>::const_iterator it = red_tris[iCurrentBuffer].begin();it!=red_tris[iCurrentBuffer].end();it++)
        {
            switch(type)
            {
                default:
                case RHOMBI:
                {
                    int iP = SplitEdge(*it,0,1,px,py,edge_splits,pts); // split A and B to get a new point P
                    red_tris[iTargetBuffer].push_back(Tri(it->p[2][0],it->p[2][1],it->index[2],px,py,iP,it->p[1][0],it->p[1][1],it->index[1]));
                    blue_tris[iTargetBuffer].push_back(Tri(px,py,iP,it->p[2][0],it->p[2][1],it->index[2],it->p[0][0],it->p[0][1],it->index[0]));
                    break;
                }
                case DARTS_AND_KITES:
                {
                    int iQ = SplitEdge(*it,0,1,qx,qy,edge_splits,pts); // split A and B to get point Q
                    int iR = SplitEdge(*it,1,2,rx,ry,edge_splits,pts); // split B and C to get point R
                    blue_tris[iTargetBuffer].push_back(Tri(rx,ry,iR,qx,qy,iQ,it->p[1][0],it->p[1][1],it->index[1]));
                    red_tris[iTargetBuffer].push_back(Tri(qx,qy,iQ,it->p[0][0],it->p[0][1],it->index[0],rx,ry,iR));
                    red_tris[iTargetBuffer].push_back(Tri(it->p[2][0],it->p[2][1],it->index[2],it->p[0][0],it->p[0][1],it->index[0],rx,ry,iR));
                    break;
                }
            }
        }
        // subdivide the blue triangles
        for(vector<Tri>::const_iterator it = blue_tris[iCurrentBuffer].begin();it!=blue_tris[iCurrentBuffer].end();it++)
        {
            switch(type)
            {
                default:
                case RHOMBI:
                {
                    int iQ = SplitEdge(*it,1,0,qx,qy,edge_splits,pts); // split B and A to get point Q
                    int iR = SplitEdge(*it,1,2,rx,ry,edge_splits,pts); // split B and C to get point R
                    red_tris[iTargetBuffer].push_back(Tri(rx,ry,iR,qx,qy,iQ,it->p[0][0],it->p[0][1],it->index[0]));
                    blue_tris[iTargetBuffer].push_back(Tri(rx,ry,iR,it->p[2][0],it->p[2][1],it->index[2],it->p[0][0],it->p[0][1],it->index[0]));
                    blue_tris[iTargetBuffer].push_back(Tri(qx,qy,iQ,rx,ry,iR,it->p[1][0],it->p[1][1],it->index[1]));
                    break;
                }
                case DARTS_AND_KITES:
                {
                    int iP = SplitEdge(*it,2,0,px,py,edge_splits,pts); // split C and A to get point P
                    blue_tris[iTargetBuffer].push_back(Tri(it->p[1][0],it->p[1][1],it->index[1],px,py,iP,it->p[0][0],it->p[0][1],it->index[0]));
                    red_tris[iTargetBuffer].push_back(Tri(px,py,iP,it->p[2][0],it->p[2][1],it->index[2],it->p[1][0],it->p[1][1],it->index[1]));
                    break;
                }
            }
        }
        iCurrentBuffer = iTargetBuffer;
    }

    // merge triangles that have abutting open edges into quads
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    {
        vector<Tri> all_tris(red_tris[iCurrentBuffer]);
        all_tris.insert(all_tris.end(),blue_tris[iCurrentBuffer].begin(),blue_tris[iCurrentBuffer].end());
        TPairIndex half_quads; // for each open edge, what is the index of its triangle?
        TPairIndex::const_iterator found;
        for(int iTri = 0; iTri<(int)all_tris.size(); iTri++)
        {
            // is this the other half of a triangle we've seen previously?
            pair<int,int> edge(all_tris[iTri].index[1],all_tris[iTri].index[2]);
            found = half_quads.find(edge);
            if(found!=half_quads.end())
            {
                // output a quad (no need to store the triangle)
                cells->InsertNextCell(4);
                cells->InsertCellPoint(all_tris[iTri].index[0]);
                cells->InsertCellPoint(all_tris[iTri].index[1]);
                cells->InsertCellPoint(all_tris[found->second].index[0]);
                cells->InsertCellPoint(all_tris[iTri].index[2]);
            }
            else
            {
                // this triangle has not yet found its other half so store it for later
                half_quads[edge] = iTri;
            }
        }
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetRandomDelaunay2D(int n_points,vtkUnstructuredGrid *mesh,int n_chems,int data_type)
{
    // make a 2D mesh by delaunay triangulation on a point cloud
    float side = sqrt((float)n_points); // spread enough for <pixel> access
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetNumberOfPoints(n_points);
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    for(vtkIdType i=0;i<(vtkIdType)n_points;i++)
    {
        pts->SetPoint(i,vtkMath::Random()*side,vtkMath::Random()*side,0);
        cells->InsertNextCell(1);
        cells->InsertCellPoint(i);
    }
    vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
    poly->SetPoints(pts);
    poly->SetPolys(cells);
    vtkSmartPointer<vtkDelaunay2D> del = vtkSmartPointer<vtkDelaunay2D>::New();
    del->SetInputData(poly);
    del->Update();
    mesh->SetPoints(del->GetOutput()->GetPoints());
    mesh->SetCells(VTK_POLYGON,del->GetOutput()->GetPolys());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetRandomVoronoi2D(int n_points,vtkUnstructuredGrid *mesh,int n_chems,int data_type)
{
    // make a 2D mesh of voronoi cells from a point cloud

    vtkSmartPointer<vtkPolyData> old_poly = vtkSmartPointer<vtkPolyData>::New();
    double side = sqrt((double)n_points); // spread enough for <pixel> access
    // first make a delaunay triangular mesh
    {
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        pts->SetNumberOfPoints(n_points);
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        for(vtkIdType i=0;i<(vtkIdType)n_points;i++)
        {
            pts->SetPoint(i,vtkMath::Random()*side,vtkMath::Random()*side,0);
            cells->InsertNextCell(1);
            cells->InsertCellPoint(i);
        }
        old_poly->SetPoints(pts);
        old_poly->SetPolys(cells);
        vtkSmartPointer<vtkDelaunay2D> del = vtkSmartPointer<vtkDelaunay2D>::New();
        del->SetInputData(old_poly);
        del->Update();
        old_poly->DeepCopy(del->GetOutput());
        old_poly->BuildLinks();
    }

    // then make polygons from the circumcenters of the neighboring triangles of each vertex

    // points: the circumcenter of each tri
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetNumberOfPoints(old_poly->GetNumberOfCells());
    double center[3]={0,0,0},p1[3],p2[3],p3[3];
    vtkSmartPointer<vtkGenericCell> cell = vtkSmartPointer<vtkGenericCell>::New();
    for(vtkIdType i=0;i<old_poly->GetNumberOfCells();i++)
    {
        old_poly->GetCell(i,cell);
        vtkIdType pt1 = cell->GetPointId(0);
        vtkIdType pt2 = cell->GetPointId(1);
        vtkIdType pt3 = cell->GetPointId(2);
        old_poly->GetPoint(pt1,p1);
        old_poly->GetPoint(pt2,p2);
        old_poly->GetPoint(pt3,p3);
        vtkTriangle::Circumcircle(p1,p2,p3,center);
        pts->SetPoint(i,center);
    }
    // polys: join the circumcenters of each neighboring tri of each point (if >2)
    vtkSmartPointer<vtkCellArray> new_cells = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkIdList> cell_ids = vtkSmartPointer<vtkIdList>::New();
    for(vtkIdType i=0;i<old_poly->GetNumberOfPoints();i++)
    {
        old_poly->GetPointCells(i,cell_ids);
        if(cell_ids->GetNumberOfIds()<=2) continue;
        // collect the points
        vector<vtkIdType> pt_ids;
        {
            const int N_POINTS = cell_ids->GetNumberOfIds();
            int iCurrentCell = 0;
            vector<bool> seen(N_POINTS,false);
            pt_ids.push_back(cell_ids->GetId(iCurrentCell));
            seen[0]=true;
            vtkSmartPointer<vtkIdList> cell_pts = vtkSmartPointer<vtkIdList>::New();
            vtkSmartPointer<vtkIdList> cell_pts2 = vtkSmartPointer<vtkIdList>::New();
            for(vtkIdType j=1;j<N_POINTS;j++)
            {
                // find a cell in the list that is a neighbor of iCurrentCell and not yet seen
                for(vtkIdType k=0;k<N_POINTS;k++)
                {
                    if(seen[k]) continue;
                    old_poly->GetCellPoints(cell_ids->GetId(iCurrentCell),cell_pts);
                    old_poly->GetCellPoints(cell_ids->GetId(k),cell_pts2);
                    cell_pts->IntersectWith(cell_pts2);
                    if(cell_pts->GetNumberOfIds()==2) // (input mesh is only triangles)
                    {
                        pt_ids.push_back(cell_ids->GetId(k));
                        seen[k] = true;
                        iCurrentCell = k;
                        break;
                    }
                }
            }
        }
        // check if all the points are within the original area (don't want the external stretched ones)
        bool is_ok = true;
        for(vector<vtkIdType>::const_iterator it=pt_ids.begin();it!=pt_ids.end();it++)
        {
            double p[3];
            pts->GetPoint(*it,p);
            if(p[0]<0 || p[0]>side || p[1]<0 || p[1]>side)
            {
                is_ok = false;
                break;
            }
        }
        if(!is_ok) continue;
        // add the cell to the mesh
        new_cells->InsertNextCell((vtkIdType)pt_ids.size(),&pt_ids[0]);
    }

    // remove unused points (they affect the bounding box)
    vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
    poly->SetPoints(pts);
    poly->SetPolys(new_cells);
    vtkSmartPointer<vtkCleanPolyData> clean = vtkSmartPointer<vtkCleanPolyData>::New();
    clean->SetInputData(poly);
    clean->PointMergingOff();
    clean->Update();
    mesh->SetPoints(clean->GetOutput()->GetPoints());
    mesh->SetCells(VTK_POLYGON,clean->GetOutput()->GetPolys());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetRandomDelaunay3D(int n_points,vtkUnstructuredGrid *mesh,int n_chems,int data_type)
{
    // TODO: we could make any number of shapes here but we need a more general mechanism,
    // e.g. input a closed surface, scatter points inside, tetrahedralize

    // make a tetrahedral mesh by delaunay tetrahedralization on a point cloud
    vtkSmartPointer<vtkPointSource> pts = vtkSmartPointer<vtkPointSource>::New();
    pts->SetNumberOfPoints(n_points);
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Scale(200,100,100); // just to make it a bit more interesting we stretch the points in one direction
    vtkSmartPointer<vtkTransformPolyDataFilter> trans = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    trans->SetTransform(transform);
    trans->SetInputConnection(pts->GetOutputPort());
    vtkSmartPointer<vtkDelaunay3D> del = vtkSmartPointer<vtkDelaunay3D>::New();
    del->SetInputConnection(trans->GetOutputPort());
    del->Update();
    mesh->DeepCopy(del->GetOutput());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetBodyCentredCubicHoneycomb(int side,vtkUnstructuredGrid* mesh,int n_chems,int data_type)
{
    // a truncated octahedron
    const double coords[24][3] = {
        {0,1,2}, {0,-1,2}, {0,1,-2}, {0,-1,-2},   // 0,1,2,3
        {0,2,1}, {0,-2,1}, {0,2,-1}, {0,-2,-1},   // 4,5,6,7
        {1,0,2}, {-1,0,2}, {1,0,-2}, {-1,0,-2},   // 8,9,10,11
        {1,2,0}, {-1,2,0}, {1,-2,0}, {-1,-2,0},   // 12,13,14,15
        {2,0,1}, {-2,0,1}, {2,0,-1}, {-2,0,-1},   // 16,17,18,19
        {2,1,0}, {-2,1,0}, {2,-1,0}, {-2,-1,0} }; // 20,21,22,23
    const int hex_faces[8][6] = {
        {0,8,16,20,12,4}, {2,6,12,20,18,10},
        {1,5,14,22,16,8}, {3,10,18,22,14,7},
        {0,4,13,21,17,9}, {2,11,19,21,13,6},
        {1,9,17,23,15,5}, {3,7,15,23,19,11} }; // xyz positive or negative: +++, ++-, +-+, +--, -++, -+-, --+, ---
    const int square_faces[6][4] = {
        {16,22,18,20}, {17,21,19,23}, {4,12,6,13},
        {5,15,7,14}, {0,9,1,8}, {2,10,3,11} }; // x=2, x=-2, y=2, y=-2, z=2, z=-2

    // stack them in a grid, merging duplicated vertices
    vtkSmartPointer<vtkAppendFilter> append = vtkSmartPointer<vtkAppendFilter>::New();
    append->MergePointsOn();
    for(int z=0;z<side*2-1;z++)
    {
        for(int y=0;y<side-z%2;y++)
        {
            for(int x=0;x<side-z%2;x++)
            {
                vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
                vector<vtkIdType> pointIds,faceStream;
                vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
                for(int i=0;i<24;i++)
                    pointIds.push_back(points->InsertNextPoint( coords[i][0]+x*4+(z%2)*2,
                        coords[i][1]+y*4+(z%2)*2, coords[i][2]+z*2)); // body-centred
                for(int i=0;i<8;i++) {
                    faceStream.push_back(6);
                    for(int j=0;j<6;j++)
                        faceStream.push_back(hex_faces[i][j]);
                }
                for(int i=0;i<6;i++) {
                    faceStream.push_back(4);
                    for(int j=0;j<4;j++)
                        faceStream.push_back(square_faces[i][j]);
                }
                ug->InsertNextCell(VTK_POLYHEDRON,24,&pointIds.front(),14,&faceStream.front());
                ug->SetPoints(points);
                append->AddInputData(ug);
            }
        }
    }
    append->Update();
    mesh->DeepCopy(append->GetOutput());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetFaceCentredCubicHoneycomb(int side,vtkUnstructuredGrid* mesh,int n_chems,int data_type)
{
    // a rhombic dodecahedron
    const double coords[14][3] = {
        {-2,0,0},{2,0,0},{0,-2,0},{0,2,0},{0,0,-2},{0,0,2}, // 0,1,2,3,4,5,
        {-1,-1,-1},{-1,-1,1},{-1,1,-1},{-1,1,1}, // 6,7,8,9
        {1,-1,-1},{1,-1,1},{1,1,-1},{1,1,1} // 10,11,12,13
    };
    const int faces[12][4] = {
        {0,7,5,9},{9,5,13,3},{13,5,11,1},{5,7,2,11},
        {0,6,2,7},{0,9,3,8},{0,8,4,6},{2,6,4,10},
        {3,12,4,8},{1,10,4,12},{1,11,2,10},{1,12,3,13}
    };

    // stack them in a grid, merging duplicated vertices
    vtkSmartPointer<vtkAppendFilter> append = vtkSmartPointer<vtkAppendFilter>::New();
    append->MergePointsOn();
    for(int z=0;z<side*2;z++)
    {
        for(int y=0;y<side;y++)
        {
            for(int x=0;x<side*2;x++)
            {
                vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
                vector<vtkIdType> pointIds,faceStream;
                vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
                for(int i=0;i<14;i++)
                    pointIds.push_back(points->InsertNextPoint( coords[i][0]+x*2+(z%2)*2,
                        coords[i][1]+y*4+(x%2)*2, coords[i][2]+z*2)); // face-centred
                for(int i=0;i<12;i++) {
                    faceStream.push_back(4);
                    for(int j=0;j<4;j++)
                        faceStream.push_back(faces[i][j]);
                }
                ug->InsertNextCell(VTK_POLYHEDRON,14,&pointIds.front(),12,&faceStream.front());
                ug->SetPoints(points);
                append->AddInputData(ug);
            }
        }
    }
    append->Update();
    mesh->DeepCopy(append->GetOutput());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void MeshGenerators::GetDiamondCells(int side,vtkUnstructuredGrid *mesh,int n_chems,int data_type)
{
    // a triakis truncated tetrahedron: 16 points, 4 hexagonal faces, 12 triangular faces
    const double RR2 = 1.0 / sqrt(2.0); // reciprocal of root 2
    const double third = 1.0 / 3.0;
    const double coords[16][3] = {
        //{-1,0,-RR2}, {1,0,-RR2}, {0,-1,RR2}, {0,1,RR2},          // (tips of the original tetrahedron)
        {-2*third,0,-2*third*RR2}, {2*third,0,-2*third*RR2},       // 0, 1
        {0,-2*third,2*third*RR2}, {0,2*third,2*third*RR2},         // 2, 3 (centroids of the snipped-off tetrahedra)
        {-third,0,-RR2}, {third,0,-RR2},                           // 4, 5
        {0,-third,RR2}, {0,third,RR2},                             // 6, 7
        {2*third,-third,-third*RR2},  {third,-2*third,third*RR2},  // 8, 9
        {2*third,third,-third*RR2},   {third,2*third,third*RR2},   // 10, 11
        {-2*third,-third,-third*RR2}, {-third,-2*third,third*RR2}, // 12, 13
        {-2*third,third,-third*RR2},  {-third,2*third,third*RR2}   // 14, 15
    };
    const int hex_faces[4][6] = { {4,5,8,9,13,12}, {5,4,14,15,11,10}, {6,7,15,14,12,13}, {7,6,9,8,10,11} };
    const int tri_faces[12][3] = { {0,12,14},{0,14,4},{0,4,12}, {1,10,8},{1,8,5},{1,5,10},
        {2,9,6},{2,6,13},{2,13,9}, {3,7,11},{3,11,15},{3,15,7} };

    // stack them in a grid, merging duplicated vertices
    vtkSmartPointer<vtkAppendFilter> append = vtkSmartPointer<vtkAppendFilter>::New();
    append->MergePointsOn();

    for(int x=0;x<side;x++)
    {
        for(int y=0;y<side;y++)
        {
            for(int z=0;z<side*2;z++)
            {
                vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
                vector<vtkIdType> pointIds,faceStream;
                vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
                for(int i=0;i<16;i++)
                {
                    double offset[3] = { x*(1+third), y*(1+third), (z/2)*RR2*(1+third) + (z%2)*2*third*RR2 };
                    double mx=0.0,my=0.0;
                    if((z/2)%2)
                    {
                        mx = 2*third;
                        my = -2*third;
                    }
                    if(z%2)
                        pointIds.push_back(points->InsertNextPoint( coords[i][0] + offset[0] + mx, coords[i][1] + offset[1] + my, coords[i][2] + offset[2] ));
                    else
                        pointIds.push_back(points->InsertNextPoint( -coords[i][1] + offset[1] + mx, coords[i][0] + offset[0] + 2*third + my, coords[i][2] + offset[2] ));
                }
                for(int i=0;i<4;i++)
                {
                    faceStream.push_back(6);
                    for(int j=0;j<6;j++)
                        faceStream.push_back(hex_faces[i][j]);
                }
                for(int i=0;i<12;i++)
                {
                    faceStream.push_back(3);
                    for(int j=0;j<3;j++)
                        faceStream.push_back(tri_faces[i][j]);
                }
                ug->InsertNextCell(VTK_POLYHEDRON,16,&pointIds.front(),16,&faceStream.front());
                ug->SetPoints(points);
                append->AddInputData(ug);
            }
        }
    }

    append->Update();
    mesh->DeepCopy(append->GetOutput());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

void sphereInversion( const double p[3], const vector<double>& q, const double r, double p_out[3] )
{
    //  reflect p in the sphere radius r center q
    const double r2 = r*r;
    const double pq2 = pow(p[0]-q[0], 2) + pow(p[1]-q[1], 2) + pow(p[2]-q[2], 2);
    const double f = r2 / pq2;
    p_out[0] = q[0] + f * ( p[0] - q[0] );
    p_out[1] = q[1] + f * ( p[1] - q[1] );
    p_out[2] = q[2] + f * ( p[2] - q[2] );
}

// ---------------------------------------------------------------------

double GetPolygonRadius( double edge_length, int num_sides ) {
    return 0.5 * edge_length / cos( M_PI * ( 0.5 - 1.0 / num_sides ) );
}

// ---------------------------------------------------------------------

void GetInversionCircleForPlaneTiling( double edge_length, int schlafli1, int schlafli2, double& R, double& d ) {
    // return the radius R and distance d from the polygon center of the inversion circle for the desired tiling
    double A = M_PI * ( 0.5 - 1.0 / schlafli1 ); // half corner angle if polygon was in Euclidean space
    double C = M_PI / schlafli2; // half corner angle required to attain desired tiling
    double B = A - C; // angle defect
    double v = 0.5 * edge_length;
    R = v / sin( B );
    d = v * tan( A ) + v / tan( B );
}

// ---------------------------------------------------------------------

void MeshGenerators::GetHyperbolicPlaneTiling(int schlafli1,int schlafli2,int num_levels,vtkUnstructuredGrid *mesh,int n_chems,int data_type)
{
    // define the central cell
    const double edge_length = 1.0;
    const int num_vertices = schlafli1;
    vector<vector<double> > vertex_coords(num_vertices,vector<double>(3));
    vector<int> faces(num_vertices);
    double r1 = GetPolygonRadius( edge_length, schlafli1 );
    for( int i = 0; i < num_vertices; ++i )
    {
        double angle = ( i + 0.5 ) * 2.0 * M_PI / schlafli1;
        vertex_coords[i][0] = r1 * cos( angle );
        vertex_coords[i][1] = r1 * sin( angle );
        vertex_coords[i][2] = 0.0;
        faces[i] = i;
    }

    // define the mirror spheres
    const int num_spheres = num_vertices;
    double R = 0.0;
    double d = 0.0;
    GetInversionCircleForPlaneTiling( edge_length, schlafli1, schlafli2, R, d );
    vector<vector<double> > sphere_centers(num_spheres,vector<double>(3));
    double n[3];
    for( int i = 0; i < num_vertices; ++i ) {
        for( int xyz = 0; xyz < 3; ++xyz )
            n[xyz] = ( vertex_coords[i][xyz] + vertex_coords[(i+1)%num_vertices][xyz] ) / 2.0;
        double nl = sqrt( n[0]*n[0] + n[1]*n[1] + n[2]*n[2] );
        sphere_centers[i][0] = n[0] * d / nl;
        sphere_centers[i][1] = n[1] * d / nl;
        sphere_centers[i][2] = n[2] * d / nl;
    }

    // make a list of lists of sphere ids to use
    vector< vector< int > > sphere_lists;
    sphere_lists.push_back( vector<int>() );
    size_t iList = 0;
    for( int iLevel = 0; iLevel < num_levels; ++iLevel ) {
        const size_t num_lists = sphere_lists.size();
        for( ; iList < num_lists; ++iList ) {
            for( int iExtraSphere = 0; iExtraSphere < num_spheres; ++iExtraSphere ) {
                vector<int> extended_list( sphere_lists[iList] );
                extended_list.push_back( iExtraSphere );
                sphere_lists.push_back( extended_list );
            }
        }
    }

    vtkSmartPointer<vtkAppendFilter> append = vtkSmartPointer<vtkAppendFilter>::New();
    append->MergePointsOn();

    vtkSmartPointer<vtkPointLocator> point_locator = vtkSmartPointer<vtkPointLocator>::New();
    vtkSmartPointer<vtkPoints> locator_points = vtkSmartPointer<vtkPoints>::New();
    double bounds[6] = {-10,10,-10,10,-10,10};
    point_locator->InitPointInsertion(locator_points,bounds);

    for( size_t iSphereList = 0; iSphereList < sphere_lists.size(); ++iSphereList )
    {
        vector<int>& sphere_list = sphere_lists[iSphereList];
        // make a cell by reflecting the starting cell in the order listed
        vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        vector<vtkIdType> pointIds;
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        double centroid[3] = {0,0,0};
        for(int iV = 0; iV < num_vertices; ++iV ) {
            double p[3] = { vertex_coords[iV][0], vertex_coords[iV][1], vertex_coords[iV][2] };
            for( size_t iSphereEntry = 0; iSphereEntry < sphere_list.size(); ++iSphereEntry )
            {
                int iSphere = sphere_list[iSphereEntry];
                sphereInversion( p, sphere_centers[iSphere], R, p );
            }
            pointIds.push_back( points->InsertNextPoint( p ) );
            centroid[0]+=p[0];
            centroid[1]+=p[1];
            centroid[2]+=p[2];
        }
        // only add this cell if we haven't seen this centroid before
        centroid[0] /= num_vertices;
        centroid[1] /= num_vertices;
        centroid[2] /= num_vertices;
        if( point_locator->IsInsertedPoint( centroid ) < 0 )
        {
            vector<vtkIdType> faceStream;
            faceStream.push_back( num_vertices );
            for( int j = 0; j < num_vertices; ++j )
                faceStream.push_back( faces[j] );
            ug->InsertNextCell(VTK_POLYGON,num_vertices,&pointIds.front(),1,&faceStream.front());
            ug->SetPoints(points);
            append->AddInputData(ug);
            point_locator->InsertNextPoint( centroid );
        }
    }

    append->Update();
    mesh->DeepCopy(append->GetOutput());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }

}

// ---------------------------------------------------------------------

double GetPolyhedronRadius( double edge_length, int schlafli1, int schlafli2 ) {
    if( schlafli1 == 4 && schlafli2 == 3 ) {
        return edge_length * sqrt( 3.0 ) / 2.0; // cube
    }
    else if( schlafli1 == 5 && schlafli2 == 3 ) {
        return edge_length * sqrt( 3.0 ) * ( 1.0 + sqrt( 5.0 ) ) / 4.0; // dodecahedron
    }
    else if( schlafli1 == 3 && schlafli2 == 5 ) {
        return edge_length * sqrt( ( 5.0 + sqrt( 5.0 ) ) / 8.0 ); // icosahedron
    }
    else throw runtime_error( "GetPolyhedronRadius : unsupported polyhedron" );
}

// ---------------------------------------------------------------------

void GetInversionSphereForSpaceTessellation( double edge_length, int schlafli1, int schlafli2, int schlafli3, double& R, double& d ) {
    // return the radius R and distance d from the polyhedron center of the inversion sphere for the desired tessellation
    double v = edge_length / 2.0; // distance from center of edge to rotation point
    double r1 = GetPolygonRadius( edge_length, schlafli1 );
    double r2 = GetPolyhedronRadius( edge_length, schlafli1, schlafli2 );
    double m1 = sqrt( r1*r1 - v*v ); // distance from polygon center to edge midpoint
    double m2 = sqrt( r2*r2 - v*v ); // distance from polyhedron center to edge midpoint
    double h = sqrt( m2*m2 - m1*m1 ); // height of the polyhedron center above the polygon
    double k = ( m2 / m1 ) * cos( M_PI / schlafli3 );
    R = ( sqrt( h*h - k*k*r1*r1 + r1*r1 ) + h * k ) / ( k*k - 1.0 );
    d = h + sqrt( R*R - r1*r1 );
}

void MeshGenerators::GetHyperbolicSpaceTessellation(int schlafli1,int schlafli2,int schlafli3,int num_levels,vtkUnstructuredGrid *mesh,int n_chems,int data_type)
{
    // implemented with help from Adam P. Goucher - many thanks!

    int BasePolyhedronType;
    if( schlafli1 == 4 && schlafli2 == 3 ) BasePolyhedronType = VTK_SOLID_CUBE;
    else if( schlafli1 == 5 && schlafli2 == 3 ) BasePolyhedronType = VTK_SOLID_DODECAHEDRON;
    else if( schlafli1 == 3 && schlafli2 == 5 ) BasePolyhedronType = VTK_SOLID_ICOSAHEDRON;
    else throw( runtime_error( "MeshGenerators::GetHyperbolicSpaceTessellation : unsupported tessellation" ) );

    const double edge_length = 1.0;

    // TODO: subdivide into cubes? (apart from {3,5,3})

    // define the central cell
    int num_vertices;
    int num_faces;
    int num_vertices_per_face;
    vector<vector<double> > vertex_coords;
    vector<vector<int> > faces;
    vector<vector<double> > sphere_centers;
    {
        vtkSmartPointer<vtkPlatonicSolidSource> source = vtkSmartPointer<vtkPlatonicSolidSource>::New();
        source->SetSolidType( BasePolyhedronType );
        const double polyhedron_scale = GetPolyhedronRadius( edge_length, schlafli1, schlafli2 );
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        transform->Scale( polyhedron_scale, polyhedron_scale, polyhedron_scale ); // scale so that edge length is as desired
        vtkSmartPointer<vtkTransformPolyDataFilter> trans = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        trans->SetTransform(transform);
        trans->SetInputConnection(source->GetOutputPort());
        trans->Update();
        num_vertices = trans->GetOutput()->GetNumberOfPoints();
        for( int i = 0; i < num_vertices; ++i ) {
            double *p = trans->GetOutput()->GetPoint( i );
            vertex_coords.push_back( vector<double>( p, p+3 ) );
        }
        num_faces = trans->GetOutput()->GetNumberOfCells();
        trans->GetOutput()->BuildCells();
        for( vtkIdType i = 0; i < num_faces; ++i ) {
            vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
            trans->GetOutput()->GetCellPoints(i,ids);
            num_vertices_per_face = ids->GetNumberOfIds();
            vector<int> vids;
            for( vtkIdType j = 0; j < ids->GetNumberOfIds(); ++j )
                vids.push_back( ids->GetId(j) );
            faces.push_back( vids );
        }
    }

    // work out how big the inversion spheres need to be to get the desired tiling
    double R,d;
    GetInversionSphereForSpaceTessellation(edge_length,schlafli1,schlafli2,schlafli3,R,d);

    // the inversion spheres lie on the lines that pass through the center of each face
    for( int i = 0; i < num_faces; ++i ) {
        double n[3] = {0,0,0};
        for( int j = 0; j < num_vertices_per_face; ++j ) {
            for( int xyz = 0; xyz < 3; ++xyz )
                n[xyz] += vertex_coords[ faces[i][j] ][ xyz ];
        }
        const double nl = sqrt( n[0]*n[0] + n[1]*n[1] + n[2]*n[2] );
        for( int xyz = 0; xyz < 3; ++xyz )
            n[xyz] *= d / nl;
        sphere_centers.push_back( vector<double>( n, n+3 ) );
    }

    // make a list of lists of sphere ids to use
    vector< vector< int > > sphere_lists;
    sphere_lists.push_back( vector<int>() );
    size_t iList = 0;
    for( int iLevel = 0; iLevel < num_levels; ++iLevel ) {
        const size_t num_lists = sphere_lists.size();
        for( ; iList < num_lists; ++iList ) {
            for( int iExtraSphere = 0; iExtraSphere < num_faces; ++iExtraSphere ) {
                vector<int> extended_list( sphere_lists[iList] );
                extended_list.push_back( iExtraSphere );
                sphere_lists.push_back( extended_list );
            }
        }
    }

    vtkSmartPointer<vtkAppendFilter> append = vtkSmartPointer<vtkAppendFilter>::New();
    append->MergePointsOn();

    vtkSmartPointer<vtkPointLocator> point_locator = vtkSmartPointer<vtkPointLocator>::New();
    vtkSmartPointer<vtkPoints> locator_points = vtkSmartPointer<vtkPoints>::New();
    double bounds[6] = {-10,10,-10,10,-10,10};
    point_locator->InitPointInsertion(locator_points,bounds);

    for( size_t iSphereList = 0; iSphereList < sphere_lists.size(); ++iSphereList )
    {
        vector<int>& sphere_list = sphere_lists[iSphereList];
        // make a cell by reflecting the starting cell in the order listed
        vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        vector<vtkIdType> pointIds;
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        double centroid[3] = {0,0,0};
        for(int iV = 0; iV < num_vertices; ++iV ) {
            double p[3] = { vertex_coords[iV][0], vertex_coords[iV][1], vertex_coords[iV][2] };
            for( size_t iSphereEntry = 0; iSphereEntry < sphere_list.size(); ++iSphereEntry )
            {
                int iSphere = sphere_list[iSphereEntry];
                sphereInversion( p, sphere_centers[iSphere], R, p );
            }
            pointIds.push_back( points->InsertNextPoint( p ) );
            centroid[0]+=p[0];
            centroid[1]+=p[1];
            centroid[2]+=p[2];
        }
        // only add this cell if we haven't seen this centroid before
        centroid[0] /= num_vertices;
        centroid[1] /= num_vertices;
        centroid[2] /= num_vertices;
        if( point_locator->IsInsertedPoint( centroid ) < 0 )
        {
            vector<vtkIdType> faceStream;
            for(int iF = 0; iF < num_faces; ++iF )
            {
                faceStream.push_back( num_vertices_per_face );
                for(int j = 0; j < num_vertices_per_face; ++j )
                    faceStream.push_back( faces[iF][j] );
            }
            ug->InsertNextCell(VTK_POLYHEDRON,num_vertices,&pointIds.front(),num_faces,&faceStream.front());
            ug->SetPoints(points);
            append->AddInputData(ug);
            point_locator->InsertNextPoint( centroid );
        }
    }

    append->Update();
    mesh->DeepCopy(append->GetOutput());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkDataArray> scalars = vtkSmartPointer<vtkDataArray>::Take( vtkDataArray::CreateDataArray( data_type ) );
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------
