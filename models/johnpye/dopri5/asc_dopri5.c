/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
 
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	DOPRI5 Runge-Kutta integrator
 
	Based on the implementation of LSODE integrator, but adapted to 
	an explicit one-step method.
*//*
	by John Pye, May 2007.
*/

#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascSignal.h>
#include <utilities/error.h>
#include <general/ospath.h>
#include <integrator/integrator.h>
#include <system/slv_client.h>
#include <system/slv_stdcalls.h>
#include "dopri5.h"

#define INTEG_DOPRI5 5

IntegratorCreateFn integrator_dopri5_create;
IntegratorParamsDefaultFn integrator_dopri5_params_default;
IntegratorSolveFn integrator_dopri5_solve;
IntegratorFreeFn integrator_dopri5_free;
IntegratorWriteMatrixFn integrator_dopri5_write_matrix;

const IntegratorInternals integrator_dopri5_internals =
	{
		integrator_dopri5_create
		,integrator_dopri5_params_default
		,integrator_analyse_ode /* note, this routine is back in integrator.c */
		,integrator_dopri5_solve
		,NULL
		,NULL /* debugfn */
		,integrator_dopri5_free
		,INTEG_DOPRI5
		,"DOPRI5"
	};

extern ASC_EXPORT int dopri5_register(void)
{
	CONSOLE_DEBUG("Registering DOPRI5...");
	return integrator_register(&integrator_dopri5_internals);
}

enum dopri5_status{
	DOPRI5_SUCCESS=1
	,DOPRI5_INTERRUPT=2
	,DOPRI5_BADINPUT=-1
	,DOPRI5_ITERLIMIT=-2
	,DOPRI5_STEPSMALL=-3
	,DOPRI5_STIFF=-4
};

typedef struct IntegratorDopri5DataStruct
{
	long n_eqns;                     /**< dimension of state vector */
	int *input_indices;              /**< vector of state vars indexes */
	int *output_indices;             /**< vector of derivative var indexes */
	struct var_variable **y_vars;    /**< NULL-terminated list of states vars */
	struct var_variable **ydot_vars; /**< NULL-terminated list of derivative vars*/
	struct rel_relation **rlist;     /**< NULL-terminated list of relevant rels
								                                      to be differentiated */

	char stop;                             /* stop requested? */
	int partitioned;                       /* partioned func evals or not */

	clock_t lastwrite;                     /* time of last call to the reporter 'write' function */
}
IntegratorDopri5Data;

/*------------------------------------------------------------------------------
	CREATE/FREE
*/

void integrator_dopri5_create(struct IntegratorSystemStruct *blsys){
	IntegratorDopri5Data *d;
	d = ASC_NEW_CLEAR(IntegratorDopri5Data);
	d->n_eqns=0;
	d->input_indices=NULL;
	d->output_indices=NULL;
	d->y_vars=NULL;
	d->ydot_vars=NULL;
	d->rlist=NULL;
	blsys->enginedata=(void*)d;
	integrator_dopri5_params_default(blsys);
	CONSOLE_DEBUG("CREATED DOPRI5");
}

void integrator_dopri5_free(void *enginedata){
	IntegratorDopri5Data d;
	d = *((IntegratorDopri5Data *)enginedata);

	if(d.input_indices)ASC_FREE(d.input_indices);
	d.input_indices = NULL;

	if(d.output_indices)ASC_FREE(d.output_indices);
	d.output_indices = NULL;

	if(d.y_vars)ASC_FREE(d.y_vars);
	d.y_vars = NULL;

	if(d.ydot_vars)ASC_FREE(d.ydot_vars);
	d.ydot_vars = NULL;

	if(d.rlist)ASC_FREE(d.rlist);
	d.rlist =  NULL;

	d.n_eqns = 0L;
}

/*------------------------------------------------------------------------------
	PARAMETERS
*/

enum dopri5_parameters{
	DOPRI5_PARAM_RTOL
	,DOPRI5_PARAM_ATOL
	,DOPRI5_PARAM_TOLVECT
	,DOPRI5_PARAM_NSTIFF
		,DOPRI5_PARAMS_SIZE
#if 0
	// more parameters for adding later...
	SolTrait *solout, /* function providing the numerical solution during integration */
	int iout,        /* switch for calling solout */

	double uround,   /* rounding unit */
	double safe,     /* safety factor */
	double fac1,     /* parameters for step size selection */
	double fac2,
	double beta,     /* for stabilized step size control */
#endif
};


int integrator_dopri5_params_default(IntegratorSystem *blsys){

	asc_assert(blsys!=NULL);
	asc_assert(blsys->engine==INTEG_DOPRI5);
	slv_parameters_t *p;
	p = &(blsys->params);

	slv_destroy_parms(p);

	if(p->parms==NULL) {
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, DOPRI5_PARAMS_SIZE);
		if(p->parms==NULL)return -1;
		p->dynamic_parms = 1;
	} else {
		asc_assert(p->num_parms == DOPRI5_PARAMS_SIZE);
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;

	slv_param_bool(p,DOPRI5_PARAM_TOLVECT
				   ,(SlvParameterInitBool) {
					   {"tolvect"
						   ,"Use per-variable tolerances?",1
						   ,"If TRUE, values of 'ode_rtol' and 'ode_atol' are taken from your"
						   " model and used in the integration. If FALSE, scalar values are"
						   " used (see 'rtol' and 'atol') and shared by all variables. See"
						   " 'itoler' in DOPRI5 code."
					   }
					   , TRUE
				   }
				  );

	slv_param_real(p,DOPRI5_PARAM_ATOL
				   ,(SlvParameterInitReal) {
					   {"atol"
						   ,"Scalar absolute error tolerance",1
						   ,"Default value of the scalar absolute error tolerance (for cases"
						   " where not specified in oda_atol var property. See 'dopri5.h' for"
						   " details"
					   }
					   , 1e-7, 1e-15, 1e10
				   }
				  );

	slv_param_real(p,DOPRI5_PARAM_RTOL
				   ,(SlvParameterInitReal) {
					   {"rtol"
						   ,"Scalar relative error tolerance",1
						   ,"Default value of the scalar relative error tolerance (for cases"
						   " where not specified in oda_rtol var property. See 'dopri5.h' for"
						   " details"
					   }
					   , 1e-7, 1e-15, 1
				   }
				  );

	slv_param_int(p,DOPRI5_PARAM_NSTIFF
				  ,(SlvParameterInitInt) {
					  {"nstiff"
						  ,"Number of steps considered stiff", 1
						  ,"Test for stiffness is activated when the current step number is a"
						  " multiple of nstiff. A negative value means no test."
					  }
					  , 1000, -1, 1e6
				  }
				 );

/*
	slv_param_bool(p,DOPRI5_PARAMS_DENSEREPORTING
				   ,(SlvParameterInitBool) {
					   {"densereporting"
						   ,"Dense reporting?",1
						   ,"If TRUE, output will be made at every sub-timestep"
						   " during integration. If false, output is only made
						   " according to the samplelist."
					   }
					   , FALSE
				   }
				  );

	slv_param_char(p,DOPRI5_PARAM_METH
				   ,(SlvParameterInitChar) {
					   {"meth"
						   ,"Integration method",1
						   ,"AM=Adams-Moulton method (for non-stiff problems), BDF=Backwards"
						   " Difference Formular (for stiff problems). See 'Description and"
						   " Use of DOPRI5', section 3.1."
					   }
					   , "BDF"
				   }
				   , (char *[]) {"AM","BDF",NULL
				   }
				  );
*/

	asc_assert(p->num_parms == DOPRI5_PARAMS_SIZE);
	return 0;
}

/* solver_var children expected for state variables */
static symchar *g_symbols[2];
#define STATERTOL g_symbols[0]
#define STATEATOL g_symbols[1]

static void InitTolNames(void){
  STATERTOL = AddSymbol("ode_rtol");
  STATEATOL = AddSymbol("ode_atol");
}


/**
	Allocates, fills, and returns the atoler or rtoler data, which may be either
	a vector of values for each variable, or may be a single value to be
	shared by all.

	State variables missing child ode_rtol will be defaulted to RTOL

	@param is_r TRUE if we want RTOL, FALSE if we want ATOL.
*/
static double *dopri5_get_artol(IntegratorSystem *blsys, int is_r, int tolvect) {

  struct Instance *toli;
  double tolval, *tol;
  int i,len;
  symchar *tolname;

  len = blsys->n_y;

  if(!tolvect){

	// single tol for all vars
	tolval = SLV_PARAM_REAL(&(blsys->params),DOPRI5_PARAM_RTOL);
	CONSOLE_DEBUG("Using RTOL = %f for all vars", tolval);

	tol = ASC_NEW(double);
	if(tol == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory");
		return tol;
	}

	*tol = tolval;
	return tol;

  }else{
    tol = ASC_NEW_ARRAY(double, blsys->n_y+1);
	if (tol == NULL) {
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory");
	  return tol;
	}

	tolval = SLV_PARAM_REAL(&(blsys->params),DOPRI5_PARAM_RTOL);

    InitTolNames(); // from where?
	if(is_r)tolname = STATERTOL;
	else tolname = STATEATOL;

    for(i=0; i<len; i++){
      toli = ChildByChar(var_instance(blsys->y[i]),tolname);
      if(toli == NULL || !AtomAssigned(toli)){
        tol[i] = tolval;
        ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Assuming value %3g"
        	"for '%s' child undefined for state variable %ld."
        	,tol[i], SCP(tolname), blsys->y_id[i]
        );
      }else{
        tol[i] = RealAtomValue(toli);
      }
    }
  }
  tol[len] = SLV_PARAM_REAL(&(blsys->params),DOPRI5_PARAM_RTOL);
  return tol;
}

/*------------------------------------------------------------------------------
  STATS
*/

/* 
	Several functions provide access to different values :
 
	xRead   x value for which the solution has been computed (x=xend after
		successful return).
 
	hRead   Predicted step size of the last accepted step (useful for a
		subsequent call to dopri5).
 
	nstepRead   Number of used steps.
	naccptRead  Number of accepted steps.
	nrejctRead  Number of rejected steps.
	nfcnRead    Number of function calls.
*/
typedef struct IntegratorDopri5StatsStruct{
	long nfcn;
	long nstep;
	long naccpt;
	long nrejct;
	double h;
	double x;
} IntegratorDopri5Stats;

/*------------------------------------------------------------------------------
  FUNCTION EVALUATION
*/

static FcnEqDiff integrator_dopri5_fex;

/**
	Evaluation function.
	@param n_eq number of equations, number of y and number of ydot.
	@param t indep var value
	@param y input vector of variable values
	@param ydot return vector of calculated derivatives
	@param user_data point to whatever we want, in this case the IntegratorSystem	
*/
static void integrator_dopri5_fex(
		unsigned n_eq, double t, double *y, double *ydot
		, void *user_data
){
	slv_status_t status;
	IntegratorSystem *blsys = (IntegratorSystem *)user_data;

	/*  slv_parameters_t parameters; pity lsode doesn't allow error returns */
	/* int i; */
	unsigned long res;

	CONSOLE_DEBUG("Calling for a function evaluation");

	/* pass the time and the unknowns back to the System */
	integrator_set_t(blsys, t);
	integrator_set_y(blsys, y);

    slv_resolve(blsys->system);

	if((res = slv_solve(blsys->system))){
		CONSOLE_DEBUG("solver returns error %ld",res);
	}

	slv_get_status(blsys->system, &status);
	if(slv_check_bounds(blsys->system,0,-1,"")){
		// TODO relay that system has gone out of bounds
	}

	/* pass the solver status to the integrator */
	res = integrator_checkstatus(status);

	integrator_output_write(blsys);

  	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve for derivatives (%d)",res);
		// TODO raise SIGINT here
	}else{
		/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"lsodedata->status = %d",lsodedata->status); */
	}

	integrator_get_ydot(blsys, ydot);
	// DONE, OK
}

/*------------------------------------------------------------------------------
  REPORTING
*/

static SolTrait integrator_dopri5_reporter;

static void integrator_dopri5_reporter(
		long nr, double xold, double x, double* y
		, unsigned n, int* irtrn, void *user_data
){
	IntegratorSystem *blsys = (IntegratorSystem *)user_data;
	integrator_output_write(blsys);
}

/*------------------------------------------------------------------------------
  SOLVE
*/

#define DOPRI5_FREE CONSOLE_DEBUG("FREE DOPRI5")

int integrator_dopri5_solve(IntegratorSystem *blsys
							, unsigned long start_index, unsigned long finish_index
){
	IntegratorDopri5Data *d;
	slv_status_t status;

	double x;
	double xend,xprev;
	unsigned long nsamples, neq;
	long nobs;
	//int  itol, itask, mf, lrw, liw;
	unsigned long index;
	//int istate, iopt;
	//double * rwork;
	//int * iwork;
	double *y, *atoler, *rtoler, *obs;
	int my_neq;
	enum dopri5_status res;

	double hmax, h;
	long nmax;
	long nstiff;

#if 0
	const char *method; /* Table 3.1 in D&Uo... */
	int miter; /* Table 3.2 in D&Uo... */
	int maxord; /* page 92 in D&Uo... */
#endif

	d = (IntegratorDopri5Data *)(blsys->enginedata);

	/* the numer of equations must be equal to blsys->n_y, the number of states */
	d->n_eqns = blsys->n_y;
	assert(d->n_eqns>0);

	d->input_indices = ASC_NEW_ARRAY_CLEAR(int, d->n_eqns);
	d->output_indices = ASC_NEW_ARRAY_CLEAR(int, d->n_eqns);

	d->y_vars = ASC_NEW_ARRAY(struct var_variable *,d->n_eqns+1);
	d->ydot_vars = ASC_NEW_ARRAY(struct var_variable *, d->n_eqns+1);

	/* set up the NLA solver here */

	/* 
		DOPRI5 should be OK to deal with any linsol/linsolqr-based solver.
		But for the moment we restrict to just QRSlv :-(
	*/
	if(strcmp(slv_solver_name(slv_get_selected_solver(blsys->system)),"QRSlv") != 0) {
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"QRSlv must be selected before integration.");
		return 1;
	}

	CONSOLE_DEBUG("Solver selected is '%s'",slv_solver_name(slv_get_selected_solver(blsys->system)));

	slv_get_status(blsys->system, &status);

	if(status.struct_singular){
		ERROR_REPORTER_HERE(ASC_USER_WARNING	
			,"The system (according to QRSlv) is structurally singular."
			" The ODE system may also be singular, but not necessarily."
		);
		/*    d->status = lsode_nok;
		return 2;*/
	}

	/* here we assume integrators.c is in charge of dynamic loading */

	/* set up parameteers for sending to DOPRI5 */

	nsamples = integrator_getnsamples(blsys);
	if (nsamples <2) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration will not be performed. The system has no end sample time defined.");
		//d->status = dopri5_nok;
		return 3;
	}
	neq = blsys->n_y;
	nobs = blsys->n_obs;

#if 0
	// TODO implement these:
	unsigned nrdens, /* number of components for which dense outpout is required */
	unsigned* icont, /* indexes of components for which dense output is required, >= nrdens */
	unsigned licont  /* declared length of icon */
#endif

	int iout = 1; /* SLV_PARAM_BOOL(&(blsys->params),DOPRI5_PARAM_DENSEREPORTING) */

	int tolvect = SLV_PARAM_BOOL(&(blsys->params),DOPRI5_PARAM_TOLVECT);

	/* samplelist_debug(blsys->samples); */

	x = integrator_getsample(blsys, 0);

	y = integrator_get_y(blsys, NULL);
	
	rtoler = dopri5_get_artol(blsys,1,tolvect);
	atoler = dopri5_get_artol(blsys,0,tolvect);

	obs = integrator_get_observations(blsys, NULL);

	// TODO check memory allocations

	h = integrator_get_stepzero(blsys);
	hmax = integrator_get_maxstep(blsys);
	/* rwork[6] = integrator_get_minstep(blsys); */ /* ignored */
	nmax = integrator_get_maxsubsteps(blsys);

	nstiff = SLV_PARAM_INT(&(blsys->params),DOPRI5_PARAM_NSTIFF);

	if(x > integrator_getsample(blsys, 2)) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid initialisation time: exceeds second timestep value");
		return 5;
	}

	/* put the values from derivative system into the record */
	integrator_setsample(blsys, start_index, x);

	integrator_output_init(blsys);

	/* -- store the initial values of all the stuff */
	integrator_output_write(blsys);
	integrator_output_write_obs(blsys);

	my_neq = (int)neq;

	blsys->currentstep = 0;
	for(index = start_index; index < finish_index; index++, 	blsys->currentstep++) {
		xend = integrator_getsample(blsys, index+1);
		xprev = x;
		asc_assert(xend > xprev);
		/* CONSOLE_DEBUG("DOPRI5 call #%lu: x = [%f,%f]", index,xprev,xend); */

# ifdef ASC_SIGNAL_TRAPS

		Asc_SignalHandlerPushDefault(SIGFPE);
		Asc_SignalHandlerPushDefault(SIGINT);

		if(SETJMP(g_fpe_env)==0) {
# endif /* ASC_SIGNAL_TRAPS */

			/* CONSOLE_DEBUG("Calling DOPRI5 with end-time = %f",xend); */

			d->lastwrite = clock();

#if 0
extern int dopri5(
        unsigned n,      /* dimension of the system <= UINT_MAX-1*/
        FcnEqDiff *fcn,   /* function computing the value of f(x,y) */
        double x,        /* initial x-value */
        double* y,       /* initial values for y */
        double xend,     /* final x-value (xend-x may be positive or negative) */

        double* rtoler,  /* relative error tolerance */
        double* atoler,  /* absolute error tolerance */
        int itoler,      /* switch for rtoler and atoler */
        SolTrait *solout, /* function providing the numerical solution during integration */
        int iout,        /* switch for calling solout */

        FILE* fileout,   /* messages stream */
        double uround,   /* rounding unit */
        double safe,     /* safety factor */
        double fac1,     /* parameters for step size selection */
        double fac2,

        double beta,     /* for stabilized step size control */
        double hmax,     /* maximal step size */
        double h,        /* initial step size */
        long nmax,       /* maximal number of allowed steps */
        int meth,        /* switch for the choice of the coefficients */

        long nstiff,     /* test for stiffness */
        unsigned nrdens, /* number of components for which dense outpout is required */
        unsigned* icont, /* indexes of components for which dense output is required, >= nrdens */
        unsigned licont  /* declared length of icon */
    );
#endif
		    res = dopri5 (my_neq, &integrator_dopri5_fex, x, y, xend
				, rtoler, atoler, tolvect, integrator_dopri5_reporter, iout
				, stdout, 0.0, 0.0, 0.0, 0.0
				, 0.0 /* beta */, hmax, h, nmax, 0
				, nstiff, 0, NULL, 0
				, (void *)blsys
			);

# ifdef ASC_SIGNAL_TRAPS
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration terminated due to float error in DOPRI5 call.");
			//dopri5_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
			DOPRI5_FREE;
			return 6;
		}
		Asc_SignalHandlerPopDefault(SIGFPE);
		Asc_SignalHandlerPopDefault(SIGINT);

# endif

	switch(res){
		case DOPRI5_SUCCESS:
			break;
		case DOPRI5_INTERRUPT:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"DOPRI5 interrupted by user");
			break;
		case DOPRI5_BADINPUT:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad input to DOPRI5");
			break;
		case DOPRI5_ITERLIMIT:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Iteration limit exceeded in DOPRI5");
			break;
		case DOPRI5_STEPSMALL:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Step size became too small in DOPRI5");
			break;
		case DOPRI5_STIFF:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Problem appears stiff in DOPRI5");
			break;
	}

	if(res<0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Furthest point reached was t = %g.\n",x);
		DOPRI5_FREE;
		return 7;
	}

	integrator_setsample(blsys, index+1, x);
	/* record when dopri5 actually came back */
	integrator_set_t(blsys, x);
	integrator_set_y(blsys, y);
	/* put x,y in d in case dopri5 got x,y by interpolation, as it does  */

	}

	integrator_output_close(blsys);

	CONSOLE_DEBUG("--- DOPRI5 done ---");
	return 0; /* success */
}
