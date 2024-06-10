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
#include "IO_XML.hpp"
#include "ImageRD.hpp"
#include "MeshRD.hpp"
#include "utils.hpp"
#include "Properties.hpp"

// VTK:
#include <vtkXMLUtilities.h>
#include <vtkXMLDataParser.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>

// STL:
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

using namespace std;

// --------------------------------------------------------------------------------

vtkStandardNewMacro(RD_XMLImageReader);
vtkStandardNewMacro(RD_XMLUnstructuredGridReader);
vtkStandardNewMacro(RD_XMLImageWriter);
vtkStandardNewMacro(RD_XMLUnstructuredGridWriter);

// ================================================================================

string RD_XMLImageReader::GetType()
{
    vtkSmartPointer<vtkXMLDataElement> rule = this->GetRDElement()->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");
    string s;
    read_required_attribute(rule,"type",s);
    return s;
}

// --------------------------------------------------------------------------------

string RD_XMLImageReader::GetName()
{
    vtkSmartPointer<vtkXMLDataElement> rule = this->GetRDElement()->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");
    string s;
    read_required_attribute(rule,"name",s);
    return s;
}

// --------------------------------------------------------------------------------

bool RD_XMLImageReader::ShouldGenerateInitialPatternWhenLoading()
{
    vtkSmartPointer<vtkXMLDataElement> initial_pattern_generator =
        this->GetRDElement()->FindNestedElementWithName("initial_pattern_generator");
    if(!initial_pattern_generator) return false; // (element is optional, defaults to false)
    const char *s = initial_pattern_generator->GetAttribute("apply_when_loading");
    if(!s) return false;
    return string(s)=="true";
}

// --------------------------------------------------------------------------------

vtkXMLDataElement* RD_XMLImageReader::GetRDElement()
{
    this->Update();
    vtkSmartPointer<vtkXMLDataElement> root = this->XMLParser->GetRootElement();
    if(!root) throw runtime_error("No XML found in file");
    vtkSmartPointer<vtkXMLDataElement> rd = root->FindNestedElementWithName("RD");
    if(!rd) throw runtime_error("RD node not found in file");
    return rd;
}

// ================================================================================

string RD_XMLUnstructuredGridReader::GetType()
{
    vtkSmartPointer<vtkXMLDataElement> rule = this->GetRDElement()->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");
    string s;
    read_required_attribute(rule,"type",s);
    return s;
}

// --------------------------------------------------------------------------------

string RD_XMLUnstructuredGridReader::GetName()
{
    vtkSmartPointer<vtkXMLDataElement> rule = this->GetRDElement()->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");
    string s;
    read_required_attribute(rule,"name",s);
    return s;
}

// --------------------------------------------------------------------------------

bool RD_XMLUnstructuredGridReader::ShouldGenerateInitialPatternWhenLoading()
{
    vtkSmartPointer<vtkXMLDataElement> initial_pattern_generator =
        this->GetRDElement()->FindNestedElementWithName("initial_pattern_generator");
    if(!initial_pattern_generator) return false; // (element is optional, defaults to false)
    const char *s = initial_pattern_generator->GetAttribute("apply_when_loading");
    if(!s) return false;
    return string(s)=="true";
}

// --------------------------------------------------------------------------------

vtkXMLDataElement* RD_XMLUnstructuredGridReader::GetRDElement()
{
    this->Update();
    vtkSmartPointer<vtkXMLDataElement> root = this->XMLParser->GetRootElement();
    if(!root) throw runtime_error("No XML found in file");
    vtkSmartPointer<vtkXMLDataElement> rd = root->FindNestedElementWithName("RD");
    if(!rd) throw runtime_error("RD node not found in file");
    return rd;
}

// ================================================================================

void RD_XMLImageWriter::SetSystem(const ImageRD* rd_system)
{
    this->system = rd_system;
}

// --------------------------------------------------------------------------------

int RD_XMLImageWriter::WritePrimaryElement(ostream& os,vtkIndent indent)
{
    vtkSmartPointer<vtkXMLDataElement> xml = this->system->GetAsXML(this->generate_initial_pattern_when_loading);
    xml->AddNestedElement(this->render_settings->GetAsXML());
    xml->PrintXML(os,indent);
    return vtkXMLImageDataWriter::WritePrimaryElement(os,indent);
}

// ================================================================================

void RD_XMLUnstructuredGridWriter::SetSystem(const MeshRD* rd_system)
{
    this->system = rd_system;
}

// ---------------------------------------------------------------------

int RD_XMLUnstructuredGridWriter::WritePrimaryElement(ostream& os,vtkIndent indent)
{
    vtkSmartPointer<vtkXMLDataElement> xml = this->system->GetAsXML(this->generate_initial_pattern_when_loading);
    xml->AddNestedElement(this->render_settings->GetAsXML());
    xml->PrintXML(os,indent);
    return vtkXMLUnstructuredGridWriter::WritePrimaryElement(os,indent);
}

// ---------------------------------------------------------------------
