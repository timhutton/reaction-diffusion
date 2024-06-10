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

#ifndef __UTILS__
#define __UTILS__

// STL:
#include <string>
#include <sstream>
#include <stdexcept>
#include <map>
#include <vector>

// VTK:
#include <vtkCommand.h>
#include <vtkSmartPointer.h>
#include <vtkXMLDataElement.h>

double get_time_in_seconds();

float frand(float lower,float upper);

double hypot2(double x,double y);

double hypot3(double x,double y,double z);

// http://www.doc.ic.ac.uk/~akf/handel-c/cgi-bin/forum.cgi?msg=551
#define STRING_FROM_LITERAL(a) #a
#define STR(a) STRING_FROM_LITERAL(a)

template <typename T> std::string to_string(const T& t)
{
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

template <typename T> bool from_string(const std::string& s,T& val)
{
    std::istringstream iss(s);
    iss >> val;
    return !iss.fail();
}
// we provide a specialized version of from_string for string types, to avoid the problem of multiple space-separated words
template <> bool from_string<std::string> (const std::string& s,std::string& val);

float* vtk_at(float* origin,int x,int y,int z,int X,int Y);

template <typename T>
void read_required_attribute(vtkXMLDataElement* e,const std::string& name,T& val)
{
    const char *str = e->GetAttribute(name.c_str());
    if(!str || !from_string(str,val))
        throw std::runtime_error(to_string(e->GetName())+" : failed to read required attribute: "+name);
}

template <typename T>
void read_optional_attribute(vtkXMLDataElement* e,const std::string& name,T& val)
{
    const char *str = e->GetAttribute(name.c_str());
    if(str && !from_string(str,val))
        throw std::runtime_error(to_string(e->GetName())+" : failed to read optional attribute: "+name);
}

std::string GetChemicalName(size_t i); ///< a, b, c, ... z, aa, ab, ...
int IndexFromChemicalName(const std::string& s);

/// Read a multiline string, outputting whitespace-trimmed lines.
std::string trim_multiline_string(const char* s);

/// Abstract interface for XML objects.
/** Allows objects to be saved/loaded to/from an XML representation. */
class XML_Object
{
    public:

        XML_Object(const vtkXMLDataElement* /*node*/) {}
        virtual ~XML_Object() {}
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const =0;
};

std::string ReplaceAllSubstrings(std::string subject, const std::string& search, const std::string& replace);
std::vector<std::string> tokenize_for_keywords(const std::string& formula);
bool UsingKeyword(const std::vector<std::string>& formula_tokens, const std::string& keyword);

class ThrowOnErrorObserver : public vtkCommand
{
public:
    static ThrowOnErrorObserver* New()
    {
        return new ThrowOnErrorObserver;
    }
    virtual void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* calldata)
    {
        switch (event)
        {
        case vtkCommand::ErrorEvent:
            throw std::runtime_error(static_cast<char*>(calldata));
            break;
        case vtkCommand::WarningEvent:
            // (could handle somehow)
            break;
        }
    }
};

#endif
