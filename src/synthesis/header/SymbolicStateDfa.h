#ifndef SYMBOLIC_STATE_DFA_H
#define SYMBOLIC_STATE_DFA_H

#include <memory>
#include <vector>

#include <cuddObj.hh>

#include"ExplicitStateDfa.h"

namespace Syft {

/**
 * \brief A DFA with symbolic states and transitions.
 */
class SymbolicStateDfa { 

 private:

  std::shared_ptr<VarMgr> var_mgr_; // may be shared among multiple SDFA
  std::size_t automaton_id_; // ID for specific DFA. Useful to retrieve information on specific DFA
  std::vector<int> initial_state_;
  CUDD::BDD final_states_;
  std::vector<CUDD::BDD> transition_function_;

  SymbolicStateDfa(std::shared_ptr<VarMgr> var_mgr);

  static std::pair<std::size_t, std::size_t> create_state_variables(
      std::shared_ptr<VarMgr>& mgr,
      std::size_t state_count);
  
  static CUDD::BDD state_to_bdd(const std::shared_ptr<VarMgr>& mgr,
				std::size_t automaton_id,
				std::size_t state);

  static CUDD::BDD state_set_to_bdd(
      const std::shared_ptr<VarMgr>& mgr,
      std::size_t automaton_id,
      const std::vector<size_t>& states);

  static std::vector<CUDD::BDD> symbolic_transition_function(
      const std::shared_ptr<VarMgr>& mgr,
      std::size_t automaton_id,
      const std::vector<CUDD::ADD>& transition_function);

 public:

  /**
   * 
  */
  SymbolicStateDfa(std::shared_ptr<Syft::VarMgr> var_mgr,
    std::size_t automaton_id,
    const std::vector<int>& initial_state,
    const std::vector<CUDD::BDD>& transition_function,
    const CUDD::BDD& final_states);

  /**
   * \brief Converts an explicit DFA to a symbolic representation.
   *
   * Encodes the state space of the DFA in a logarithmic number of state
   * variables, using BDDs to represent the transition function and the set of
   * final states.
   *
   * \param explicit_dfa The explicit DFA to be converted.
   * \return The symbolic representation of the DFA.
   */
  static SymbolicStateDfa from_explicit(const ExplicitStateDfa& explicit_dfa);

  /**
   * \brief Creates a simple automaton that remembers the value of predicates.
   *
   * \param var_mgr The variable manager to use to create the state variables.
   * \param predicates A vector of BDDs over named variables representing the
   *   predicates to remember.
   * \return A symbolic DFA with one state variable for each predicate. The i-th
   *   state variable records whether \a predicates[i] was true in the last
   *   step. The initial state has all state variables set to 0. All states are
   *   accepting states.
   */
  static SymbolicStateDfa from_predicates(std::shared_ptr<VarMgr> var_mgr,
					  std::vector<CUDD::BDD> predicates);

  /**
   * \brief Returns the variable manager.
   */
  std::shared_ptr<VarMgr> var_mgr() const;

  /**
   * \brief Returns the automaton ID.
   *
   * This ID can be used to retrieve the associated state variables from the
   * variable manager.
   */
  std::size_t automaton_id() const;
  
  /**
   * \brief Returns the bitvector representing the initial state of the DFA.
   */
  std::vector<int> initial_state() const;

  /**
   * \brief Updates the initial state of the DFA
   * \param new_initial_state the new initial state of the DFA
   */
  void set_initial_state(const std::vector<int>& new_initial_state);

  /**
   * \brief Returns the BDD representing the initial state of the DFA.
   */
  CUDD::BDD initial_state_bdd() const;

  /**
   * \brief Returns the BDD encoding the set of final states.
   */
  CUDD::BDD final_states() const;

  /**
   * \brief Returns the transition function of the DFA as a vector of BDDs.
   *
   * The BDD in index \a i computes the value of state variable \a i in the
   * next step, given the current values of the state and alphabet variables.
   */
  std::vector<CUDD::BDD> transition_function() const;

  /**
   * \brief Turns the set of invalid states into a sink.
   *
   * All transitions out of the invalid states are redirected to the sink state
   * 0, and the invalid states are removed from the set of final states.
   *
   * \param invalid_states A BDD representing the set of invalid states.
   */
  void prune_invalid_states(const CUDD::BDD& invalid_states);

  /**
   * \brief Saves the symbolic representation of the DFA in a .dot file.
   *
   * The file includes both the BDDs representing the transition function and
   * the BDD representing the final states.
   *
   * \param filename The name of the file to save the symbolic DFA to.
   */
  void dump_dot(const std::string& filename) const;

    /**
     * \brief Returns a product of symbolic DFAs.
     *
     * \param dfa_vector symbolic DFAs
     * \return A symbolic DFA of the product.
     */
  static SymbolicStateDfa product(const std::vector<SymbolicStateDfa>& dfa_vector);

    /**
     * \brief Composes a planning domain DFA with other DFAs
     * 
     * \param dfa_vector vector with symbolic DFAs. dfa_vector[0] must be the domain DFA
     * \return The symbolic DFA of the composition
     */
  static SymbolicStateDfa domain_compose(std::vector<SymbolicStateDfa>& dfa_vector);

  static SymbolicStateDfa negation(const SymbolicStateDfa& dfa);

  static std::vector<int> state_to_binary(std::size_t state,
                                            std::size_t bit_count);


SymbolicStateDfa restriction(const CUDD::BDD& invalid_states) const;

}; 
} 
#endif // SYMBOLIC_STATE_DFA_H
