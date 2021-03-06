/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Integrator API for ASCEND, for solving systems of ODEs and/or DAEs.
*//*
	by John Pye, May 2006
	based on parts of Integrators.c in the Tcl/Tk interface directory, heavily
	modified to provide a non-GUI-specific API and modularised for multiple
	integration engines.
*/
#include <time.h>
#include <string.h>

#include <ascend/general/panic.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/link.h>


#include <ascend/system/slv_common.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/block.h>

#include <ascend/solver/solver.h>

#include "integrator.h"
#include "samplelist.h"

#define ANALYSE_DEBUG
/* #define SOLVE_DEBUG */
/* #define CLASSIFY_DEBUG */
/* #define DESTROY_DEBUG */
/* #define ATOL_DEBUG */

/*------------------------------------------------------------------------------
   The following names are of solver_var children or attributes
 * we support (at least temporarily) to determine who is a state and
 * who matching derivative.
 * These should be supported directly in a future solveratominst.
 */

static symchar *g_symbols[3];

#define STATEFLAG g_symbols[0]
/*
	Integer child. 0= algebraic, 1 = state, 2 = derivative, 3 = 2nd deriv etc
	independent variable is -1.
*/
#define INTEG_OTHER_VAR -1L
#define INTEG_ALGEBRAIC_VAR 0L
#define INTEG_STATE_VAR 1L

#define STATEINDEX g_symbols[1]
/* Integer child. all variables with the same STATEINDEX value are taken to
 * be derivatives of the same state variable. We really need a compiler
 * that maintains this info by backpointers, but oh well until then.
 */
#define OBSINDEX g_symbols[2]
/* Integer child. All variables with OBSINDEX !=0 will be sent to the
	IntegratorOutputWriteObsFn allowing output to a file, graph, console, etc.
 */


/** Temporary catcher of dynamic variable and observation variable data */
struct Integ_var_t {
  long index;
  long type;
  struct Integ_var_t *derivative;
  struct Integ_var_t *derivative_of;
  struct var_variable *i;
  int varindx; /**< index into slv_get_master_vars_list, or -1 if not there */
  int isstate;
};

/*------------------------------------------------------------------------------
  forward declarations
*/

/* abstractions of setup/teardown procedures for the specific solvers */
void integrator_create_engine(IntegratorSystem *sys);
void integrator_free_engine(IntegratorSystem *sys);

IntegratorAnalyseFn integrator_analyse_ode;

typedef void (IntegratorVarVisitorFn)(IntegratorSystem *sys, struct var_variable *var, const int *varindx);
void integrator_visit_system_vars(IntegratorSystem *sys,IntegratorVarVisitorFn *visitor);
IntegratorVarVisitorFn integrator_ode_classify_var;
IntegratorVarVisitorFn integrator_dae_classify_var;
IntegratorVarVisitorFn integrator_classify_indep_var;
IntegratorVarVisitorFn integrator_dae_show_var;

static int integrator_sort_obs_vars(IntegratorSystem *sys);
static void integrator_print_var_stats(IntegratorSystem *sys);
static int integrator_check_indep_var(IntegratorSystem *sys);

static int Integ_CmpDynVars(struct Integ_var_t *v1, struct Integ_var_t *v2);
static int Integ_CmpObs(struct Integ_var_t *v1, struct Integ_var_t *v2);
static void Integ_SetObsId(struct var_variable *v, long oindex);

static long DynamicVarInfo(struct var_variable *v,long *vindex, IntegratorSystem *sys);
static struct var_variable *ObservationVar(struct var_variable *v, long *oindex);
static void IntegInitSymbols(void);

/*------------------------------------------------------------------------------
  INSTANTIATION AND DESTRUCTION
*/

/**
	Create a new IntegratorSystem and assign a slv_system_t to it.
*/
IntegratorSystem *integrator_new(slv_system_t slvsys, struct Instance *inst){
	IntegratorSystem *sys;

	if (slvsys == NULL) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"slvsys is NULL!");
		return NULL;
	}

	sys = ASC_NEW_CLEAR(IntegratorSystem);
	sys->system = slvsys;
	sys->instance = inst;

	sys->engine = INTEG_UNKNOWN;
	sys->internals = NULL;

	sys->states = NULL; sys->derivs = NULL;
	sys->dynvars = NULL; sys->obslist = NULL; sys->indepvars = NULL;

	sys->y_id = NULL;
	sys->obs_id = NULL;
	sys->y = NULL;
	sys->ydot = NULL;
	sys->obs = NULL;
	sys->n_y = 0;
	return sys;
}

/**
	Carefully trash any data in the IntegratorSystem that we own,
	then destroy the IntegratorSystem struct.

	Note that the integrator doesn't own the samplelist.

	@param sys will be destroyed and set to NULL.
*/
void integrator_free(IntegratorSystem *sys){
	if(sys==NULL)return;

	integrator_free_engine(sys);

	/* CONSOLE_DEBUG("Engine freed, destroying internal data"); */

	if(sys->states != NULL)gl_destroy(sys->states);
	if(sys->derivs != NULL)gl_destroy(sys->derivs);

	if(sys->dynvars != NULL)gl_free_and_destroy(sys->dynvars);    /* we own the objects in dynvars */
	if(sys->obslist != NULL)gl_free_and_destroy(sys->obslist);    /* and obslist */
	if (sys->indepvars != NULL)gl_free_and_destroy(sys->indepvars);  /* and indepvars */

	if(sys->y_id != NULL)ASC_FREE(sys->y_id);
	if(sys->obs_id != NULL)ASC_FREE(sys->obs_id);
	if(sys->y != NULL)ASC_FREE(sys->y);
	if(sys->ydot != NULL)ASC_FREE(sys->ydot);
	if(sys->obs != NULL)ASC_FREE(sys->obs);

	slv_destroy_parms(&(sys->params));

	ASC_FREE(sys);
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("Destroyed IntegratorSystem");
#endif
	sys=NULL;
}

/**
	Utility function to retreive pointers to the symbols we'll be looking for
	in the instance hierarchy.
*/
static void IntegInitSymbols(void){
	STATEFLAG = AddSymbol("ode_type");
	STATEINDEX = AddSymbol("ode_id");
	OBSINDEX = AddSymbol("obs_id");
}

/*------------------------------------------------------------------------------
  INTEGRATOR ENGINE
*/

/**
	Local function that holds the list of available integrators. The value
	returned is NOT owned by the called.

	@param free_space if 0, call as normal. if 1, free the list and maybe do some
	cleaning up etc. Should be called whenever a simulation is destroyed.
*/
static struct gl_list_t *integrator_get_list(int free_space){
	static int init = 0;
	static struct gl_list_t *L;
	int i, error;
	/* standard integrators that we will register */
	static char *defaultintegrators[] = {
		"lsode"
		,"ida"
		,NULL
	};

	if(free_space){
		if(init && L){
			gl_destroy(L);
			L = NULL;
		}
		init = 0;
		return NULL;
	}
	if(!init){
		L = gl_create(10);
		init = 1; /* set init to 1 here now, since this will be called
			recursively from the LoadArchiveLibrary call. */

		/* CONSOLE_DEBUG("REGISTERING STANDARD SOLVER ENGINES"); */
		for(i=0; defaultintegrators[i]!=NULL;++i){
			error = package_load(defaultintegrators[i],NULL);
			if(error){
				ERROR_REPORTER_HERE(ASC_PROG_ERR
					,"Unable to register integrator '%s' (error %d)."
					,defaultintegrators[i],error
				);
			}else{
				CONSOLE_DEBUG("Integrator '%s' registered OK",defaultintegrators[i]);
			}
		}
	}
	return L;
}

/**
	Return gl_list of IntegratorInternals. C++ will use this to produce a
	nice little list of integrator names that can be used in Python :-/
*/
const struct gl_list_t *integrator_get_engines(){
	return integrator_get_list(0);
}

struct gl_list_t *integrator_get_engines_growable(){
	return integrator_get_list(0);
}

void integrator_free_engines(){
	integrator_get_list(1);
}

/* return 0 on success */
int integrator_set_engine(IntegratorSystem *sys, const char *name){
	struct gl_list_t *L = integrator_get_engines_growable();
	int i;
	const IntegratorInternals *I, *Ifound=NULL;
	for(i=1; i <= gl_length(L); ++i){
		I = gl_fetch(L,i);
		if(strcmp(I->name,name)==0){
			Ifound = I;
			break;
		}
	}
	if(Ifound){
		/** @TODO tests for applicability of this engine... */

		CONSOLE_DEBUG("Setting engine...");
		if(Ifound->engine == sys->engine){
			// already set...
			return 0;
		}

		// clean up if old engine is there
		if(sys->engine!=INTEG_UNKNOWN){
#ifdef DESTROY_DEBUG
			CONSOLE_DEBUG("Freeing memory used by old integrator engine");
#endif
			integrator_free_engine(sys);
#ifdef DESTROY_DEBUG
			CONSOLE_DEBUG("done");
#endif
		}

		sys->engine = Ifound->engine;

		sys->internals = Ifound;
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid engine name '%s'",name);
		return 1;
	}

	asc_assert(sys->internals);
	integrator_create_engine(sys);
	return 0;
}

/**
	@TODO rename this
*/
const IntegratorInternals *integrator_get_engine(const IntegratorSystem *sys){
	return sys->internals;
}

/**
	Free any engine-specific  data that was required for the solution of
	this system. Note that this data is pointed to by sys->enginedata.

	@TODO rename this, bad choice/confusing name.
*/
void integrator_free_engine(IntegratorSystem *sys){
	if(sys->engine==INTEG_UNKNOWN)return;
	if(sys->enginedata){
		if(sys->internals){
#ifdef DESTROY_DEBUG
			CONSOLE_DEBUG("Running engine's freefn");
#endif
			(sys->internals->freefn)(sys->enginedata);
#ifdef DESTROY_DEBUG
			CONSOLE_DEBUG("Done with freefn");
#endif
			sys->enginedata=NULL;
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to free engine data: no sys->internals");
		}
	}
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("Done with integrator_free_engine");
#endif
}

/**
	Create enginedata memory if required for this solver. This doesn't include
	allocating computation space, since we assume that this stage all we know
	is that we want to use a specified integrator engine, not the full details
	of the problem at hand. Allocating space inside enginedata should be done
	during the solve stage (and freed inside integrator_free_engine)

	@TODO rename this
*/
void integrator_create_engine(IntegratorSystem *sys){
	asc_assert(sys);
	asc_assert(sys->engine!=INTEG_UNKNOWN);
	asc_assert(sys->internals);
	asc_assert(sys->internals->createfn);
	asc_assert(sys->enginedata==NULL);
	(sys->internals->createfn)(sys);
}

int integrator_register(const IntegratorInternals *integ){
	/* get the current list of registered engines */
	struct gl_list_t *L;
	L = integrator_get_engines_growable();

	CONSOLE_DEBUG("REGISTERING INTEGRATOR");
	CONSOLE_DEBUG("There were %lu registered integrators", gl_length(integrator_get_list(0)));

	int i;
	IntegratorInternals *I;
	for(i=1; i <= gl_length(L); ++i){
		I = (IntegratorInternals *)gl_fetch(L,i);
		if(strcmp(integ->name,I->name)==0){
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Integrator with name '%s' is already registered",integ->name);
			return 0;
		}
		if(integ->engine == I->engine){
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Integrator with ID '%d' is already registered",integ->engine);
			return 0;
		}
	}

	CONSOLE_DEBUG("Adding engine '%s'",integ->name);

	gl_append_ptr(L,(void *)integ);

	CONSOLE_DEBUG("There are now %lu registered integrators", gl_length(integrator_get_list(0)));
	return 0;
}

/*------------------------------------------------------------------------------
  PARAMETERS
*/

/**
	Reset the parameters in this IntegratorSystem to the default ones for this
	Integrator.

	@return 0 on success, 1 on error
*/
int integrator_params_default(IntegratorSystem *sys){
	asc_assert(sys->engine!=INTEG_UNKNOWN);
	asc_assert(sys->internals);
	return (sys->internals->paramsdefaultfn)(sys);
}

int integrator_params_get(const IntegratorSystem *sys, slv_parameters_t *parameters){
	asc_assert(sys!=NULL);
	asc_assert(sys->params.num_parms > 0);
	memcpy(parameters,&(sys->params),sizeof(slv_parameters_t));
	return 0;
}

int integrator_params_set(IntegratorSystem *sys, const slv_parameters_t *parameters){
	asc_assert(sys!=NULL);
	asc_assert(parameters!=NULL);
	memcpy(&(sys->params),parameters,sizeof(slv_parameters_t));
	return 0;
}

/*------------------------------------------------------------------------------
  ANALYSIS

  Provide two modes in order to provide analysis suitable for solution of both
  ODEs (the previous technique) and DAEs (new code). These share a common
  "visit" method that needs to eventually be integrated with the code in
  <solver/analyze.c>. For the moment, we're just hacking in to the compiler.
*/

/**
	Locate the independent variable.

	@NOTE For the purpose of GUI design, this needs	to work independent of the
	integration engine being used. @ENDNOTE
*/
int integrator_find_indep_var(IntegratorSystem *sys){
	int result = 0;
#ifdef ANALYSE_DEBUG
	char *varname;
#endif

	/* if the indep var has been found, we don't look again (we assume the user won't fiddle with ode_type!) */
	if(sys->x != NULL){
		CONSOLE_DEBUG("sys->x already set");
		return 0; /* success */
	}

	/* create a clear indepvars list */
	if(sys->indepvars!=NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"indepvars should be NULL at this point");
		return 1; /* error */
	}
	sys->indepvars = gl_create(10L);

	IntegInitSymbols();

	CONSOLE_DEBUG("Looking for independent var...");
	integrator_visit_system_vars(sys,&integrator_classify_indep_var);
#ifdef ANALYSE_DEBUG
	if(gl_length(sys->indepvars)){
		CONSOLE_DEBUG("Found %lu indepvars",gl_length(sys->indepvars));
	}else{
		CONSOLE_DEBUG("NO INDEP VARS FOUND");
	}
#endif

	/* after visiting the instance tree, look at the candidates and return 0 on success */
	result = integrator_check_indep_var(sys);

	/* whatever happens, we clean up afterwards */
	gl_free_and_destroy(sys->indepvars);
	sys->indepvars = NULL;

#ifdef ANALYSE_DEBUG
	asc_assert(sys->system);
	if(!result){
		asc_assert(sys->x);
		varname = var_make_name(sys->system, sys->x);
		CONSOLE_DEBUG("Indep var is '%s'",varname);
		ASC_FREE(varname);
	}else{
		CONSOLE_DEBUG("No indep var was found");
	}
#endif

	/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Returning result %d",result); */

	return result;
}

/**
	Perform whatever additional problem is required so that the system can be
	integrated as a dynamical system with the IntegrationEngine chosen.

	We can always assume that sys->system has had analyse_make_problem called on
	it, so all the variable lists (etc) will be there already.

	@return 0 on success
*/
int integrator_analyse(IntegratorSystem *sys){
	int res;

#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("Analysing integration system...");
#endif
	asc_assert(sys);
	if(sys->engine==INTEG_UNKNOWN){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No engine selected: can't analyse");
		return 1;
	}
	asc_assert(sys->engine!=INTEG_UNKNOWN);
	asc_assert(sys->internals);

	if(!sys->indepvars || !gl_length(sys->indepvars)){
		if(integrator_find_indep_var(sys)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Independent variable problem: abandoning integration");
			return 2;
		}
#ifdef ANALYSE_DEBUG
		else{
			CONSOLE_DEBUG("got 0 from  integrator_find_indep_var");
		}
#endif
	}

	res = (sys->internals->analysefn)(sys);
#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("integrator_analyse returning %d",res);
#endif
	return res;
}


void integrator_visit_system_vars(IntegratorSystem *sys,IntegratorVarVisitorFn *visitfn){
  struct var_variable **vlist;
  int i, vlen;

  /* visit all the slv_system_t master var lists to collect vars */
  /* find the vars mostly in this one */
  vlist = slv_get_solvers_var_list(sys->system);
  vlen = slv_get_num_solvers_vars(sys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(sys, vlist[i], &i);
  }

  /*
  CONSOLE_DEBUG("Checked %d vars",vlen);
  integrator_print_var_stats(sys);
  */

  /* probably nothing here, but gotta check. */
  vlist = slv_get_master_par_list(sys->system);
  vlen = slv_get_num_master_pars(sys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(sys, vlist[i], NULL);
  }

  /*
  CONSOLE_DEBUG("Checked %d pars",vlen);
  integrator_print_var_stats(sys);
  */

  /* might find t here */
  vlist = slv_get_master_unattached_list(sys->system);
  vlen = slv_get_num_master_unattached(sys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(sys, vlist[i], NULL);
  }

  /* CONSOLE_DEBUG("Checked %d unattached",vlen); */
}
/**
	Analyse the ODE structure. We can assume that the independent variable was
	already found.

	@return 0 on success
*/
int integrator_analyse_ode(IntegratorSystem *sys){
  struct Integ_var_t *v1,*v2;
  long half,i,len;
  int happy=1;
  char *varname1, *varname2;

  asc_assert(sys->system!=NULL);

  if(strcmp(slv_solver_name(slv_get_selected_solver(sys->system)),"QRSlv")!=0){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System must have solver 'QRSlv' assigned to it before integration");
	return 2;
  }
  CONSOLE_DEBUG("Checked that NLA solver is set to '%s'",slv_solver_name(slv_get_selected_solver(sys->system)));

  CONSOLE_DEBUG("Starting ODE analysis");
  IntegInitSymbols();

  /* collect potential states and derivatives */
  sys->indepvars = gl_create(10L);  /* t var info */
  sys->dynvars = gl_create(200L);  /* y ydot var info */
  sys->obslist = gl_create(100L);  /* obs info */
  if (sys->dynvars == NULL
    || sys->obslist == NULL
    || sys->indepvars == NULL
  ){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 1;
  }

  sys->nstates = sys->nderivs = 0;

  integrator_visit_system_vars(sys,&integrator_ode_classify_var);

  integrator_print_var_stats(sys);

  /* check sanity of state and var lists */

  len = gl_length(sys->dynvars);
  half = len/2;
  CONSOLE_DEBUG("NUMBER OF DYNAMIC VARIABLES = %ld",half);

  if (len % 2 || len == 0L || sys->nstates != sys->nderivs ) {
    /* list length must be even for vars to pair off */
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"n_y != n_ydot, or no dynamic vars found. Fix your indexing.");
    return 3;
  }
  gl_sort(sys->dynvars,(CmpFunc)Integ_CmpDynVars);
  if (gl_fetch(sys->dynvars,len)==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Mysterious NULL found!");
    return 4;
  }
  sys->states = gl_create(half);   /* state vars Integ_var_t references */
  sys->derivs = gl_create(half);   /* derivative var atoms */
  for (i=1;i < len; i+=2) {
    v1 = (struct Integ_var_t *)gl_fetch(sys->dynvars,i);
    v2 = (struct Integ_var_t *)gl_fetch(sys->dynvars,i+1);
    if (v1->type!=1  || v2 ->type !=2 || v1->index != v2->index) {
      varname1 = var_make_name(sys->system,v1->i);
	  varname2 = var_make_name(sys->system,v2->i);

      ERROR_REPORTER_HERE(ASC_USER_ERROR,"Mistyped or misindexed dynamic variables: %s (%s = %ld,%s = %ld) and %s (%s = %ld,%s = %ld).",
             varname1, SCP(STATEFLAG),v1->type,SCP(STATEINDEX),v1->index,
             varname2, SCP(STATEFLAG),v2->type,SCP(STATEINDEX),v2->index
		);
      ASC_FREE(varname1);
      ASC_FREE(varname2);
      happy=0;
      break;
    } else {
      gl_append_ptr(sys->states,(POINTER)v1);
      gl_append_ptr(sys->derivs,(POINTER)v2->i);
    }
  }
  if (!happy) {
	ERROR_REPORTER_HERE(ASC_USER_ERROR,"Problem with ode_id and ode_type values");
    return 5;
  }
  sys->n_y = half;
  sys->y = ASC_NEW_ARRAY(struct var_variable *, half);
  sys->ydot = ASC_NEW_ARRAY(struct var_variable *, half);
  sys->y_id = ASC_NEW_ARRAY(int, half);
  if (sys->y==NULL || sys->ydot==NULL || sys->y_id==NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 6;
  }
  for (i = 1; i <= half; i++) {
    v1 = (struct Integ_var_t *)gl_fetch(sys->states,i);
    sys->y[i-1] = v1->i;
    sys->y_id[i-1] = v1->index;
    sys->ydot[i-1] = (struct var_variable *)gl_fetch(sys->derivs,i);
  }

  if(integrator_sort_obs_vars(sys)){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error sorting observation variables");
	return 7;
  }

  /* FIX all states */
  for(i=0; i<sys->n_y; ++i){
	if(!var_fixed(sys->y[i])){
      ERROR_REPORTER_HERE(ASC_USER_WARNING,"Fixing state %d",i);
	  var_set_fixed(sys->y[i], TRUE);
	}
  }

  /* don't need the gl_lists now that we have arrays for everyone */
  gl_destroy(sys->states);
  gl_destroy(sys->derivs);
  gl_free_and_destroy(sys->indepvars);  /* we own the objects in indepvars */
  gl_free_and_destroy(sys->dynvars);    /* we own the objects in dynvars */
  gl_free_and_destroy(sys->obslist);    /* and obslist */
  sys->states = NULL;
  sys->derivs = NULL;
  sys->indepvars = NULL;
  sys->dynvars = NULL;
  sys->obslist = NULL;

  /* analysis completed OK */
  return 0;
}

/**
	Reindex observations. Sort if the user mostly numbered. Take natural order
	if user just booleaned.

	@return 0 on success
*/
static int integrator_sort_obs_vars(IntegratorSystem *sys){
  int half, i, len = 0;
  struct Integ_var_t *v2;

  half = sys->n_y;
  len = gl_length(sys->obslist);
  /* we shouldn't be seeing NULL here ever except if malloc fail. */
  if (len > 1L) {
    half = ((struct Integ_var_t *)gl_fetch(sys->obslist,1))->index;
    /* half != 0 now because we didn't collect 0 indexed vars */
    for (i=2; i <= len; i++) {
      if (half != ((struct Integ_var_t *)gl_fetch(sys->obslist,i))->index) {
        /* change seen. sort and go on */
        gl_sort(sys->obslist,(CmpFunc)Integ_CmpObs);
        break;
      }
    }
  }
  for (i = half = 1; i <= len; i++) {
    v2 = (struct Integ_var_t *)gl_fetch(sys->obslist,i);
    if (v2==NULL) {
      /* we shouldn't be seeing NULL here ever except if malloc fail. */
      gl_delete(sys->obslist,i,0); /* should not be gl_delete(so,i,1) */
    } else {
      Integ_SetObsId(v2->i,half);
      v2->index = half++;
    }
  }

  /* obslist now uniquely indexed, no nulls */
  /* make into arrays */
  half = gl_length(sys->obslist);
  sys->obs = ASC_NEW_ARRAY(struct var_variable *,half);
  sys->obs_id = ASC_NEW_ARRAY(int, half);
  if ( sys->obs==NULL || sys->obs_id==NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 1;
  }
  sys->n_obs = half;
  for (i = 1; i <= half; i++) {
    v2 = (struct Integ_var_t *)gl_fetch(sys->obslist,i);
    sys->obs[i-1] = v2->i;
    sys->obs_id[i-1] = v2->index;
  }

  return 0;
}

static void integrator_print_var_stats(IntegratorSystem *sys){
	int v = gl_length(sys->dynvars);
	int i = gl_length(sys->indepvars);
	CONSOLE_DEBUG("Currently %d vars, %d indep",v,i);
}

/**
	Check sanity of the independent variable.

	@return 0 on success
*/
static int integrator_check_indep_var(IntegratorSystem *sys){
  int len, i;
  struct Integ_var_t *info;
  char *varname;

  if(sys->x){
	CONSOLE_DEBUG("Indep var already assigned");
	return 1;
  }

  /* check the sanity of the independent variable */
  len = gl_length(sys->indepvars);
  if (!len) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"No independent variable found.");
    return 2;
  }
  if (len > 1) {
	ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
    FPRINTF(ASCERR,"Excess %ld independent variables found:",
      len);
    for(i=1; i <=len;i++) {
      info = (struct Integ_var_t *)gl_fetch(sys->indepvars,i);
      if(info==NULL)continue;

      varname = var_make_name(sys->system,info->i);
      FPRINTF(ASCERR," %s",varname);
      ASC_FREE(varname);
    }
    FPRINTF(ASCERR , "\nSet the '%s' flag on all but one of these to %s >= 0.\n"
        , SCP(STATEFLAG),SCP(STATEFLAG)
	);
	error_reporter_end_flush();
    return 3;
  }else{
    info = (struct Integ_var_t *)gl_fetch(sys->indepvars,1);
    sys->x = info->i;
  }
  asc_assert(gl_length(sys->indepvars)==1);
  asc_assert(sys->x);
  return 0;
}

/*------------------------------------------------------------------------------
  CLASSIFICATION OF VARIABLES (for ANALYSIS step)
*/

#define INTEG_ADD_TO_LIST(info,TYPE,INDEX,VAR,VARINDX,LIST) \
	info = ASC_NEW(struct Integ_var_t); \
	if(info==NULL){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory (INTEG_VAR_NEW)"); \
		return; \
	} \
	info->type=TYPE; \
	info->index=INDEX; \
	info->i=VAR; \
	info->derivative=NULL; \
	info->derivative_of=NULL; \
    if(VARINDX==NULL){ \
		info->varindx = -1; \
	}else{ \
		info->varindx = *VARINDX; \
	} \
	gl_append_ptr(LIST,(void *)info); \
	info = NULL

/**
	In a DAE, it's either the (single) independent variable, or it's a
	variable in the model.

	I'm not sure what we should be doing with variables that are already
	present as derivatives of other variables, I guess those ones need to be
	removed from the list in a second pass?
*/
void integrator_dae_classify_var(IntegratorSystem *sys
		, struct var_variable *var, const int *varindx
){
	struct Integ_var_t *info;
	long type,index;

	/* filter for recognition of solver_vars */
	var_filter_t vfilt;
	vfilt.matchbits = VAR_SVAR;
	vfilt.matchvalue = VAR_SVAR;

	asc_assert(var != NULL && var_instance(var)!=NULL );

	if( var_apply_filter(var,&vfilt) ) {
		if(!var_active(var)){
			CONSOLE_DEBUG("VARIABLE IS NOT ACTIVE");
			return;
		}

		/* only non-fixed variables are accepted */
		if(!var_fixed(var)){
			/* get the ode_type and ode_id of this solver_var */
			type = DynamicVarInfo(var,&index,sys);

			if(type==INTEG_OTHER_VAR){
				/* if the var's type is -1, it's independent */
				INTEG_ADD_TO_LIST(info,INTEG_OTHER_VAR,0,var,varindx,sys->indepvars);
			}else{
				if(type < 0)type=0;
				/* any other type of var is in the DAE system, at least for now */
				INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->dynvars);
			}
		}
#if 1
		else{
			/* fixed variable, only include it if ode_type == 1 */
			type = DynamicVarInfo(var,&index,sys);
			if(type==INTEG_STATE_VAR){
				INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->dynvars);
			}
		}
#endif

		/* if the var's obs_id > 0, add it to the observation list */
		if(ObservationVar(var,&index) != NULL && index > 0L) {
			INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->obslist);
		}
	}
}

/**
	Inspect a specific variable and work out what type it is (what 'ode_type' it
	has) and what other variable(s) it corresponds to (ie dydt corresponds to
	y as a derivative).

	@TODO add ability to create new variables for 'missing' derivative vars?
*/
void integrator_ode_classify_var(IntegratorSystem *sys, struct var_variable *var
		, const int *varindx
){
  struct Integ_var_t *info;
  long type,index;

  var_filter_t vfilt;
  vfilt.matchbits = VAR_SVAR;
  vfilt.matchvalue = VAR_SVAR;

  asc_assert(var != NULL && var_instance(var)!=NULL );

  if( var_apply_filter(var,&vfilt) ) {
	/* it's a solver var: what type of variable? */
    type = DynamicVarInfo(var,&index,sys);

    if(type==INTEG_ALGEBRAIC_VAR){
		/* no action required */
	}else if(type==INTEG_OTHER_VAR){
		/* i.e. independent var */
        INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->indepvars);
	}else if(type>=INTEG_STATE_VAR){
        INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->dynvars);
        if(type == 1){
          sys->nstates++;
        }else if(type == 2){ /* what about higher-order derivatives? -- JP */
          sys->nderivs++;
        }else{
		  ERROR_REPORTER_HERE(ASC_USER_WARNING,"Higher-order (>=2) derivatives are not supported in ODEs.");
		}	}

    if(ObservationVar(var,&index) != NULL && index > 0L) {
		INTEG_ADD_TO_LIST(info,0L,index,var,varindx,sys->obslist);
    }
  }
}

/**
	Look at a variable and determine if it's the independent variable or not.
	This is just for the purpose of the integrator_find_indep_var function,
	which is a utility function provided for use by the GUI.
*/
void integrator_classify_indep_var(IntegratorSystem *sys
		, struct var_variable *var, const int *varindx
){
	struct Integ_var_t *info;
	long type,index;
	var_filter_t vfilt;
#ifdef CLASSIFY_DEBUG
	char *varname;
#endif

	asc_assert(var != NULL && var_instance(var)!=NULL );
	vfilt.matchbits = VAR_SVAR;
	vfilt.matchvalue = VAR_SVAR;

#ifdef CLASSIFY_DEBUG
	varname = var_make_name(sys->system,var);
#endif

	if( var_apply_filter(var,&vfilt) ) {
		type = DynamicVarInfo(var,&index,sys);

		if(type==INTEG_OTHER_VAR){
			/* i.e. independent var */
#ifdef CLASSIFY_DEBUG
			CONSOLE_DEBUG("Var '%s' added to indepvars",varname);
#endif
			INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->indepvars);
#ifdef CLASSIFY_DEBUG
		}else{
			CONSOLE_DEBUG("Var '%s' not correct ode_type",varname);
#endif
		}
#ifdef CLASSIFY_DEBUG
	}else{
		CONSOLE_DEBUG("Var '%s' failed filter",varname);
#endif
	}

#ifdef CLASSIFY_DEBUG
	ASC_FREE(varname);
#endif
}


/**
	Look at a variable, and if it is an 'ODE variable' (it has a child instance
	named 'ode_type') return its type, which will be either:
		- INTEG_OTHER_VAR (if 'ode_type' is -1)
		- INTEG_ALGEBRAIC_VAR (if 'ode_type' is zero or any negative value < -1)
		- INTEG_STATE_VAR (if 'ode_type' is 1)
		- values 2, 3 or up, indicating derivatives (1st deriv=2, 2nd deriv=3, etc)

	If the parameter 'index' is not null, the value of 'ode_id' will be stuffed
	there.
*/
static long DynamicVarInfo(struct var_variable *v,long *index, IntegratorSystem *sys){
  struct Instance *c, *d, *i;
	int type;

  i = var_instance(v);

  asc_assert(i!=NULL);
  asc_assert(STATEFLAG!=NULL);
  asc_assert(STATEINDEX!=NULL);
  c = ChildByChar(i,STATEFLAG);
  d = ChildByChar(i,STATEINDEX);



  /* lazy evaluation is important in the following if */
  if(c == NULL
      || d == NULL
      || InstanceKind(c) != INTEGER_INST
      || InstanceKind(d) != INTEGER_INST
      || !AtomAssigned(c)
      || (!AtomAssigned(d) && GetIntegerAtomValue(c) != INTEG_OTHER_VAR)
  ){
		type = getOdeType(sys->instance,i);
		if(type != 0) {
			if(index !=NULL){
				*index = getOdeId(sys->instance,i);
			}
 			return type;
		}

    return INTEG_ALGEBRAIC_VAR;
  }
  if (index != NULL) {
    *index = GetIntegerAtomValue(d);
  }
  return GetIntegerAtomValue(c);
}

/**
	Looks at the given variable checks if it is an 'observation variable'. This
	means that it has its 'obs_id' child instance set to a non-zero value.

	If the variable is an observation variable, its index value ('obs_id') is
	stuff into *index (provided index!=NULL), and the pointer to the original
	instance is rtruend.

	If it's not an observation variable, we return NULL and *index is untouched.
 */
static struct var_variable *ObservationVar(struct var_variable *v, long *index){
  struct Instance *c,*i;
  i = var_instance(v);
  asc_assert(i!=NULL);
  c = ChildByChar(i,OBSINDEX);
  if( c == NULL || InstanceKind(c) != INTEGER_INST || !AtomAssigned(c)) {
    return NULL;
  }
  if (index != NULL) {
    *index = GetIntegerAtomValue(c);
  }
  return v;
}

/*------------------------------------------------------------------------------
  RUNNING THE SOLVER
*/

/*
	Make the call to the actual integrator we've selected, for the range of
	time values specified. The sys contains all the specifics.

	Return 0 on success
*/
int integrator_solve(IntegratorSystem *sys, long i0, long i1){

	long nstep;
	unsigned long start_index=0, finish_index=0;
	asc_assert(sys!=NULL);

	asc_assert(sys->internals);
	asc_assert(sys->engine!=INTEG_UNKNOWN);

	nstep = integrator_getnsamples(sys)-1;
	/* check for at least 2 steps and dimensionality of x vs steps here */

	if (i0<0 || i1 <0) {
		/* removed completely inappropriate interactive code here */
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Console input of integration limits has been disabled!");
		return -1;
	} else {
		start_index=i0;
		finish_index =i1;
		if (start_index >= (unsigned long)nstep) {
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Start point (=%lu) must be an index in the range [0,%li]."
				,start_index,nstep
			);
			return -2;
		}
		if (finish_index > (unsigned long)nstep) {
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"End point (=%lu) must be an index in the range [0,%li]."
				,finish_index,nstep
			);
			return -3;
		}
    }

	if(finish_index <= start_index) {
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"End point comes before start point! (start=%lu, end=%lu)"
			,start_index,finish_index
		);
		return -4;
	}

	CONSOLE_DEBUG("RUNNING INTEGRATION...");

	return (sys->internals->solvefn)(sys,start_index,finish_index);
}

/*---------------------------------------------------------------
  HANDLING THE LIST OF TIMESTEMPS
*/

#define GETTER_AND_SETTER(TYPE,NAME) \
	void integrator_set_##NAME(IntegratorSystem *sys, TYPE val){ \
		sys->NAME=val; \
	} \
	TYPE integrator_get_##NAME(IntegratorSystem *sys){ \
		return sys->NAME; \
	}

GETTER_AND_SETTER(SampleList *,samples) /*;*/
GETTER_AND_SETTER(double,maxstep) /*;*/
GETTER_AND_SETTER(double,minstep) /*;*/
GETTER_AND_SETTER(double,stepzero) /*;*/
GETTER_AND_SETTER(int,maxsubsteps) /*;*/
#undef GETTER_AND_SETTER

long integrator_getnsamples(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	asc_assert(sys->samples!=NULL);
	return samplelist_length(sys->samples);
}

double integrator_getsample(IntegratorSystem *sys, long i){
	asc_assert(sys!=NULL);
	asc_assert(sys->samples!=NULL);
	return samplelist_get(sys->samples,i);
}

void integrator_setsample(IntegratorSystem *sys, long i,double xi){
	asc_assert(sys!=NULL);
	asc_assert(sys->samples!=NULL);
	samplelist_set(sys->samples,i,xi);
}

const dim_type *integrator_getsampledim(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	asc_assert(sys->samples!=NULL);
	return samplelist_dim(sys->samples);
}

ASC_DLLSPEC long integrator_getcurrentstep(IntegratorSystem *sys){
	return sys->currentstep;
}

/*------------------------------------------------------------------------------
  GET/SET VALUE OF THE INDEP VARIABLE
*/

/**
	Retrieve the value of the independent variable (time) from ASCEND
	and return it as a double.
*/
double integrator_get_t(IntegratorSystem *sys){
	asc_assert(sys->x!=NULL);
	return var_value(sys->x);
}

/**
	Set the value of the independent variable (time) in ASCEND.
*/
void integrator_set_t(IntegratorSystem *sys, double value){
  var_set_value(sys->x, value);
  /* CONSOLE_DEBUG("set_t = %g", value); */
}

/*------------------------------------------------------------------------------
  PASSING DIFFERENTIAL VARIABLES AND THEIR DERIVATIVES TO/FROM THE SOLVER
*/
/**
	Retrieve the current values of the derivatives of the y-variables
	and stick them in the/an array that the integrator will use.

	If the pointer 'y' is NULL, the necessary space is allocated (and
	must be freed somewhere else).
*/
double *integrator_get_y(IntegratorSystem *sys, double *y) {
  long i;

  if (y==NULL) {
    y = ASC_NEW_ARRAY_CLEAR(double, sys->n_y+1);
    /* C y[0]  <==> ascend d.y[1]  <==>  f77 y(1) */
  }

  for (i=0; i< sys->n_y; i++) {
	asc_assert(sys->y[i]!=NULL);
    y[i] = var_value(sys->y[i]);
    /* CONSOLE_DEBUG("ASCEND --> y[%ld] = %g", i+1, y[i]); */
  }
  return y;
}

/**
	Take the values of the differential variables from the array that the
	integrator uses, and use them to update the values of the corresponding
	variables in ASCEND.
*/
void integrator_set_y(IntegratorSystem *sys, double *y) {
  long i;
#ifdef SOLVE_DEBUG
  char *varname;
#endif

  for (i=0; i < sys->n_y; i++) {
	asc_assert(sys->y[i]!=NULL);
    var_set_value(sys->y[i],y[i]);
#ifdef SOLVE_DEBUG
	varname = var_make_name(sys->system, sys->y[i]);
	CONSOLE_DEBUG("y[%ld] = %g --> '%s'", i+1, y[i], varname);
	ASC_FREE(varname);
#endif
  }
}

/**
	Send the values of the derivatives of the 'y' variables to the solver.
	Allocate space for an array if necessary.

	Any element in sys->ydot that is NULL will be passed over (the value
	won't be modified in dydx).
*/
double *integrator_get_ydot(IntegratorSystem *sys, double *dydx) {
  long i;

  if (dydx==NULL) {
    dydx = ASC_NEW_ARRAY_CLEAR(double, sys->n_y+1);
    /* C dydx[0]  <==> ascend d.dydx[1]  <==>  f77 ydot(1) */
  }

  for (i=0; i < sys->n_y; i++) {
    if(sys->ydot[i]!=NULL){
		dydx[i] = var_value(sys->ydot[i]);
	}
    /* CONSOLE_DEBUG("ASCEND --> ydot[%ld] = %g", i+1, dydx[i]); */
  }
  return dydx;
}

void integrator_set_ydot(IntegratorSystem *sys, double *dydx) {
	long i;
#ifdef SOLVE_DEBUG
	char *varname;
#endif
	for (i=0; i < sys->n_y; i++) {
		if(sys->ydot[i]!=NULL){
    		var_set_value(sys->ydot[i],dydx[i]);
#ifdef SOLVE_DEBUG
			varname = var_make_name(sys->system, sys->ydot[i]);
			CONSOLE_DEBUG("ydot[%ld] = \"%s\" = %g --> ASCEND", i+1, varname, dydx[i]);
			ASC_FREE(varname);
		}else{
			CONSOLE_DEBUG("ydot[%ld] = %g (internal)", i+1, dydx[i]);
#endif
		}
	}
}

/**
	Retrieve the values of 'ode_atol' properties of each of y-variables,
	for use in setting absolute error tolerances for the Integrator.

	If the pointer 'atol' is NULL, the necessary space is allocated (and
	must be freed somewhere else).
*/
double *integrator_get_atol(IntegratorSystem *sys, double *atol){
	long i;
#ifdef ATOL_DEBUG
	char *varname;
#endif

	if (atol==NULL) {
		atol = ASC_NEW_ARRAY_CLEAR(double, sys->n_y);
	}

	for (i=0; i< sys->n_y; i++) {
		asc_assert(sys->y[i]!=NULL);
		atol[i] = var_odeatol(sys->y[i]);
		asc_assert(atol[i]!=-1);
#ifdef ATOL_DEBUG
		varname = var_make_name(sys->system,sys->y[i]);
		CONSOLE_DEBUG("%s.ode_atol = %8.2e",varname,atol[i]);
		ASC_FREE(varname);
#endif
	}
	return atol;
}

/*-------------------------------------------------------------
  RETRIEVING OBSERVATION DATA
*/

/**
   This function takes the inst in the solver and returns the vector of
   observation variables that are located in the submodel d.obs array.
*/
double *integrator_get_observations(IntegratorSystem *sys, double *obsi) {
  long i;

  if (obsi==NULL) {
    obsi = ASC_NEW_ARRAY_CLEAR(double, sys->n_obs+1);
  }

  /* C obsi[0]  <==> ascend d.obs[1] */

  for (i=0; i < sys->n_obs; i++) {
    obsi[i] = var_value(sys->obs[i]);
    /* CONSOLE_DEBUG("*get_d_obs[%ld] = %g\n", i+1, obsi[i]); */
  }
  return obsi;
}

struct var_variable *integrator_get_observed_var(IntegratorSystem *sys, const long i){
	asc_assert(i>=0);
	asc_assert(i<sys->n_obs);
	return sys->obs[i];
}

/**
	@NOTE Although this shouldn't be required for implementation of solver
	engines, this is useful for GUI reporting of integration results.
*/
struct var_variable *integrator_get_independent_var(IntegratorSystem *sys){
	return sys->x;
}

int integrator_write_matrix(const IntegratorSystem *sys, FILE *fp,const char *type){
	asc_assert(sys);
	asc_assert(sys->enginedata);
	asc_assert(sys->internals);
	asc_assert(sys->internals->name);
	if(sys->internals->writematrixfn){
		if(type!=NULL && strcmp(type,"")==0)type = NULL;
		return (sys->internals->writematrixfn)(sys,fp,type);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Integrator '%s' defines no write_matrix function.",sys->internals->name);
		return -1;
	}
}

int integrator_debug(const IntegratorSystem *sys, FILE *fp){
	asc_assert(sys);
	asc_assert(sys->enginedata);
	asc_assert(sys->internals);
	asc_assert(sys->internals->name);
	if(sys->internals->debugfn){
		return (sys->internals->debugfn)(sys,fp);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Integrator '%s' defines no write_matrix function.",sys->internals->name);
		return -1;
	}
}

/*----------------------------------------------------
	Build an analytic jacobian for solving the state system

	This necessarily ugly piece of code attempts to create a unique
	list of relations that explicitly contain the variables in the
	given input list. The utility of this information is that we know
	exactly which relations must be differentiated, to fill in the
	df/dy matrix. If the problem has very few derivative terms, this will
	be of great savings. If the problem arose from the discretization of
	a pde, then this will be not so useful. The decision wether to use
	this function or to simply differentiate the entire relations list
	must be done before calling this function.

	Final Note: the callee owns the array, but not the array elements.
 */
#define AVG_NUM_INCIDENT 4


/**
	This function helps to arrange the observation variables in a sensible order.
	The 'obs_id' child instance of v, if present, is assigned the value of the
	given parameter 'index'.
*/
static void Integ_SetObsId(struct var_variable *v, long index){
  struct Instance *c, *i;
  i = var_instance(v);
  asc_assert(i!=NULL);
  c = ChildByChar(i,OBSINDEX);
  if( c == NULL || InstanceKind(c) != INTEGER_INST || !AtomAssigned(c)) {
    return;
  }
  SetIntegerAtomValue(c,index,0);
}

/**
	Compares observation structs. NULLs should end up at far end.
*/
static int Integ_CmpObs(struct Integ_var_t *v1, struct Integ_var_t *v2){
  if(v1 == NULL)return 1;
  if(v2 == NULL)return -1;
  if(v1->index > v2->index)return 1;
  if(v1->index == v2->index)return 0;
  return -1;
}

/**
	Compares dynamic vars structs. NULLs should end up at far end.
	List should be sorted primarily by index and then by type, in order
	of increasing value of both.
*/
static int Integ_CmpDynVars(struct Integ_var_t *v1, struct Integ_var_t *v2){
  if(v1 == NULL)return 1;
  if(v2 == NULL)return -1;
  if(v1->index > v2->index)return 1;
  if(v1->index != v2->index)return -1;
  if(v1->type > v2->type)return 1;
  return -1;
}
/*----------------------------
  Output handling to the GUI/interface.
*/

int integrator_set_reporter(IntegratorSystem *sys
	, IntegratorReporter *reporter
){
	asc_assert(sys!=NULL);
	sys->reporter = reporter;
	/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"INTEGRATOR REPORTER HOOKS HAVE BEEN SET\n"); */
	return 1;
}

int integrator_output_init(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	asc_assert(sys->reporter!=NULL);
	if(sys->reporter->init!=NULL){
		/* call the specified output function */
		return (*(sys->reporter->init))(sys);
	}
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter init method");
	return 1;
}

int integrator_output_write(IntegratorSystem *sys){
	static int reported_already=0;
	asc_assert(sys!=NULL);
	if(sys->reporter->write!=NULL){
		return (*(sys->reporter->write))(sys);
	}
	if(!reported_already){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter write method (this message only shown once)");
		reported_already=1;
	}
	return 1;
}

int integrator_output_write_obs(IntegratorSystem *sys){
	static int reported_already=0;
	asc_assert(sys!=NULL);
	if(sys->reporter->write_obs!=NULL){
		return (*(sys->reporter->write_obs))(sys);
	}
	if(!reported_already){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter write_obs method (this message only shown once)");
		reported_already=1;
	}
	return 1;
}

int integrator_output_close(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	if(sys->reporter->close!=NULL){
		return (*(sys->reporter->close))(sys);
	}
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter close method");
	return 1;
}

/**
	Decode status codes from the integrator, and output them via FPRINTF.

	@return 0 on status ok (converged), <0 on unrecognised state, >0 on recognised error state.
*/
int integrator_checkstatus(slv_status_t status) {
	if(status.converged){
		return 0;
	}

	if(status.diverged){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"The derivative system did not converge."
			" Integration will terminate."); return 1;
	}else if(status.inconsistent){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A numerically inconsistent state was discovered while"
			" calculating derivatives. Integration will terminate."); return 1;
	}else if(status.time_limit_exceeded){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"The time limit was exceeded while calculating"
			" derivatives. Integration will terminate."); return 1;
	}else if(status.iteration_limit_exceeded){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"The iteration limit was exceeded while calculating"
			" derivatives. Integration will terminate."); return 1;
	}else if(status.panic){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration cancelled by user."); return 1;
	}

	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unrecognised solver state (non converged)");
	return -1;
}
