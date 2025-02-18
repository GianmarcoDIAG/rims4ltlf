/*
*
* definition of class MaximallyPermissiveSynthesizer
*/

#include"MaximallyPermissiveSynthesizer.h"

namespace Syft {
    MaximallyPermissiveSynthesizer::MaximallyPermissiveSynthesizer(
        std::shared_ptr<VarMgr> var_mgr,
        const std::string& domain_file,
        const std::string& init_file,
        const std::string& ltlf_file
    ) : var_mgr_(var_mgr), domain_file_(domain_file), init_file_(init_file), ltlf_file_(ltlf_file) {}

    std::pair<SynthesisResult, MaxSet> MaximallyPermissiveSynthesizer::run() {
        std::pair<SynthesisResult, MaxSet> result;

        // 1. construct DFA of planning domain
        std::cout << "[MaxSyft] Transforming PDDL into DFA...";
        Stopwatch pddl2dfa;
        pddl2dfa.start();

        Domain domain(var_mgr_, domain_file_, init_file_);
        SymbolicStateDfa domain_sdfa = domain.to_symbolic();

        auto pddl2dfa_t = pddl2dfa.stop().count() / 1000.0;
        running_times_.push_back(pddl2dfa_t);
        std::cout << "Done [" << pddl2dfa_t << " s]" << std::endl;
        
        // 2. construct DFA of LTLf formula
        // i. read LTLf goal from file
        std::cout << "[MaxSyft] Transforming LTLf goal into DFA..." << std::flush;
        Stopwatch ltlf2dfa;
        ltlf2dfa.start();

        std::ifstream ltlf_stream(ltlf_file_);
        std::string ltlf_goal;
        std::getline(ltlf_stream, ltlf_goal);

        // ii. parse LTLf goal
        ltlf_goal = parse_goal(domain, ltlf_goal);

        // iii. LTLf -> DFA
        ExplicitStateDfaMona goal_mona_dfa = ExplicitStateDfaMona::dfa_of_formula(ltlf_goal);
        ExplicitStateDfa goal_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, goal_mona_dfa);
        SymbolicStateDfa goal_sdfa = SymbolicStateDfa::from_explicit(goal_dfa);

        auto ltlf2dfa_t = ltlf2dfa.stop().count() / 1000.0;
        running_times_.push_back(ltlf2dfa_t);
        std::cout << "Done [" <<  ltlf2dfa_t << " s]" << std::endl;

        // 3. solve game
        std::cout << "[MaxSyft] Synthesis of maximally permissive strategy..." << std::flush;
        Stopwatch synthesis;
        synthesis.start();

        std::vector<SymbolicStateDfa> game_sdfas = {domain_sdfa, goal_sdfa};
        SymbolicStateDfa dfa_game = SymbolicStateDfa::domain_compose(game_sdfas);
        CUDD::BDD invariant_bdd = domain.get_invariants_bdd();

        ReachabilityMaxSetSynthesizer synthesizer(
            dfa_game,
            Player::Agent,
            Player::Agent,
            dfa_game.final_states(),
            invariant_bdd
        );
        result.first = synthesizer.run();
        result.second = synthesizer.AbstractMaxSet(result.first);

        auto synthesis_t = synthesis.stop().count() / 1000.0;
        running_times_.push_back(synthesis_t);
        std::cout << "Done [" << synthesis_t << " s]" <<  std::endl;

        return result;
    }

    std::string MaximallyPermissiveSynthesizer::parse_goal(const Domain& domain, std::string& goal) const {
        std::string parsed_goal = goal;

        // get maps from: action names to props; and var names to bdds
        std::unordered_map<std::string, std::string> action_names_to_props =
            domain.get_action_name_to_props();
        std::unordered_map<std::string, CUDD::BDD> var_name_to_bdd =
            var_mgr_->get_name_to_variable();

        // copy is needed because of mismatch between SPOT's and Lydia's syntax
        std::string copy = goal;
        boost::algorithm::replace_all(copy, "true", "tt");
    
        // parse formula with spot parser to get props
        // formula spot_intent = parse_formula(intent.c_str());
        formula spot_intent = parse_formula(copy.c_str());
        std::vector<std::string> props = get_props(spot_intent);

        // perform substituion
        for (auto& p : props) {
            if (p == "tt") continue;
            if (var_name_to_bdd.find(p) == var_name_to_bdd.end()) { // p is not a fluent
                auto it = action_names_to_props.find(p);
                if (it != action_names_to_props.end()) {
                    size_t pos = parsed_goal.find(p);
                    while (pos != std::string::npos) {
                        parsed_goal.replace(pos, p.size(), it->second);
                        pos = parsed_goal.find(p, pos + it->second.size());
                    } 
                }
                else throw std::runtime_error(p + " is neither a fluent nor an action name");
            }
        }    
        return parsed_goal;
    }
}