/* Copyright 1996 Michael Messollen. All rights reserved. */
///////////////////////////////////////////////////////////////////////////////
// emacs edit mode for this file is -*- C++ -*-
// static char * rcsid = "$Id: MVMultiHensel.cc,v 1.2 1997-06-09 15:56:00 Singular Exp $";
///////////////////////////////////////////////////////////////////////////////
// FACTORY - Includes
#include <factory.h>
// Factor - Includes
#include "tmpl_inst.h"
#include "helpstuff.h"

#ifdef HENSELDEBUG
#  define DEBUGOUTPUT
#else
#  undef DEBUGOUTPUT
#endif

#include "debug.h"
#include "timing.h"

///////////////////////////////////////////////////////////////
// some class definition needed in MVMultiHensel
///////////////////////////////////////////////////////////////
typedef bool Boolean;

 class DiophantForm {
 public:
   CanonicalForm One;
   CanonicalForm Two;
   inline DiophantForm& operator=( const DiophantForm&  value ){
     if ( this != &value ){
       One = value.One;
       Two = value.Two;
     }
     return *this;
   }
 };

// We remember an already calculated value; simple class for RememberArray
class RememberForm {
public:
  inline RememberForm operator=( CanonicalForm & value ){
    this->calculated = true; 
    this->poly = value;
    return *this;
  }
  Boolean calculated;
  CanonicalForm poly;
};

// Array to remember already calculated values; used for the diophantine 
// equation s*f + t*g = x^i 
class RememberArray {
public:
// operations performed on arrays
  RememberArray( int sz ){
    size = sz;
    ia = new RememberForm[size];
//    assert( ia != 0 ); // test if we got the memory
    init( sz );
  }
  ~RememberArray(){ delete [] ia; }
  inline RememberForm& operator[]( int index ){
    return ia[index];
  }
protected:
  void init( int ){
    for ( int ix=0; ix < size; ++ix)
      ia[ix].calculated=false;
  }
// internal data representation
  int size;
  RememberForm *ia;
};


///////////////////////////////////////////////////////////////
// Solve the Diophantine equation: ( levelU == mainvar )     //
//            s*F1 + t*F2 = (mainvar)^i                      //
// Returns s and t.                                          //
///////////////////////////////////////////////////////////////
static DiophantForm 
diophant( int levelU , const CanonicalForm & F1 , const CanonicalForm & F2 , int i , RememberArray & A, RememberArray & B ) {
  DiophantForm Retvalue;
  CanonicalForm s,t,q,r;
  Variable x(levelU);

  DEBOUT(cout, "diophant: called with: ", F1);
  DEBOUT(cout, "  ", F2); DEBOUTLN(cout, "  ", i);

  // Did we solve the diophantine equation yet? 
  // If so, return the calculated values
  if ( A[i].calculated && B[i].calculated ){
    Retvalue.One=A[i].poly; 
    Retvalue.Two=B[i].poly;
    return Retvalue;
  }

  // Degrees ok? degree(F1,mainvar) + degree(F2,mainvar) <= i ?
  if ( (degree(F1,levelU) + degree(F2,levelU) ) <= i ) {
#ifdef HAVE_SINGULAR
    extern void WerrorS(char *);
    WerrorS("libfac: diophant ERROR: degree too large!  ");
#else
    cerr << "libfac: diophant ERROR: degree too large!  " 
	 << (degree(F1,levelU) + degree(F2,levelU) ) <<endl;
#endif
    Retvalue.One=F1;Retvalue.Two=F2;
    return Retvalue;
  }

  if ( i == 0 ) { // call the extended gcd
    r=extgcd(F1,F2,s,t);
    // check if gcd(F1,F2) <> 1 , i.e. F1 and F2 are not relatively prime 
    if ( ! r.isOne() ){ 
#ifdef HAVE_SINGULAR
      extern void WerrorS(char *);
      WerrorS("libfac: diophant ERROR: F1 and F2 are not relatively prime! ");
#else
      cerr << "libfac: diophant ERROR: " << F1 << "  and  " << F2 
	   << "  are not relatively prime!" << endl;
#endif
      Retvalue.One=F1;Retvalue.Two=F2;
      return Retvalue;
    }
    Retvalue.One = s; Retvalue.Two = t;
  }
  else { // recursively call diophant
    Retvalue=diophant(levelU,F1,F2,i-1,A,B); 
    Retvalue.One *= x; // createVar(levelU,1);
    Retvalue.Two *= x; // createVar(levelU,1);
    // Check degrees.

    if ( degree(Retvalue.One,levelU) > degree(F2,levelU) ){
      // Make degree(Retvalue.one,mainvar) < degree(F2,mainvar)
      divrem(Retvalue.One,F2,q,r);
      Retvalue.One = r; Retvalue.Two += F1*q;
    }
    else {
      if ( degree(Retvalue.Two,levelU) >= degree(F1,levelU) ){
	// Make degree(Retvalue.Two,mainvar) <= degree(F1,mainvar)
	divrem(Retvalue.Two,F1,q,r);
	Retvalue.One += F2*q; Retvalue.Two = r;
      }
    }

  }
  A[i].poly = Retvalue.One ; 
  B[i].poly = Retvalue.Two ;
  A[i].calculated = true ; B[i].calculated = true ;

  DEBOUT(cout, "diophant: Returnvalue is: ", Retvalue.One);
  DEBOUTLN(cout, "  ", Retvalue.Two);

  return  Retvalue;
}

///////////////////////////////////////////////////////////////
// A more efficient way to solve s*F1 + t*F2 = W             //
// as in Wang and Rothschild [Wang&Roth75].                  //
///////////////////////////////////////////////////////////////
static CanonicalForm 
make_delta( int levelU, const CanonicalForm & W, 
	    const CanonicalForm & F1, const CanonicalForm & F2, 
	    RememberArray & A, RememberArray & B){
  CanonicalForm Retvalue;
  DiophantForm intermediate;

  DEBOUT(cout, "make_delta: W= ", W);
  DEBOUTLN(cout, "  degree(W,levelU)= ", degree(W,levelU) );

  if ( levelU == level(W) ){ // same level, good
    for ( CFIterator i=W; i.hasTerms(); i++){
      intermediate=diophant(levelU,F1,F2,i.exp(),A,B);
      Retvalue += i.coeff() * intermediate.One;
    }
  }
  else{ // level(W) < levelU ; i.e. degree(w,levelU) == 0
    intermediate=diophant(levelU,F1,F2,0,A,B);
    Retvalue = W * intermediate.One;
  }
  DEBOUTLN(cout, "make_delta: Returnvalue= ", Retvalue);
  return Retvalue;
}

static CanonicalForm 
make_square( int levelU, const CanonicalForm & W, 
	     const CanonicalForm & F1, const CanonicalForm & F2, 
	     RememberArray & A, RememberArray & B){
  CanonicalForm Retvalue;
  DiophantForm intermediate;

  DEBOUT(cout, "make_square: W= ", W );
  DEBOUTLN(cout, "  degree(W,levelU)= ", degree(W,levelU));

  if ( levelU == level(W) ){ // same level, good
    for ( CFIterator i=W; i.hasTerms(); i++){
      intermediate=diophant(levelU,F1,F2,i.exp(),A,B);
      Retvalue += i.coeff() * intermediate.Two;
    }
  }
  else{ // level(W) < levelU ; i.e. degree(w,levelU) == 0
    intermediate=diophant(levelU,F1,F2,0,A,B);
    Retvalue = W * intermediate.Two;
  }
  DEBOUTLN(cout, "make_square: Returnvalue= ", Retvalue);

  return Retvalue;
}


///////////////////////////////////////////////////////////////
// Multivariat Hensel routine for two factors F and G .      //
// U is the monic univariat polynomial; we manage two arrays //
// to remember already calculated values for the diophantine //
// equation. This is suggested by Joel Moses [Moses71] .     //
// Return the fully lifted factors.                          //
///////////////////////////////////////////////////////////////
static DiophantForm 
mvhensel( const CanonicalForm & U , const CanonicalForm & F , 
	  const CanonicalForm & G , const SFormList & Substitutionlist){
  CanonicalForm V,Fk=F,Gk=G,Rk,W,D,S;
  int  levelU=level(U), degU=subvardegree(U,levelU); // degree(U,{x_1,..,x_(level(U)-1)})
  DiophantForm Retvalue;
  RememberArray A(degree(F)+degree(G)+1);
  RememberArray B(degree(F)+degree(G)+1);

  DEBOUTLN(cout, "mvhensel called with: U= ", U);
  DEBOUTLN(cout, "                      F= ", F);
  DEBOUTLN(cout, "                      G= ", G);
  DEBOUTLN(cout, "                   degU= ", degU);

  V=change_poly(U,Substitutionlist,0); // change x_i <- x_i + a_i for all i
  Rk = F*G-V;
#ifdef HENSELDEBUG2
  cout << "mvhensel: V = " << V << endl
       << "          Fk= " << F << endl
       << "          Gk= " << G << endl
       << "          Rk= " << Rk << endl;
#endif
  for ( int k=2; k<=degU+1; k++){//2; k++){//degU+1; k++){
    W = mod_power(Rk,k,levelU);
#ifdef HENSELDEBUG2
    cout << "mvhensel: Iteration: " << k << endl;
    cout << "mvhensel: W= " << W << endl;
#endif
    D = make_delta(levelU,W,F,G,A,B);
#ifdef HENSELDEBUG2
    cout << "mvhensel: D= " << D << endl;
#endif
    S = make_square(levelU,W,F,G,A,B);
#ifdef HENSELDEBUG2
    cout << "mvhensel: S= " << S << endl;
#endif
    Rk += S*D - D*Fk - S*Gk;
#ifdef HENSELDEBUG2
    cout << "mvhensel: Rk= " << Rk << endl;
#endif
    Fk -= S;
#ifdef HENSELDEBUG2
    cout << "mvhensel: Fk= " << Fk << endl;
#endif
    Gk -= D;
#ifdef HENSELDEBUG2
    cout << "mvhensel: Gk= " << Gk << endl;
#endif
    if ( Rk.isZero() ) break;
  }
  Retvalue.One = change_poly(Fk,Substitutionlist,1);
  Retvalue.Two = change_poly(Gk,Substitutionlist,1);

  DEBOUTLN(cout, "mvhensel: Retvalue: ", Retvalue.One);
  DEBOUTLN(cout, "                    ", Retvalue.Two);

  return Retvalue ;
}

///////////////////////////////////////////////////////////////
// Recursive Version of MultiHensel.                         //
///////////////////////////////////////////////////////////////
CFFList 
multihensel( const CanonicalForm & mF, const CFFList & Factorlist, 
	     const SFormList & Substitutionlist){
  CFFList Returnlist,factorlist=Factorlist;
  DiophantForm intermediat;
  CanonicalForm Pl,Pr;
  int n = factorlist.length();

  DEBOUT(cout, "multihensel: called with ", mF);
  DEBOUTLN(cout, "  ", factorlist);

  if ( n == 1 ) {
    Returnlist.append(CFFactor(mF,1));
  }
  else { 
    if ( n == 2 ){
      intermediat= mvhensel(mF, factorlist.getFirst().factor(), 
			    Factorlist.getLast().factor(), 
			    Substitutionlist);
      Returnlist.append(CFFactor(intermediat.One,1));
      Returnlist.append(CFFactor(intermediat.Two,1));
    }
    else { // more then two factors
#ifdef HENSELDEBUG2
      cout << "multihensel: more than two factors!" << endl;
#endif
      Pl=factorlist.getFirst().factor(); factorlist.removeFirst();
      Pr=Pl.genOne();
      for ( ListIterator<CFFactor> i=factorlist; i.hasItem(); i++ )
	Pr *=  i.getItem().factor() ; 
#ifdef HENSELDEBUG2
      cout << "multihensel: Pl,Pr, factorlist: " << Pl << "  " << Pr 
	   << "  " << factorlist << endl;
#endif
      intermediat= mvhensel(mF,Pl,Pr,Substitutionlist);
      Returnlist.append(CFFactor(intermediat.One,1));
      Returnlist=Union( multihensel(intermediat.Two,factorlist,Substitutionlist), Returnlist);
    }
  }

  return Returnlist;
}

///////////////////////////////////////////////////////////////
// Generalized Hensel routine.                               //
// mF is the monic multivariat polynomial, Factorlist is the //
// list of factors, Substitutionlist represents the ideal    //
// <x_1+a_1, .. , x_r+a_r>, where r=level(mF)-1 .            //
// Returns the list of fully lifted factors.                 //
///////////////////////////////////////////////////////////////
CFFList 
MultiHensel( const CanonicalForm & mF, const CFFList & Factorlist, 
	     const SFormList & Substitutionlist){
  CFFList Returnlist,Retlistinter,factorlist=Factorlist,Ll;
  CFFListIterator i;
  DiophantForm intermediat;
  CanonicalForm Pl,Pr;
  int n = factorlist.length(),h=n/2, k;

  DEBOUT(cout, "MultiHensel: called with ", mF);
  DEBOUTLN(cout, "  ", factorlist);
  DEBOUT(cout,"           : n,h = ", n);
  DEBOUTLN(cout,"  ", h);

  if ( n == 1 ) {
    Returnlist.append(CFFactor(mF,1));
  }
  else { 
    if ( n == 2 ){
      intermediat= mvhensel(mF, factorlist.getFirst().factor(), 
			    Factorlist.getLast().factor(), 
			    Substitutionlist);
      Returnlist.append(CFFactor(intermediat.One,1));
      Returnlist.append(CFFactor(intermediat.Two,1));
    }
    else { // more then two factors
      for ( k=1 ; k<=h; k++){
	Ll.append(factorlist.getFirst());
	factorlist.removeFirst();
      }

      DEBOUTLN(cout, "MultiHensel: Ll= ", Ll);
      DEBOUTLN(cout, "     factorlist= ", factorlist);

      Pl = 1; Pr = 1;
      for ( i = Ll; i.hasItem(); i++)
	Pl *= i.getItem().factor();
      DEBOUTLN(cout, "MultiHensel: Pl= ", Pl);
      for ( i = factorlist; i.hasItem(); i++)
	Pr *= i.getItem().factor();
      DEBOUTLN(cout, "MultiHensel: Pr= ", Pr);
      intermediat = mvhensel(mF,Pl,Pr,Substitutionlist);
      // divison test for intermediat.One and intermediat.Two ?
      CanonicalForm a,b;
      // we add a division test now for intermediat.One and intermediat.Two
      if ( mydivremt (mF,intermediat.One,a,b) && b == mF.genZero() )
	Retlistinter.append(CFFactor(intermediat.One,1) );
      if ( mydivremt (mF,intermediat.Two,a,b) && b == mF.genZero() )
	Retlistinter.append(CFFactor(intermediat.Two,1)  );

      Ll = MultiHensel(intermediat.One, Ll, Substitutionlist);
      Returnlist = MultiHensel(intermediat.Two, factorlist, Substitutionlist);
      Returnlist = Union(Returnlist,Ll);

      Returnlist = Union(Retlistinter,Returnlist);

    }
  }
  return Returnlist;
}

/*
$Log: not supported by cvs2svn $
Revision 1.4  1997/04/25 22:40:02  michael
changed cerr and cout messages for use with Singular
Version for libfac-0.2.1

*/
