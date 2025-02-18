#include "spotparser.h"

#include <spot/tl/formula.hh>
#include <spot/tl/parse.hh>


#include <string>
#include <cstdio>
#include <iostream> 
#include <sstream> 

formula parse_formula(const char* ltl_str)
{
    auto pf1 = spot::parse_infix_psl(ltl_str);
	if (pf1.format_errors(std::cerr))
	{
        std::string f(ltl_str);
		std::cerr << "Error LTL formula: " << f << std::endl;
		exit(-1);
	}
	// return negation normal form
  formula res = get_nnf(pf1.f);
	return res;
}

bool ensure_safey_ltl(formula& f)
{
    // operators not allowed
     if(f.kind() == op::F || f.kind() == op::U  || f.kind() == op::M || f.kind() == op::Implies || f.kind() == op::Equiv)
    {
        return false;
    }else if(f.kind() == op::R  || f.kind() == op::W  )
    {
        formula f1 = f[0], f2 = f[1];
        return ensure_safey_ltl(f1) && ensure_safey_ltl(f2);
    }else if(f.kind() == op::And || f.kind() == op::Or)
    {
        for(formula child: f)
        {
            if(!ensure_safey_ltl(child)) 
            {
                return false;
            }
        }
        return true;
    }else if(f.kind() == op::X || f.kind() == op::G) {
        formula f1 = f[0];
        return ensure_safey_ltl(f1);
    }else if(f.kind() == op::Not)
    {
        formula f1 = f[0];
        return f1.kind() == op::ap;
    }else if( f.kind() == op::ap) {
        return true;
    }
    if(f.kind() != op::tt && f.kind() != op::ff)
    {
        cerr << "Error formula in ensure_safey_ltl(): " << f << endl;
        exit(-1);
    }
    return true;
}

void decompose_formula(vector<formula>& list, formula f)
{
    // top operator must be AND
    if (f.kind() == op::And)
	{
		for (formula child : f)
		{
            formula neg_formula = formula::Not(child);
            neg_formula = get_nnf(neg_formula);
			list.push_back(neg_formula);
		}
	}
	else
	{
        formula neg_formula = formula::Not(f);
        neg_formula = get_nnf(neg_formula);
		list.push_back(neg_formula);
	}
}

void decompose_formula(vector<string>& formula_strs, formula f)
{
    vector<formula> list_formulas;
    decompose_formula(list_formulas, f);
    formula_strs.clear();
    for(formula subformulas : list_formulas) 
    {
        std::stringstream buffer;
        buffer << subformulas;
        formula_strs.push_back(buffer.str());
    }
}

// push negation into inner formulas
formula push_not_in(formula &f)
{
  formula res;
  if (f.kind() == op::ap)
  {
    res = formula::unop(op::Not, f);
  }
  else if (f.is_ff())
  {
    res = f.tt();
  }
  else if (f.is_tt())
  {
    res = f.ff();
  }
  else
  {
    formula lft, rgt, l, r, res2;
    std::vector<formula> lst;
    switch (f.kind())
    {
    case op::Not:
      lft = f[0];
      res = get_nnf(lft);
      break;
    case op::X:
      // !(X a) = X[!] !a
      lft = f[0];
      r = push_not_in(lft);
      res = formula::unop(op::strong_X, r);
      break;
    case op::strong_X:
      // !(X[!] a) = X !a
      lft = f[0];
      r = push_not_in(lft);
      res = formula::unop(op::X, r);
      break;
    case op::G:
      // !(G a) = F !a
      lft = f[0];
      r = push_not_in(lft);
      res = formula::unop(op::F, r);
      break;
    case op::F:
      // !(F b) = G !b
      lft = f[0];
      r = push_not_in(lft);
      res = formula::unop(op::G, r);
      break;
    case op::U:
      // !(a U b) = !a R !b
      lft = f[0];
      rgt = f[1];
      l = push_not_in(lft);
      r = push_not_in(rgt);
      res = formula::R(l, r);
      break;
    // weak Until
    // φ W ψ ≡ (φ U ψ) ∨ G φ ≡ φ U (ψ ∨ G φ) ≡ ψ R (ψ ∨ φ)
    // !(p W q) = !(G p | (p U q)) = (F !p ) & (!p R !q)
    case op::W:
      l = f[0];
      r = f[1];
      l = push_not_in(l);
      r = push_not_in(r);
      // res2 = ! p;
      res2 = formula::multop(op::And, {l, r}); 
      // input is ! (p W q) === !q U (!p && !q) === ! (q R (p || q))
      res = formula::U(r, res2);
      break;
    case op::R:
      // !(a R b) = !a U !b
      lft = f[0];
      rgt = f[1];
      l = push_not_in(lft);
      r = push_not_in(rgt);
      res = formula::U(l, r);
      break;
    case op::And:
      // !(a & b & ... & c) = !a | !b | ... | !c
      for (formula child : f)
      {
        res = push_not_in(child);
        lst.push_back(res);
      }
      res = formula::multop(op::Or, lst);
      break;
    case op::Or:
      // !(a | b | ... | c) = !a & !b & ... & !c
      for (formula child : f)
      {
        res = push_not_in(child);
        lst.push_back(res);
      }
      res = formula::multop(op::And, lst);
      break;
    case op::Implies:
      // !(a -> b) = a & !b
      lft = f[0];
      rgt = f[1];
      l = get_nnf(lft);
      r = push_not_in(rgt);
      res = formula::multop(op::And, {l, r});
      break;
    case op::Equiv:
      // !(a <-> b) = !(a->b) | !(b->a) = (a | b) & (! a | !b)
      lft = f[0];
      rgt = f[1];
      l = formula::binop(op::Implies, lft, rgt);
      r = formula::binop(op::Implies, rgt, lft);
      res2 = formula::multop(op::And, {l, r});
      res = push_not_in(res2);
      break;
    default:
      throw runtime_error("Error formula in push_not_in()");
      exit(-1);
    }
  }
  return res;
}

// get negation normal form of a formula
formula get_nnf(formula &f)
{
  formula res;
  if (f.kind() == op::ap || f.kind() == op::tt || f.kind() == op::ff)
  {
    res = f;
  }
  else
  {
    formula l = f[0], r, lft, rgt, res2;
    formula lt, rt;
    std::vector<formula> lst;
    switch (f.kind())
    {
    case op::Not:
      res = push_not_in(l);
      break;
    case op::X:
      r = get_nnf(l);
      res = formula::unop(op::X, r);
      break;
    case op::strong_X:
      r = get_nnf(l);
      res = formula::unop(op::strong_X, r);
      break;
    case op::G:
      r = get_nnf(l);
      res = formula::unop(op::G, r);
      break;
    case op::F:
      r = get_nnf(l);
      res = formula::unop(op::F, r);
      break;
    case op::U:
      r = f[1];
      lft = get_nnf(l);
      rgt = get_nnf(r);
      res = formula::U(lft, rgt);
      break;
    // weak Until
    // φ W ψ ≡ (φ U ψ) ∨ G φ ≡ φ U (ψ ∨ G φ) ≡ ψ R (ψ ∨ φ)
    // p W q = G p | (p U q) = (! F !p ) | (p U q)
    case op::W:
      r = f[1];
      l = get_nnf(l);
      r = get_nnf(r);
      // res2 = ! p;
      res2 = formula::multop(op::Or, {l, r});
      res = formula::R(r, res2);
      break;
    case op::R:
      r = f[1];
      lft = get_nnf(l);
      rgt = get_nnf(r);
      res = formula::R(lft, rgt);
      break;
    case op::And:
      for (formula child : f)
      {
        l = get_nnf(child);
        lst.push_back(l);
      }
      res = formula::multop(op::And, lst);
      break;
    case op::Or:
      for (formula child : f)
      {
        l = get_nnf(child);
        lst.push_back(l);
      }
      res = formula::multop(op::Or, lst);
      break;
    case op::Implies:
      r = f[1];
      lft = formula::Not(l);
      rgt = formula::multop(op::Or, {lft, r});
      res = get_nnf(rgt);
      break;
    case op::Equiv:
      // a <-> b = (a->b) & (b->a)
      r = f[1];
      lt = formula::binop(op::Implies, l, r);
      lft = get_nnf(lt);
      rt = formula::binop(op::Implies, r, l);
      rgt = get_nnf(rt);
      res = formula::multop(op::And, {lft, rgt});
      break;
    default:
      cerr << "Formula: " << f << ". ";
      throw runtime_error("Error formula in get_nnf()");
      exit(-1);
    }
  }
  return res;
}

std::vector<std::string> get_props(formula &f){
    std::vector<std::string> names;
    if (f.is_leaf()){
        std::string name = f.ap_name();
        if (std::find(names.begin(), names.end(), name) == names.end()) {
            names.push_back(name);
        }
    }else{
        for (formula child : f)
        {
            std::vector<std::string> child_names =  get_props(child);
            for(auto name : child_names){
                if (std::find(names.begin(), names.end(), name) == names.end()) {
                    names.push_back(name);
                }
            }
        }
    }
    return names;
}

formula progr_last(formula& f) {
  formula res;
  if (f.kind() == op::tt || f.kind() == op::ff) {
    res = f;
  } else {
    formula l = f.size() > 0 ? f[0] : formula();
    formula r, lft, rgt, lt;
    std::vector<formula> lst;
    switch (f.kind()) {
      case op::ap:
        res = formula::ff();
        break;
      case op::Not: // TODO. Is not only in front of atoms? In theory, yes
        // Then, the progression is defined as for atoms
        res = formula::ff();
        // r = progr_last(l);
        // res = formula::unop(op::Not, r);
        break;
      case op::X: // weak next
        res = formula::tt();
        break;
      case op::strong_X: // strong next
        res = formula::ff();
        break;
      case op::G:
        res = formula::tt();
        break;
      case op::F:
        res = formula::ff();
        break;
      case op::U:
        if (f.size() < 2) throw runtime_error("Error: U operator requires two children");
        res = formula::ff();
        break;
      case op::R:
        if (f.size() < 2) throw runtime_error("Error: R operator requires two children");
        res = formula::tt();
        break;
      case op::And:
        for (formula child : f) {
          r = progr_last(child);
          lst.push_back(r);
        }
        res = formula::multop(op::And, lst);
        break;
      case op::Or:
        for (formula child: f) {
          r = progr_last(child);
          lst.push_back(r);
        }
        res = formula::multop(op::Or, lst);
        break;
      case op::Implies:
        if (f.size() < 2) throw runtime_error("Error: Implies operator requires two children");
        r = f[1];
        lft = formula::Not(l);
        rgt = formula::multop(op::Or, {lft, rgt});
        res = progr_last(rgt);
        break;
      case op::Equiv:
        // a <-> b = (a -> b) & (b -> a)
        if (f.size() < 2) throw runtime_error("Error: Equiv operator requires two children");
        r = f[1];
        lft = formula::binop(op::Implies, l, r);
        rgt = formula::binop(op::Implies, r, l);
        res = formula::multop(op::And, {lft, rgt});
        res = progr_last(res);
        break;
      default: 
        cerr << "Formula: " << f << ". ";
        throw runtime_error("Error formula in progr_last()");
        exit(-1);
    }
  }
  return res;
}

formula progr_not_last(formula& f, std::map<formula, formula>* m) {
  formula res;
  if (f.kind() == op::tt || f.kind() == op::ff) {
    res = f;
  } else {
    formula l = f.size() > 0 ? f[0] : formula();
    formula r, lft, rgt, res2;
    std::vector<formula> lst;
    switch (f.kind()) {
      case op::ap:
        res = (*m)[f];
        break;
      case op::Not:
        r = progr_not_last(l, m);
        res = formula::unop(op::Not, r);
        break;
      case op::X:
        res = l;
        break;
      case op::strong_X:
        res = l;
        break;
      case op::G:
        lft = progr_not_last(l, m);
        // rgt = formula::unop(op::X, formula::unop(op::G, l));
        rgt = formula::unop(op::G, l);
        res = formula::multop(op::And, {lft, rgt});
        break;
      case op::F:
        lft = progr_not_last(l, m);
        // rgt = formula::unop(op::strong_X, formula::unop(op::F, l));
        rgt = formula::unop(op::F, l);
        res = formula::multop(op::Or, {lft, rgt});
        break;
      case op::U:
        if (f.size() < 2) throw runtime_error("Error: U operator requires two children");
        r = f[1];
        lft = progr_not_last(r, m); // progr(\phi_2, m)
        rgt = progr_not_last(l, m); // progr(\phi_1, m)
        res2 = formula::binop(op::U, l, r); // \phi_1 U \phi_2
        // res2 = formula::unop(op::strong_X ,res2); // X[!](\phi_1 U \phi_2)
        res2 = formula::multop(op::And, {rgt, res2}); // progr(\phi_1, m) && (\phi_1 U \phi_2)
        res = formula::multop(op::Or, {lft, res2}); // progr(\phi_2, m) || (progr(\phi_1, m) && (\phi_1 U \phi_2))
        break;
      case op::R:
        if (f.size() < 2) throw runtime_error("Error: R operator requires two children");
        r = f[1];
        lft = progr_not_last(r, m); // progr(\phi_2, m)
        rgt = progr_not_last(l, m); // progr(\phi_1, m)
        res2 = formula::binop(op::R, l, r); // \phi_1 R \phi_2
        // res2 = formula::unop(op::X, res2); // X(\phi_1 R \phi_2)
        res2 = formula::multop(op::Or, {rgt, res2}); // (progr(\phi_1, m) || (\phi_1 R \phi_2))
        res = formula::multop(op::And, {lft, res2}); // progr(\phi_2, m) && (progr(\phi_1, m) || (\phi_1 R \phi_2))
        break;
      case op::And:
        for (formula child : f) {
          l = progr_not_last(child, m);
          lst.push_back(l);
        }
        res = formula::multop(op::And, lst);
        break;
      case op::Or:
        for (formula child : f) {
          l = progr_not_last(child, m);
          lst.push_back(l);
        }
        res = formula::multop(op::Or, lst);
        break;
      case op::Implies:
        if (f.size() < 2) throw runtime_error("Error: Implies operator requires two children");
        r = f[1];
        lft = formula::Not(l);
        rgt = formula::multop(op::Or, {lft, rgt});
        res = progr_not_last(rgt, m);
        break;
      case op::Equiv:
        if (f.size() < 2) throw runtime_error("Error: Equiv operator requires two children");
        r = f[1];
        lft = formula::binop(op::Implies, l, r);
        rgt = formula::binop(op::Implies, r, l);
        res = formula::multop(op::And, {lft, rgt});
        res = progr_not_last(res, m);
        break;
      default:
        cerr << "Formula: " << f << ". ";
        throw runtime_error("Error formula in progr_not_last()");
        exit(-1);
    }
  }
  return res;
}

std::pair<formula, formula> progr(formula& f, std::map<formula, formula>* m) {
  std::pair<formula, formula> res;
  res.first = progr_not_last(f, m);
  res.second = progr_last(res.first);
  return res;
}
  