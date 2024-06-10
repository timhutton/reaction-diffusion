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

#ifndef __MESHRD__
#define __MESHRD__

// local:
#include "AbstractRD.hpp"

// VTK:
#include <vtkType.h>
class vtkUnstructuredGrid;
class vtkCellLocator;

/// Base class for mesh-based systems.
class MeshRD : public AbstractRD
{
    public:

        MeshRD(int data_type);

        void SaveFile(const char* filename,
            const Properties& render_settings,
            bool generate_initial_pattern_when_loading) const override;

        void Update(int n_steps) override;

        float GetX() const override;
        float GetY() const override;
        float GetZ() const override;

        void SetNumberOfChemicals(int n, bool reallocate_storage = false) override;

        bool HasEditableFormula() const override { return true; }

        std::string GetFileExtension() const override { return MeshRD::GetFileExtensionStatic(); }
        static std::string GetFileExtensionStatic() { return "vtu"; }

        int GetNumberOfCells() const override;

        void GenerateInitialPattern() override;
        void BlankImage(float value = 0.0f) override;
        virtual void CopyFromMesh(vtkUnstructuredGrid* mesh2);
        void SaveStartingPattern() override;
        void RestoreStartingPattern() override;

        void InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings) override;

        void GetAsMesh(vtkPolyData *out,const Properties& render_settings) const override;
        void GetAs2DImage(vtkImageData *out,const Properties& render_settings) const override;
        void SetFrom2DImage(int iChemical, vtkImageData *im) override;
        bool Is2DImageAvailable() const override { return false; }

        int GetArenaDimensionality() const override;

        float GetValue(float x,float y,float z,const Properties& render_settings) override;
        void SetValue(float x,float y,float z,float val,const Properties& render_settings) override;
        void SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings) override;

        void GetMesh(vtkUnstructuredGrid* mesh) const;

        size_t GetMemorySize() const override;

        std::vector<float> GetData(int i_chemical) const override;

    protected: // functions

        void AddPhasePlot(  vtkRenderer* pRenderer,float scaling,float low,float high,float posX,float posY,float posZ,
                            int iChemX,int iChemY,int iChemZ) override;

        /// work out which cells are neighbors of each other
        void ComputeCellNeighbors(TNeighborhood neighborhood_type);

        void CreateCellLocatorIfNeeded();

        void FlipPaintAction(PaintAction& cca) override;

    protected: // variables

        vtkSmartPointer<vtkUnstructuredGrid> mesh;             ///< the cell data contains a named array for each chemical ('a', 'b', etc.)
        vtkSmartPointer<vtkUnstructuredGrid> starting_pattern; ///< we save the starting pattern, to allow the user to reset

        int max_neighbors;
        std::vector<int> cell_neighbor_indices;   ///< index of each neighbor of a cell
        std::vector<float> cell_neighbor_weights; ///< diffusion coefficient between each cell and a neighbor

        vtkSmartPointer<vtkCellLocator> cell_locator; ///< Returns a cell ID when given a 3D location

    private: // deliberately not implemented, to prevent use

        MeshRD(MeshRD&);
        MeshRD& operator=(MeshRD&);
};

#endif
