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

// Local:
#include "overlays.hpp"

// STL:
#include <vector>

/// Generates image/mesh patterns by drawing a series of overlays.
class InitialPatternGenerator
{
    public:

        InitialPatternGenerator();
        ~InitialPatternGenerator();

        void ReadFromXML(vtkXMLDataElement* node);
        vtkSmartPointer<vtkXMLDataElement> GetAsXML(bool generate_initial_pattern_when_loading) const;

        size_t GetNumberOfOverlays() const { return this->overlays.size(); }
        Overlay& GetOverlay(size_t i) { return *this->overlays[i]; }
        const Overlay& GetOverlay(size_t i) const { return *this->overlays[i]; }

        /// Create a generator suitable for Gray-Scott, so that new patterns can start working immediately.
        void CreateDefaultInitialPatternGenerator(size_t num_chemicals);
        bool ShouldZeroFirst() const { return this->zero_first; }

    private:

        void RemoveAllOverlays();

        std::vector<std::unique_ptr<Overlay>> overlays;
        bool zero_first;
};
