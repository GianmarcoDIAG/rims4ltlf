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
#include"MaximallyPermissiveSynthesizer.h"
using namespace std;

double sum_vec(const std::vector<double>& v) {
    double sum = 0;
    for (const auto& d : v) sum += d;
    return sum;
}

int main(int argc, char** argv) {
    CLI::App app {
        "MaxSyft4FOND: a tool for LTLf maximally permissive synthesis in FOND domains"
    };

    std::string domain_file, problem_file, ltlf_file, out_file;
    bool print_domain = false, save_result = false;

    CLI::Option* domain_file_opt =
        app.add_option("-d,--domain-file", domain_file, "Path to PDDL domain file") ->
        required() -> check(CLI::ExistingFile);

    CLI::Option* problem_file_opt =
        app.add_option("-p,--problem-file", problem_file, "Path to PDDL problem file") ->
        required() -> check(CLI::ExistingFile);

    CLI::Option* intentions_file_opt =
        app.add_option("-g,--ltlf-goal-file", ltlf_file, "Path to LTLf goal file") ->
        required() -> check(CLI::ExistingFile);

    CLI::Option* out_file_opt =
        app.add_option("-o,--output-file", out_file, "Store results to file in csv. Stores:\nPDDL domain file\nPDDL problem file\nLTLf goal file\nPDDL2DFA (s)\nLTLf2DFA (s)\nSynthesis (s)\nRuntime (s)");

    CLI11_PARSE(app, argc, argv);

    std::shared_ptr<Syft::VarMgr> var_mgr = std::make_shared<Syft::VarMgr>();

    Syft::MaximallyPermissiveSynthesizer synthesizer(
        var_mgr,
        domain_file,
        problem_file,
        ltlf_file
    );

    auto result = synthesizer.run();

    auto running_times = synthesizer.get_running_times();
    auto run_time = sum_vec(running_times);

    if (result.first.realizability) std::cout << "[MaxSyft] Synthesis is REALIZABLE [" << run_time << " s]" << std::endl;
    else std::cout << "[MaxSyft] Synthesis is UNREALIZABLE [" << run_time << " s]" << std::endl;


    if (!(std::filesystem::exists(out_file))) {
        std::ofstream outstream(out_file);
        outstream << "PDDL domain,PDDL problem,LTLf goal,PDDL2DFA (s),LTLf2DFA (s),Synthesis (s),Runtime (s)"<<std::endl;
        outstream << domain_file << "," << problem_file << "," << ltlf_file << "," << running_times[0] << "," << running_times[1] << "," << running_times[2] << "," << sum_vec(running_times) << std::endl;
    } else {
        std::ofstream outstream(out_file, std::ofstream::app);
        outstream << domain_file << "," << problem_file << "," << ltlf_file << "," << running_times[0] << "," << running_times[1] << "," << running_times[2] << "," << sum_vec(running_times) << std::endl;
    }

    return 0;
}