/*  Copyright 2011-2021 The Ready Bunch

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
#include "AbstractRD.hpp"
#include "overlays.hpp"

// STL:
#include <algorithm>

// SSE:
#if USE_SSE
#include <xmmintrin.h>
#endif

using namespace std;

// ---------------------------------------------------------------------

AbstractRD::AbstractRD(int data_type)
    : use_local_memory(false)
    , timesteps_taken(0)
    , need_reload_formula(true)
    , is_modified(false)
    , wrap(true)
    , neighborhood_type(TNeighborhood::VERTEX_NEIGHBORS)
    , x_spacing_proportion(0.05)
    , y_spacing_proportion(0.1)
    , accuracy(Accuracy::Medium)
{
    this->InternalSetDataType(data_type);

    #if defined(USE_SSE)
        // disable accurate handling of denormals and zeros, for speed
        #if (defined(__i386__) || defined(__x64_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_IX86))
         int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
         int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
         _mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
        #endif
    #endif // (USE_SSE)

    this->canonical_neighborhood_type_identifiers[TNeighborhood::VERTEX_NEIGHBORS] = "vertex";
    this->canonical_neighborhood_type_identifiers[TNeighborhood::EDGE_NEIGHBORS] = "edge";
    this->canonical_neighborhood_type_identifiers[TNeighborhood::FACE_NEIGHBORS] = "face";
    for(map<TNeighborhood,string>::const_iterator it = this->canonical_neighborhood_type_identifiers.begin();it != this->canonical_neighborhood_type_identifiers.end();it++)
        this->recognized_neighborhood_type_identifiers[it->second] = it->first;
}

// ---------------------------------------------------------------------

AbstractRD::~AbstractRD()
{
}

// ---------------------------------------------------------------------

void AbstractRD::SetFormula(string s)
{
    s.erase(s.find_last_not_of(" \t\n\r") + 1); // trim trailing whitespace
    if(s != this->formula)
        this->need_reload_formula = true;
    this->formula = s;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::SetRuleName(std::string s)
{
    this->rule_name = s;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::SetDescription(std::string s)
{
    this->description = s;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

int AbstractRD::GetNumberOfParameters() const
{
    return (int)this->parameters.size();
}

// ---------------------------------------------------------------------

std::string AbstractRD::GetParameterName(int iParam) const
{
    return this->parameters[iParam].name;
}

// ---------------------------------------------------------------------

float AbstractRD::GetParameterValue(int iParam) const
{
    return this->parameters[iParam].value;
}

// ---------------------------------------------------------------------

float AbstractRD::GetParameterValueByName(const std::string& name) const
{
    for (const Parameter& parameter : this->parameters)
    {
        if (parameter.name == name)
        {
            return parameter.value;
        }
    }
    throw runtime_error("ImageRD::GetParameterValueByName : parameter name not found: "+name);
}

// ---------------------------------------------------------------------

void AbstractRD::AddParameter(const std::string& name,float val)
{
    this->parameters.push_back({ name, val });
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::DeleteParameter(int iParam)
{
    this->parameters.erase(this->parameters.begin()+iParam);
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::DeleteAllParameters()
{
    this->parameters.clear();
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::SetParameterName(int iParam,const string& s)
{
    this->parameters[iParam].name = s;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::SetParameterValue(int iParam,float val)
{
    this->parameters[iParam].value = val;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

bool AbstractRD::IsParameter(const string& name) const
{
    return find_if(this->parameters.begin(), this->parameters.end(),
        [&](const Parameter& param) { return param.name == name; }) != this->parameters.end();
}

// ---------------------------------------------------------------------

void AbstractRD::SetModified(bool m)
{
    this->is_modified = m;
}

// ---------------------------------------------------------------------

void AbstractRD::SetFilename(const string& s)
{
    this->filename = s;
}

// ---------------------------------------------------------------------

void AbstractRD::InitializeFromXML(vtkXMLDataElement* rd, bool& warn_to_update)
{
    // check whether we should warn the user that they need to update Ready
    {
        int i;
        read_required_attribute(rd,"format_version",i);
        warn_to_update = (i > this->ready_format_version);
        // (we will still proceed and try to read the file but it might fail or give poor results)
    }

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // rule_name:
    string str;
    read_required_attribute(rule,"name",str);
    this->SetRuleName(str);

    // wrap-around
    const char *s = rule->GetAttribute("wrap");
    if(!s) this->wrap = true;
    else this->wrap = (string(s)=="1");

    // neighborhood specifiers

    s = rule->GetAttribute("neighborhood_type");
    if(!s) this->neighborhood_type = TNeighborhood::VERTEX_NEIGHBORS;
    else if(this->recognized_neighborhood_type_identifiers.find(s)==this->recognized_neighborhood_type_identifiers.end())
        throw runtime_error("Unrecognized neighborhood_type");
    else this->neighborhood_type = this->recognized_neighborhood_type_identifiers[s];

    // parameters:
    this->DeleteAllParameters();
    for(int i=0;i<rule->GetNumberOfNestedElements();i++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = rule->GetNestedElement(i);
        if(string(node->GetName())!="param") continue;
        string name;
        s = node->GetAttribute("name");
        if(!s) throw runtime_error("Failed to read param attribute: name");
        name = trim_multiline_string(s);
        s = node->GetCharacterData();
        float f;
        if(!s || !from_string(s,f)) throw runtime_error("Failed to read param value");
        this->AddParameter(name,f);
    }

    // description:
    vtkSmartPointer<vtkXMLDataElement> xml_description = rd->FindNestedElementWithName("description");
    if(!xml_description) this->SetDescription(""); // optional, default is empty string
    else this->SetDescription(trim_multiline_string(xml_description->GetCharacterData()));

    // initial_pattern_generator:
    this->initial_pattern_generator.ReadFromXML(rd->FindNestedElementWithName("initial_pattern_generator"));
}

// ---------------------------------------------------------------------

// TODO: ImageRD could inherit from XML_Object (but as VTKFile element, not RD element!)
vtkSmartPointer<vtkXMLDataElement> AbstractRD::GetAsXML(bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<vtkXMLDataElement> rd = vtkSmartPointer<vtkXMLDataElement>::New();
    rd->SetName("RD");
    rd->SetIntAttribute("format_version", this->ready_format_version);
    // (Use this for when the format changes so much that the user will get better results if they update their Ready. File reading will still proceed but may fail.)

    // description
    vtkSmartPointer<vtkXMLDataElement> description_node = vtkSmartPointer<vtkXMLDataElement>::New();
    description_node->SetName("description");
    string desc = this->GetDescription();
    desc = ReplaceAllSubstrings(desc, "\n", "\n      "); // indent the lines
    description_node->SetCharacterData(desc.c_str(), (int)desc.length());
    rd->AddNestedElement(description_node);

    // rule
    vtkSmartPointer<vtkXMLDataElement> rule = vtkSmartPointer<vtkXMLDataElement>::New();
    rule->SetName("rule");
    rule->SetAttribute("name",this->GetRuleName().c_str());
    rule->SetAttribute("type",this->GetRuleType().c_str());
    if(this->HasEditableWrapOption())
        rule->SetIntAttribute("wrap",this->GetWrap()?1:0);
    rule->SetAttribute("neighborhood_type",this->canonical_neighborhood_type_identifiers.find(this->neighborhood_type)->second.c_str());
    for(int i=0;i<this->GetNumberOfParameters();i++)    // parameters
    {
        vtkSmartPointer<vtkXMLDataElement> param = vtkSmartPointer<vtkXMLDataElement>::New();
        param->SetName("param");
        param->SetAttribute("name",this->GetParameterName(i).c_str());
        string s = to_string(this->GetParameterValue(i));
        param->SetCharacterData(s.c_str(),(int)s.length());
        rule->AddNestedElement(param);
    }
    rd->AddNestedElement(rule);

    rd->AddNestedElement(initial_pattern_generator.GetAsXML(generate_initial_pattern_when_loading));

    return rd;
}

// ---------------------------------------------------------------------

void AbstractRD::CreateDefaultInitialPatternGenerator(size_t num_chemicals)
{
    this->initial_pattern_generator.CreateDefaultInitialPatternGenerator(num_chemicals);
}

// ---------------------------------------------------------------------

bool AbstractRD::CanUndo() const
{
    return !this->undo_stack.empty() && this->undo_stack.front().done;
}

// ---------------------------------------------------------------------

bool AbstractRD::CanRedo() const
{
    return !this->undo_stack.empty() && !this->undo_stack.back().done;
}

// ---------------------------------------------------------------------

void AbstractRD::SetUndoPoint()
{
    // paint events are treated as a block until (e.g.) mouse up calls this function
    if(!this->undo_stack.empty())
        this->undo_stack.back().last_of_group = true;
}

// ---------------------------------------------------------------------

void AbstractRD::Undo()
{
    if(!this->CanUndo()) throw runtime_error("AbstractRD::Undo() : attempt to undo when undo not possible");

    // find the last done paint action, undo backwards from there until the previous last_of_group (exclusive)
    vector<PaintAction>::reverse_iterator rit;
    for(rit=this->undo_stack.rbegin();!rit->done;rit++); // skip over actions that have already been undone
    while(true)
    {
        this->FlipPaintAction(*rit);
        rit++;
        if(rit==this->undo_stack.rend() || rit->last_of_group)
            break;
    }
}

// ---------------------------------------------------------------------

void AbstractRD::Redo()
{
    if(!this->CanRedo()) throw runtime_error("AbstractRD::Redo() : attempt to redo when redo not possible");

    // find the first undone paint action, redo forwards from there until the next last_of_group (inclusive)
    vector<PaintAction>::iterator it;
    for(it=this->undo_stack.begin();it->done;it++); // skip over actions that have already been done
    do
    {
        this->FlipPaintAction(*it);
        if(it->last_of_group)
            break;
        it++;
    } while(it!=this->undo_stack.end());
}

// ---------------------------------------------------------------------

void AbstractRD::StorePaintAction(int iChemical,int iCell,float old_val)
{
    // forget all stored undone actions
    while(!this->undo_stack.empty() && !this->undo_stack.back().done)
        this->undo_stack.pop_back();
    // add the new paint action
    PaintAction pa;
    pa.iCell = iCell;
    pa.iChemical = iChemical;
    pa.val = old_val; // (the cell itself stores the new val, we just need the old one)
    pa.done = true;
    pa.last_of_group = false;
    this->undo_stack.push_back(pa);
}

// ---------------------------------------------------------------------

std::string AbstractRD::GetNeighborhoodType() const
{
    return this->canonical_neighborhood_type_identifiers.find(this->neighborhood_type)->second;
}

// ---------------------------------------------------------------------

int AbstractRD::GetDataType() const
{
    return this->data_type;
}

// ---------------------------------------------------------------------

void AbstractRD::SetDataType(int type)
{
    this->InternalSetDataType(type);
    const bool reallocate_storage = true;
    this->SetNumberOfChemicals(this->n_chemicals, reallocate_storage);
    this->GenerateInitialPattern();
    // TODO: would be nice to keep current pattern somehow, instead of reallocating and reinitializing
}

// ---------------------------------------------------------------------

void AbstractRD::InternalSetDataType(int type)
{
    switch( type ) {
        default:
        case VTK_FLOAT:
            this->data_type = VTK_FLOAT;
            this->data_type_size = sizeof( float );
            this->data_type_string = "float";
            this->data_type_suffix = "f";
            break;
        case VTK_DOUBLE:
            this->data_type = VTK_DOUBLE;
            this->data_type_size = sizeof( double );
            this->data_type_string = "double";
            this->data_type_suffix = "";
    }
}

// ---------------------------------------------------------------------
