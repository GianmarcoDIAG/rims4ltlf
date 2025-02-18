/*
* definition of class IntentMgr
*/

#include"IntentMgr.h"

namespace Syft {
    IntentMgr::IntentMgr(
        std::shared_ptr<Syft::VarMgr> var_mgr,
        const std::string& domain_file,
        const std::string& problem_file,
        const std::string& intentions_file
    ):  var_mgr_(var_mgr) {
        Syft::Stopwatch pddl2dfa;
        pddl2dfa.start();
        // construct domain

        domain_ = std::make_unique<Domain>(var_mgr, domain_file, problem_file);
        SymbolicStateDfa domain_dfa = domain_->to_symbolic(); 

        // add state vars evaluations
        for (const auto& eval : domain_dfa.initial_state())
            state_var_evals_.push_back(eval);
        // add state vars transition functions
        for (const auto& bdd : domain_dfa.transition_function())
            state_var_transitions_.push_back(bdd);
        // add fluent into IMS atoms
        for (const auto& f : domain_->get_vars())
            atoms_.push_back(std::move(parse_formula(f.c_str())));
        // add action props into IMS atoms
        for (const auto& a : domain_->get_action_vars())
            atoms_.push_back(std::move(parse_formula(a.c_str())));
        

        // debug
        // var_mgr_->print_varmgr();
        domain_->print_domain();

        dfas_.push_back(domain_dfa);

        std::cout << "[rims4ltlf][init] domain DFA constructed. ";
        auto pddl2dfa_s = pddl2dfa.stop().count() / 1000.0;
        std::cout << "Done [" << pddl2dfa_s << " s]" << std::endl;

        

        // read intentions from file into suitable vector
        std::vector<std::string> input_ltlf_intents;
        std::string ltlf_intent;
        std::ifstream intentions_file_stream(intentions_file);
        while (std::getline(intentions_file_stream, ltlf_intent)){
            // parse current intention to match symbols used in domain
            ltlf_intent = parse_intent(*domain_, ltlf_intent);
            input_ltlf_intents.push_back(ltlf_intent);
        }

        // debug
        // std::cout << "Parsed LTLf intentions: " << std::endl;
        // for (const auto& intent : input_ltlf_intents) 
            // std::cout << intent << std::endl;
        // std::cout << std::endl;

        // call to internal init function 
        // std::vector<std::string> ltlf_intentionts;
        std::vector<std::pair<spot::formula, spot::formula>> formulas;
        init(input_ltlf_intents, formulas, 0);
    }

    void IntentMgr::run() {
        std::string command;
        while (true) {
            std::cout << "[rims4ltlf][run] insert a command (use help for list of available commands): ";
            std::getline(std::cin, command);
            if (command == "help") help();
            else if (command == "halt") {halt(); return;}
            else if (command == "get_domain_state") get_domain_state();
            else if (command == "get_intentions_length") get_intentions_length();
            else if (command == "get_all_intentions") get_all_intentions();
            else if (command.rfind("get_intention", 0) == 0) {
                int open_bracket = command.find("("), close_bracket = command.find(")", open_bracket);
                int k = std::stoi(command.substr(open_bracket+1, close_bracket - open_bracket - 1));
                get_intention(k);
            }
            else if (command == "is_final") is_final();
            else if (command == "get_all_actions") get_all_actions();
            else if (command.rfind("is_winning", 0) == 0) {
                int open_bracket = command.find("("), close_bracket = command.find(")", open_bracket);
                int action_id = std::stoi(command.substr(open_bracket+1, close_bracket - open_bracket - 1));
                is_winning(action_id);
            }
            else if (command.rfind("is_certainly_progressing", 0) == 0) {
                int open_bracket = command.find("("), close_bracket = command.find(")", open_bracket);
                int action_id = std::stoi(command.substr(open_bracket+1, close_bracket - open_bracket - 1));
                is_certainly_progressing(action_id);
            }
            else if (command == "get_all_winning_actions") get_all_winning_actions();
            else if (command == "get_all_certainly_progressing_actions") get_all_certainly_progressing_actions();
            else if (command.rfind("drop", 0) == 0) {
                int open_bracket = command.find("("), close_bracket = command.find(")", open_bracket);
                std::string intentions_str = command.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                std::vector<std::string> intentions_list;
                boost::split(intentions_list, intentions_str, boost::is_any_of(",")); 
                std::vector<int> intentions_ids;
                for (const auto& intention : intentions_list)
                    intentions_ids.push_back(std::stoi(intention));
                drop(intentions_ids);
            }
            else if (command.rfind("do_action", 0) == 0) {
                int open_bracket = command.find("("), close_bracket = command.find(")", open_bracket);
                int action_id = std::stoi(command.substr(open_bracket+1, close_bracket - open_bracket - 1));
                do_action(action_id);
            }
            else if (command.rfind("is_realizable_and_weak_adopt", 0) == 0) {
                int open_bracket = command.find("("), close_bracket = command.rfind(")");
                std::string argument_str = command.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                std::vector<std::string> argument_list;
                boost::split(argument_list, argument_str, boost::is_any_of(",")); 
                int intention_id = std::stoi(argument_list[1]);
                if (intention_id > dfas_.size()) std::cout << "Intention ID: " << intention_id << " is NOT VALID. Intention ID must be between: " << 1 << " and " << dfas_.size() << std::endl;  
                else is_realizable_and_weak_adopt(argument_list[0], intention_id);
            }
            else if (command.rfind("is_realizable_and_strong_adopt", 0) == 0) {
                int open_bracket = command.find("("), close_bracket = command.rfind(")");
                std::string argument_str = command.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                std::vector<std::string> argument_list;
                boost::split(argument_list, argument_str, boost::is_any_of(",")); 
                int intention_id = std::stoi(argument_list[1]);
                if (intention_id > dfas_.size()) std::cout << "Intention ID: " << intention_id << " is NOT VALID. Intention ID must be between: " << 1 << " and " << dfas_.size() << std::endl;  
                else is_realizable_and_strong_adopt(argument_list[0], intention_id);
            }
            else if (command.rfind("is_realizable", 0) == 0) {
                IntentionIsRealizableResult realizability_result;
                int open_bracket = command.find("("), close_bracket = command.rfind(")");
                std::string argument_str = command.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                std::vector<std::string> argument_list;
                boost::split(argument_list, argument_str, boost::is_any_of(",")); 
                int intention_id = std::stoi(argument_list[1]);
                if (intention_id > dfas_.size()) std::cout << "Intention ID: " << intention_id << " is NOT VALID. Intention ID must be between: " << 1 << " and " << dfas_.size() << std::endl;  
                else realizability_result = is_realizable(argument_list[0], intention_id);
                std::string user_choice;
                std::cout << "[rims4ltlf][run] do you want to adopt the intention (type: w for weak adoption [adopts intention iff realizable with all currently adopted intentions]; s for strong adoption [adopts intention iff realizable with all higher priority intentions and drops unrealizable lower priority intentions])? " << std::flush;
                std::getline(std::cin, user_choice);
                if (user_choice == "w") weak_adopt(argument_list[0], intention_id, realizability_result);
                else if (user_choice == "s") strong_adopt(argument_list[0], intention_id, realizability_result);
                else std::cout << "[rims4ltlf][run] invalid input. No adoption" << std::endl;
            }
            // else if (command == "debug_print") debug_print();
            // else if (command == "debug_parser") debug_parser();
            else std::cout << "[rims4ltlf][run] user command is NOT VALID (use help for list of available commands)" << std::endl;
        }
    }
    
    std::string IntentMgr::parse_intent(const Domain& domain, std::string& intent) const {
        std::string parsed_intent = intent;

        // get maps from: action names to props; and var names to bdds
        std::unordered_map<std::string, std::string> action_names_to_props =
            domain.get_action_name_to_props();
        std::unordered_map<std::string, CUDD::BDD> var_name_to_bdd =
            var_mgr_->get_name_to_variable();

        // copy is needed because of mismatch between SPOT's and Lydia's syntax
        std::string copy = intent;
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
                    size_t pos = parsed_intent.find(p);
                    while (pos != std::string::npos) {
                        parsed_intent.replace(pos, p.size(), it->second);
                        pos = parsed_intent.find(p, pos + it->second.size());
                    } 
                }
                else throw std::runtime_error(p + " is neither a fluent nor an action name");
            }
        }    
        return parsed_intent;
    }

    void IntentMgr::init(
        const std::vector<std::string>& input_ltlf_intents,
        std::vector<std::pair<spot::formula, spot::formula>>& formulas,
        int current_id) {
            if (current_id == input_ltlf_intents.size()) {formulas_ = formulas; return;}
            Syft::Stopwatch intention2dfa;
            intention2dfa.start();

            std::string current_intent = input_ltlf_intents.at(current_id);
            std::cout << "[rims4ltlf][init] current intention is: " << current_intent << std::endl;
            spot::formula intent = parse_formula(current_intent.c_str());

            // debug // parsing works
            // std::cout << "Intetion formula parsed with SPOT: " << intent << std::endl;
            // std::cout << "[rims4ltlf][init] progressing wrt initial state: " << std::flush;

            auto init_state_interpretation = get_init_state_interpretation();
            auto progr_intent = progr(intent, &init_state_interpretation);
            
            // debug // progression works
            // std::cout << "Intention progressed wrt init state: " << progr_intent.first << ". (holds on last: " << progr_intent.second << ")"  << std::endl;
            // std::cout << "Done" << std::endl;

            std::cout << "[rims4ltlf][init] transforming to DFA..." << std::flush; 
            ExplicitStateDfaMona intent_mona_dfa = ExplicitStateDfaMona::dfa_of_formula(current_intent);
            // debug
            // intent_mona_dfa.dfa_print();
            ExplicitStateDfa intent_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, intent_mona_dfa);
            SymbolicStateDfa intent_sym_dfa = SymbolicStateDfa::from_explicit(intent_dfa);
            // std::cout << "Done" << std::endl;

            // std::cout << "[rims4ltlf][init] initializing intention DFA..." << std::flush;
            // creates initial evaluation vector
            // intention DFA reads:
            // 1. domain initial state;
            // 2. dummy start action (encoded as 11...1)
            std::vector<int> domain_state = dfas_[0].initial_state();
            std::vector<int> initial_eval_vector(var_mgr_->total_variable_count(), 0);
            for (int i = 0; i < domain_state.size(); ++i) 
                initial_eval_vector[i] = domain_state[i];
            for (int i = domain_state.size(); i < domain_state.size() + var_mgr_->output_variable_count(); ++i)
                initial_eval_vector[i] = 1;
            

            // debug
            // var_mgr_->print_varmgr();
            // std::cout << "Initial evaluation vector: " << std::flush;
            // for (const auto& b : initial_eval_vector) std::cout << b;
            // std::cout << ". Size: " << initial_eval_vector.size() << std::endl;

            // debug
            // std::cout << "Intention DFA initial state: " << std::flush;
            // for (const auto& b : intent_sym_dfa.initial_state()) std::cout << b;
            // std::cout << ". Size: " << intent_sym_dfa.initial_state().size() << std::endl;

            // evaluate trans funcs of intention and update IntentionsManager

            // construct composed transition functions for state vars
            std::vector<CUDD::BDD> substitution_vector = var_mgr_->make_compose_vector(dfas_[0].automaton_id(), dfas_[0].transition_function());

            std::vector<int> new_intention_state;
            for (const auto& bdd : intent_sym_dfa.transition_function()) {
                auto eval = bdd.Eval(initial_eval_vector.data()).IsOne();
                new_intention_state.push_back(eval);
                state_var_evals_.push_back(eval);
                // state_var_transitions_.push_back(bdd);
                state_var_transitions_.push_back(bdd.VectorCompose(substitution_vector));
            }
            intent_sym_dfa.set_initial_state(new_intention_state);

            auto intention2dfa_time = intention2dfa.stop().count() / 1000.0; 
            std::cout << "Done [" << intention2dfa_time << " s]" << std::endl;

            // debug
            // std::cout << "Updated intention DFA state: " << std::flush;
            // for (const auto& b: intent_sym_dfa.initial_state()) std::cout << b;
            // std::cout << ". Size: " << intent_sym_dfa.initial_state().size() << std::endl;

            // 2. construct and solve DFA game
            Syft::Stopwatch intention2game;
            intention2game.start();

            std::cout << "[rims4ltlf][init] constructing and solving game for the intention..." << std::flush;
            std::vector<SymbolicStateDfa> intention_dfas = {dfas_[0], intent_sym_dfa};
            SymbolicStateDfa intent_game = SymbolicStateDfa::domain_compose(intention_dfas);
            CUDD::BDD invariant_bdd = domain_->get_invariants_bdd();

            // debug
            // var_mgr_->print_varmgr();
            // intent_game.dump_dot("dfa.dot");
            ReachabilityMaxSetSynthesizer synthesizer(
                intent_game,
                Player::Agent,
                Player::Agent,
                intent_game.final_states(),
                invariant_bdd
            );
            SynthesisResult intention_result = synthesizer.run();

            auto intention2game_time = intention2game.stop().count() / 1000.0;
            std::cout << "Done [" << intention2game_time << " s]" << std::endl;

            // debug
            // if (intention_result.realizability)
                // std::cout << "Current intention is realizable" << std::endl;
            // else std::cout << "Current intention is NOT realizable" << std::endl;

            // if intention is not realizable, move to next intention
            if (!intention_result.realizability) {
                std::cout << "[rims4ltlf][init] current intention is UNREALIZABLE. Moving to next intention" << std::endl;
                adoption_times_.push_back(intention2dfa_time + intention2game_time);
                return init(input_ltlf_intents, formulas, current_id+1);
            }

            // restrict maximally permissive strategy to agent actions satisfying preconditions
            std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;
            std::size_t agent_error_index = domain_dfa_vars - 2;
            CUDD::BDD agent_error_bdd = (dfas_[0].transition_function())[agent_error_index];

            CUDD::BDD intention_win_strategy = std::move(synthesizer.AbstractMaxSet(intention_result).deferring_strategy * !agent_error_bdd);

            // solve game for all intentions
            Syft::Stopwatch adoption4intention;
            adoption4intention.start();

            std::cout << "[rims4ltlf][init] constructing and solving game for all intentions..." << std::flush;
            std::vector<SymbolicStateDfa> dfa_vector = dfas_;
            dfa_vector.push_back(intent_sym_dfa);
            SymbolicStateDfa game_arena = SymbolicStateDfa::domain_compose(dfa_vector);

            // restriction to winning regions
            // CUDD::BDD state_space = var_mgr_->cudd_mgr()->bddOne();
            // for (const auto& win_region : win_regions_)
                // state_space *= win_region;
            // state_space *= intention_result.winning_states;
            // restriction to winning strategies
            CUDD::BDD state_space = var_mgr_->cudd_mgr()->bddOne();
            for (const auto& win_strategy : win_strategies_)
                state_space *= win_strategy;
            state_space *= intention_win_strategy;

            ReachabilityMaxSetSynthesizer game_synthesizer(
                game_arena,
                Player::Agent,
                Player::Agent,
                game_arena.final_states(),
                invariant_bdd * state_space
            );
            SynthesisResult result = game_synthesizer.run();

            // if all intentions are not realizable, move to next intention
            if (!result.realizability) {
                auto adoption4intention_time = adoption4intention.stop().count() / 1000.0;
                std::cout << "Done [" << adoption4intention_time << " s]" << std::endl;
                std::cout << "[rims4ltlf][init] the intention is UNREALIZABLE with higher priority intentions. Moving to next intention" << std::endl;
                return init(input_ltlf_intents, formulas, current_id+1);
            }

            // formulas_.push_back(progr_intent);
            formulas.push_back(progr_intent);
            // win_regions_.push_back(intention_result.winning_states);
            win_strategies_.push_back(intention_win_strategy);
            dfas_.push_back(intent_sym_dfa);
            max_set_strategy_ = game_synthesizer.AbstractMaxSet(result);
            // TODO. Is this restriction necessary if we restrict to strategies ? 
            max_set_strategy_.deferring_strategy *= !agent_error_bdd;
            max_set_strategy_.nondeferring_strategy *= !agent_error_bdd;
            // std::cout << "Done" << std::endl;

            auto adoption4intention_time = adoption4intention.stop().count() / 1000.0;
            std::cout << "Done [" << adoption4intention_time << " s]" << std::endl;
            std::cout << "[rims4ltlf][init] the intention is REALIZABLE with higher priority intentions" << std::endl;
            adoption_times_.push_back(intention2dfa_time + intention2game_time + adoption4intention_time);
            // 5. recursive call
            return init(input_ltlf_intents, formulas, current_id+1);
        }

    void IntentMgr::help() const {
        std::cout << "[rims4ltlf][run][help] Available commands: help halt get_domain_state get_intentions_length get_all_intentions get_intention(k) is_final get_all_actions is_winning(action_id) is_certainly_progressing(action_id) get_all_winning_actions get_all_certainly_progressing_actions drop(intentions_list) do_action(action_id) is_realizable(ltlf_intention, k)" << std::endl;
        std::cout << "[rims4ltlf][run][help] help -- prints a list of available commands" << std::endl;
        std::cout << "[rims4ltlf][run][help] halt -- terminates the execution of the IMS" << std::endl;
        std::cout << "[rims4ltlf][run][help] get_domain_state -- prints the current state of the domain" << std::endl;
        std::cout << "[rims4ltlf][run][help] get_intentions_length -- prints the number of currently adopted intentions" << std::endl;
        std::cout << "[rims4ltlf][run][help] get_all_intentions -- prints the currently adopted intentions with their priorities" << std::endl; 
        std::cout << "[rims4ltlf][run][help] get_intention(k) -- prints the intentions with priority k" << std::endl;
        std::cout << "[rims4ltlf][run][help] is_final -- returns true iff current IMS state is final for all intentions" << std::endl;
        std::cout << "[rims4ltlf][run][help] get_all_actions -- returns the list of all agent actions" << std::endl;
        std::cout << "[rims4ltlf][run][help] is_winning(action_id) -- returns true iff action with action_id is winning for all intentions in current IMS state" << std::endl;
        std::cout << "[rims4ltlf][run][help] is_certainly_progressing(action_id) -- returns true iff action with action_id progresses all intentions in current IMS state" << std::endl;
        std::cout << "[rims4ltlf][run][help] get_all_winning_actions -- returns the list of all winning agent actions in current IMS state" << std::endl;
        std::cout << "[rims4ltlf][run][help] get_all_certainly_progressing_actions -- returns the list of all certainly progressing agent actions in current IMS state" << std::endl;
        std::cout << "[rims4ltlf][run][help] drop(intentions_list) -- drops comma-separated intentions in list intentions_list " << std::endl;
        std::cout << "[rims4ltlf][run][help] do_action(action_id) -- executes action with action_id iff action is winning; reads env reaction from user" << std::endl;
        std::cout << "[rims4ltlf][run][help] is_realizable(ltlf_intention, k) -- checks realizability of ltlf_intention at priority k in current IMS state; prints the list of compatible intentions and asks the user if they want to adopt the intention" << std::endl;
        std::cout << "[rims4ltlf][run][help] is_realizable_and_weak_adopt(ltlf_intention, k) -- checks realizability of lflf_intention at priority k in current IMS state; adopts the intention iff realizable with all currently adopted intentions" << std::endl;
        std::cout << "[rims4ltlf][run][help] is_realizable_and_strong_adopt(ltlf_intention, k) -- checks realizability of ltlf_intention at priority k in current IMS state; adopts the intention iff realizable with higher priority intentions and drops lower priority intentions unrealizable with it" << std::endl;
    }

    void IntentMgr::get_domain_state() const {
        std::vector<int> domain_vector_state = dfas_[0].initial_state();
        std::vector<std::string> domain_vars = domain_->get_vars();
        std::string domain_state = "";
        std::cout << "[rims4ltlf][run][get_domain_state] Current domain state is: " << std::endl;
        for (int i = 0; i < domain_vector_state.size() - 1; ++i)
            if (domain_vector_state[i] == 1) domain_state +=  domain_vars[i] + ", ";
        if (domain_vector_state[domain_vector_state.size() - 2] == 1) std::cout << "ag_err, " << std::flush;
        if (domain_vector_state[domain_vector_state.size() - 1] == 1) std::cout << "env_err, " << std::flush;
        domain_state = ("{" + domain_state.substr(0, domain_state.size() - 2) + "}");
        std::cout << domain_state << std::endl;
    }

    bool IntentMgr::is_final() const {
        bool result = false;
    
        std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;
        std::size_t agent_error_index = domain_dfa_vars - 2;
        std::size_t env_error_index = domain_dfa_vars - 1;
        CUDD::BDD agent_error_bdd = var_mgr_->get_state_variables(dfas_[0].automaton_id()).at(agent_error_index);
        CUDD::BDD env_error_bdd = var_mgr_->get_state_variables(dfas_[0].automaton_id()).at(env_error_index);

        CUDD::BDD final_states_bdd = var_mgr_->cudd_mgr()->bddOne();
        for (int i = 1; i < dfas_.size(); ++i)
            final_states_bdd = final_states_bdd * dfas_[i].final_states();

        // debug
        // var_mgr_->print_varmgr();
        // std::cout << "Final states BDD: " << final_states_bdd << std::endl;

        // 1. construct evaluation vector
        std::vector<int> eval_vector;
        // domain vars
        for (int i = 0; i < domain_dfa_vars; ++i)
            eval_vector.push_back(state_var_evals_[i]);
        // agent actions; can be anything; set to 1s for simplicity
        for (int i = 0; i < var_mgr_->output_variable_count(); ++i)
            eval_vector.push_back(1);
        // env reactions; can be anything; set to 1s for simplicity
        for (int i = 0; i < var_mgr_->input_variable_count(); ++i)
            eval_vector.push_back(1);
        // intention DFAs state vars
        for(int i = domain_dfa_vars; i < state_var_evals_.size(); ++i)
            eval_vector.push_back(state_var_evals_[i]);

        // debug
        // var_mgr_->print_varmgr();
        // std::cout << "Current state vars evaluation vector: " << std::flush;
        // for (const auto& b : state_var_evals_)
            // std::cout << b;
        // std::cout << ". Size: " << state_var_evals_.size() << std::endl;

        // std::cout << "Current evaluation vector: " << std::flush;
        // for (const auto& b : eval_vector)
            // std::cout << b;
        // std::cout << ". Size: " << eval_vector.size() << std::endl;        

        // 2. evaluate current state over final states BDD
        if (final_states_bdd.Eval(eval_vector.data()).IsOne()) {
            std::cout << "[rims4ltlf][run][is_final] current IMS state is FINAL for all intentions" << std::endl;
            result = true;
        } else std::cout << "[rims4ltlf][run][is_final] current IMS state is NOT FINAL for all intentions" << std::endl;

        // 3. print information about error states as well
        if (agent_error_bdd.Eval(eval_vector.data()).IsOne())
            std::cout << "[rims4ltlf][run][is_final] current IMS state is agent error" << std::endl;
        if (env_error_bdd.Eval(eval_vector.data()).IsOne())
            std::cout << "[rims4ltlf][run][is_final] current IMS state is environment error" << std::endl;

        // 4. return statement
        return result;
    }

    void IntentMgr::get_all_intentions() const {
        if (formulas_.size() == 0) {std::cout << "[rims4ltlf][get_all_intentions] no intention is currently adopted" << std::endl; return;}
        std::cout << "[rims4ltlf][run][get_all_intentions] The adopted intentions are: " << std::endl;
        for (int i = 0; i < formulas_.size(); ++i)
            std::cout << "Intention: " << formulas_[i].first << ". Holds on last: " << formulas_[i].second << ". Priority: " << i+1 << std::endl;
    }

    void IntentMgr::get_intention(int k) const {
        if (formulas_.size() == 0)
            std::cout << "[rims4ltlf][run][get_intention] No intention is currently adopted" << std::endl;
        else if (k <= 0 || k > formulas_.size())
            std::cout << "[rims4ltlf][run][get_intention] Invalid priority. Priority must be between 1 and " << formulas_.size() << std::endl; 
        else 
            std::cout << "[rims4ltlf][run][get_intention] Intention with priority " << k << " is " << formulas_[k-1].first << ". Holds on last: " << formulas_[k-1].second << std::endl;
    }

    void IntentMgr::get_all_actions() const {
        std::cout << "Agent actions: " << std::endl;

        auto id_to_action_name = domain_->get_id_to_action_name();

        for (const auto& p : id_to_action_name) 
            std::cout << "ID: " << p.first << ". Action: " << p.second << std::endl;

        return;
    }

    bool IntentMgr::is_winning(int action_id) const {
        auto id_to_action_name = domain_->get_id_to_action_name();
        std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;

        // 1. construct evaluation vector
        std::vector<int> eval_vector;
        // domain vars
        for (int i = 0; i < domain_dfa_vars; ++i)
            eval_vector.push_back(state_var_evals_[i]);
        // agent vars; set to binary representation of action_id
        std::vector<int> action_vec = Utils::to_bits(action_id, var_mgr_->output_variable_count());
        for (int i = 0; i < action_vec.size(); ++i)
            eval_vector.push_back(action_vec[i]);
        // env vars; can be anything; set to 1s for simplicity
        for (int i = 0; i < var_mgr_->input_variable_count(); ++i)
            eval_vector.push_back(1);
        // intention DFAs state vars
        for (int i = domain_dfa_vars; i < state_var_evals_.size(); ++i)
            eval_vector.push_back(state_var_evals_[i]);

        // debug
        // std::cout << "Action ID binary representation: " << std::flush;
        // for (const auto& b : action_vec) 
            // std::cout << b;
        // std::cout << std::endl;

        // var_mgr_->print_varmgr();
        // std::cout << "Current state vars evaluation vector: " << std::flush;
        // for (const auto& b : state_var_evals_)
            // std::cout << b;
        // std::cout << ". Size: " << state_var_evals_.size() << std::endl;

        // std::cout << "Current evaluation vector: " << std::flush;
        // for (const auto& b : eval_vector)
            // std::cout << b;
        // std::cout << ". Size: " << eval_vector.size() << std::endl;        
        
        // 2. check if agent action is winning
        if (max_set_strategy_.deferring_strategy.Eval(eval_vector.data()).IsOne()) {
            std::cout << "[rims4ltlf][run][is_winning] action ID: " << action_id << " with name: " << id_to_action_name[action_id] << " is WINNING in current IMS state" << std::endl;
            return true;
        } else {
            std::cout << "[rims4ltlf][run][is_winning] action ID: " << action_id << " with name: " << id_to_action_name[action_id] << " is NOT WINNING in current IMS state" << std::endl;
            return false;
        }
    }

    bool IntentMgr::is_certainly_progressing(int action_id) const {
        auto id_to_action_name = domain_->get_id_to_action_name();
        std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;

        // 1. construct evaluation vector
        std::vector<int> eval_vector;
        // domain vars
        for (int i = 0; i < domain_dfa_vars; ++i)
            eval_vector.push_back(state_var_evals_[i]);
        // agent vars; set to binary representation of action_id
        std::vector<int> action_vec = Utils::to_bits(action_id, var_mgr_->output_variable_count());
        for (int i = 0; i < action_vec.size(); ++i)
            eval_vector.push_back(action_vec[i]);
        // env vars; can be anything; set to 1s for simplicity
        for (int i = 0; i < var_mgr_->input_variable_count(); ++i)
            eval_vector.push_back(1);
        // intention DFAs state vars
        for (int i = domain_dfa_vars; i < state_var_evals_.size(); ++i)
            eval_vector.push_back(state_var_evals_[i]);

        // debug
        // std::cout << "Action ID binary representation: " << std::flush;
        // for (const auto& b : action_vec) 
            // std::cout << b;
        // std::cout << std::endl;

        // var_mgr_->print_varmgr();
        // std::cout << "Current state vars evaluation vector: " << std::flush;
        // for (const auto& b : state_var_evals_)
            // std::cout << b;
        // std::cout << ". Size: " << state_var_evals_.size() << std::endl;

        // std::cout << "Current evaluation vector: " << std::flush;
        // for (const auto& b : eval_vector)
            // std::cout << b;
        // std::cout << ". Size: " << eval_vector.size() << std::endl;        
        
        // 2. check if agent action is winning
        if (max_set_strategy_.nondeferring_strategy.Eval(eval_vector.data()).IsOne()) {
            std::cout << "[rims4ltlf][run][is_certainly_progressing] action ID: " << action_id << " with name: " << id_to_action_name[action_id] << " is PROGRESSING in current IMS state" << std::endl;
            return true;
        } else {
            std::cout << "[rims4ltlf][run][is_certainly_progressing] action ID: " << action_id << " with name: " << id_to_action_name[action_id] << " is NOT PROGRESSING in current IMS state" << std::endl;
            return false;
        }
    }

    void IntentMgr::get_all_winning_actions() const {
        auto id_to_action_name = domain_->get_id_to_action_name();
        std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;
        std::cout << "[rims4ltlf][run][get_all_winning_actions] determining all WINNING actions in current IMS state..." << std::endl;

        for (int action_id = 0; action_id < id_to_action_name.size(); ++action_id) {
            // 1. construct evaluation vector
            std::vector<int> eval_vector;
            // domain vars
            for (int i = 0; i < domain_dfa_vars; ++i)
                eval_vector.push_back(state_var_evals_[i]);
            // agent vars; set to binary representation of agent action
            std::vector<int> action_vec = Utils::to_bits(action_id, var_mgr_->output_variable_count());
            for (int i = 0; i < action_vec.size(); ++i)
                eval_vector.push_back(action_vec[i]);
            // env vars; can be anything: set to 1s for simplicity
            for (int i = 0; i < var_mgr_->input_variable_count(); ++i)
                eval_vector.push_back(1);
            // intention DFAs state vars
            for (int i = domain_dfa_vars; i < state_var_evals_.size(); ++i)
                eval_vector.push_back(state_var_evals_[i]);

            // 2. check if agent action is winning
            if (max_set_strategy_.deferring_strategy.Eval(eval_vector.data()).IsOne()) std::cout << "Action ID: " << action_id << " with name: " << id_to_action_name[action_id] << " is WINNING in current IMS state" << std::endl;            
        }
        // for (int i = 0; i < id_to_action_name.size(); ++i)
        // is_winning(i);
        std::cout << "[rims4ltlf][run][get_all winning_actions] Done" << std::endl;
    }

    void IntentMgr::get_all_certainly_progressing_actions() const {
        
        auto id_to_action_name = domain_->get_id_to_action_name();
        std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;
        std::cout << "[rims4ltlf][run][get_all_certainly_progressing_actions] determining all CERTAINLY PROGRESSING actions in current IMS state..." << std::endl;

        for (int action_id = 0; action_id < id_to_action_name.size(); ++action_id) {
            // 1. construct evaluation vector
            std::vector<int> eval_vector;
            // domain vars
            for (int i = 0; i < domain_dfa_vars; ++i)
                eval_vector.push_back(state_var_evals_[i]);
            // agent vars; set to binary representation of agent action
            std::vector<int> action_vec = Utils::to_bits(action_id, var_mgr_->output_variable_count());
            for (int i = 0; i < action_vec.size(); ++i)
                eval_vector.push_back(action_vec[i]);
            // env vars; can be anything: set to 1s for simplicity
            for (int i = 0; i < var_mgr_->input_variable_count(); ++i)
                eval_vector.push_back(1);
            // intention DFAs state vars
            for (int i = domain_dfa_vars; i < state_var_evals_.size(); ++i)
                eval_vector.push_back(state_var_evals_[i]);

            // 2. check if agent action is nondeferring
            if (max_set_strategy_.nondeferring_strategy.Eval(eval_vector.data()).IsOne()) std::cout << "Action ID: " << action_id << " with name: " << id_to_action_name[action_id] << " is CERTAINLY PROGRESSING in current IMS state" << std::endl;            
        }
        std::cout << "[rims4ltlf][run][get_all_certainly_progressing_actions] Done" << std::endl;
        // auto id_to_action_name = domain_->get_id_to_action_name();
        // std::cout << "[rims4ltlf][get_all_certainly_progressing_actions] determining all CERTAINLY PROGRESSING ACTIONS in current IMS state..." << std::endl;
        // for (int i = 0; i < id_to_action_name.size(); ++i)
            // is_certainly_progressing(i);
        // std::cout << "[rims4ltlf][get_all_certainly_progressing_actions] Done" << std::endl;
    }

    void IntentMgr::drop(std::vector<int>& ids) {
        // preliminary checks
        if (formulas_.size() == 0) {std::cout << "[rims4ltlf][run][drop] no intention is currently adopted" << std::endl; return;}
        for (const auto& id : ids) {
            if (id <= 0 || id > formulas_.size()) 
                {std::cout << "[rims4ltlf][run][drop] invalid priority. Priority must be between 1 and " << formulas_.size() << std::endl; return;}
        }

        // needed to remove intentions correctly
        std::sort(ids.begin(), ids.end(), std::greater<int>());

        // drop selected intentions
        Syft::Stopwatch intents2drop;
        intents2drop.start();

        std::cout << "[rims4ltlf][run][drop] dropping selected intentions..." << std::flush;
        for (const auto& id: ids) {
            formulas_.erase(formulas_.begin() + (id - 1));
            dfas_.erase(dfas_.begin() + id);
            win_strategies_.erase(win_strategies_.begin() + (id-1));
        }
        // std::cout << "Done" << std::endl;

        // std::cout << "[rims4ltlf][drop] constructing and solving game for the remaining intentions..." << std::flush;
        SymbolicStateDfa new_game_arena = SymbolicStateDfa::domain_compose(dfas_);
        CUDD::BDD new_state_space = 
            var_mgr_-> cudd_mgr() -> bddOne();
        // for (const auto& win_region : win_regions_)
            // new_state_space *= win_region;
        for (const auto& win_strategy : win_strategies_)
            new_state_space *= win_strategy;
        CUDD::BDD invariant_bdd = domain_->get_invariants_bdd();

        ReachabilityMaxSetSynthesizer new_intentions_game(
            new_game_arena,
            Player::Agent,
            Player::Agent,
            new_game_arena.final_states(),
            invariant_bdd * new_state_space
        );
        SynthesisResult new_synthesis_result = new_intentions_game.run();

        // std::cout << "[rims4ltlf][drop] Updating information..." << std::flush;
        max_set_strategy_ = new_intentions_game.AbstractMaxSet(new_synthesis_result);

        // restricts maximally permissive strategy to agent actions satisfying preconditions
        std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;
        std::size_t agent_error_index = domain_dfa_vars - 2;
        CUDD::BDD agent_error_bdd = (dfas_[0].transition_function())[agent_error_index];

        max_set_strategy_.deferring_strategy *= !agent_error_bdd;
        max_set_strategy_.nondeferring_strategy *= !agent_error_bdd;
        // std::cout << "Done" << std::endl;
        auto intents2drop_time = intents2drop.stop().count() / 1000.0;
        std::cout << "Done [" << intents2drop_time << " s]" << std::endl;

        return;
    }

    void IntentMgr::do_action(int action_id) {
        auto id_to_action_name = domain_->get_id_to_action_name();
        std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;
        int react_id;
        
        // 1. check if action is winning
        std::cout << "[rims4ltlf][run][do_action] checking if action is winning..." << std::endl;
        bool action_is_winning = is_winning(action_id);
        if (!action_is_winning) {std::cout << "[rims4ltlf][do_action] action is not executed"  << std::endl; return;} 

        std::cout << "[rims4ltlf][run][do_action] action is executed; insert env reaction ID: " << std::flush;
        std::cin >> react_id;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // gets rid of newline char

        // 2. construct eval vector
        std::vector<int> eval_vector;
        // domain vars
        for (int i = 0; i < domain_dfa_vars; ++i)
            eval_vector.push_back(state_var_evals_[i]);
        // agent vars
        std::vector<int> action_vec = Utils::to_bits(action_id, var_mgr_->output_variable_count());
        for (int i = 0; i < action_vec.size(); ++i)
            eval_vector.push_back(action_vec[i]);
        // env vars
        std::vector<int> react_vec = Utils::to_bits(react_id, var_mgr_->input_variable_count());
        for (int i = 0; i < react_vec.size(); ++i)
            eval_vector.push_back(react_vec[i]);
        // intention DFAs state vars
        for (int i = domain_dfa_vars; i < state_var_evals_.size(); ++i)
            eval_vector.push_back(state_var_evals_[i]);

        // debug
        // std::cout << "Evaluation vector: " << std::flush;
        // for (const auto& b : eval_vector) std::cout << b;
        // std::cout << ". Size: " << eval_vector.size() << std::endl;

        // TODO. Combine all for loops below in one for loop?
        // 3. update state of all stored DFAs
        for (auto& dfa : dfas_) {
            std::vector<int> new_dfa_state;
            for (const auto& bdd : dfa.transition_function())
                new_dfa_state.push_back(bdd.Eval(eval_vector.data()).IsOne());
            dfa.set_initial_state(new_dfa_state);
        }

        // 4. update state of all stored vars
        std::vector<int> new_state_var_evals;
        for (int i = 0; i < state_var_transitions_.size(); ++i)
            new_state_var_evals.push_back(state_var_transitions_[i].Eval(eval_vector.data()).IsOne());
        
        state_var_evals_ = new_state_var_evals;

        // 5. progress adopted intentionsupdating
        std::cout << "[rims4ltlf][run][do_action] progressing intentions..." << std::flush;
        auto interpretation = get_interpretation(action_vec);
        for (int i = 0; i < formulas_.size(); ++i)
            formulas_[i] = progr(formulas_[i].first, &interpretation);
        std::cout << "Done" << std::endl;

        return;
    }

    void IntentMgr::debug_print() const {
        var_mgr_->print_varmgr();
        std::cout << "Current state vars vector: " << std::endl;
        for (const auto& b : state_var_evals_) std::cout << b;
        std::cout << std::endl;
    }

    IntentionIsRealizableResult IntentMgr::is_realizable(std::string& candidate_intention, int priority) {
        IntentionIsRealizableResult result;

        // needed to restrict max set strategy to legal agent actions
        std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;
        std::size_t agent_error_index = domain_dfa_vars - 2;
        CUDD::BDD agent_error_bdd = (dfas_[0].transition_function())[agent_error_index];

        std::cout << "[rims4ltlf][run][is_realizable] candidate intention is: " << candidate_intention << std::endl;
        candidate_intention = parse_intent(*domain_, candidate_intention);

        // transform candidate intention into DFA
        Syft::Stopwatch intent2dfa;
        intent2dfa.start();

        std::cout << "[rims4ltlf][run][is_realizable] transforming intention to DFA..." << std::flush;
        ExplicitStateDfaMona candidate_intention_dfa = ExplicitStateDfaMona::dfa_of_formula(candidate_intention);
        ExplicitStateDfa candidate_intention_edfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, candidate_intention_dfa);
        SymbolicStateDfa candidate_intention_sdfa = SymbolicStateDfa::from_explicit(candidate_intention_edfa);
        // std::cout << "Done" << std::endl;

        // std::cout << "[rims4ltlf][run][is_realizable] initializing intention DFA..." << std::flush;
        // New state vars are created above
        // Store them in IntentMgr

        // create evaluation vector
        std::size_t domain_dfa_size = dfas_[0].initial_state().size();
        std::vector<int> eval_vector;
        // domain vars
        for (int i = 0; i < domain_dfa_size; ++i)
            eval_vector.push_back(state_var_evals_[i]);
        // agent vars set to 11...1
        for (int i = 0; i < var_mgr_->output_variable_count(); ++i)
            eval_vector.push_back(1);
        // env vars set to 11...1
        for (int i = 0; i < var_mgr_->input_variable_count(); ++i)
            eval_vector.push_back(1);
        // intention DFAs state vars
        for (int i = domain_dfa_size; i < state_var_evals_.size(); ++i)
            eval_vector.push_back(state_var_evals_[i]);
        // candidate intention DFA vars
        // must be added to avoid wrong evaluations
        for (const auto& i : candidate_intention_sdfa.initial_state())
            eval_vector.push_back(i);

        // debug
        // std::cout << "Current evaluation vector: " << std::flush;
        // for (const auto& i : eval_vector) std::cout << i << std::flush;
        // std::cout << ". Size: " << eval_vector.size() << std::endl;
        
        std::vector<CUDD::BDD> substitution_vector = var_mgr_->make_compose_vector(dfas_[0].automaton_id(), dfas_[0].transition_function());

        std::vector<int> candidate_intention_state;
        for (const auto& bdd : candidate_intention_sdfa.transition_function()) {
            auto eval = bdd.Eval(eval_vector.data()).IsOne();
            candidate_intention_state.push_back(eval);
            state_var_evals_.push_back(eval);
            state_var_transitions_.push_back(bdd.VectorCompose(substitution_vector));
        }
        candidate_intention_sdfa.set_initial_state(candidate_intention_state);

        auto intent2dfa_time = intent2dfa.stop().count() / 1000.0;

        std::cout << "Done [" << intent2dfa_time << " s]" << std::endl;

        Syft::Stopwatch dfa2game;
        dfa2game.start();

        std::cout << "[rims4ltlf][run][is_realizable] constructing and solving game for candidate intention..." << std::flush;

        std::vector<SymbolicStateDfa> candidate_intention_game_dfas = {dfas_[0], candidate_intention_sdfa};
        SymbolicStateDfa candidate_intention_game = SymbolicStateDfa::domain_compose(candidate_intention_game_dfas);
        CUDD::BDD invariant_bdd = domain_->get_invariants_bdd();

        ReachabilityMaxSetSynthesizer candidate_intention_synthesizer(
            candidate_intention_game,
            Player::Agent,
            Player::Agent,
            candidate_intention_game.final_states(),
            invariant_bdd
        );
        SynthesisResult candidate_intention_result = candidate_intention_synthesizer.run();

        result.candidate_intention_win_strategy = 
            std::move(candidate_intention_synthesizer.AbstractMaxSet(candidate_intention_result).deferring_strategy * !agent_error_bdd);
        candidate_intention_result.winning_states;
        result.candidate_intention_dfa = std::make_unique<SymbolicStateDfa>(std::move(candidate_intention_sdfa));
        // std::cout << "Done" << std::endl;

        if (!candidate_intention_result.realizability) {
            result.max_set_strategy.deferring_strategy = var_mgr_->cudd_mgr()->bddZero();
            result.max_set_strategy.nondeferring_strategy = var_mgr_->cudd_mgr()->bddZero();
            result.compatible_intentions = {};
            auto dfa2game_time = dfa2game.stop().count() / 1000.0;
            std::cout << "Done [" << dfa2game_time << " s]" << std::endl;
            std::cout << "[rims4ltlf][is_realizable] candidate intention is UNREALIZABLE" << std::endl;
            return result;
        }

        if (priority > 1) {
            std::cout << "[rims4ltlf][is_realizable] constructing and solving game for higher priority intentions..." << std::flush;
            std::vector<SymbolicStateDfa> higher_priority_dfas = {dfas_[0]};
            CUDD::BDD state_space = var_mgr_->cudd_mgr()->bddOne();
            for (int i = 1; i < priority; ++i) {
                higher_priority_dfas.push_back(dfas_[i]);
                // state_space *= win_regions_[i-1]; 
                state_space *= win_strategies_[i-1];
            }
            higher_priority_dfas.push_back(*result.candidate_intention_dfa);
            state_space *= result.candidate_intention_win_strategy;
            SymbolicStateDfa game_arena = SymbolicStateDfa::domain_compose(higher_priority_dfas);
            ReachabilityMaxSetSynthesizer game_synthesizer(
                game_arena,
                Player::Agent,
                Player::Agent,
                game_arena.final_states(),
                domain_->get_invariants_bdd() * state_space 
            );
            SynthesisResult game_result = game_synthesizer.run();
            // std::cout << "Done" << std::endl;

            if (!game_result.realizability) {
                result.max_set_strategy.deferring_strategy = var_mgr_->cudd_mgr()->bddZero();
                result.max_set_strategy.nondeferring_strategy = var_mgr_->cudd_mgr()->bddZero();
                result.compatible_intentions = {};
                auto dfa2game_time = dfa2game.stop().count() / 1000.0;
                std::cout << "Done [" << dfa2game_time << " s]" << std::endl;
                std::cout << "[rims4ltlf][is_realizable] candidate intention is UNREALIZABLE with higher priority intentions" << std::endl;
                return result;
            } else {
                for (int i = 1; i < priority; ++i) result.compatible_intentions.push_back(i);
                result.max_set_strategy = game_synthesizer.AbstractMaxSet(game_result);
                // restrict maximally permissive strategy to agent actions satisfying preconditions
                result.max_set_strategy.deferring_strategy *= !agent_error_bdd;
                result.max_set_strategy.nondeferring_strategy *= !agent_error_bdd;
            }        
        } else {
            result.max_set_strategy = candidate_intention_synthesizer.AbstractMaxSet(candidate_intention_result);
            result.max_set_strategy.deferring_strategy *= !agent_error_bdd;
            result.max_set_strategy.nondeferring_strategy *= !agent_error_bdd;
        }

        auto dfa2game_time = dfa2game.stop().count() / 1000.0;
        std::cout << "Done [" << dfa2game_time << " s]" << std::endl;
        std::cout << "[rims4ltlf][is_realizable] candidate intention is REALIZABLE with higher priority intentions. Checking lower priority intentions..." << std::endl;
        is_realizable_aux(
            result.compatible_intentions,
            *result.candidate_intention_dfa, 
            result.candidate_intention_win_strategy,
            result.max_set_strategy,
            priority);
        std::cout << "[rims4ltlf][is_realizable] the ids of COMPATIBLE INTENTIONS are: " << std::flush;
        for (const auto& i: result.compatible_intentions) std::cout << i << " ";
        std::cout << std::endl;
        return result;
    }

    void IntentMgr::is_realizable_aux(
        std::vector<int>& compatible_intentions,
        const SymbolicStateDfa& candidate_intention_sdfa, 
        const CUDD::BDD& candidate_intention_win_region,
        MaxSet& max_set_strategy,
        int current_id) {
            if (current_id > formulas_.size()) return;

            // needed to restrict max set strategy to legal agent actions
            std::size_t domain_dfa_vars = domain_->get_vars().size() + 2;
            std::size_t agent_error_index = domain_dfa_vars - 2;
            CUDD::BDD agent_error_bdd = (dfas_[0].transition_function())[agent_error_index];

            std::cout << "[rims4ltlf][run][is_realizable] current intention is: " << formulas_[current_id - 1].first << std::endl;

            Syft::Stopwatch intent2game;
            intent2game.start();

            std::cout << "[rims4ltlf][run][is_realizable] constructing and solving game..." << std::flush;
            std::vector<SymbolicStateDfa> game_arena_dfas = {dfas_[0]};
            CUDD::BDD state_space = var_mgr_->cudd_mgr()->bddOne();
            for (const auto& i : compatible_intentions) {
                game_arena_dfas.push_back(dfas_[i]);
                // state_space *= win_regions_[i-1];
                state_space *= win_strategies_[i-1];
            } 
            // add intention with ID current_id
            game_arena_dfas.push_back(dfas_[current_id]);
            // state_space *= win_regions_[current_id - 1];
            state_space *= win_strategies_[current_id-1];
            // add candidate_intention
            game_arena_dfas.push_back(candidate_intention_sdfa);
            state_space *= candidate_intention_win_region; 
            SymbolicStateDfa game_arena = SymbolicStateDfa::domain_compose(game_arena_dfas);
            ReachabilityMaxSetSynthesizer game_synthesizer(
                game_arena,
                Player::Agent,
                Player::Agent,
                game_arena.final_states(),
                domain_->get_invariants_bdd() * state_space 
            );
            SynthesisResult game_result = game_synthesizer.run();
            if (!game_result.realizability) {
                auto intent2game_time = intent2game.stop().count() / 1000.0;
                std::cout << "Done [" << intent2game_time << "s]" << std::endl;
                std::cout << "[rims4ltlf][run][is_realizable] the intention is UNREALIZABLE with higher priority intentions. Moving to next intention" << std::endl;
            } else {
                std::cout << "[rims4ltlf][run][is_realizable] the intention is REALIZABLE with higher priority intentions. Updating information..." << std::flush;
                compatible_intentions.push_back(current_id);
                max_set_strategy = game_synthesizer.AbstractMaxSet(game_result);
                max_set_strategy.deferring_strategy *= !agent_error_bdd;
                max_set_strategy.nondeferring_strategy *= !agent_error_bdd;
                auto intent2game_time = intent2game.stop().count() / 1000.0;
                std::cout << "Done [" << intent2game_time << "s]" << std::endl;
            }
            is_realizable_aux(compatible_intentions, candidate_intention_sdfa, candidate_intention_win_region, max_set_strategy, current_id+1);
    }

    void IntentMgr::is_realizable_and_weak_adopt(std::string& candidate_ltlf_intention, int priority) {
        std::cout << "[rims4ltlf][run][is_realizable_and_weak_adopt] checking REALIZABILITY of candidate intention" << std::endl;
        // std::string original_candidate_intention = candidate_ltlf_intention;
        auto is_realizable_result = is_realizable(candidate_ltlf_intention, priority);

        // adopts the intention iff it is compatible with all currently adopted intentions
        if (is_realizable_result.compatible_intentions.size() == formulas_.size()) {
            Syft::Stopwatch intent2adopt;
            intent2adopt.start();

            std::cout << "[rims4ltlf][run][is_realizable_and_weak_adopt] candidate intention is REALIZABLE with all current adopted intentions. Adopting... " << std::flush;
            auto intent = parse_formula(candidate_ltlf_intention.c_str());
            auto init_state_interpretation = get_init_state_interpretation();
            auto progr_intent = progr(intent, &init_state_interpretation);
            // ltlf_intentions_.insert(ltlf_intentions_.begin() + priority - 1, candidate_ltlf_intention);
            formulas_.insert(formulas_.begin() + priority - 1, progr_intent);
            dfas_.insert(dfas_.begin() + priority, *is_realizable_result.candidate_intention_dfa);
            // win_regions_.insert(win_regions_.begin() + priority - 1, is_realizable_result.candidate_intention_win_region);            
            win_strategies_.insert(win_strategies_.begin() + priority - 1, is_realizable_result.candidate_intention_win_strategy);            
            max_set_strategy_ = is_realizable_result.max_set_strategy;

            auto intent2adopt_time = intent2adopt.stop().count() / 1000.0;
            std::cout << "Done [" << intent2adopt_time << " s]" << std::endl;
        } else {
            std::cout << "[rims4ltlf][run][is_realizable_and_weak_adopt] candidate intention is UNREALIZABLE with all currently adopted intentions. Adoption aborted" << std::endl;
        }
    }

    void IntentMgr::weak_adopt(
        const std::string& candidate_intention,
        int priority,
        const IntentionIsRealizableResult& realizability_result) {
        if (realizability_result.compatible_intentions.size() == formulas_.size()) {
            Syft::Stopwatch intent2adopt;
            intent2adopt.start();

            std::cout << "[rims4ltlf][run][weak_adopt] candidate intention is REALIZABLE with all current adopted intentions. Adopting... " << std::flush;
            auto intent = parse_formula(candidate_intention.c_str());
            auto init_state_interpretation = get_init_state_interpretation();
            auto progr_intent = progr(intent, &init_state_interpretation);
            formulas_.insert(formulas_.begin() + priority - 1, progr_intent);
            // ltlf_intentions_.insert(ltlf_intentions_.begin() + priority - 1, candidate_intention);
            dfas_.insert(dfas_.begin() + priority, *realizability_result.candidate_intention_dfa);
            // win_regions_.insert(win_regions_.begin() + priority - 1, realizability_result.candidate_intention_win_region);            
            win_strategies_.insert(win_strategies_.begin() + priority - 1, realizability_result.candidate_intention_win_strategy);            
            max_set_strategy_ = realizability_result.max_set_strategy;

            auto intent2adopt_time = intent2adopt.stop().count() / 1000.0;
            std::cout << "Done [" << intent2adopt_time << " s]" << std::endl;
        } else std::cout << "[rims4ltlf][run][weak_adopt] candidate intention is UNREALIZABLE with all currently adopted intentions. Adoption aborted" << std::endl;
    }

    void IntentMgr::is_realizable_and_strong_adopt(std::string& candidate_ltlf_intention, int priority) {
        std::cout << "[rims4ltlf][run][is_realizable_and_strong_adopt] checking REALIZABILITY of candidate intention" << std::endl; 
        auto is_realizable_result = is_realizable(candidate_ltlf_intention, priority);

        // if (is_realizable_result.compatible_intentions.size() == 0) {std::cout << "[rims4ltlf][is_realizable_and_strong_adopt] candidate intention is UNREALIZABLE with higher priority intentions. Adoption aborted" << std::endl; return;}
        if (is_realizable_result.compatible_intentions.size() < priority - 1) {std::cout << "[rims4ltlf][strong_adopt] candidate intention is UNREALIZABLE with higher priority intentions. Adoption aborted" << std::endl; return;}

        Syft::Stopwatch intent2adopt;
        intent2adopt.start();

        std::cout << "[rims4ltlf][run][is_realizable_and_strong_adopt] candidate intention is REALIZABLE with higher priority intentions. Adopting..." << std::flush; 
        auto intent = parse_formula(candidate_ltlf_intention.c_str());
        auto init_state_interpretation = get_init_state_interpretation();
        auto progr_intent = progr(intent, &init_state_interpretation);
        // ltlf_intentions_.insert(ltlf_intentions_.begin() + priority - 1, candidate_ltlf_intention);
        formulas_.insert(formulas_.begin() + priority - 1, progr_intent);
        dfas_.insert(dfas_.begin() + priority, *is_realizable_result.candidate_intention_dfa);
        // win_regions_.insert(win_regions_.begin() + priority - 1, is_realizable_result.candidate_intention_win_region);
        win_strategies_.insert(win_strategies_.begin() + priority - 1, is_realizable_result.candidate_intention_win_strategy);
        max_set_strategy_ = is_realizable_result.max_set_strategy;

        auto intent2adopt_time = intent2adopt.stop().count() / 1000.0;
        std::cout << "Done [" << intent2adopt_time << " s]" << std::endl;

        Syft::Stopwatch intents2drop;
        intents2drop.start();

        std::cout << "[rims4ltlf][run][is_realizable_and_strong_adopt] dropping UNREALIZABLE lower priority intentions..." << std::flush;
        for (int i = formulas_.size(); i > priority; --i)
            if (std::find(is_realizable_result.compatible_intentions.begin(), is_realizable_result.compatible_intentions.end(), i-1) == is_realizable_result.compatible_intentions.end()) {
                // TODO. Can we craft an example so as to see when strong adoption works?
                // ltlf_intentions_.erase(ltlf_intentions_.begin()+(i-1));
                formulas_.erase(formulas_.begin()+(i-1));
                dfas_.erase(dfas_.begin() + (i));
                // win_regions_.erase(win_regions_.begin() + (i-1));
                win_strategies_.erase(win_strategies_.begin() + (i-1));
            }

        auto intents2drop_time = intents2drop.stop().count() / 1000.0;
        std::cout << "Done [" << intents2drop_time << " s]" << std::endl;

        return;
    }

    void IntentMgr::strong_adopt(
        const std::string& candidate_intention,
        int priority,
        const IntentionIsRealizableResult& realizablity_result) {
            // if (realizablity_result.compatible_intentions.size() == 0) {std::cout << "[rims4ltlf][strong_adopt] candidate intention is UNREALIZABLE with higher priority intentions. Adoption aborted" << std::endl; return;}
            if (realizablity_result.compatible_intentions.size() < priority - 1) {std::cout << "[rims4ltlf][strong_adopt] candidate intention is UNREALIZABLE with higher priority intentions. Adoption aborted" << std::endl; return;}

            Syft::Stopwatch intent2adopt;
            intent2adopt.start();

            std::cout << "[rims4ltlf][run][strong_adopt] candidate intention is REALIZABLE with higher priority intentions. Adopting..." << std::flush; 
            auto intent = parse_formula(candidate_intention.c_str());
            auto init_state_interpretation = get_init_state_interpretation();
            auto progr_intent = progr(intent, &init_state_interpretation);
            // ltlf_intentions_.insert(ltlf_intentions_.begin() + priority - 1, candidate_intention);
            formulas_.insert(formulas_.begin() + priority - 1, progr_intent);
            dfas_.insert(dfas_.begin() + priority, *realizablity_result.candidate_intention_dfa);
            // win_regions_.insert(win_regions_.begin() + priority - 1, realizablity_result.candidate_intention_win_region);
            win_strategies_.insert(win_strategies_.begin() + priority - 1, realizablity_result.candidate_intention_win_strategy);
            max_set_strategy_ = realizablity_result.max_set_strategy;

            auto intent2adopt_time = intent2adopt.stop().count() / 1000.0;
            std::cout << "Done [" << intent2adopt_time << " s]" << std::endl;

            Syft::Stopwatch intents2drop;
            intents2drop.start();

            std::cout << "[rims4ltlf][run][strong_adopt] dropping UNREALIZABLE lower priority intentions..." << std::flush;
            for (int i = formulas_.size(); i > priority; --i) {
                if (std::find(realizablity_result.compatible_intentions.begin(), realizablity_result.compatible_intentions.end(), i-1) == realizablity_result.compatible_intentions.end()) {
                    // ltlf_intentions_.erase(ltlf_intentions_.begin()+(i-1));
                    // std::cout << "Current ID: " << i-1 << " is being dropped" << std::endl;
                    formulas_.erase(formulas_.begin() + (i-1));
                    dfas_.erase(dfas_.begin() + (i));
                    // win_regions_.erase(win_regions_.begin() + (i-1));
                    win_strategies_.erase(win_strategies_.begin() + (i-1));
                }
            }

            auto intents2drop_time = intents2drop.stop().count() / 1000.0;
            std::cout << "Done [" << intents2drop_time << " s]" << std::endl;
    }

    std::map<formula, formula> IntentMgr::get_init_state_interpretation() const {
        std::size_t domain_vars = domain_->get_vars().size();
        std::size_t act_vars = domain_->get_action_vars().size();
        std::map<formula, formula> init_state_interpretation;
        // interpretation of fluent atoms
        for (int i = 0; i < domain_vars; ++i) {
            if (state_var_evals_[i] == 1) init_state_interpretation[atoms_[i]] = formula::tt();
            else if (state_var_evals_[i] == 0) init_state_interpretation[atoms_[i]] = formula::unop(op::Not, formula::tt());
        }
        // interpretation of action atoms
        // set to 11...1 (i.e., start action) in initial state
        for (int i =  domain_vars; i < domain_vars + act_vars; ++i)
            init_state_interpretation[atoms_[i]] = formula::tt();
        return init_state_interpretation;
    }

    std::map<formula, formula> IntentMgr::get_interpretation(const std::vector<int>& act_vec) const {
        std::size_t domain_vars = domain_->get_vars().size();
        std::size_t act_vars = domain_->get_action_vars().size();
        std::map<formula, formula> interpretation;
        for (int i = 0; i < domain_vars; ++i) {
            if (state_var_evals_[i] == 1) interpretation[atoms_[i]] = formula::tt();
            else if (state_var_evals_[i] == 0) interpretation[atoms_[i]] = formula::unop(op::Not, formula::tt());
        }
        for (int i = domain_vars; i < domain_vars + act_vars; ++i) {
            if (act_vec[i - domain_vars] == 1) interpretation[atoms_[i]] = formula::tt();
            else if (act_vec[i - domain_vars] == 0) interpretation[atoms_[i]] = formula::unop(op::Not, formula::tt());
        }
        return interpretation;
    }

    void IntentMgr::debug_parser() const {
        return;
    }
}
