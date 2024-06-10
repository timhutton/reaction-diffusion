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
class AbstractRD;
class Properties;

// STL:
#include <memory>

std::unique_ptr<AbstractRD> MakeNewImage1D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewImage2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewImage3D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewGeodesicSphere(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewTorus(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewTriangularMesh(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewHexagonalMesh(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewRhombilleTiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewPenroseP3Tiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewPenroseP2Tiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewDelaunay2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewVoronoi2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewDelaunay3D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewBodyCentredCubicHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewFaceCentredCubicHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewDiamondHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewHyperbolicPlane(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
std::unique_ptr<AbstractRD> MakeNewHyperbolicSpace(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
