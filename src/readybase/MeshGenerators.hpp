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
class vtkUnstructuredGrid;

/// Methods for generating meshes from scratch.
namespace MeshGenerators 
{
    /// Subdivides an icosahedron to get a sphere evenly covered with triangles.
    void GetGeodesicSphere(int n_subdivisions,vtkUnstructuredGrid* mesh,int n_chems,int data_type);

    /// Subdivides a torus with quadrilaterals.
    void GetTorus(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems,int data_type);

    /// Makes a planar mesh of triangles.
    void GetTriangularMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems,int data_type);

    /// Makes a planar mesh of hexagons.
    void GetHexagonalMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems,int data_type);

    /// Makes a planar mesh using the rhombille tiling.
    void GetRhombilleTiling(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems,int data_type);

    /// Make a planar Penrose tiling, using either rhombi (type=0) or darts and kites (type=1).
    void GetPenroseTiling(/*int*/double n_subdivisions,int type,vtkUnstructuredGrid* mesh,int n_chems,int data_type);
    // (workaround for LLVM/Clang issue: lld-link : error : undefined symbol: __powidf2)

    /// Make a 2D Delaunay triangulation from a random set of points
    void GetRandomDelaunay2D(int n_points,vtkUnstructuredGrid *mesh,int n_chems,int data_type);

    /// Make a 2D Voronoi mesh from a random set of points
    void GetRandomVoronoi2D(int n_points,vtkUnstructuredGrid *mesh,int n_chems,int data_type);

    /// Applies the Delaunay algorithm to scattered points to get a mesh of tetrahedra.
    void GetRandomDelaunay3D(int n_points,vtkUnstructuredGrid* mesh,int n_chems,int data_type);

    /// Make a honeycomb from truncated octahedra.
    void GetBodyCentredCubicHoneycomb(int side,vtkUnstructuredGrid* mesh,int n_chems,int data_type);

    /// Make a honeycomb from rhombic dodecahedra.
    void GetFaceCentredCubicHoneycomb(int side,vtkUnstructuredGrid* mesh,int n_chems,int data_type);

    /// Make triakis truncated tetrahedra - the Voronoi cells of the carbon atoms in a diamond lattice.
    void GetDiamondCells(int side,vtkUnstructuredGrid *mesh,int n_chems,int data_type);

    // Make a hyperbolic plane tiling such as {3,7} or {4,5} at the specified recursion level
    void GetHyperbolicPlaneTiling(int schlafli1,int schlafli2,int num_levels,vtkUnstructuredGrid *mesh,int n_chems,int data_type);

    // Make a hyperbolic space tessellation such as {4,3,5} at the specified recursion level
    void GetHyperbolicSpaceTessellation(int schlafli1,int schlafli2,int schlafli3,int num_levels,vtkUnstructuredGrid *mesh,int n_chems,int data_type);
}
