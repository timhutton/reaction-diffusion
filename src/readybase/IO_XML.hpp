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
class ImageRD;
class MeshRD;
class Properties;

// VTK:
#include <vtkXMLImageDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>

// -------------------------------------------------------------------

/// Reads *.vti files, our extended version of VTK's XML format for vtkImageData.
class RD_XMLImageReader : public vtkXMLImageDataReader
{
    public:

        vtkTypeMacro(RD_XMLImageReader, vtkXMLImageDataReader);
        static RD_XMLImageReader* New();

        std::string GetType();
        std::string GetName();
        vtkXMLDataElement* GetRDElement();
        bool ShouldGenerateInitialPatternWhenLoading();

    protected:

        RD_XMLImageReader() {}

};

// -------------------------------------------------------------------

/// Reads *.vtu files, our extended version of VTK's XML format for vtkUnstructuredGrid.
class RD_XMLUnstructuredGridReader : public vtkXMLUnstructuredGridReader
{
    public:

        vtkTypeMacro(RD_XMLUnstructuredGridReader, vtkXMLUnstructuredGridReader);
        static RD_XMLUnstructuredGridReader* New();

        std::string GetType();
        std::string GetName();
        vtkXMLDataElement* GetRDElement();
        bool ShouldGenerateInitialPatternWhenLoading();

    protected:

        RD_XMLUnstructuredGridReader() {}

};

// -------------------------------------------------------------------

/// Used to write vtkImageData to XML, with an added RD section containing rule information.
class RD_XMLImageWriter : public vtkXMLImageDataWriter
{
    public:

        vtkTypeMacro(RD_XMLImageWriter, vtkXMLImageDataWriter);
        static RD_XMLImageWriter* New();

        void SetSystem(const ImageRD* rd_system);
        void SetRenderSettings(const Properties* settings) { this->render_settings = settings; }
        void GenerateInitialPatternWhenLoading() { this->generate_initial_pattern_when_loading = true; }

    protected:

        RD_XMLImageWriter() : system(NULL), generate_initial_pattern_when_loading(false) {}

        static vtkSmartPointer<vtkXMLDataElement> BuildRDSystemXML(ImageRD* system);

        int WritePrimaryElement(ostream& os,vtkIndent indent) override;

    protected:

        const ImageRD* system;
        const Properties* render_settings;
        bool generate_initial_pattern_when_loading;
};

// ---------------------------------------------------------------------

/// Used to write vtkUnstructuredGrid to XML, with an added RD section containing rule information.
class RD_XMLUnstructuredGridWriter : public vtkXMLUnstructuredGridWriter
{
    public:

        vtkTypeMacro(RD_XMLUnstructuredGridWriter, vtkXMLUnstructuredGridWriter);
        static RD_XMLUnstructuredGridWriter* New();

        void SetSystem(const MeshRD* rd_system);
        void SetRenderSettings(const Properties* settings) { this->render_settings = settings; }
        void GenerateInitialPatternWhenLoading() { this->generate_initial_pattern_when_loading = true; }

    protected:

        RD_XMLUnstructuredGridWriter() : system(NULL), generate_initial_pattern_when_loading(false) {}

        static vtkSmartPointer<vtkXMLDataElement> BuildRDSystemXML(MeshRD* system);

        int WritePrimaryElement(ostream& os,vtkIndent indent) override;

    protected:

        const MeshRD* system;
        const Properties* render_settings;
        bool generate_initial_pattern_when_loading;
};

// -------------------------------------------------------------------
