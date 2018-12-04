#include <vector>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include "LogicDesign.hpp"
#include "Util.hpp"
using namespace std;
using namespace SynthFramework::Polymer;
using namespace SynthFramework;

void PrintUsage(char *xname) {
    cerr << "Usage:" << endl << xname << " [options] file" << endl;
    cerr << "Valid options:" << endl;
    cerr << "\t-v : enable debug output" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    cout << "POLYMER version ";
    Console_SetForeColour(COLOUR_CYAN);
    cout << "0.1-alpha" << endl;
    Console_ResetColour();
    cout << "(C) 2016 David Shah" << endl;
    cout << "This program is provided without warranty" << endl;
    if(argc < 2) {
        PrintUsage(argv[0]);
    }
    int argCount = 0;
    string inFileName;
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            if(argv[i][1] == 'v') {
                verbosity = MSG_DEBUG;
            } else if(argv[i][1] == '-') {
                //todo: more arguments
            } else {
                PrintUsage(argv[0]);
            }
        } else {
            if(argCount == 0) {
                inFileName = string(argv[i]);
                argCount++;
            }
        }

    }
    if(argCount != 1)
        PrintUsage(argv[0]);

    LogicDesign *des = new LogicDesign();

    ifstream codein(inFileName);
    string entname = inFileName;
    string folder = "";
    //extract filename to use as entity name
    if(entname.find_last_of('/') != string::npos) {
        folder = entname.substr(0,entname.find_last_of('/'));
        entname = entname.substr(entname.find_last_of('/') + 1);
        des->searchPath.push_back(folder); //search source file folder first for submodules
    }
    des->searchPath.push_back("."); //then search current working directory

    if(entname.find('.') != string::npos) {
        entname = entname.substr(0, entname.find('.'));
    }

    if(entname == "") {
        entname = "polymer_design";
    }
    clock_t start = clock();

    des->designName = entname;
    //des->technology = new Artix7Technology();
    des->LoadDesign(string((istreambuf_iterator<char>(codein)), istreambuf_iterator<char>()));
    des->SynthesiseAndOptimiseDesign();
    des->AnalyseTiming();
    des->PipelineDesign();
    des->AnalysePostPipelineTiming();

    PrintMessage(MSG_NOTE, des->technology->PrintResourceUsage(des));

    ofstream vhdlout(string(argv[1]) + ".vhd");
    vhdlout << des->GenerateVHDL();
    double time = ((double)(clock() - start)) / CLOCKS_PER_SEC;

    PrintMessage(MSG_NOTE, "total synthesis time was " + to_string(time) + " seconds");

    return 0;
}
