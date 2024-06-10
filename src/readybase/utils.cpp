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
#include "utils.hpp"

// VTK:
#include <vtkMath.h>

// STL:
#include <algorithm>
#include <limits>
#include <random>
#include <vector>

// ---------------------------------------------------------------------------------------------------------

#ifdef _WIN32
    #include <sys/timeb.h>
    #include <sys/types.h>
    #include <winsock.h>
    // http://www.linuxjournal.com/article/5574
    void gettimeofday(struct timeval* t,void* timezone)
    {
        struct _timeb timebuffer;
        #if _MSC_VER < 1400
            _ftime( &timebuffer );
        #else
            // MSVC 2005+
            _ftime_s( &timebuffer );
        #endif
        t->tv_sec = (long)timebuffer.time;
        t->tv_usec = 1000 * timebuffer.millitm;
    }
#else
    #include <sys/time.h>
#endif

// ---------------------------------------------------------------------------------------------------------

using namespace std;

// ---------------------------------------------------------------------------------------------------------

double get_time_in_seconds()
{
    struct timeval tod_record;
    gettimeofday(&tod_record, 0);
    return double(tod_record.tv_sec) + double(tod_record.tv_usec) / 1.0e6;
}

// ---------------------------------------------------------------------------------------------------------

float frand(float lower,float upper)
{
    random_device rd;
    default_random_engine re(rd());
    uniform_real_distribution<float> unif(lower, upper);
    return unif(re);
}

// ---------------------------------------------------------------------------------------------------------

double hypot2(double x,double y)
{
    return sqrt(x*x+y*y);
}

// ---------------------------------------------------------------------------------------------------------

double hypot3(double x,double y,double z)
{
    return sqrt(x*x+y*y+z*z);
}

// ---------------------------------------------------------------------------------------------------------

float* vtk_at(float* origin,int x,int y,int z,int X,int Y)
{
    // single-component vtkImageData scalars are stored as: float,float,... for consecutive x, then y, then z
    return origin + x + X*(y + Y*z);
}

// ---------------------------------------------------------------------------------------------------------

template <> bool from_string<string> (const string& s,string& val)
{
    val = s;
    return true;
}

// ---------------------------------------------------------------------------------------------------------

string GetChemicalName(size_t i)
{
    if(i<26)
        return string( 1, 'a' + static_cast<char>(i) );
    else if(i<26*27)
        return GetChemicalName(i/26-1) + GetChemicalName(i%26);
    throw runtime_error("GetChemicalName: out of range");
}

// ---------------------------------------------------------------------------------------------------------

int IndexFromChemicalName(const string& s)
{
    for(int i=0;i<26*27;i++)
        if(s==GetChemicalName(i))
            return i;
    throw runtime_error("IndexFromChemicalName: unrecognised chemical name: "+s);
}

// ---------------------------------------------------------------------------------------------------------

// read a multiline string, strip leading whitespace from all lines
string trim_multiline_string(const char* s)
{
    istringstream iss(s);
    string item;
    vector<string> vs;
    size_t minLeadingWhitespace = numeric_limits<size_t>::max();
    while(getline(iss, item, '\n'))
    {
        size_t leadingWhitespace = item.find_first_not_of(" \t");
        if(leadingWhitespace!=string::npos)
        {
            minLeadingWhitespace = min(minLeadingWhitespace, leadingWhitespace);
            vs.push_back( item );
        }
        else
        {
            if( !vs.empty() && !vs.back().empty() )
                vs.push_back("");
        }
    }
    ostringstream oss;
    for( size_t i=0; i < vs.size(); i++ )
    {
        item = vs[i];
        if( item.size() > minLeadingWhitespace)
            oss << item.substr(minLeadingWhitespace);
        if( i < vs.size()-1 )
            oss << "\n";
    }
    return oss.str();
}

// ---------------------------------------------------------------------------------------------------------

string ReplaceAllSubstrings(string subject, const string& search, const string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

// ---------------------------------------------------------------------------------------------------------

vector<string> tokenize_for_keywords(const string& formula)
{
    // customized tokenize for when searching for keywords in formula rules: whole words only
    vector<string> tokens;
    string token;
    for (char c : formula)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9'))
        {
            token += c;
        }
        else
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token = "";
            }
        }
    }
    if (!token.empty())
    {
        tokens.push_back(token);
    }
    return tokens;
}

// ---------------------------------------------------------------------------------------------------------

bool UsingKeyword(const vector<string>& formula_tokens, const string& keyword)
{
    return find(formula_tokens.begin(), formula_tokens.end(), keyword) != formula_tokens.end();
    // TODO: parse properly: ignore comments, not in string, etc.
}

// -------------------------------------------------------------------------
