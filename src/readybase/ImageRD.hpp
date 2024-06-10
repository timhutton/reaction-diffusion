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

#ifndef __IMAGERD__
#define __IMAGERD__

// local:
#include "AbstractRD.hpp"

// VTK:
class vtkImageData;
class vtkAssignAttribute;
class vtkRearrangeFields;
class vtkUnstructuredGrid;

/// Base class for image-based systems.
class ImageRD : public AbstractRD
{
    public:

        ImageRD(int data_type);
        ~ImageRD();

        void SaveFile(const char* filename,
            const Properties& render_settings,
            bool generate_initial_pattern_when_loading) const override;

        void Update(int n_steps) override;

        bool HasEditableDimensions() const  override { return true; }
        float GetX() const override;
        float GetY() const override;
        float GetZ() const override;
        void SetDimensions(int x,int y,int z) override;
        void SetDimensionsAndNumberOfChemicals(int x,int y,int z,int nc);

        int GetNumberOfCells() const override;

        void SetNumberOfChemicals(int n, bool reallocate_storage = false) override;

        void GenerateInitialPattern() override;
        void BlankImage(float value = 0.0f) override;
        void GetImage(vtkImageData* im) const;
        virtual void CopyFromImage(vtkImageData* im);
        virtual void CopyFromMesh(
            vtkUnstructuredGrid* mesh,
            const int num_chemicals,
            const size_t target_chemical,
            const size_t largest_dimension,
            const float value_inside,
            const float value_outside);
        void SaveStartingPattern() override;
        void RestoreStartingPattern() override;

        void InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings) override;

        std::string GetFileExtension() const override { return ImageRD::GetFileExtensionStatic(); }
        static std::string GetFileExtensionStatic() { return "vti"; }

        void GetAsMesh(vtkPolyData *out,const Properties& render_settings) const override;
        void GetAs2DImage(vtkImageData *out,const Properties& render_settings) const override;
        void SetFrom2DImage(int iChemical, vtkImageData *im) override;
        bool Is2DImageAvailable() const override { return true; }

        float GetValue(float x,float y,float z,const Properties& render_settings) override;
        void SetValue(float x,float y,float z,float val,const Properties& render_settings) override;
        void SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings) override;

        size_t GetMemorySize() const override;

        std::vector<float> GetData(int i_chemical) const override;

    protected:

        std::vector<vtkSmartPointer<vtkImageData>> images; ///< one for each chemical

        // we save the starting pattern, to allow the user to reset
        vtkSmartPointer<vtkImageData> starting_pattern;

        double image_top1D;        /// topmost location of the 1D image strips
        double image_ratio1D;     /// proportions of the 1D image strips

    protected:

        vtkImageData* GetImage(int iChemical) const;

        void AddPhasePlot(vtkRenderer* pRenderer,float scaling,float low,float high,float posX,float posY,float posZ,
                            int iChemX,int iChemY,int iChemZ) override;

        /// use to change the dimensions or the number of chemicals
        virtual void AllocateImages(int x,int y,int z,int nc,int data_type);

        void DeallocateImages();

        static vtkSmartPointer<vtkImageData> AllocateVTKImage(int x,int y,int z,int data_type);

        int GetArenaDimensionality() const override;

        void FlipPaintAction(PaintAction& cca) override;

        // some saved handles into the pipeline, for manual updates to workaround a named arrays problem
        vtkAssignAttribute *assign_attribute_filter;
        vtkRearrangeFields *rearrange_fields_filter;

    private:

        void InitializeVTKPipeline_1D(vtkRenderer* pRenderer,const Properties& render_settings);
        void InitializeVTKPipeline_2D(vtkRenderer* pRenderer,const Properties& render_settings);
        void InitializeVTKPipeline_3D(vtkRenderer* pRenderer,const Properties& render_settings);

    private: // deliberately not implemented, to prevent use

        ImageRD(ImageRD&);
        ImageRD& operator=(ImageRD&);
};

#endif
