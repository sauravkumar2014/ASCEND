#include "solverparameters.h"
#include "solverparameteriterator.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace std;

//-------------------------------
// SolverParameters

SolverParameters::SolverParameters(const slv_parameters_t &p) : p(p){
	//cerr << "CREATED SOLVERPARAMETERS" << endl;
}

SolverParameters::SolverParameters(){
	/* cerr << "CONSTRUCTED SOLVERPARAMETERS NULL" << endl; */
}

SolverParameters::SolverParameters(const SolverParameters &old) : p(old.p){
	// copy constructor
}

const string
SolverParameters::toString() const{
	stringstream ss;
	ss << "SOLVERPARAMETERS:TOSTRING:" << endl;
	ss << "Number of parameters: " << p.num_parms << endl;

	iterator i;
	for(i = begin(); i<end(); ++i){
		SolverParameter p = *i;
#if 0
		ss << p.getLabel() << " [" << p.getDescription() << "]: ";
#else
		ss << p.getName() << ": ";
#endif
		if(p.isInt()){
			ss << p.getIntValue();
		}else if(p.isReal()){
			ss << p.getRealValue();
		}else if(p.isBool()){
			ss << (p.getBoolValue() ? "true" : "false");
		}else if(p.isStr()){
			ss << p.getStrValue();
		}else{
			throw runtime_error("Unhandled case");
		}
		ss << endl;
	}
	return ss.str();
}

const SolverParameterIterator 
SolverParameters::begin() const{
	return SolverParameterIterator(p.parms);
}

const SolverParameterIterator 
SolverParameters::end() const{
	return SolverParameterIterator(p.parms+p.num_parms);
}

const int 
SolverParameters::getLength() const{
	return p.num_parms;
}

SolverParameter
SolverParameters::getParameter(const int &index) const{
	return SolverParameter(&(p.parms[index]));
}

SolverParameter 
SolverParameters::getParameter(const string &name) const{
	for(int i=0;i<p.num_parms;++i){
		//cerr << "Looking at " << p.parms[i].name << endl;
		if(p.parms[i].name==name){
			return SolverParameter(&(p.parms[i]));
		}
	}
	stringstream ss;
	ss << "Invalid parameter name '" << name << "'";
	throw runtime_error(ss.str());
}

slv_parameters_t &
SolverParameters::getInternalType(){
	return p;
}

