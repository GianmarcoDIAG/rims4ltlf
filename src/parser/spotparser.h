#ifndef SPOT_WANT_STRONG_X
#define SPOT_WANT_STRONG_X 1
#define SPOT_HAS_STRONG_X 1
#endif

#ifndef SPOTPARSER_H
#define SPOTPARSER_H

#include <spot/tl/formula.hh>

#include <vector>
#include <string>
#include <map>

using namespace spot;
using namespace std;

/**
Use spot parser to parse an input LTL formula
**/
formula parse_formula(const char* ltl_str);

/**
check whether the input LTL formula is a safety LTL in negation normal form
**/
bool ensure_safey_ltl(formula& f);

/**
decompose the LTL formula to a list of subformulas
note here that subformulas will be negated and translated into nnf before return
**/
void decompose_formula(vector<formula>& list, formula f);

/**
decompose the LTL formula to a list of strings of subformulas in negation normal form
**/
void decompose_formula(vector<string>& formula_strs, formula f);

/**
push the Not operator in front of aps
*/
formula push_not_in(formula &f);
/**
compute the the negation normal form of an input LTL formula 
*/
formula get_nnf(formula &f);

/**
collect the set of props of a formula
*/
std::vector<std::string> get_props(formula &f);

/**
* \brief progresses an LTLf formula wrt given interpretation
* \param f. LTLf fromula being progressed
* \param m. Interpretation
* \return pair with progression of LTLf formula: first is the progression if !Last; second is progression if Last
*/
std::pair<formula, formula> progr(formula& f, std::map<formula, formula>* m);

/**
 * \brief progresses an LTLf formula if Last
 * \param f. LTLf formula being progressed
 * \return formula progressed if Last
 */
formula progr_last(formula& f);

/**
 * \brief progresses an LTLf formula if not Last
 * \param f. LTLf formula being progressed
 * \param m. Interpretation
 * \return formula progressed if not Last
 */
formula progr_not_last(formula& f, std::map<formula, formula>* m);

#endif