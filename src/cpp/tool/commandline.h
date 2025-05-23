/*
 * Copyright (c) 2020-2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2020-2025 NVIDIA CORPORATION & AFFILIATES
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Visualizing and Communicating Errors in Rendered Images
// Ray Tracing Gems II, 2021,
// by Pontus Andersson, Jim Nilsson, and Tomas Akenine-Moller.
// Pointer to the chapter: https://research.nvidia.com/publication/2021-08_Visualizing-and-Communicating.

// Visualizing Errors in Rendered High Dynamic Range Images
// Eurographics 2021,
// by Pontus Andersson, Jim Nilsson, Peter Shirley, and Tomas Akenine-Moller.
// Pointer to the paper: https://research.nvidia.com/publication/2021-05_HDR-FLIP.

// FLIP: A Difference Evaluator for Alternating Images
// High Performance Graphics 2020,
// by Pontus Andersson, Jim Nilsson, Tomas Akenine-Moller,
// Magnus Oskarsson, Kalle Astrom, and Mark D. Fairchild.
// Pointer to the paper: https://research.nvidia.com/publication/2020-07_FLIP.

// Code by Pontus Ebelin (formerly Andersson), Jim Nilsson, and Tomas Akenine-Moller.

#pragma once

#include <map>
#include <set>
#include <vector>
#include <cassert>
#include <sstream>
#include <algorithm>

template<typename T>
class unique_vector;

template<typename T>
class unique_vector_iterator
{
private:
    size_t mPosition;
    const unique_vector<T>* mpUniqueVector;

public:
    unique_vector_iterator(const unique_vector<T>* pVector, size_t position) : mPosition(position), mpUniqueVector(pVector) { }
    bool operator!=(const unique_vector_iterator& other) const { return this->mPosition != other.mPosition; }
    T operator*(void) const { return this->mpUniqueVector->at(this->mPosition); }
    const unique_vector_iterator& operator++(void) { this->mPosition++; return *this; }
    unique_vector_iterator operator+(size_t v) const { return unique_vector_iterator(this->mpUniqueVector, this->mPosition + v); }
};

template<typename T>
class unique_vector
{
public:
    typedef unique_vector_iterator<T> iterator;

private:
    std::vector<T> mVector;

public:
    void push_back(T value) { if (!this->contains(value)) this->mVector.push_back(value); }
    T at(size_t index) const { return this->mVector.at(index); }
    T operator[](size_t index) const { return this->mVector.at(index); }
    T& operator[](size_t index) { return this->mVector.at(index); }
    size_t size(void) const { return this->mVector.size(); }
    bool contains(T value) const { return std::find(this->mVector.begin(), this->mVector.end(), value) != mVector.end(); }

    iterator begin(size_t begin = 0) const { return iterator(this, begin); }
    iterator end(size_t end = std::string::npos) const { return iterator(this, (end != std::string::npos && end <= mVector.size() ? end : mVector.size())); }

    // Conversion to const std::vector.
    operator const std::vector<T>() const { return mVector; }
    operator std::vector<T>() { return mVector; }
};

typedef struct
{
    std::string longName;
    std::string shortName;
    int numArgs;  // -1: '+' if required, '*' if not required.
    bool required;
    std::string meta;
    std::string help;
    std::string value;
} commandline_option;

typedef struct
{
    std::string description;
    std::vector<commandline_option> options;
} commandline_options;

static const commandline_options getAllowedCommandLineOptions(const bool cpptool = true)
{
    commandline_options commandLineOptions =
    {
        "Compute FLIP between reference.<png|exr> and test.<png|exr>.\n"
        "Reference and test(s) must have same resolution and format.\n"
        "If pngs are entered, LDR-FLIP will be evaluated. If exrs are entered, HDR-FLIP will be evaluated.\n"
        "For HDR-FLIP, the reference is used to automatically calculate start exposure and/or stop exposure and/or number of exposures, if they are not entered.\n",
        {
        { "help", "h", 0, false, "", "show this help message and exit" },
        { "reference", "r", 1, true, "REFERENCE", "Relative or absolute path (including file name and extension) for reference image" },
        { "test", "t", -1, true, "TEST", "Relative or absolute path(s) (including file name and extension) for test image(s)" },
        { "pixels-per-degree", "ppd", 1, false, "PIXELS-PER-DEGREE", "Observer's number of pixels per degree of visual angle. Default corresponds to\nviewing the images at 0.7 meters from a 0.7 meter wide 4K display" },
        { "viewing-conditions", "vc", 3, false, "MONITOR-DISTANCE MONITOR-WIDTH MONITOR-WIDTH-PIXELS", "Distance to monitor (in meters), width of monitor (in meters), width of monitor (in pixels).\nDefault corresponds to viewing the monitor at 0.7 meters from a 0.7 meters wide 4K display" },
        { "tone-mapper", "tm", 1, false, "ACES | HABLE | REINHARD", "Tone mapper used for HDR-FLIP. Supported tone mappers are ACES, Hable, and Reinhard (default: ACES)" },
        { "num-exposures", "n", 1, false, "NUM-EXPOSURES", "Number of exposures between (and including) start and stop exposure used to compute HDR-FLIP" },
        { "start-exposure", "cstart", 1, false, "C-START", "Start exposure used to compute HDR-FLIP" },
        { "stop-exposure", "cstop", 1, false, "C-STOP", "Stop exposure used to comput HDR-FLIP" },
        { "verbosity", "v", 1, false, "VERBOSITY", "Level of verbosity (default: 2).\n0: no printed output,\n\t\t1: print mean FLIP error,\n\t\t2: print pooled FLIP errors, PPD, and evaluation time and (for HDR-FLIP) start and stop exposure and number of exposures"},
        { "basename", "b", 1, false, "BASENAME", "Basename for outfiles, e.g., error and exposure maps. Only compatible with one test image as input" },
        { "textfile", "txt", 0, false, "", "Save text file with pooled FLIP values (mean, weighted median and weighted 1st and 3rd quartiles as well as minimum and maximum error)" },
        { "csv", "c", 1, false, "CSV_FILENAME", "Write results to a csv file. Input is the desired file name (including .csv extension).\nResults are appended if the file already exists" },
        { "histogram", "hist", 0, false, "", "Save weighted histogram of the FLIP error map(s)" },
        { "y-max", "", 1, true, "", "Set upper limit of weighted histogram's y-axis" },
        { "log", "lg", 0, false, "", "Use logarithmic scale on y-axis in histogram" },
        { "exclude-pooled-values", "epv", 0, false, "", "Do not include pooled FLIP values in the weighted histogram" },
        { "save-ldr-images", "sli", 0, false, "", "Save all exposure compensated and tone mapped LDR images (png) used for HDR-FLIP" },
        { "save-ldrflip", "slf", 0, false, "", "Save all LDR-FLIP maps used for HDR-FLIP" },
        { "no-magma", "nm", 0, false, "", "Save FLIP error maps in grayscale instead of magma" },
        { "no-exposure-map", "nexm", 0, false, "", "Do not save the HDR-FLIP exposure map" },
        { "no-error-map", "nerm", 0, false, "", "Do not save the FLIP error map" },
        { "directory", "d", 1, false, "Relative or absolute path to save directory"},
    } };

    // Add C++ specific options.
    std::vector<commandline_option> cppSpecific = {};
    if (cpptool)
    {
        cppSpecific =
        {
            { "exit-on-test", "et", 0, false, "", "Do exit(EXIT_FAILURE) if the selected FLIP QUANTITY is greater than THRESHOLD"},
            { "exit-test-parameters", "etp", 2, false, "QUANTITY = {MEAN (default) | WEIGHTED-MEDIAN | MAX} THRESHOLD (default = 0.05) ", "Test parameters for selected quantity and threshold value (in [0,1]) for exit on test"},
        };
    }
    for (auto opt : cppSpecific)
    {
        commandLineOptions.options.push_back(opt);
    }

    return commandLineOptions;
}


class commandline
{
private:
    std::string mCommand;
    commandline_options mAllowedOptions;
    std::map<std::string, std::vector<std::string>> mOptions;
    std::vector<std::string> mArguments;
    bool mError = false;
    std::string mErrorString;

public:
    commandline() = default;

    commandline(int argc, char* argv[], const commandline_options& allowedOptions = {}) :
        mAllowedOptions(allowedOptions)
    {
        clear();
        parse(argc, argv, allowedOptions);
    }

    bool getError(void) const 
    {
        return mError;
    }

    std::string getErrorString(void) const
    {
        return mErrorString;
    }

    void clear(void)
    {
        mCommand = "";
        mOptions.clear();
        mArguments.clear();
    }

    bool parse(int argc, char* argv[], const commandline_options& allowedOptions = {})
    {
        mCommand = argv[0];

        std::string tOption = "";
        int atArg = 1;

        std::string longOptionName;
        std::string shortOptionName;
        while (atArg < argc)
        {
            bool atOption = false;
            bool optionHasArgument = false;
            int numOptionArguments = 0;

            std::string arg(argv[atArg]);

            if (arg[0] == '-')
            {
                bool bIsLong = (arg[1] == '-');
                std::string optionString = arg.substr(bIsLong ? 2 : 1);

                bool bFound = false;
                for (auto allowedOption : allowedOptions.options)
                {
                    longOptionName = allowedOption.longName;
                    shortOptionName = allowedOption.shortName;
                    if ((bIsLong && longOptionName == optionString) || (!bIsLong && shortOptionName == optionString))
                    {
                        bFound = true;
                        atOption = true;
                        tOption = optionString;
                        optionHasArgument = (allowedOption.numArgs != 0);
                        numOptionArguments = allowedOption.numArgs;
                        break;
                    }
                }

                if (atOption)
                {
                    if (!bFound)
                    {
                        this->mError = true;
                        this->mErrorString = "Option not found!";
                        return false;
                    }

                    if (optionHasArgument)
                    {
                        //  Eat option arguments.
                        atArg++;
                        while (atArg < argc && numOptionArguments != 0)
                        {
                            arg = argv[atArg];
                            if (arg[0] == '-' && numOptionArguments < 0)
                            {
                                break;
                            }
                            this->mOptions[longOptionName].push_back(arg);
                            if (numOptionArguments > 0)
                            {
                                numOptionArguments--;
                            }
                            atArg++;
                        }
                    }
                    else
                    {
                        this->mOptions[longOptionName].push_back("");
                        atArg++;
                    }
                }
                else
                {
                    this->mArguments.push_back(arg);
                    atArg++;
                }

            }
            else
            {
                this->mArguments.push_back(arg);
                atArg++;
            }
        }

        return true;
    }

    bool parse(std::string commandLine, const commandline_options& allowedOptions = {})
    {
        std::vector<std::string> vCommandLine;
        std::string command;
        std::stringstream ss(commandLine);

        while (ss >> command)
            vCommandLine.push_back(command);

        std::vector<char*> vpCommandLine;
        for (size_t i = 0; i < vCommandLine.size(); i++)
            vpCommandLine.push_back(const_cast<char *>(vCommandLine[i].c_str()));

        return parse(int(vpCommandLine.size()), &vpCommandLine[0], allowedOptions);
    }

    inline const std::string& getCommand(void) const
    {
        return mCommand;
    }

    inline size_t getNumArguments(void) const
    {
        return this->mArguments.size();
    }

    inline const std::string getArgument(size_t index) const
    {
        assert(index < this->mArguments.size());
        return this->mArguments[index];
    }

    const std::vector<std::string>& getArguments(void) const
    {
        return this->mArguments;
    }

    const std::string& getOptionValue(const std::string& option, size_t n = 0) const
    {
        return this->mOptions.at(option)[n];
    }

    std::vector<std::string>& getOptionValues(const std::string& option)
    {
        return (std::vector<std::string>&)this->mOptions.at(option);
    }

    const std::string& getOption(const std::string& option, size_t index) const
    {
        static const std::string emptyString;

        auto& options = this->mOptions.at(option);

        if (index < options.size())
        {
            return options[index];
        }

        return emptyString;
    }

    size_t getNumOptionValues(const std::string& option) const
    {
        return this->mOptions.at(option).size();
    }

    bool optionSet(const std::string& option) const
    {
        return this->mOptions.find(option) != this->mOptions.end();
    }

    void print(void)
    {
        commandline_options& ao = this->mAllowedOptions;

        std::string command = mCommand.substr(mCommand.find_last_of("/\\") + 1);

        size_t numOptionalOptions = 0;
        for (auto& o : ao.options)
        {
            if (o.required == false)
            {
                numOptionalOptions++;
            }
        }

        std::cout << "usage: " << command;
        if (numOptionalOptions > 0)
        {
            std::cout << " [OPTIONS]";
            if (numOptionalOptions > 1)
            {
                std::cout << "...";
            }
        }

        for (auto& o : ao.options)
        {
            if (o.required)
            {
                std::cout << " -" << o.shortName << "";
                if (o.numArgs != 0)
                {
                    std::cout << " " << o.meta;
                    if (o.numArgs == -1)
                    {
                        std::cout << " [...]";
                    }
                }
            }
        }
        std::cout << "\n\n";

        std::cout << ao.description << "\n\n";

        for (auto& o : ao.options)
        {
            if (o.shortName != "")
            {
                std::cout << " -" << o.shortName;
                if (o.longName != "")
                {
                    std::cout << ",";
                }
            }
            if (o.longName != "")
            {
                std::cout << " --" << o.longName;
            }

            if (o.numArgs != 0)
            {
                std::cout << " " << o.meta;
                if (o.numArgs == -1)
                {
                    std::cout << " [...]";
                }
            }
            std::cout << "\n";

            if (o.help != "")
            {
                size_t loc = o.help.find("\n");
                if (loc != std::string::npos)
                {
                    o.help.replace(loc, 1, "\n\t\t");
                }

                std::cout << "\t\t" << o.help << "\n";
            }
        }
    }
};
