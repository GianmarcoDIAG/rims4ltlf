/*
* declares class IntentMgr
* manages LTLf intentions
*/

#ifndef INTENTMGR_H
#define INTENTMGR_H

#include<string>
#include<fstream>
#include<boost/algorithm/string/predicate.hpp>
#include<boost/algorithm/string/classification.hpp>
#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string/trim.hpp>
#include<boost/algorithm/string/replace.hpp>
#include<boost/algorithm/string.hpp>
#include<unordered_set>
#include<cuddObj.hh>
#include"SymbolicStateDfa.h"
#include"Domain.h"
#include"spotparser.h"
#include"ReachabilityMaxSetSynthesizer.h"
#include"Utils.h"
#include"Stopwatch.h"

namespace Syft {
    struct IntentionIsRealizableResult {
        std::vector<int> compatible_intentions;
        CUDD::BDD candidate_intention_win_strategy;
        std::unique_ptr<SymbolicStateDfa> candidate_intention_dfa;
        MaxSet max_set_strategy;
    };

    class IntentMgr {
        private:
            // data members
            std::shared_ptr<Syft::VarMgr> var_mgr_;
            // i-th entry of vectors above give:
            // current evaluation of i-th state var;
            // transition function of i-th state var
            std::vector<int> state_var_evals_;
            std::vector<CUDD::BDD> state_var_transitions_;
            std::unique_ptr<Domain> domain_;
            std::vector<SymbolicStateDfa> dfas_;
            std::vector<CUDD::BDD> win_strategies_;
            MaxSet max_set_strategy_;
            std::vector<spot::formula> atoms_; // contains fluents and agent actions symbols
            std::vector<std::pair<spot::formula, spot::formula>> formulas_; // contains adopted intentions

            std::vector<double> adoption_times_;

            // private member functions 
            // void init(
                // const std::vector<std::string>& input_ltlf_intents, 
                // std::vector<std::string>& ltlf_intents,
                // int currend_id
            // );

            void init(
                const std::vector<std::string>& input_ltlf_intents,
                std::vector<std::pair<spot::formula, spot::formula>>& formulas,
                int current_id
            );

            std::string parse_intent(const Domain& domain, std::string& intent) const;

            // modifies its non const arguments
            void is_realizable_aux(
                std::vector<int>& compatible_intentions,  
                const SymbolicStateDfa& candidate_intention_sdfa, 
                const CUDD::BDD& candidate_intention_win_strategy,
                MaxSet& max_set_strategy,
                int current_id
            );

            void weak_adopt(
                const std::string& candidate_intention,
                int priority,
                const IntentionIsRealizableResult& realizablity_result
            );

            void strong_adopt(
                const std::string& candidate_intention,
                int priority,
                const IntentionIsRealizableResult& realizablity_result
            );

            std::map<formula, formula> get_init_state_interpretation() const; 

            std::map<formula, formula> get_interpretation(const std::vector<int>& act_vec) const;

        public: 
            IntentMgr(
                std::shared_ptr<Syft::VarMgr> var_mgr,
                const std::string& domain_file,
                const std::string& problem_file,
                const std::string& intentions_file
            );

            void run();

            void help() const;

            void get_domain_state() const;

            void get_all_intentions() const;

            void get_intention(int k) const;

            bool is_final() const;

            bool is_winning(int action_id) const;

            bool is_certainly_progressing(int action_id) const;

            void get_all_actions() const;

            void get_all_winning_actions() const;

            void get_all_certainly_progressing_actions() const;

            void drop(std::vector<int>& ids);

            void do_action(int action_id);

            IntentionIsRealizableResult is_realizable(std::string& candidate_ltlf_intention, int priority);

            void is_realizable_and_weak_adopt(std::string& candidate_ltlf_intention, int priority);

            void is_realizable_and_strong_adopt(std::string& candidate_ltlf_intention, int priority);

            void get_intentions_length() const {
                std::cout << "[ims4ltlf][run][get_intentions_length] The number of adopted intentions is: " << formulas_.size() << std::endl;
            }

            void halt() const {
                std::cout << "[ims4ltlf][run][halt] Terminating execution of the IMS" << std::endl;
            }

            void debug_print() const;

            void debug_parser() const;

            std::vector<double> get_adoption_times() const {return adoption_times_;}

    };
}
#endif