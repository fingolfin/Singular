/* emacs edit mode for this file is -*- C++ -*- */
/* $Id: cf_reval.cc,v 1.2 1997-06-19 12:23:58 schmidt Exp $ */

#include <config.h>

#include "assert.h"

#include "cf_defs.h"
#include "cf_reval.h"


REvaluation::REvaluation( const REvaluation & e )
{
    if ( e.gen == 0 )
	gen = 0;
    else
	gen = e.gen->clone();
    values = e.values;
}

REvaluation::~REvaluation()
{
    if ( gen != 0 )
	delete gen;
}

REvaluation&
REvaluation::operator= ( const REvaluation & e )
{
    if ( this != &e ) {
	values = e.values;
	if ( e.gen == 0 )
	    gen = 0;
	else
	    gen = e.gen->clone();
    }
    return *this;
}

void
REvaluation::nextpoint()
{
    int n = values.max();
    for ( int i = values.min(); i <= n; i++ )
	values[i] = gen->generate();
}
