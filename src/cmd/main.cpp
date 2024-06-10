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

// cxxopts:
#include <cxxopts.hpp>

// STL:
#include <cstdlib>
#include <iostream>

// readybase:
#include <AbstractRD.hpp>
#include <OpenCL_utils.hpp>
#include <OpenCLImageRD.hpp>
#include <Properties.hpp>
#include <scene_items.hpp>
#include <SystemFactory.hpp>

using namespace std;

// -------------------------------------------------------------------------------------------------------------

// Please keep the following in sync with the blurb below!:
// -------------------------------------------------------------------------------------------------------------
/*
        Ready commandline utility that uses readybase as a processing (and data-retrieval) back-end.

        It responds to various commandline arguments to load and print out parts of the system,
        and can also still be used (like it used to work) to iterate the system to a specified number
        of steps, and save it to a new vti file.

        The various print options can facilitate the import of ready simulations into other applications, (such
        as Houdini), without the need to actually link the ready libraries. This includes reagent initial-states,
        (via -m), be ready for some large lumps of text on stdout when using that argument.

        Please let the Ready team (especially Dan Wills) know if there is something that you wish to print
        that currently isn't supported.
*/
// -------------------------------------------------------------------------------------------------------------
void printSeparator()
{
    cout << "================================\n";
}

int main(int argc,char *argv[])
{
    vtkObject::GlobalWarningDisplayOff();

    std::string blurb = "Ready commandline utility that uses readybase as a processing (and data-retrieval) back-end.\n"
                        "\n"
                        "It responds to various commandline arguments to load and print out parts of the system,\n"
                        "and can also still be used (like it used to work) to iterate the system to a specified number\n"
                        "of steps, and save it to a new vti file.\n"
                        "\n"
                        "The various print options can facilitate the import of ready simulations into other applications, (such\n"
                        "as Houdini), without the need to actually link the ready libraries. This includes reagent initial-states,\n"
                        "(via -m), be ready for some large lumps of text on stdout when using that argument.\n"
                        "\n"
                        "Please let the Ready team (especially Dan Wills) know if there is something that you wish to print\n"
                        "that currently isn't supported.\n";

    int numiter = 1000;
    bool print_kernel = false;
    bool print_formula = false;
    bool print_rule_info = false;
    bool print_reagent_info = false;
    bool print_parameter_info = false;
    bool print_render_settings = false;
    bool print_formula_description = false;
    bool print_initial_state_images = false;
    std::string vti_in;
    std::string vti_out;
    int opencl_platform = 0;
    int opencl_device = 0;
    bool verbose = false;

    cxxopts::Options options("rdy", "Command-line version of Ready");
    try
    {
        options.add_options()
            ("h,help", "Print the help message")
            // Went to 0 iterations by default. Need to provide option for flags
            ("n,num-iterations", "Number of iterations to run before saving", cxxopts::value<int>(numiter)->default_value("0"))
            ("k,print-kernel", "Print (full) OpenCL kernel source (when possible)", cxxopts::value<bool>(print_kernel)->default_value("false"))
            ("f,print-formula", "Print kernel formula (when possible)", cxxopts::value<bool>(print_formula)->default_value("false"))
            ("u,print-rule-info", "Print rule info", cxxopts::value<bool>(print_rule_info)->default_value("false"))
            ("r,print-reagent-info", "Print reagent info", cxxopts::value<bool>(print_reagent_info)->default_value("false"))
            ("p,print-parameter-info", "Print parameter info", cxxopts::value<bool>(print_parameter_info)->default_value("false"))
            ("s,print-render-settings", "Print render Settings", cxxopts::value<bool>(print_render_settings)->default_value("false"))
            ("d,print-formula-description", "Print formula Description", cxxopts::value<bool>(print_formula_description)->default_value("false"))
            ("m,print-initial-state-images", "Print initial state images (Warning: May be large!)", cxxopts::value<bool>(print_initial_state_images)->default_value("false"))
            ("i,vti-in", "VTI file to load (required)", cxxopts::value<string>(vti_in))
            ("o,vti-out", "VTI file to save (optional)", cxxopts::value<string>(vti_out))
            // TODO don't crash if incorrect, fail more gracefully!
            ("l,opencl-platform", "OpenCL platform number (Currently will crash if incorrect!)", cxxopts::value<int>(opencl_platform))
            ("g,opencl-device", "OpenCL device number (Currently will crash if incorrect!)", cxxopts::value<int>(opencl_device))
            ("v,verbose", "Verbose output.", cxxopts::value<bool>(verbose)->default_value("false"))
            ;
    }
    catch (const cxxopts::OptionSpecException& e)
    {
        cout << "Caught spec exception: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        cout << "Caught unknown exception" << endl;
        return EXIT_FAILURE;
    }

    try {
        const cxxopts::ParseResult args = options.parse(argc, argv);
        if (args.count("help"))
        {
            cout << blurb << endl;
            cout << options.help() << endl;
            return EXIT_SUCCESS;
        }
        if (args.count("vti-in") == 0)
        {
            cout << "Missing required argument: vti-in" << endl;
            cout << options.help() << endl;
            return EXIT_FAILURE;
        }
    }
    catch (const cxxopts::OptionParseException& e)
    {
        cout << "Argument error: " << e.what() << endl;
        cout << options.help() << endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        cout << "Caught unknown exception" << endl;
        return EXIT_FAILURE;
    }

    const bool file_exists = static_cast<bool>(std::ifstream(vti_in));
    if (!file_exists)
    {
        cout << "File does not exist: " << vti_in << endl;
        return EXIT_FAILURE;
    }

    const bool is_opencl_available = OpenCL_utils::IsOpenCLAvailable();
    if( is_opencl_available )
    {
        if (verbose)
        {
            cout << "OpenCL found.\n";
        }
    } else {
        // Still print (despite not verbose) since it's a warning:
        cout << "Warning: OpenCL not found! (This may not bode well for what's about to happen..).\n";
    }

    Properties render_settings("render_settings");
    SetDefaultRenderSettings(render_settings);

    unique_ptr<AbstractRD> system;
    try
    {
        // Read the file
        if (verbose)
        {
            cout << "Loading VTI file: " << vti_in << "...\n";
        }
        bool warn_to_update;
        try {
            system = SystemFactory::CreateFromFile( vti_in.c_str(), is_opencl_available, opencl_platform,
                                                    opencl_device, render_settings, warn_to_update );
            if (verbose)
            {
                cout << "Loaded VTI: " << vti_in.c_str() << "\n";
            }

            system->Update( 0 );
            if (verbose)
            {
                cout << "System updated to zeroth step..\n";
            }

            if ( print_reagent_info )
            {
                int num_chemicals = system->GetNumberOfChemicals();
                int xres = system->GetX();
                int yres = system->GetY();
                int zres = system->GetZ();
                int arena_dimensionality = system->GetArenaDimensionality();
                cout << "\n";
                cout << "Reagent info:\n";
                printSeparator();
                //cout << "================================\n";
                cout << "num_chemicals=" << num_chemicals << "\n";
                cout << "arena_dimensionality=" << arena_dimensionality << "\n";
                cout << "xres=" << xres << "\n";
                cout << "yres=" << yres << "\n";
                cout << "zres=" << zres << "\n";
                cout << "================================\n";
            }

            if ( print_kernel )
            {
                cout << "\n";
                cout << "Kernel source:\n";
                cout << "================================\n";
                cout << system->GetKernel();
                cout << "================================\n";
            }

            if ( print_formula )
            {
                cout << "\n";
                cout << "Kernel formula:\n";
                cout << "================================\n";
                // Maybe work out how to strip leading whitespace from lines in this:
                cout << system->GetFormula();
                cout << "================================\n";
            }

            if ( print_rule_info )
            {
                cout << "\n";
                cout << "Rule info:\n";
                cout << "================================\n";
                cout << "type=" << system->GetRuleType() << "\n";
                cout << "name=" << system->GetRuleName() << "\n";
                cout << "neighborhood_type=" << system->GetNeighborhoodType() << "\n";
                cout << "================================\n";
            }
            if ( print_parameter_info )
            {
                int nparams = system->GetNumberOfParameters();

                cout << "\n";
                cout << "Parameter info:\n";
                cout << "================================\n";
                for ( int ix=0; ix < nparams; ix++ )
                {
                    std::string parm_name = system->GetParameterName( ix );
                    float parm_value = system->GetParameterValue( ix );
                    cout << parm_name << "=" << parm_value << "\n";
                }
                cout << "================================\n";
            }

            if ( print_render_settings )
            {
                cout << "\n";
                cout << "Render settings:\n";
                cout << "================================\n";

                int num_properties = render_settings.GetNumberOfProperties();
                cout << "Number of properties is: " << num_properties << "\n";
                for (int i=0;i<num_properties;i++)
                {
                    Property this_property = render_settings.GetProperty( i );

                    std::string property_name = this_property.GetName();
                    std::string property_type = this_property.GetType();

                    cout << "    name: " << property_name << ", type: " << property_type << ", value:";
                    if (property_type == "int")
                    {
                        int property_value = this_property.GetInt();
                        cout << " " << property_value << "\n";
                    } else if (property_type == "bool") {
                        bool property_value = this_property.GetBool();
                        cout << " " << property_value << "\n";
                    } else if (property_type == "float") {
                        float property_value = this_property.GetFloat();
                        cout << " " << property_value << "\n";
                    } else if (property_type == "color") {
                        float property_value_r, property_value_g, property_value_b;
                        this_property.GetColor( property_value_r, property_value_g, property_value_b);
                        cout << " (" << property_value_r << "," << property_value_g << "," << property_value_b << ")\n";
                    } else if (property_type == "chemical") {
                        std::string property_value = this_property.GetChemical();
                        cout << " " << property_value << "\n";
                    } else if (property_type == "axis") {
                        std::string property_value = this_property.GetAxis();
                        cout << " " << property_value << "\n";
                    }
                }
                cout << "================================\n";
            }

            if ( print_formula_description )
            {
                cout << "\n";
                cout << "Formula description:\n";
                cout << "================================\n";
                cout << system->GetDescription();
                cout << "================================\n";
            }

            if ( print_initial_state_images )
            {
                int num_chemicals = system->GetNumberOfChemicals();
                cout << "\n";
                cout << "Parameter initial-state images:\n";
                cout << "================================\n";
                // I'm just going to gloss over numberOfScalarComponents for the moment! (just hope it's always 1 or above!)
                // so given that, in theory num_chemicals and nc should be equal!

                // Update to get everything initialised (and copy in the initial states)..
                system->Update(0);

                for ( int ix=0; ix < num_chemicals; ix++ )
                {
                    cout << "\n";
                    cout << "nchem: " << ix << "\n";

                    cout << "xres=" << system->GetX() << "\n";
                    cout << "yres=" << system->GetY() << "\n";
                    cout << "zres=" << system->GetZ() << "\n";

                    // not in bytes!
                    unsigned long reagent_size = system->GetX() * system->GetY() * system->GetZ();

                    cout << "Reagent size is: " << reagent_size << "\n";

                    const vector<float> rd_data = system->GetData( ix );

                    cout << "\nRD data for reagent " << ix << ": [ ";
                    for (unsigned long rix=0; rix<rd_data.size(); rix++)
                    {
                        cout << rd_data[rix];
                        if ( rix < rd_data.size()-1 )
                        {
                            cout << ",";
                        }
                    }
                    cout << " ]\n";
                }
                cout << "================================\n";
            }
        } catch(const exception& e) {
            cout << "Error creating system!:\n" << e.what() << "\n";
            return EXIT_FAILURE;
        }

        if( warn_to_update )
            cout << "This pattern was created with a newer version of Ready. You should update your copy.\n";

        if ( numiter > 0 )
        {
            cout << "Run the simulation for " << numiter << " steps...\n";
            system->Update( numiter );

            if ( !vti_out.empty() )
            {
                // save something out
                cout << "Saving file as " << vti_out << " ...\n";
                try {
                    system->SaveFile( vti_out.c_str(), render_settings, false );
                } catch(const exception& e) { //doesn't catch segfaults! :/
                    cout << "Something went wrong when saving file to: " << vti_out.c_str() << "\n";
                    cout << e.what() << "\n";
                }
            } else {
                cout << "Output file not specified, not saving anything.\n";
            }
        } else {
            if (verbose)
            {
                cout << "Zero iterations, simulation skipped.\n";
            }
        }
    }
    catch(const exception& e)
    {
        cout << "Error:\n" << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// -------------------------------------------------------------------------------------------------------------
