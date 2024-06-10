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

#ifndef __ABSTRACTRD__
#define __ABSTRACTRD__

// local:
#include "InitialPatternGenerator.hpp"
class Overlay;
class Properties;

// VTK:
#include <vtkSmartPointer.h>
class vtkXMLDataElement;
class vtkRenderer;
class vtkPolyData;
class vtkImageData;

// STL:
#include <string>
#include <vector>
#include <map>

/// Abstract base class for all reaction-diffusion systems.
class AbstractRD
{
    public:

        AbstractRD(int data_type);
        virtual ~AbstractRD();

        /// Load this pattern from an RD element.
        virtual void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update);
        /// Retrieve an RD element for this pattern, suitable for saving to file.
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML(bool generate_initial_pattern_when_loading) const;

        /// Called to progress the simulation by N steps.
        virtual void Update(int n_steps) =0;

        /// Some implementations (e.g. inbuilt ones) cannot have their number_of_chemicals edited.
        virtual bool HasEditableNumberOfChemicals() const { return true; }
        int GetNumberOfChemicals() const { return this->n_chemicals; }
        virtual void SetNumberOfChemicals(int n, bool reallocate_storage = false) =0;

        /// How many timesteps have we advanced since being initialized?
        int GetTimestepsTaken() const { return this->timesteps_taken; }

        /// The formula is a piece of code (currently either an OpenCL snippet or a full OpenCL kernel) that drives the system.
        std::string GetFormula() const { return this->formula; }
        /// Throws std::runtime_error with information if the formula doesn't work.
        virtual void TestFormula(std::string /*program_string*/) {}
        /// Changes the system's formula. The kernel will be reloaded on the next update step.
        void SetFormula(std::string s);
        /// Some implementations (e.g. inbuilt ones) cannot have their formula edited.
        virtual bool HasEditableFormula() const =0;
        /// Return the full OpenCL kernel (if available, else the empty string).
        virtual std::string GetKernel() const { return ""; }

        /// Returns e.g. "inbuilt", "formula", "kernel", as in the XML.
        virtual std::string GetRuleType() const =0;
        virtual std::string GetFileExtension() const =0;

        std::string GetRuleName() const { return this->rule_name; }
        void SetRuleName(std::string s);

        std::string GetDescription() const { return this->description; }
        void SetDescription(std::string s);

        virtual int GetNumberOfCells() const =0;

        // most implementations have parameters that can be edited and changed
        // (will cause errors if they don't match the inbuilt names, the formula or the kernel)
        int GetNumberOfParameters() const;
        std::string GetParameterName(int iParam) const;
        float GetParameterValue(int iParam) const;
        float GetParameterValueByName(const std::string& name) const;
        bool IsParameter(const std::string& name) const;
        virtual void AddParameter(const std::string& name,float val);
        virtual void DeleteParameter(int iParam);
        virtual void DeleteAllParameters();
        virtual void SetParameterName(int iParam,const std::string& s);
        virtual void SetParameterValue(int iParam,float val);

        /// Should the user be asked if they want to save this pattern?
        bool IsModified() const { return this->is_modified; }
        void SetModified(bool m);

        virtual void SaveFile(const char* filename,const Properties& render_settings,
            bool generate_initial_pattern_when_loading) const =0;
        std::string GetFilename() const { return this->filename; }
        void SetFilename(const std::string& s);

        virtual void GenerateInitialPattern() =0;
        virtual void BlankImage(float value = 0.0f) =0;

        /// Create a generator suitable for Gray-Scott, so that new patterns can start working immediately.
        void CreateDefaultInitialPatternGenerator(size_t num_chemicals);

        virtual void InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings) =0;
        virtual void SaveStartingPattern() =0;
        virtual void RestoreStartingPattern() =0;

        virtual bool HasEditableDimensions() const { return false; }
        virtual float GetX() const =0;
        virtual float GetY() const =0;
        virtual float GetZ() const =0;
        virtual void SetDimensions(int /*x*/,int /*y*/,int /*z*/) {}

        /// Only some implementations (e.g. FullKernelOpenCLImageRD) can have their block size edited.
        virtual bool HasEditableBlockSize() const { return false; }
        virtual int GetBlockSizeX() const { return 1; } ///< e.g. block size may be 4x1x1 for kernels that use float4 (like FormulaOpenCLImageRD)
        virtual int GetBlockSizeY() const { return 1; }
        virtual int GetBlockSizeZ() const { return 1; }
        virtual void SetBlockSizeX(int /*n*/) {}
        virtual void SetBlockSizeY(int /*n*/) {}
        virtual void SetBlockSizeZ(int /*n*/) {}

        bool GetUseLocalMemory() const { return this->use_local_memory; }
        void SetUseLocalMemory(bool val) { this->use_local_memory = val; this->need_reload_formula = true; }

        virtual bool HasEditableWrapOption() const { return false; }
        bool GetWrap() const { return this->wrap; }
        virtual void SetWrap(bool w) { this->wrap = w; }

        virtual bool HasEditableAccuracyOption() const { return false; }
        enum class Accuracy { Low, Medium, High };
        Accuracy GetAccuracy() const { return this->accuracy; }
        virtual void SetAccuracy(Accuracy acc) { this->accuracy = acc; }

        /// Retrieve the current 3D object as a vtkPolyData.
        virtual void GetAsMesh(vtkPolyData *out,const Properties& render_settings) const =0;

        /// Retrieve the current 2D plane as a vtkImageData.
        virtual void GetAs2DImage(vtkImageData *out,const Properties& render_settings) const =0;
        /// Sets the values for a certain chemical from an image
        virtual void SetFrom2DImage(int iChemical, vtkImageData *im) = 0;
        /// Indicates whether GetAs2DImage() and SetFrom2DImage() can be called.
        virtual bool Is2DImageAvailable() const =0;

        /// Retrieve the dimensionality of the system volume, irrespective of the cells within it.
        virtual int GetArenaDimensionality() const =0;

        /// Retrieve the value at a given location.
        virtual float GetValue(float x,float y,float z,const Properties& render_settings) =0;
        /// Set the value at a given location.
        virtual void SetValue(float x,float y,float z,float val,const Properties& render_settings) =0;
        /// Set the value of all cells within radius r of a given location. The radius is expressed as a proportion of the diagonal of the bounding box.
        virtual void SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings) =0;

        bool CanUndo() const; ///< Returns true if there is anything to undo.
        bool CanRedo() const; ///< Returns true if there is anything to redo.
        virtual void Undo();  ///< Rewind all actions until the previous undo point.
        virtual void Redo();  ///< Redo all actions until the next undo point.
        void SetUndoPoint();  ///< Set an undo point, e.g on mouse up. All actions between undo points are grouped into one block.

        std::string GetNeighborhoodType() const;

        /// Retrieve the data type used for storing values (VTK_FLOAT or VTK_DOUBLE)
        int GetDataType() const;

        /// Returns whether this system allows the data type to be changed or not
        virtual bool HasEditableDataType() const =0;

        /// Change the data type used for storing values (VTK_FLOAT or VTK_DOUBLE)
        void SetDataType(int type);

        /// Returns the total memory size that will need to be transferred to the GPU
        virtual size_t GetMemorySize() const =0;

        virtual std::vector<float> GetData(int i_chemical) const =0;

        struct Parameter {
            std::string name;
            float value;
        };

    protected: // typedefs

        enum class TNeighborhood { VERTEX_NEIGHBORS, EDGE_NEIGHBORS, FACE_NEIGHBORS };
        // (edge neighbors include face neighbors; vertex neighbors include edge neighbors and face neighbors)

    protected: // variables

        std::string rule_name, description;

        int n_chemicals;

        int data_type;
        size_t data_type_size;
        std::string data_type_string;
        std::string data_type_suffix;
        bool use_local_memory;

        InitialPatternGenerator initial_pattern_generator;

        std::vector<Parameter> parameters;

        int timesteps_taken;

        std::string formula;
        bool need_reload_formula;

        std::string filename;
        bool is_modified;

        bool wrap; ///< should the data wrap-around or have a boundary?

        /// We only allow undo for paint actions.
        struct PaintAction {
            int iChemical,iCell;
            bool done,last_of_group;
            float val; // value to paint to undo/redo this action
        };
        std::vector<PaintAction> undo_stack;

        TNeighborhood neighborhood_type;

        std::map<TNeighborhood,std::string> canonical_neighborhood_type_identifiers;
        std::map<std::string,TNeighborhood> recognized_neighborhood_type_identifiers;

        double x_spacing_proportion;    /// spatial separation for rendering multiple chemicals, as a proportion of X
        double y_spacing_proportion;    /// spatial separation for rendering multiple chemicals, as a proportion of Y

        Accuracy accuracy;

    protected: // functions

        /// Advance the RD system by n timesteps.
        virtual void InternalUpdate(int n_steps)=0;

        virtual void AddPhasePlot(vtkRenderer* pRenderer, float scaling, float low, float high, float posX, float posY, float posZ,
            int iChemX, int iChemY, int iChemZ) =0;
        virtual void FlipPaintAction(PaintAction& cca) =0; ///< Undo/redo this paint action.
        void StorePaintAction(int iChemical,int iCell,float old_val); ///< Implementations call this when performing undo-able paint actions.

    private: // functions

        void InternalSetDataType(int type);

    private: // constants

        static const int ready_format_version = 6;
};

#endif
