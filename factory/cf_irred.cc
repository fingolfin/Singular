/* emacs edit mode for this file is -*- C++ -*- */
/* $Id: cf_irred.cc,v 1.2 1997-06-19 12:24:26 schmidt Exp $ */

#include <config.h>

#include "assert.h"

#include "cf_defs.h"
#include "canonicalform.h"
#include "cf_random.h"

static bool
is_irreducible ( const CanonicalForm & f )
{
    CFFList F = factorize( f );
    return F.length() == 1 && F.getFirst().exp() == 1;
}

CanonicalForm
find_irreducible ( int deg, CFRandom & gen, const Variable & x )
{
    CanonicalForm result;
    int i;
    do {
	result = power( x, deg );
	for ( i = deg-1; i >= 0; i-- )
	    result += gen.generate() * power( x, i );
    } while ( ! is_irreducible( result ) );
    return result;
}
