#include<sys/stat.h>
#include<cstring>
#include<iostream>
#include<istream>
#include<memory>
#include<CLI/CLI.hpp>
#include"IntentMgr.h"
#include"VarMgr.h"
#include"Domain.h"
#include"Stopwatch.h"
using namespace std;

int main(int argc, char** argv) {

    CLI::App app {
        "rims4ltlf: a tool for LTLf intentions management"
    };

    std::string domain_file, problem_file, intentions_file;
    bool print_domain = false, save_results = false; 

    CLI::Option* domain_file_opt =
        app.add_option("-d,--domain-file", domain_file, "Path to PDDL domain file") ->
        required() -> check(CLI::ExistingFile);

    CLI::Option* problem_file_opt =
        app.add_option("-p,--problem-file", problem_file, "Path to PDDL problem file") ->
        required() -> check(CLI::ExistingFile);

    CLI::Option* intentions_file_opt =
        app.add_option("-i,--intentions-file", intentions_file, "Path to LTLf intentions file") ->
        required() -> check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    std::shared_ptr<Syft::VarMgr> var_mgr = std::make_shared<Syft::VarMgr>();

    Syft::IntentMgr intent_mgr(var_mgr, domain_file, problem_file, intentions_file);

    intent_mgr.run();

    return 0;
}   
