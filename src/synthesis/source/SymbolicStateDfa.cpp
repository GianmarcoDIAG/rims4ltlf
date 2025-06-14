#include "SymbolicStateDfa.h"

namespace Syft {

SymbolicStateDfa::SymbolicStateDfa(std::shared_ptr<VarMgr> var_mgr)
  : var_mgr_(std::move(var_mgr))
{}

SymbolicStateDfa::SymbolicStateDfa(std::shared_ptr<Syft::VarMgr> var_mgr,
    std::size_t automaton_id,
    const std::vector<int>& initial_state,
    const std::vector<CUDD::BDD>& transition_function,
    const CUDD::BDD& final_states) : 
      var_mgr_(std::move(var_mgr)), 
      automaton_id_(automaton_id),
      initial_state_(initial_state),
      transition_function_(transition_function),
      final_states_(final_states) {}

std::pair<std::size_t, std::size_t> SymbolicStateDfa::create_state_variables(
    std::shared_ptr<VarMgr>& var_mgr,
    std::size_t state_count) {
  // Largest state index that needs to be represented
  std::size_t max_state = state_count - 1;
  
  std::size_t bit_count = 0;

  // Number of iterations equals the log of the state count
  while (max_state > 0) {
    ++bit_count;
    max_state >>= 1;
  }

  std::size_t automaton_id = var_mgr->create_state_variables(bit_count);
  
  return std::make_pair(bit_count, automaton_id);
}

std::vector<int> SymbolicStateDfa::state_to_binary(std::size_t state,
						   std::size_t bit_count) {
  std::vector<int> binary_representation;

  while (state != 0) {
    // Add the least significant bit of the state to the binary representation
    binary_representation.push_back(state & 1);

    // Shift right
    state >>= 1;
  }

  // Fill rest of the vector with zeroes up to bit_count
  binary_representation.resize(bit_count);

  // Note that the binary representation goes from least to most significant bit

  return binary_representation;
}

CUDD::BDD SymbolicStateDfa::state_to_bdd(
    const std::shared_ptr<VarMgr>& var_mgr,
    std::size_t automaton_id,
    std::size_t state) {
  std::size_t bit_count = var_mgr->state_variable_count(automaton_id);
  std::vector<int> binary_representation = state_to_binary(state, bit_count);
  
  return var_mgr->state_vector_to_bdd(automaton_id, binary_representation);
}

CUDD::BDD SymbolicStateDfa::state_set_to_bdd(
    const std::shared_ptr<VarMgr>& var_mgr,
    std::size_t automaton_id,
    const std::vector<size_t>& states) {
  CUDD::BDD bdd = var_mgr->cudd_mgr()->bddZero();

  for (std::size_t state : states) {
    bdd |= state_to_bdd(var_mgr, automaton_id, state);
  }

  return bdd;
}

// TODO (Gianmarco).
// this function creates a transition function as a function 
// \eta: 2^Z x 2^Y x 2^X -> 2^Z s.t. \eta(Z, Y, X) = Z' 
// iff Z=s and Z'=s' and \delta(s, (Y, X)) = s'
// I want to obtain a Boolean formula 
// \eta defined over 2^Z x 2^Y x 2^X x 2^Z s.t. \eta(Z, Y, X, Z') 
// iff Z=s and Z'=s' and \delta(s, (Y, X)) = s'
std::vector<CUDD::BDD> SymbolicStateDfa::symbolic_transition_function(
    const std::shared_ptr<VarMgr>& var_mgr,
    std::size_t automaton_id,
    const std::vector<CUDD::ADD>& transition_function) {
  std::size_t bit_count = var_mgr->state_variable_count(automaton_id);
  std::vector<CUDD::BDD> symbolic_transition_function(
      bit_count, var_mgr->cudd_mgr()->bddZero());

  for (std::size_t j = 0; j < transition_function.size(); ++j) {
    CUDD::BDD state_bdd = state_to_bdd(var_mgr, automaton_id, j);

    for (std::size_t i = 0; i < bit_count; ++i) {
      // BddIthBit counts from the least-significant bit
      CUDD::BDD jth_component = state_bdd & transition_function[j].BddIthBit(i);

      symbolic_transition_function[i] |= jth_component;
    }
  }

  return symbolic_transition_function;
}

SymbolicStateDfa SymbolicStateDfa::from_explicit(
    const ExplicitStateDfa& explicit_dfa) {
  std::shared_ptr<VarMgr> var_mgr = explicit_dfa.var_mgr();

  auto count_and_id = create_state_variables(var_mgr,
					     explicit_dfa.state_count());
  std::size_t bit_count = count_and_id.first;
  std::size_t automaton_id = count_and_id.second;
  
  std::vector<int> initial_state = state_to_binary(explicit_dfa.initial_state(),
						   bit_count);
  
  CUDD::BDD final_states = state_set_to_bdd(var_mgr, automaton_id,
					    explicit_dfa.final_states());
  
  std::vector<CUDD::BDD> transition_function = symbolic_transition_function(
      var_mgr, automaton_id, explicit_dfa.transition_function());

  SymbolicStateDfa symbolic_dfa(var_mgr);
  symbolic_dfa.automaton_id_ = automaton_id;
  symbolic_dfa.initial_state_ = std::move(initial_state);
  symbolic_dfa.final_states_ = std::move(final_states);
  symbolic_dfa.transition_function_ = std::move(transition_function);

  return symbolic_dfa;
}

std::shared_ptr<VarMgr> SymbolicStateDfa::var_mgr() const {
  return var_mgr_;
}

std::size_t SymbolicStateDfa::automaton_id() const {
  return automaton_id_;
}
  
std::vector<int> SymbolicStateDfa::initial_state() const {
  return initial_state_;
}

void SymbolicStateDfa::set_initial_state(const std::vector<int>& new_initial_state) {
  initial_state_ = new_initial_state;
}

CUDD::BDD SymbolicStateDfa::initial_state_bdd() const {
  return state_to_bdd(var_mgr_, automaton_id_, 0);
}

CUDD::BDD SymbolicStateDfa::final_states() const {
  return final_states_;
}

std::vector<CUDD::BDD> SymbolicStateDfa::transition_function() const {
  return transition_function_;
}

void SymbolicStateDfa::prune_invalid_states(const CUDD::BDD& invalid_states) {
  for (CUDD::BDD& bit_function : transition_function_)
    bit_function &= !invalid_states;
  // Remove invalid states from the set of accepting states
  final_states_ &= !invalid_states;
}

void SymbolicStateDfa::dump_dot(const std::string& filename) const {
  std::vector<std::string> function_labels =
	    var_mgr_->state_variable_labels(automaton_id_);
  function_labels.push_back("Final");
  /*std::cout << "Labels are: " << std::endl;
  for (auto l : function_labels) {
    std::cout << l << " ";
  }
  std::cout << std::endl;*/

  std::vector<CUDD::ADD> adds;
  adds.reserve(transition_function_.size() + 1);

  for (const CUDD::BDD& bdd : transition_function_) {
	  adds.push_back(bdd.Add());
  }

  adds.push_back(final_states_.Add());

  var_mgr_->dump_dot(adds, function_labels, filename);
}

SymbolicStateDfa SymbolicStateDfa::from_predicates(
    std::shared_ptr<VarMgr> var_mgr,
    std::vector<CUDD::BDD> predicates) {
  std::size_t predicate_count = predicates.size();
  std::vector<int> initial_state(predicate_count, 0);
  CUDD::BDD final_states = var_mgr->cudd_mgr()->bddOne();
  std::size_t automaton_id = var_mgr->create_state_variables(predicate_count);

  SymbolicStateDfa dfa(std::move(var_mgr));
  dfa.automaton_id_ = automaton_id;
  dfa.initial_state_ = std::move(initial_state);
  dfa.transition_function_ = std::move(predicates);
  dfa.final_states_ = std::move(final_states);

  return dfa;
}

SymbolicStateDfa SymbolicStateDfa::product(const std::vector<SymbolicStateDfa>& dfa_vector) {
    if (dfa_vector.size() < 1) {
        throw std::runtime_error("Incorrect usage of automata product");
    }
    std::shared_ptr<VarMgr> var_mgr = dfa_vector[0].var_mgr();
    std::vector<std::size_t> automaton_ids;
    std::vector<int> initial_state;
    CUDD::BDD final_states = var_mgr->cudd_mgr()->bddOne();
    std::vector<CUDD::BDD> transition_function;

    for (SymbolicStateDfa dfa : dfa_vector) {
        automaton_ids.push_back(dfa.automaton_id());

        std::vector<int> dfa_initial_state = dfa.initial_state();
        initial_state.insert(initial_state.end(), dfa_initial_state.begin(), dfa_initial_state.end());

        final_states = final_states & dfa.final_states();
        std::vector<CUDD::BDD> dfa_transition_function = dfa.transition_function();
        transition_function.insert(transition_function.end(), dfa_transition_function.begin(), dfa_transition_function.end());
    }

    std::size_t product_automaton_id = var_mgr->create_product_state_space(automaton_ids);

    SymbolicStateDfa product_automaton(var_mgr);
    product_automaton.automaton_id_ = product_automaton_id;
    product_automaton.initial_state_ = std::move(initial_state);
    product_automaton.final_states_ = std::move(final_states);
    product_automaton.transition_function_ = std::move(transition_function);

    return product_automaton;
}

SymbolicStateDfa SymbolicStateDfa::domain_compose(std::vector<SymbolicStateDfa>& dfa_vector) {
    // order of variables is:
    // (F, Act, React, Z_1, ..., Z_n)
    std::shared_ptr<VarMgr> var_mgr = dfa_vector[0].var_mgr();
    std::vector<std::size_t> automaton_ids;
    std::vector<int> initial_state;
    CUDD::BDD final_states = var_mgr->cudd_mgr()->bddOne();
    std::vector<CUDD::BDD> transition_function;

    // get ID of composed DFA
    for (int i = 0; i < dfa_vector.size(); ++i) 
      automaton_ids.push_back(dfa_vector[i].automaton_id());
    std::size_t composed_automaton_id = var_mgr->create_product_state_space(automaton_ids);

    // initial state
    for (const auto& dfa : dfa_vector) {
      std::vector<int> dfa_initial_state = dfa.initial_state();
      initial_state.insert(initial_state.end(), dfa_initial_state.begin(), dfa_initial_state.end());
    }
    
    // debug
    // std::cout << "Composed automaton initial state: " << std::flush;
    // for (const auto& b : initial_state) std::cout << b;
    // std::cout << ". Size: " << initial_state.size() << std::endl;;

    // transition function
    // 1. creates substitution vector
    std::vector<CUDD::BDD> substitution_vector = var_mgr->make_compose_vector(dfa_vector[0].automaton_id(), dfa_vector[0].transition_function());

    // debug
    // std::cout << "Substitution vector for VectorCompose: " << std::flush;
    // for (const auto& bdd : substitution_vector) std::cout << bdd << std::endl;
    // std::cout << "Size of substitution vector: " << substitution_vector.size() << std::endl;

    // 2. creates transition function 
    // a. domain BDDs
    for (const auto& bdd : dfa_vector[0].transition_function()) 
      transition_function.push_back(bdd);
    // b. intentions (composed) BDDs
    for (int i = 1; i < dfa_vector.size(); ++i) {
      std::vector<CUDD::BDD> intention_transition_function =
        dfa_vector[i].transition_function();
      for (const auto&bdd : intention_transition_function)
        transition_function.push_back(bdd.VectorCompose(substitution_vector));
    }

    // debug
    // std::cout << "Composed transition function size: " << transition_function.size() << std::endl;

    // final states
    // conjunction of all intentions final states
    for (int i = 1; i < dfa_vector.size(); ++i)
      final_states = final_states * dfa_vector[i].final_states();

    std::size_t agent_error_index = var_mgr->get_state_variables(dfa_vector[0].automaton_id()).size() - 2;
    std::size_t env_error_index = var_mgr->get_state_variables(dfa_vector[0].automaton_id()).size() - 1;

    CUDD::BDD agent_error_bdd = var_mgr->get_state_variables(dfa_vector[0].automaton_id()).at(agent_error_index);
    CUDD::BDD env_error_bdd = var_mgr->get_state_variables(dfa_vector[0].automaton_id()).at(env_error_index);

    final_states = (!agent_error_bdd) * (env_error_bdd + final_states);

    // debug
    // std::cout << "Final states: " << final_states << std::endl;
     
    SymbolicStateDfa composed_automaton(var_mgr);
    composed_automaton.automaton_id_ = composed_automaton_id;
    composed_automaton.initial_state_ = std::move(initial_state);
    composed_automaton.final_states_ = std::move(final_states);
    composed_automaton.transition_function_ = std::move(transition_function);
    
    return composed_automaton;
}

SymbolicStateDfa SymbolicStateDfa::negation(const SymbolicStateDfa& dfa) {
  // i. New DFA points same VarMgr
  std::shared_ptr<VarMgr> var_mgr = dfa.var_mgr();
  // ii. Negated automaton ID
  std::size_t negated_automaton_id = var_mgr->create_state_variables(var_mgr->state_variable_count(dfa.automaton_id_));
  // iii. Same Initial State
  std::vector<int> initial_state = dfa.initial_state();
  /// iv. Same Transition function
  std::vector<CUDD::BDD> transition_function = dfa.transition_function();
  // v. Swap final states and non final states
  CUDD::BDD negated_final_states = !(dfa.final_states()); 

  // vi. Construct and instantiates negated automaton
  SymbolicStateDfa negated_dfa(var_mgr);
  negated_dfa.automaton_id_ =  negated_automaton_id;
  negated_dfa.initial_state_ = initial_state;
  negated_dfa.final_states_ = negated_final_states;
  negated_dfa.transition_function_ = transition_function;

  return negated_dfa;
}

SymbolicStateDfa SymbolicStateDfa::restriction(const CUDD::BDD& invalid_states) const {

  std::shared_ptr<VarMgr> var_mgr = this->var_mgr();
  // std::size_t restriction_id = var_mgr->create_state_variables(var_mgr->state_variable_count(this->automaton_id_));
  std::size_t restriction_id = var_mgr->copy_state_variables(this->automaton_id());
  std::vector<int> initial_state = this->initial_state();
  std::vector<CUDD::BDD> restriction_transitions;
  // if the current state is an invalid state, sned every transition to 0
  for (const CUDD::BDD& bit_function : this->transition_function_) {
    CUDD::BDD restricted_bit_function = (bit_function * (!invalid_states));
    restriction_transitions.push_back(restricted_bit_function);
  }
  // remove invalid states from the set of final states 
  CUDD::BDD restriction_final_states = (final_states_ * (!invalid_states));
  
  SymbolicStateDfa restricted_dfa(var_mgr);
  restricted_dfa.automaton_id_ =  restriction_id;
  restricted_dfa.initial_state_ = initial_state;
  restricted_dfa.final_states_ = restriction_final_states;
  restricted_dfa.transition_function_ = restriction_transitions;

  return restricted_dfa;

}

}

