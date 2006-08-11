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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#include <errno.h>

#include "dr.h"
#include "tmy.h"

#include <utilities/config.h>
#include <general/ospath.h>
#include <utilities/ascMalloc.h>
#include <utilities/error.h>
#include <utilities/ascPanic.h>

/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

/*
	declare the possible formats that we will accept ('format' child in the
	DATA instance of the external relation
*/

#define FMTS(D,X) D(TMY2) X D(CSV) X D(TDV)

#define ENUM(F_) DATAREADER_FORMAT_##F_
#define COMMA ,
typedef enum{
	FMTS(ENUM,COMMA),
	DATAREADER_INVALID_FORMAT
} DataReaderFormat;
#undef ENUM
#undef COMMA


/*------------------------------------------------------------------------------
  API FUNCTIONS
*/

/**
	Create a data reader object, with the filename specified. The filename
	will be searched for in a specified path, eg ASCENDLIBRARY.
*/		
DataReader *datareader_new(const char *fn){
	DataReader *d;
	
	d = ASC_NEW(DataReader);
	d->fn = fn;
	d->fp = NULL;
	d->f = NULL;
	d->noutputs = 0;

	d->datafn=NULL;
	d->headerfn=NULL;
	d->eoffn=NULL;

	CONSOLE_DEBUG("Datareader created...");
	return d;
}

/**
	Set data file format
	@return 0 on success
*/
int datareader_set_format(DataReader *d, const char *format){

#define STR(N) #N
#define COMMA ,
	const char *fmts[]={ FMTS(STR,COMMA) };
#undef STR
#undef COMMA

	int i;

	CONSOLE_DEBUG("FORMAT '%s'",format);

	DataReaderFormat found = DATAREADER_INVALID_FORMAT;
	for(i=0; i < DATAREADER_INVALID_FORMAT; ++i){
		if(strcmp(format,fmts[i])==0){
			found = (DataReaderFormat)i;
			break;
		}
	}

	CONSOLE_DEBUG("FOUND DATA FORMAT %d",found);

	switch(found){
		case DATAREADER_FORMAT_TMY2:
			d->headerfn=&datareader_tmy2_header;
			d->datafn=&datareader_tmy2_data;
			d->eoffn=&datareader_tmy2_eof;
			d->indepfn=&datareader_tmy2_time;
			d->valfn=&datareader_tmy2_vals;
			break;
		case DATAREADER_FORMAT_TDV:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Tab delimited values (TDV) format not yet implemenented.");
			return 1;
		case DATAREADER_FORMAT_CSV:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Comma-separated values (TDV) format not yet implemenented.");
			return 1;
		default:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unknown file format '%s' specified",format);
			return 1;
	}

	return 0;
}

/**	
	Initialise the datareader: open the file, check the number of columns, etc.
	@return 0 on success

	@TODO search for the file in the ASCENDLIBRARY if not found immediately
*/
int datareader_init(DataReader *d){
	ospath_stat_t s;

	d->fp = ospath_new(d->fn);
	if(d->fp==NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid filepath");
		return 1;
	}

	if(ospath_stat(d->fp,&s)){
		if(errno==ENOENT){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"The file '%s' does not exist.",d->fn);
			return 1;
		}else{
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"The file '%s' can not be accessed.",d->fn);
			return 1;
		}
	}

	CONSOLE_DEBUG("About to open the data file");

	d->f = ospath_fopen(d->fp,"r");
	if(d->f == NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unable to open file '%s' for reading.",d->fn);
		return 1;
	}

	CONSOLE_DEBUG("Data file open ok");

	asc_assert(d->headerfn);
	asc_assert(d->eoffn);
	asc_assert(d->datafn);

	if((*d->headerfn)(d)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error processing file header in '%s'",d->fn);
		fclose(d->f);
		return 1;
	}
	
	while(! (*d->eoffn)(d)){
		if((*d->datafn)(d)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error reading file data in '%s'",d->fn);
			fclose(d->f);
			return 1;
		}
	}
	fclose(d->f);

	d->i = 0; /* set current position to zero */

	return 0;
}

/**
	Shut down the data reader and deallocate any memory owned by it, then
	free the memory at d.
*/
int datareader_delete(DataReader *d){
	if(d->fp){
		ospath_free(d->fp);
		d->fp = NULL;
	}
	if(d->f){
		fclose(d->f);
		d->f = NULL;
	}
	ASC_FREE(d);
	return 0;
}

/**
	Return the number of inputs (independent variables) supplied in the
	DataReader's current file. Can only be 1 at this stage.
*/
int datareader_num_inputs(const DataReader *d){
	return d->ninputs;
}

/**
	Return the number of outputs (dependent variables) found in the DataReader's
	current file. Should be one or more.
*/
int datareader_num_outputs(const DataReader *d){
	return d->noutputs;
}


int datareader_locate(DataReader *d, double t, double *t1, double *t2){
	(*d->indepfn)(d,t1);
	if(*t1 > t && d->i > 0){
		/* start of current interval is too late */
		do{
			/* CONSOLE_DEBUG("STEPPING BACK (d->i = %d currently)",d->i); */
			--(d->i);
			(*d->indepfn)(d,t1);
		}while(*t1 > t && d->i > 0);
	}
	/* now either d->i==0 or t1 < t*/
	/* CONSOLE_DEBUG("d->i==%d",d->i); */
	++d->i;
	(*d->indepfn)(d,t2);
	if(*t2 <= t){
		/* end of current interface is too early */
		do{
			/* CONSOLE_DEBUG("STEPPING FORWARD (d->i = %d currently)",d->i); */
			++(d->i);
			(*d->indepfn)(d,t2);
		}while(*t2 < t && d->i < d->ndata);
	}
	/* CONSOLE_DEBUG("d->i==%d",d->i); */

	if(d->i == d->ndata || d->i == 0){
		return 1;
	}

	/* CONSOLE_DEBUG("INTERVAL OK"); */
	return 0;
}

/**
	Return an interpolated set of output values for the given input values.
	This should be computed such that the output values are smooth in their 
	first derivatives.

	The required memory for the inputs and outputs must be allocated by the
	caller, and indicated by the pointers 'inputs' and 'outputs'.

	@see datareader_deriv
	@TODO implement this
*/
int datareader_func(DataReader *d, double *inputs, double *outputs){
	int i;
	double t1[1], t2[1];
	double v1[2], v2[2];

	double t;
	t = inputs[0];

	/* CONSOLE_DEBUG("EVALUATING"); */

	asc_assert(d->indepfn);

	if(datareader_locate(d,t,t1,t2)){
		CONSOLE_DEBUG("LOCATION ERROR");
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Time value t=%f is out of range",t);
		return 1;
	}

	/* CONSOLE_DEBUG("LOCATED OK, d->i = %d, t1 = %f, t2 = %f", d->i, *t1, *t2); */

	(*d->valfn)(d,v2);
	--d->i;
	(*d->valfn)(d,v1);
	
	for(i=0;i<d->noutputs;++i){
		outputs[i]=v1[i]+(t-*t1)/(*t2-*t1)*(v2[i]-v1[i]);
	}
	
	return 0;
}

/**
	Return an interpolated set of output derivatives for the given input
	values. These should be smooth.
	@see datareader_func
	@TODO implement this
*/
int datareader_deriv(DataReader *d, double *inputs, double *jacobian){
	int i;
	double t1[1], t2[1];
	double v1[2], v2[2];

	double t;
	t = inputs[0];

	/* CONSOLE_DEBUG("EVALUATING"); */

	asc_assert(d->indepfn);

	if(datareader_locate(d,t,t1,t2)){
		CONSOLE_DEBUG("LOCATION ERROR");
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Time value t=%f is out of range",t);
		return 1;
	}		

	(*d->valfn)(d,v2);
	--d->i;
	(*d->valfn)(d,v1);
	
	for(i=0;i<d->noutputs;++i){
		jacobian[i]=(v2[i]-v1[i])/(*t2-*t1);
	}
	
	return 0;
}


