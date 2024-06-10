#  Copyright 2011-2024 The Ready Bunch
#
#  This file is part of Ready.
#
#  Ready is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Ready is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Ready. If not, see <http://www.gnu.org/licenses/>.
#
#--------------------------------------------------------------------------

cmake_minimum_required( VERSION 2.6...3.5 )
# - The min version is enforced (older versions of CMake will give an error). It should be set to the oldest
#   version that we know works with our build on every platform.
# - The max version tells CMake not to use any CMake features newer than this, in case they break something.
#   Keep it updated to the newest version that we know works with our build on all platforms.

project( Ready )

set( READY_VERSION 0.11.0 )   # check matches Help/about.html
add_definitions( -D READY_VERSION=${READY_VERSION} )

if( APPLE OR WIN32 )
  # app names are usually capitalized on macOS and Windows
  set( APP_NAME Ready )
else()
  # Linux binaries are usually all lowercase
  set( APP_NAME ready )
endif()
set( CMD_NAME rdy ) # command-line version

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#-------------------------------------------source files----------------------------------------------

set( BASE_SOURCES      # core code used in all executables
  src/readybase/AbstractRD.hpp                src/readybase/AbstractRD.cpp
  src/readybase/ImageRD.hpp                   src/readybase/ImageRD.cpp
  src/readybase/GrayScottImageRD.hpp          src/readybase/GrayScottImageRD.cpp
  src/readybase/OpenCLImageRD.hpp             src/readybase/OpenCLImageRD.cpp
  src/readybase/FormulaOpenCLImageRD.hpp      src/readybase/FormulaOpenCLImageRD.cpp
  src/readybase/FullKernelOpenCLImageRD.hpp   src/readybase/FullKernelOpenCLImageRD.cpp
  src/readybase/MeshRD.hpp                    src/readybase/MeshRD.cpp
  src/readybase/GrayScottMeshRD.hpp           src/readybase/GrayScottMeshRD.cpp
  src/readybase/OpenCLMeshRD.hpp              src/readybase/OpenCLMeshRD.cpp
  src/readybase/FormulaOpenCLMeshRD.hpp       src/readybase/FormulaOpenCLMeshRD.cpp
  src/readybase/FullKernelOpenCLMeshRD.hpp    src/readybase/FullKernelOpenCLMeshRD.cpp
  src/readybase/OpenCL_MixIn.hpp              src/readybase/OpenCL_MixIn.cpp
  src/readybase/OpenCL_utils.hpp              src/readybase/OpenCL_utils.cpp
  src/readybase/IO_XML.hpp                    src/readybase/IO_XML.cpp
  src/readybase/overlays.hpp                  src/readybase/overlays.cpp
  src/readybase/Properties.hpp                src/readybase/Properties.cpp
  src/readybase/utils.hpp                     src/readybase/utils.cpp
  src/readybase/stencils.hpp                  src/readybase/stencils.cpp
  src/readybase/OpenCL_Dyn_Load.h             src/readybase/OpenCL_Dyn_Load.c
  src/readybase/MeshGenerators.hpp            src/readybase/MeshGenerators.cpp
  src/readybase/SystemFactory.hpp             src/readybase/SystemFactory.cpp
  src/readybase/scene_items.hpp               src/readybase/scene_items.cpp
  src/readybase/InitialPatternGenerator.hpp   src/readybase/InitialPatternGenerator.cpp
  src/readybase/colormaps.hpp
  src/extern/PerlinNoise.hpp
)

set( GUI_SOURCES      # GUI code used only in Ready
  src/gui/IDs.hpp
  src/gui/wxutils.hpp                      src/gui/wxutils.cpp
  src/gui/dialogs.hpp                      src/gui/dialogs.cpp
  src/gui/prefs.hpp                        src/gui/prefs.cpp
  src/gui/app.hpp                          src/gui/app.cpp
  src/gui/frame.hpp                        src/gui/frame.cpp
  src/gui/HelpPanel.hpp                    src/gui/HelpPanel.cpp
  src/gui/InfoPanel.hpp                    src/gui/InfoPanel.cpp
  src/gui/HtmlInfo.hpp                     src/gui/HtmlInfo.cpp
  src/gui/PatternsPanel.hpp                src/gui/PatternsPanel.cpp
  src/gui/vtk_pipeline.hpp                 src/gui/vtk_pipeline.cpp
  src/gui/InteractorStylePainter.hpp       src/gui/InteractorStylePainter.cpp
  src/gui/wxVTKRenderWindowInteractor.h    src/gui/wxVTKRenderWindowInteractor.cxx
  src/gui/RecordingDialog.hpp              src/gui/RecordingDialog.cpp
  src/gui/ImportImageDialog.hpp            src/gui/ImportImageDialog.cpp
  src/gui/MakeNewSystem.hpp                src/gui/MakeNewSystem.cpp
)

set( CMD_SOURCES      # code used only in the command-line version
  src/cmd/main.cpp
  src/extern/cxxopts-2.2.1/cxxopts.hpp  # https://github.com/jarro2783/cxxopts
)

set( RESOURCES
  resources/ready.rc
  resources/appicon.ico
  resources/appicon16.ico
  resources/appicon32.ico
  resources/appicon48.ico
  resources/appicon.xpm
  resources/Info.plist.in
  resources/app.icns
  resources/file.icns
)

set( PATTERN_FILES
  Patterns/Meinhardt1982/stripes.vti       Patterns/Meinhardt1982/zebra.vtu
  Patterns/advection.vti
  Patterns/bilaplacian_interpolation.vti
  Patterns/Schlogl.vti
  Patterns/heat_equation.vti
  Patterns/Turing1952/spots.vti            Patterns/Turing1952/spots_noisy.vti
  Patterns/Turing1952/turing_parameter_map.vti
  Patterns/kernel_test.vti
  Patterns/parameter_modulation_demo.vti
  Patterns/parameter_modulation_demo2.vti  Patterns/parameter_modulation_demo2_3D.vti
  Patterns/heat_equation_interpolation.vti
  Patterns/Ginzburg-Landau/complex_Ginzburg-Landau.vti                Patterns/Ginzburg-Landau/complex_Ginzburg-Landau_1D.vti
  Patterns/Ginzburg-Landau/complex_Ginzburg-Landau_magnitude.vti
  Patterns/wave_equation.vti               Patterns/wave_soliton.vti
  Patterns/oregonator.vti
  Patterns/Brusselator.vti
  Patterns/SmoothLife2011/smoothglider.vti     Patterns/SmoothLife2011/smoothlifeL.vti     Patterns/SmoothLife2011/glider_3D.vti
  Patterns/SmoothLife2011/gaussian-smoothlife.vti     Patterns/SmoothLife2011/smoothlifeL_parameter_map.vti
  Patterns/Purwins1999/glider.vti              Patterns/Purwins1999/glider_3D.vti          Patterns/Purwins1999/multiGlider.vti
  Patterns/CPU-only/grayscott_1D.vti
  Patterns/CPU-only/grayscott_2D.vti
  Patterns/CPU-only/grayscott_3D.vti
  Patterns/FitzHugh-Nagumo/tip-splitting.vti
  Patterns/FitzHugh-Nagumo/tip-splitting_3D.vti
  Patterns/FitzHugh-Nagumo/spiral_turbulence.vti
  Patterns/FitzHugh-Nagumo/pulsate.vti
  Patterns/FitzHugh-Nagumo/squid_axon.vti
  Patterns/FitzHugh-Nagumo/Ising_regime.vti
  Patterns/FitzHugh-Nagumo/FHN_parameter_map.vti
  Patterns/GrayScott1984/bunny.vtu
  Patterns/GrayScott1984/lion.vtu
  Patterns/GrayScott1984/Lesmes_noisy.vti
  Patterns/GrayScott1984/noisy_solitons_mitosis.vti
  Patterns/GrayScott1984/parameter-map.vti
  Patterns/GrayScott1984/parameter-map_3D.vti
  Patterns/GrayScott1984/Pearson1993.vti
  Patterns/GrayScott1984/self-replicating_spots.vti
  Patterns/GrayScott1984/U-Skate/Hutton-and-helix-gliders.vti
  Patterns/GrayScott1984/U-Skate/Munafo_glider.vti
  Patterns/GrayScott1984/U-Skate/o-ring_2D.vti
  Patterns/CellularAutomata/Bays_3D.vti
  Patterns/CellularAutomata/Conway_life.vti
  Patterns/CellularAutomata/life_torus.vtu
  Patterns/CellularAutomata/larger-than-life.vti
  Patterns/CellularAutomata/Buss_hex.vtu
  Patterns/CellularAutomata/tri_life.vtu
  Patterns/CellularAutomata/hex_B2oS2m34_gliders.vtu
  Patterns/CellularAutomata/PenroseTilings/life.vtu
  Patterns/CellularAutomata/PenroseTilings/life_oscillators.vtu
  Patterns/CellularAutomata/PenroseTilings/Goucher_glider.vtu
  Patterns/CellularAutomata/PenroseTilings/Imai_glider_B2SC4.vtu
  Patterns/CellularAutomata/PenroseTilings/Goucher_loops.vtu
  Patterns/CellularAutomata/Salt/salt2D_demo.vti
  Patterns/CellularAutomata/Salt/salt3D_circular330.vti
  Patterns/CellularAutomata/FCC3333/random.vtu
  Patterns/CellularAutomata/FCC3333/glider.vtu
  Patterns/Yang2002/Yang_1.vti    Patterns/Yang2002/Yang_2b.vti  Patterns/Yang2002/Yang_2c.vti
  Patterns/Yang2002/Yang_2d.vti   Patterns/Yang2002/Yang_3a.vti  Patterns/Yang2002/Yang_3b.vti
  Patterns/Yang2002/Yang_3c.vti   Patterns/Yang2002/Yang_3d.vti  Patterns/Yang2002/Yang_4.vti
  Patterns/Yang2003/Fig2.vti
  Patterns/Yang2003/Fig3a.vti     Patterns/Yang2003/Fig3b.vti    Patterns/Yang2003/Fig3c.vti
  Patterns/McCabe2010/McCabe.vti               Patterns/McCabe2010/McCabe_simple.vti
  Patterns/McCabe2010/McCabe_additive2b.vti    Patterns/McCabe2010/McCabe_additive2a.vti
  Patterns/McCabe2010/fast-mccabe.vti
  Patterns/Kytta2007/Fig5.7a.vti  Patterns/Kytta2007/Fig5.7c.vti
  Patterns/Kytta2007/Fig5.8c.vti  Patterns/Kytta2007/Fig5.8d.vti Patterns/Kytta2007/Fig5.8e.vti
  Patterns/Kytta2007/Fig5.8f.vti  Patterns/Kytta2007/Fig5.8g.vti
  Patterns/Yang2006/jumping.vti   Patterns/Yang2006/jumping_cGL.vti
  Patterns/Schrodinger1926/packet.vti             Patterns/Schrodinger1926/packet_reflect.vti
  Patterns/Schrodinger1926/packet_pass.vti        Patterns/Schrodinger1926/quantum_tunnelling.vti
  Patterns/Schrodinger1926/packet_reflect2D.vti   Patterns/Schrodinger1926/two_slit.vti
  Patterns/Froese2014/Froese2014_Fig3.vti
  Patterns/Pennybacker2013/phyllotaxis_fibonacci.vti Patterns/Pennybacker2013/phyllotaxis_hexagons.vti
  Patterns/Pennybacker2013/spots.vti                 Patterns/Pennybacker2013/stripes.vti
  Patterns/Pennybacker2013/parameter_map.vti
  Patterns/Experiments/TimHutton/LifeBlur.vti
  Patterns/Experiments/TimHutton/mutually-catalytic_spots.vti
  Patterns/Experiments/TimHutton/mutually-catalytic_spots_2.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_forwardEuler.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_2stepAdamsBashforth.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_3stepAdamsBashforth.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_modifiedEuler.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_midpointMethod.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_HeunsMethod.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_LaxFriedrichs.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_LaxWendroff.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_RungeKutta4.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_staggeredLeapfrog.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_forwardEuler_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_2stepAdamsBashforth_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_3stepAdamsBashforth_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_modifiedEuler_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_midpointMethod_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_HeunsMethod_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_LaxFriedrichs_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_LaxWendroff_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_RungeKutta4_double.vti
  Patterns/Experiments/TimHutton/NumericalMethods/advection_staggeredLeapfrog_double.vti
  Patterns/Experiments/DanWills/cglrd_ramps_example.vti
  Patterns/Experiments/DanWills/grayscott-historyWave_fuseWorms.vti
  Patterns/Experiments/DanWills/grayscott-historyWave_moreLifelike.vti
  Patterns/Experiments/DanWills/grayscott-historyWaveDC_solitonsAndWorms_init.vti
  Patterns/Experiments/DanWills/orbits_explodey_init.vti
  Patterns/Experiments/DanWills/orbits_sharpWaves-init.vti
  Patterns/Experiments/DanWills/grayscott-historyWave_coralGrow.vti
  Patterns/Experiments/DanWills/grayscott-historyWaveDC_movingWormsAndSolitons.vti
  Patterns/Experiments/DanWills/orbitsNova_buildAndDestroyBlobs.vti
  Patterns/Experiments/DanWills/orbits_epochs.vti
  Patterns/Experiments/DanWills/grayscott-historyWaveDC_dualPhaseSolitonWormFlow.vti
  Patterns/Experiments/DanWills/heat-equation-boiling-history-wave_glidersUnstable.vti
  Patterns/Experiments/DanWills/grayscott-historyWaveDC_fibrousHistoryFill_wavePartition_init2.vti
  Patterns/Experiments/DanWills/grayscott-historyWave_ddd_bubbleDynasties_init.vti
  Patterns/Experiments/DanWills/grayscott-historyWave_temporalWeb_mgsz_init.vti
  Patterns/Experiments/DanWills/grayscott-withWaveCoupling_ddd_cauli_init.vti
  Patterns/Experiments/DanWills/grayscott-evolvingMask_ddd_frothierEchoes_init.vti
  Patterns/Experiments/DanWills/grayscott-historyWave_fuseWorms-wiggletrains_init.vti
  Patterns/Experiments/DanWills/orbits_nova_ddd_chicken_init.vti
  Patterns/Experiments/DanWills/orbits_ddd_questionSpaceships-answer_init_djw.vti
  Patterns/Experiments/DanWills/orbits_ddd_epochs_init.djw.vti
  Patterns/Experiments/DanWills/grayscott-sharpenTweak_cellTrailDetail_512px_init.vti
  Patterns/Experiments/SimonGladman/vermiformSolitons.vti
  Patterns/Experiments/CornusAmmonis/MandelbrotWorms.vti
  Patterns/Experiments/CornusAmmonis/mutually-catalytic-pinwheels.vti
  Patterns/Experiments/CornusAmmonis/mutually-catalytic-spots-second-order-sobel.vti
  Patterns/Experiments/CornusAmmonis/pearson-chained-edge-detectors.vti
  Patterns/Experiments/CornusAmmonis/surfing-solitons.vti
  Patterns/Experiments/CornusAmmonis/tip-splitting-web.vti
  Patterns/Experiments/CornusAmmonis/worms2.vti
  Patterns/Experiments/CornusAmmonis/bz-warpsharp.vti
  Patterns/Experiments/CornusAmmonis/smoke.vti
  Patterns/Experiments/CornusAmmonis/smoke-ising.vti
  Patterns/Experiments/CornusAmmonis/splats.vti
  Patterns/Experiments/DaveMann/smiley_faces.vti
  Patterns/Experiments/TihaVonGhyczy/RockScissorPaper.vti
  Patterns/Experiments/TihaVonGhyczy/sobel_waves.vti
  Patterns/Agmon2014/cells.vti      Patterns/Agmon2014/cells_double.vti
  Patterns/Agmon2014/oil-water.vti
  Patterns/Guo2014/guo.vti
  Patterns/Maginu1975/maginu_parameter_map.vti
  Patterns/Kryuchkov2020/Drosophila_corneal_nanocoatings.vti
  Patterns/RosenzweigMacArthur1963/hk_parameter_map.vti
  Patterns/RosenzweigMacArthur1963/hm_parameter_map.vti
  Patterns/RosenzweigMacArthur1963/predator-prey.vti
  Patterns/RosenzweigMacArthur1963/predator-prey_1D.vti
  Patterns/LotkaVolterra1926/Lotka-Volterra.vti      Patterns/LotkaVolterra1926/Lotka-Volterra_1D.vti
  Patterns/Kobayashi1993/crystal.vti                 Patterns/Kobayashi1993/crystals.vti
  Patterns/Kobayashi1993/laplacian_growth.vti
  Patterns/Kobayashi1993/laplacian_growth_3D.vti     Patterns/Kobayashi1993/laplacian_growth_3D_corner.vti
  Patterns/Morozov2008/Fig2.vti      Patterns/Morozov2008/Fig4.vti      Patterns/Morozov2008/Fig5678_delta_map.vti
  Patterns/KuramotoSivashinsky1978/Kuramoto-Sivashinsky.vti
  Patterns/KuramotoSivashinsky1978/Kuramoto-Sivashinsky_multistable.vti
  Patterns/KuramotoSivashinsky1978/Kuramoto-Sivashinsky_travelling_waves.vti
  Patterns/KuramotoSivashinsky1978/Kuramoto-Sivashinsky_travelling_waves2.vti
  Patterns/KortewegDeVries1895/kdv.vti
  Patterns/KardarParisiZhang1986/erosion.vti
  Patterns/KardarParisiZhang1986/uniform_snowfall.vti
  Patterns/KardarParisiZhang1986/drainage_erosion.vti
  Patterns/shallow_water_equations.vti
)

set( HELP_FILES
  Help/about.gif              Help/about.html
  Help/action.html            Help/credits.html
  Help/file.html              Help/help.html
  Help/mouse.html             Help/quickstart.html
  Help/tips.html              Help/changes.html
  Help/edit.html              Help/formats.html
  Help/index.html             Help/problems.html
  Help/view.html              Help/introduction.html
  Help/writing_new_rules.html
)

set( OTHER_FILES
  ./README.txt
  ./COPYING.txt
  ./TODO.txt
  ./BUILD.txt
  ./CMakeLists.txt
  src/FindOpenCL.cmake
  src/Doxyfile.in
  resources/logo.png
  resources/Icons/22px/icon-pointer.png
  resources/Icons/22px/draw-freehand.png
  resources/Icons/22px/draw-brush.png
  resources/Icons/22px/color-picker.png
  resources/Icons/22px/document-new.png
  resources/Icons/22px/document-open.png
  resources/Icons/22px/document-save.png
  resources/Icons/22px/document-revert.png
  resources/Icons/22px/media-playback-start_green.png
  resources/Icons/22px/media-playback-pause_red.png
  resources/Icons/22px/media-seek-forward.png
  resources/Icons/22px/media-seek-backward.png
  resources/Icons/22px/media-skip-backward_modified.png
  resources/Icons/22px/media-record.png
  resources/Icons/22px/system-run.png
  resources/Icons/22px/list-add_gray.png
  resources/Icons/22px/camera-photo.png
  resources/Icons/32px/icon-pointer.png
  resources/Icons/32px/draw-freehand.png
  resources/Icons/32px/draw-brush.png
  resources/Icons/32px/color-picker.png
  resources/Icons/32px/document-new.png
  resources/Icons/32px/document-open.png
  resources/Icons/32px/document-save.png
  resources/Icons/32px/document-revert.png
  resources/Icons/32px/media-playback-start_green.png
  resources/Icons/32px/media-playback-pause_red.png
  resources/Icons/32px/media-seek-forward.png
  resources/Icons/32px/media-seek-backward.png
  resources/Icons/32px/media-skip-backward_modified.png
  resources/Icons/32px/media-record.png
  resources/Icons/32px/system-run.png
  resources/Icons/32px/list-add_gray.png
  resources/Icons/32px/camera-photo.png
  resources/Cursors/pencil-cursor.png
  resources/Cursors/brush-cursor.png
  resources/Cursors/picker-cursor.png
  Scripts/Houdini/Dop_rd_object.hdanc
  Scripts/Houdini/Dop_rd_solver.hdanc
  Scripts/Houdini/Object_rd_template_object.hdanc
  Scripts/Houdini/README.md
  Scripts/Houdini/Sop_rd_object_post_process.hdanc
)

#-------------------------------------------VTK----------------------------------------------

find_package( VTK NO_MODULE REQUIRED )
if( VTK_VERSION VERSION_LESS "7.0.0" )
  set( VTK_COMPONENTS
    vtkCommonColor
    vtkCommonCore
    vtkFiltersCore
    vtkFiltersExtraction
    vtkFiltersGeometry
    vtkFiltersModeling
    vtkFiltersSources
    vtkFiltersTexture
    vtkImagingColor
    vtkImagingStencil
    vtkInteractionStyle
    vtkIOImage
    vtkIOGeometry
    vtkIOPLY
    vtkIOXML
    vtkRenderingCore
    vtkRenderingAnnotation
    vtkRenderingFreeType
    vtkRenderingOpenGL
  )
elseif( VTK_VERSION VERSION_LESS "8.90.0" )
  set( VTK_COMPONENTS
    vtkCommonColor
    vtkCommonCore
    vtkFiltersCore
    vtkFiltersExtraction
    vtkFiltersGeometry
    vtkFiltersModeling
    vtkFiltersSources
    vtkFiltersTexture
    vtkImagingColor
    vtkImagingStencil
    vtkInteractionStyle
    vtkIOImage
    vtkIOGeometry
    vtkIOPLY
    vtkIOXML
    vtkRenderingCore
    vtkRenderingAnnotation
    vtkRenderingFreeType
    vtkRenderingOpenGL2
  )
else()
  set( VTK_COMPONENTS
    CommonColor
    CommonCore
    FiltersCore
    FiltersExtraction
    FiltersGeometry
    FiltersModeling
    FiltersSources
    FiltersTexture
    ImagingColor
    ImagingStencil
    InteractionStyle
    IOImage
    IOGeometry
    IOPLY
    IOXML
    RenderingCore
    RenderingAnnotation
    RenderingFreeType
    RenderingOpenGL2
  )
endif()
find_package( VTK COMPONENTS ${VTK_COMPONENTS} REQUIRED )
if( VTK_VERSION VERSION_LESS "8.90.0" )
  include( ${VTK_USE_FILE} )
endif()

#-------------------------------------------wxWidgets----------------------------------------------

if( APPLE )
  # on macOS you might want to to use locally installed wxWidgets headers and libs:
  # set( wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config )
  # set( wxWidgets_wxrc_EXECUTABLE /usr/local/bin/wxrc )         # not used, but no harm leaving it in
elseif(UNIX)
  # remove -rdynamic from link options on Linux to reduce size by about 1.2MB
  set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" )
endif()

# wxWidgets is required to build the project
if(UNIX AND NOT APPLE)
  SET(WXGLCANVASLIBS "gl")
endif()
FIND_PACKAGE( wxWidgets COMPONENTS html aui ${WXGLCANVASLIBS} core adv base REQUIRED)
INCLUDE( ${wxWidgets_USE_FILE} )

if( MSVC )
  # To avoid link errors in Release build in Visual Studio: unresolved external symbol onAssert
  # Possibly because we build wxWidgets with static linking?
  # This flag still allows debugging into wxWidgets code.
  add_definitions( -DwxDEBUG_LEVEL=0 )
endif()

#-------------------------------------------wxVTK----------------------------------------------

# The following allows you to access wxGLCanvas for GTK
if(WIN32)
  set(GUI_EXECUTABLE WIN32)
else()
  if(APPLE)
    SET(GUI_EXECUTABLE MACOSX_BUNDLE)
    IF(VTK_USE_COCOA)
      SET_SOURCE_FILES_PROPERTIES(
        src/gui/wxVTKRenderWindowInteractor.cxx
        PROPERTIES COMPILE_FLAGS "-ObjC++")
    ENDIF(VTK_USE_COCOA)
  else()
    find_package(PkgConfig)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_libraries(${GTK3_LIBRARIES})
  endif()
endif()

#-------------------------------------------OpenCL----------------------------------------------

set( CMAKE_MODULE_PATH ${Ready_SOURCE_DIR}/src )
# (we include our own FindOpenCL.cmake until the time that CMake comes with its own)

# we need to build against OpenCL
find_package( OpenCL REQUIRED )
include_directories( ${OPENCL_INCLUDE_DIRS} )
if( APPLE )
  link_libraries( ${OPENCL_LIBRARIES} ) # on MacOSX we assume that OpenCL is available
endif()

#---------------copy installation files to build folder (helps with testing)--------------------

foreach( file ${PATTERN_FILES} ${HELP_FILES} ${RESOURCES} ${OTHER_FILES} )
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${file}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
  )
  list( APPEND files_dest "${CMAKE_CURRENT_BINARY_DIR}/${file}" )
endforeach()

add_custom_target( CopyFiles ALL DEPENDS ${files_dest} )

#-------------------------------- build ------------------------------------------------------

# link the C runtime statically
set( FLAG_VARS
  CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
  CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
)
foreach( flag_var ${FLAG_VARS} )
  string( REGEX REPLACE "/MD" "/MT" new_flags "${${flag_var}}" )
  set( ${flag_var} "${new_flags}" CACHE STRING "" FORCE )
endforeach()

set( USE_SSE "YES" CACHE BOOL "Set to false if your CPU doesn't have SSE support.")
if( USE_SSE )
  # enable SSE to allow us to set flags to avoid denormals, and relax the floating-point accuracy for speed
  if( MSVC )
    if( CMAKE_CL_64 )
      set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fp:fast" )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast" )
    else() # avoid warning on x64, which always comes with SSE2
      set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2 /fp:fast" )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2 /fp:fast" )
    endif()
  else()
    add_definitions( -msse2 -ffast-math )
  endif()
  add_definitions( -DUSE_SSE )
endif()

if( APPLE )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
if( APPLE OR UNIX )
  # use same settings as in makefiles
  add_definitions( -D_LARGE_FILES )
endif()

# workaround to suppress multiple clang warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND VTK_VERSION VERSION_LESS "8.90.0")
  add_compile_options(-Wno-inconsistent-missing-override)
endif()

# create base library used by all executables
add_library( readybase STATIC ${BASE_SOURCES} )
target_include_directories( readybase PUBLIC src/readybase src/extern )
target_link_libraries( readybase ${VTK_LIBRARIES} )
if( VTK_VERSION VERSION_GREATER_EQUAL "8.90.0" )
  vtk_module_autoinit(
    TARGETS readybase
    MODULES ${VTK_LIBRARIES}
  )
endif()

# create command-line utility
add_executable( ${CMD_NAME} ${CMD_SOURCES} )
target_include_directories( ${CMD_NAME} PRIVATE src/extern/cxxopts-2.2.1 )
target_link_libraries( ${CMD_NAME} readybase ${CMAKE_DL_LIBS})

# create GUI application
add_executable( ${APP_NAME} ${GUI_EXECUTABLE} ${GUI_SOURCES} ${RESOURCES} )
target_include_directories( ${APP_NAME} PRIVATE src/gui resources )
target_link_libraries( ${APP_NAME} readybase ${wxWidgets_LIBRARIES} )

if( APPLE )
  # create Info.plist (using Info.plist.in) and PkgInfo files inside .app bundle
  add_custom_target( app_bundle
    COMMAND sed -e "s/VERSION/${READY_VERSION}/" ${CMAKE_SOURCE_DIR}/resources/Info.plist.in >Ready.app/Contents/Info.plist
    COMMAND echo -n "APPLReDy" >Ready.app/Contents/PkgInfo
  )
  add_dependencies( ${APP_NAME} app_bundle )

  # copy *.icns files into Resources directory inside .app bundle
  set_source_files_properties( ${CMAKE_SOURCE_DIR}/resources/app.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
  set_source_files_properties( ${CMAKE_SOURCE_DIR}/resources/file.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )

  # remove unreachable functions and data, and don't add debug info (reduces app size by about 12MB)
  target_link_libraries( ${APP_NAME} -Wl,-dead_strip -Wl,-S )

  # horrible hack to avoid Undefined symbol "___isPlatformVersionAtLeast" using wxMac 3.1.3 on Mojave
  # set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/11.0.0/lib/darwin/libclang_rt.osx.a" )
endif()

# avoid security warnings
if( MSVC )
  add_definitions( /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS )
endif()

if( CMAKE_COMPILER_IS_GNUCXX )
  set( CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s" )   # strip release binary, for smaller file size
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl" )                 # allow dynamic linking (found needed on Gentoo)
endif()

#----------------------------------------doxygen------------------------------------------------

find_package( Doxygen )
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()
# (if doxygen is found then 'make doc' should produce html documentation of the source code)

#------------------------------------------test-------------------------------------------------

enable_testing()

# Test that we can run rdy and print out the help information
add_test(
  NAME rdy_help
  COMMAND ${CMD_NAME} -h
)

# Test that we can load each pattern and compile its kernel
foreach(pattern_file ${PATTERN_FILES})
  add_test(
    NAME load_${pattern_file}
    COMMAND ${CMD_NAME} -i "${pattern_file}" -v
  )
endforeach()

# Test that we can run one pattern for 100 steps and save it out
add_test(
  NAME rdy_run
  COMMAND ${CMD_NAME} -i Patterns/CPU-only/grayscott_1D.vti -n 100 -o gs_100.vti -v
)

# And then read it in again
add_test(
  NAME rdy_run2
  COMMAND ${CMD_NAME} -i gs_100.vti -v
)

#----------------------------------------install------------------------------------------------

# put Ready in the root of the installation folder instead of in "bin"
install( TARGETS ${APP_NAME} ${CMD_NAME} DESTINATION "." )

# install our source files, resource files, pattern files, help files and text files
foreach( source_file ${BASE_SOURCES} ${GUI_SOURCES} ${CMD_SOURCES} ${RESOURCES} ${PATTERN_FILES} ${HELP_FILES} ${OTHER_FILES} )
  get_filename_component( path_name "${source_file}" PATH )
  install( FILES "${source_file}" DESTINATION ${path_name} )
endforeach()

#----------------------------------------package----------------------------------------------

if( APPLE )
  set( CPACK_SYSTEM_NAME "Mac" )  # nicer than "Darwin"
elseif( UNIX )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( CPACK_SYSTEM_NAME "Linux-64bit" )
  else()
    set( CPACK_SYSTEM_NAME "Linux-32bit" )
  endif()
elseif( WIN32)
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( CPACK_SYSTEM_NAME "Windows-64bit" )
  else()
    set( CPACK_SYSTEM_NAME "Windows-32bit" )
  endif()
endif()
if( NOT USE_SSE )
  set( CPACK_SYSTEM_NAME "${CPACK_SYSTEM_NAME}-noSSE" )
endif()
set( CPACK_GENERATOR "ZIP" )
set( CPACK_PACKAGE_VERSION "${READY_VERSION}" )
set( CPACK_SOURCE_GENERATOR "ZIP" )
include( CPack )
