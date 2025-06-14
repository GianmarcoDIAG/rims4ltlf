#include "VarMgr.h"

#include <cstring>
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace Syft {

VarMgr::VarMgr() {
  mgr_ = std::make_shared<CUDD::Cudd>();
}

void VarMgr::print_varmgr() const {
  // prints the number of managed automata
  std::cout << "Number of managed automata: " << state_variables_.size() << std::endl;

  // prints number of vars
  std::cout << "Number of variables: " << state_variable_count_ + input_variables_.size() + output_variables_.size() << std::endl;

  // prints named variables
  std::cout << "Named variables (name, var): " << std::endl;

  for (const auto& name_var : name_to_variable_)
    std::cout << "Name: " << name_var.first << ". Var: " << name_var.second << std::endl;
  
  std::cout << "Var indexes (index, name): " << std::endl;

  for (const auto& index_name: index_to_name_)
    std::cout << "Index: " << index_name.first << ". Name: " << index_name.second << std::endl;

  // prints X vars
  std::cout << "Input variables: " << std::endl;
  for (const auto& var: input_variables_) std::cout << "Var: " << var << std::endl;

  // prints Y vars
  std::cout << "Output variables: " << std::endl;
  for (const auto& var: output_variables_) std::cout << "Var: " << var << std::endl;  

  // prints Z vars and initial state of each managed automaton
  for (int i = 0; i < state_variables_.size(); ++i) {
    std::cout << "Automaton ID " << i << " state variables: " << std::endl;
    for (const auto& var: state_variables_[i]) std::cout << var << " ";
    std::cout << std::endl;
  }
}

std::unordered_map<int, std::string> VarMgr::get_index_to_name() const {
  return index_to_name_;
}

std::unordered_map<std::string, CUDD::BDD> VarMgr::get_name_to_variable() const {
  return name_to_variable_;
}

bool VarMgr::is_input_variable(const std::string& var) const {
  CUDD::BDD bdd_var = name_to_variable_.at(var);
  if (std::find(input_variables_.begin(), input_variables_.end(), bdd_var) != input_variables_.end())
    return true;
  return false;
}

bool VarMgr::is_output_variable(const std::string& var) const {
  return !(is_input_variable(var));
}

void VarMgr::create_named_variables(
    const std::vector<std::string>& variable_names) {
  for (const std::string& name : variable_names) {
    if (name_to_variable_.find(name) == name_to_variable_.end()) {
      CUDD::BDD new_variable = mgr_->bddVar();
      int new_index = new_variable.NodeReadIndex();
      name_to_variable_[name] = new_variable;
      index_to_name_[new_index] = name;
    }
  }
}

void VarMgr::create_input_variables(
  const std::vector<std::string>& input_vars
) {
  for (const std::string& input_var : input_vars) {
    input_variables_.push_back(name_to_variable_[input_var]);
    }
}

void VarMgr::create_output_variables(
  const std::vector<std::string>& output_vars
) {
  for (const std::string& output_var : output_vars) {
    output_variables_.push_back(name_to_variable_[output_var]);
    }
}

void VarMgr::assign_state_variables(
  std::size_t automaton_id,
  const std::vector<std::string>& vars
) {for (const auto& v : vars) state_variables_[automaton_id].push_back(name_to_variable_[v]);}

std::size_t VarMgr::create_named_state_variables(const std::vector<std::string>& vars) {
  std::size_t automaton_id = state_variables_.size();

  state_variables_.emplace_back();
  state_variables_[automaton_id].reserve(vars.size());

  for (int i = 0; i < vars.size(); ++i) {
    CUDD::BDD new_state_var = mgr_->bddNewVarAtLevel(0);
    state_variables_[automaton_id].push_back(new_state_var);
    name_to_variable_[vars[i]] = new_state_var;
    index_to_name_[new_state_var.NodeReadIndex()] = vars[i]; 
  }

  state_variable_count_ += vars.size();
  return automaton_id;
}

std::size_t VarMgr::create_state_variables(std::size_t variable_count) {
  std::size_t automaton_id = state_variables_.size();

  // Creates an additional space for variables at index automaton_id,
  // then reserves enough memory for all the new variables
  state_variables_.emplace_back();
  state_variables_[automaton_id].reserve(variable_count);

  for (std::size_t i = 0; i < variable_count; ++i) {
    // Creates a new variable at the top of the variable ordering
    CUDD::BDD new_state_variable = mgr_->bddNewVarAtLevel(0);
    
    state_variables_[automaton_id].push_back(new_state_variable);
  }

  state_variable_count_ += variable_count;

  return automaton_id;
}

std::size_t VarMgr::create_product_state_space(
    const std::vector<std::size_t>& automaton_ids) {
  std::size_t product_automaton_id = state_variables_.size();

  state_variables_.emplace_back();

  for (std::size_t automaton_id : automaton_ids) {
    state_variables_[product_automaton_id].insert(
	state_variables_[product_automaton_id].end(),
	state_variables_[automaton_id].begin(),
	state_variables_[automaton_id].end());
  }

  return product_automaton_id;
}

CUDD::BDD VarMgr::state_variable(std::size_t automaton_id, std::size_t i)
    const {
  return state_variables_[automaton_id][i];
}

std::vector<CUDD::BDD> VarMgr::get_state_variables(std::size_t automaton_id) const {
  return state_variables_[automaton_id];
}
  
CUDD::BDD VarMgr::state_vector_to_bdd(std::size_t automaton_id,
				      const std::vector<int>& state_vector)
    const {
  CUDD::BDD bdd = mgr_->bddOne();
  
  for (std::size_t i = 0; i < state_vector.size(); ++i) {
    if (state_vector[i]) {
      bdd &= state_variables_[automaton_id][i];
    } else {
      bdd &= !state_variables_[automaton_id][i];
    }
  }

  return bdd;
}

void VarMgr::partition_variables(const std::vector<std::string>& input_names,
				 const std::vector<std::string>& output_names) {
  if (!input_variables_.empty() || !output_variables_.empty()) {
    throw std::runtime_error(
	"Error: Only one input-output partition is allowed.");
  }

//  if (input_names.size() + output_names.size() != index_to_name_.size()) {
//    throw std::runtime_error(
//	"Error: Input-output partition is the wrong size.");
//  }

    if (input_names.size() + output_names.size() != index_to_name_.size()) {
        throw std::runtime_error(
                "Error: Input-output partition is the wrong size.");
    }
  
  for (const std::string& input_name : input_names) {
    input_variables_.push_back(name_to_variable(input_name));
  }

  for (const std::string& output_name : output_names) {
    output_variables_.push_back(name_to_variable(output_name));
  }
}

std::shared_ptr<CUDD::Cudd> VarMgr::cudd_mgr() const {
  return mgr_;
}

CUDD::BDD VarMgr::name_to_variable(const std::string& name) const {
  return name_to_variable_.at(name);
}

std::string VarMgr::index_to_name(int index) const {
  return index_to_name_.at(index);
}

std::size_t VarMgr::total_variable_count() const {
  // return name_to_variable_.size() + total_state_variable_count();
  return state_variable_count_ + input_variables_.size() + output_variables_.size();
}
  
std::size_t VarMgr::total_state_variable_count() const {
  std::size_t count = 0;
  for (const auto& state_var_vec : state_variables_) count += state_var_vec.size();
  return count;
  // return state_variable_count_;
}

std::size_t VarMgr::state_variable_count(std::size_t automaton_id) const {
  return state_variables_[automaton_id].size();
}
  
std::size_t VarMgr::input_variable_count() const {
  return input_variables_.size();
}

std::size_t VarMgr::output_variable_count() const {
  return output_variables_.size();
}

CUDD::BDD VarMgr::input_cube() const {
  return mgr_->computeCube(input_variables_);
}

CUDD::BDD VarMgr::output_cube() const {
  return mgr_->computeCube(output_variables_);
}

std::vector<int> VarMgr::make_eval_vector(
    std::size_t automaton_id, const std::vector<int>& state_vector) const {
  std::vector<int> eval_vector(total_variable_count(), 0);

  for (std::size_t i = 0; i < state_variables_[automaton_id].size(); ++i) {
    std::size_t index = state_variables_[automaton_id][i].NodeReadIndex();
    eval_vector[index] = state_vector[i];
  }

  return eval_vector;
}

std::vector<CUDD::BDD> VarMgr::make_compose_vector(
    std::size_t automaton_id, const std::vector<CUDD::BDD>& state_bdds) const {
  std::vector<CUDD::BDD> compose_vector(total_variable_count(),
					mgr_->bddZero());

  // All named variables get mapped to the variable itself
  for (const auto& name_and_variable : name_to_variable_) {
    CUDD::BDD variable = name_and_variable.second;
    std::size_t index = variable.NodeReadIndex();
    compose_vector[index] = variable;
  }

  // State variables for other automaton ids get mapped to the variable itself
  for (std::size_t id = 0; id < automaton_id; ++id) {
    for (const CUDD::BDD& variable : state_variables_[id]) {
      std::size_t index = variable.NodeReadIndex();
      compose_vector[index] = variable;
    }
  }

  for (std::size_t id = automaton_id + 1; id < state_variables_.size(); ++id) {
    for (const CUDD::BDD& variable : state_variables_[id]) {
      std::size_t index = variable.NodeReadIndex();
      compose_vector[index] = variable;
    }
  }

  // The i-th state variable gets mapped to the i-th BDD from the input
  for (std::size_t i = 0; i < state_variables_[automaton_id].size(); ++i) {
    std::size_t index = state_variables_[automaton_id][i].NodeReadIndex();
    compose_vector[index] = state_bdds[i];
  }

  // std::cout << "Size of composition vector: " << compose_vector.size() << std::endl;

  // std::cout << "Composition vector: " << std::endl;
  // for (const auto& v: compose_vector) std::cout << v << std::endl;
  // std::cout << std::endl;

  return compose_vector;
}
  
std::vector<std::string> VarMgr::variable_labels() const {
  std::vector<std::string> labels(total_variable_count());
  for (const auto& index_and_name : index_to_name_) {;
    labels[index_and_name.first] = index_and_name.second;
  }

  for (std::size_t id = 0; id < state_variables_.size(); ++id) {
    for (std::size_t i = 0; i < state_variables_[id].size(); ++i) {
      std::size_t index = state_variables_[id][i].NodeReadIndex();
      labels[index] = "A" + std::to_string(id) + ":Z" + std::to_string(i);
    }
  }

  return labels;
}

std::vector<std::string> VarMgr::output_variable_labels() const {
  std::vector<std::string> labels(output_variable_count());

  for (std::size_t i = 0; i < output_variable_count(); ++i) {
    std::size_t index = output_variables_[i].NodeReadIndex();
    labels[i] = index_to_name(index);
  }

  return labels;
}

std::vector<std::string> VarMgr::input_variable_labels() const {
  std::vector<std::string> labels(input_variable_count());

  for (std::size_t i = 0; i < input_variable_count(); ++i) {
    std::size_t index = input_variables_[i].NodeReadIndex();
    labels[i] = index_to_name(index);
  }

  return labels;
}

std::vector<std::string> VarMgr::state_variable_labels(
    std::size_t automaton_id) const {
	std::vector<std::string> labels;
	labels.reserve(total_state_variable_count());

	std::string id_string = "A" + std::to_string(automaton_id);

	for (std::size_t i = 0; i < state_variables_[automaton_id].size(); ++i) {
		labels.push_back(id_string + ":Z" + std::to_string(i));
	}

	return labels;
}

void VarMgr::dump_dot(const CUDD::ADD& add, const std::string& filename) const {
  std::vector<CUDD::ADD> single(1, add);
  std::vector<std::string> single_label(1, "");
  dump_dot(single, single_label, filename);
}

void VarMgr::dump_dot(const std::vector<CUDD::ADD>& adds,
                      const std::vector<std::string>& function_labels,
                      const std::string& filename) const {
	std::vector<std::string> variable_labels = this->variable_labels();
  std::size_t variable_count = variable_labels.size();
  std::vector<char*> inames(variable_count);

  for (std::size_t i = 0; i < variable_count; ++i) {
    std::string label = variable_labels[i];

    // Return value of label.c_str() is temporary, so need to make a copy
    inames[i] = new char[label.length() + 1]; // need space for '\0' terminator
    std::strcpy(inames[i], label.c_str());
  }

  std::size_t function_count = function_labels.size();
  std::vector<char*> onames(function_count);

  for (std::size_t i = 0; i < function_count; ++i) {
    std::string label = function_labels[i];

    // Return value of label.c_str() is temporary, so need to make a copy
    onames[i] = new char[label.length() + 1]; // need space for '\0' terminator
    std::strcpy(onames[i], label.c_str());
  }

  FILE* fp = fopen(filename.c_str(), "w");

  mgr_->DumpDot(adds, inames.data(), onames.data(), fp);

  fclose(fp);

  for (std::size_t i = 0; i < variable_count; ++i) {
    delete[] inames[i];
  }

  for (std::size_t i = 0; i < function_count; ++i) {
    delete[] onames[i];
  }
}

std::size_t VarMgr::automaton_num() const {
  return state_variables_.size();
}

std::size_t VarMgr::copy_state_variables(std::size_t automaton_id) {

  std::size_t copied_vars_id = state_variables_.size();

  state_variables_.emplace_back();

  state_variables_[copied_vars_id].insert(
    state_variables_[copied_vars_id].end(),
    state_variables_[automaton_id].begin(),
    state_variables_[automaton_id].end()
  );

  return copied_vars_id;
}
  
}
