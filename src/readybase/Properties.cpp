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

#include "Properties.hpp"

// local:
#include "scene_items.hpp"

// STL:
#include <algorithm>
#include <string>
#include <stdexcept>

using namespace std;

void Property::ReadFromXML(vtkXMLDataElement* node)
{
    if(this->type=="float")
        read_required_attribute(node,"value",this->f1);
    else if(this->type=="int")
        read_required_attribute(node,"value",this->i);
    else if(this->type=="bool")
    {
        read_required_attribute(node,"value",this->s);
        if(this->s=="true") this->b = true;
        else if(this->s=="false") this->b = false;
        else throw runtime_error("Property::ReadFromXML : unrecognised bool value: "+this->s);
    }
    else if(this->type=="color")
    {
        read_required_attribute(node,"r",this->f1);
        read_required_attribute(node,"g",this->f2);
        read_required_attribute(node,"b",this->f3);
    }
    else if(this->type=="chemical")
    {
        read_required_attribute(node,"value",this->s);
        IndexFromChemicalName(this->s);
    }
    else if(this->type=="axis")
    {
        read_required_attribute(node,"value",this->s);
        if(this->s != "x" && this->s != "y" && this->s != "z")
            throw runtime_error("Property::ReadFromXML : unrecognised axis: "+this->s);
    }
    else if(this->type=="colormap")
    {
        read_required_attribute(node,"value",this->s);
        if(find(begin(SupportedColorMaps), end(SupportedColorMaps), this->s) == end(SupportedColorMaps))
            throw runtime_error("Property::ReadFromXML : unrecognised colormap: "+this->s);
    }
    else throw runtime_error("Property::ReadFromXML : unrecognised type: "+this->type);
}

vtkSmartPointer<vtkXMLDataElement> Property::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
    node->SetName(this->name.c_str());
    if(this->type=="float")
        node->SetFloatAttribute("value",this->f1);
    else if(this->type=="int")
        node->SetIntAttribute("value",this->i);
    else if(this->type=="bool")
        node->SetAttribute("value",this->b?"true":"false");
    else if(this->type=="color")
    {
        node->SetFloatAttribute("r",f1);
        node->SetFloatAttribute("g",f2);
        node->SetFloatAttribute("b",f3);
    }
    else if(this->type=="chemical")
        node->SetAttribute("value",this->s.c_str());
    else if(this->type=="axis")
        node->SetAttribute("value",this->s.c_str());
    else if(this->type=="colormap")
        node->SetAttribute("value",this->s.c_str());
    else throw runtime_error("Property::GetAsXML : unrecognised type: "+this->type);
    return node;
}

// ==============================================================================================

void Properties::OverwriteFromXML(vtkXMLDataElement *node)
{
    this->set_name = string(node->GetName());
    for(int i=0;i<node->GetNumberOfNestedElements();i++)
    {
        vtkXMLDataElement *propnode = node->GetNestedElement(i);
        this->GetProperty(propnode->GetName()).ReadFromXML(propnode);
    }
}

vtkSmartPointer<vtkXMLDataElement> Properties::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> root = vtkSmartPointer<vtkXMLDataElement>::New();
    root->SetName(this->set_name.c_str());
    for(vector<Property>::const_iterator it=this->properties.begin();it!=this->properties.end();it++)
        root->AddNestedElement(it->GetAsXML());
    return root;
}

const Property& Properties::GetProperty(const std::string& name) const
{
    for(vector<Property>::const_iterator it=this->properties.begin();it!=this->properties.end();it++)
    {
        if(it->GetName()==name)
            return *it;
    }
    throw runtime_error("Properties::GetProperty : unrecognised property: "+name);
}

Property& Properties::GetProperty(const std::string& name)
{
    for(vector<Property>::iterator it=this->properties.begin();it!=this->properties.end();it++)
    {
        if(it->GetName()==name)
            return *it;
    }
    throw runtime_error("Properties::GetProperty : unrecognised property: "+name);
}

void Properties::AddProperty(Property p)
{
    this->properties.push_back(p);
}

bool Properties::IsProperty(const std::string& name)
{
    for(vector<Property>::const_iterator it=this->properties.begin();it!=this->properties.end();it++)
    {
        if(it->GetName()==name)
            return true;
    }
    return false;
}
