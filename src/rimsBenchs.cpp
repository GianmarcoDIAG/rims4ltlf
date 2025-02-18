#include<sys/stat.h>
#include<cstring>
#include<iostream>
#include<istream>
#include<memory>
#include<CLI/CLI.hpp>
#include<filesystem>
#include"IntentMgr.h"
#include"VarMgr.h"
#include"Domain.h"
#include"Stopwatch.h"
using namespace std;

int count_intentions(const std::string& filename) {
    int n = 0;
    std::string _; 
    std::ifstream instream(filename);
    while (std::getline(instream, _))
        n += 1;
    return n;
}

int main(int argc, char** argv) {

    CLI::App app {
        "rims4ltlf-benchs: CLI tool for executing rims4ltlf experiments"
    };

    std::string domain_file, problem_file, intentions_file, out_file = "";
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
    
    CLI::Option* out_file_opt =
        app.add_option("-o,--out-file", out_file, "Path to output csv file");

    CLI11_PARSE(app, argc, argv);

    std::shared_ptr<Syft::VarMgr> var_mgr = std::make_shared<Syft::VarMgr>();

    Syft::Stopwatch watch;
    watch.start();
    // only initialization for experiments
    Syft::IntentMgr intent_mgr(var_mgr, domain_file, problem_file, intentions_file);

    auto runtime = watch.stop().count() / 1000.0;
    auto adoption_times = intent_mgr.get_adoption_times();
    std::string adoption_times_string = ("[" + std::to_string(adoption_times[0]));
    for (int i = 1; i < adoption_times.size(); ++i) 
        adoption_times_string += ("-" + std::to_string(adoption_times[i]));
    adoption_times_string += "]";

    if (!(std::filesystem::exists(out_file))) {
        std::ofstream outstream(out_file);
        outstream << "PDDL domain,PDDL problem,Intentions,Number of Intentions (n),Intention Adoption Times (s),Time (s)" << std::endl;
        outstream << domain_file << "," << problem_file << "," << intentions_file << ","  << count_intentions(intentions_file) << "," << adoption_times_string << "," << runtime << std::endl;
    } else {
        std::ofstream outstream(out_file, std::ofstream::app);
        outstream << domain_file << "," << problem_file << "," << intentions_file << "," << count_intentions(intentions_file) << "," << adoption_times_string << "," << runtime << std::endl;
    }
    return 0;
}