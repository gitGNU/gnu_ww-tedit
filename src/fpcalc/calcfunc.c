#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "calcpriv.h"
#include "calc_tab.h"

/////////////////////////////////////////////////////////////////////////
//
// Implementations of constants (which are represented as pure functions)
// and some not-so-standard math functions
//
//

#define CONST_PI 3.14159265358979323846
#define CONST_E  2.71828182845904523536

static double pi ( void )
{
  return CONST_PI;
};

static double e ( void )
{
  return CONST_E;
};

// abs() for long values. There seems not to be a standard function
// for this.
//
static long iabs ( long x )
{
  return x < 0 ? -x : x;
};

// a wrapper for rand() that randomizes the seed the first time it
// is called.
//
static long my_rand ( void )
{
  static BOOLEAN srandCalled = FALSE;

  if (!srandCalled)
  {
    srand( (unsigned)time( NULL ) );
    srandCalled = TRUE;
  }

  return rand();
};

// A convinient function that returns a random integer number in
// the range [0 .. num-1]
//
static long my_random ( long num )
{
  return (my_rand() * num) / (RAND_MAX/*+1*/);
};

// Return the integer part of a floating point value.
//
static long my_trunc ( double x )
{
  double intPart;

  modf( x, &intPart );
  return (long)intPart;
};

// Round a FP value to the nearest integer.
//
static long my_round ( double x )
{
  return my_trunc( x < 0 ? x - 0.5 : x + 0.5 );
};

// Log() with specifiable base
//
static double my_log ( double base, double x )
{
  return log( x ) / log( base );
};

// Log with base 2
//
static double my_log2 ( double x )
{
  return my_log( 2.0, x );
};

// Convert from radians to degrees
//
static double deg ( double x )
{
  return x * (180.0 / CONST_PI);
};

// Convert from degrees to radians
//
static double rad ( double x )
{
  return x * (CONST_PI / 180.0);
};

///////////////////// End of user-defined functions /////////////////////

/*
  Function signatures
  ====================

  Each function exported to the user by the calculator is described by
  its signatures. The types in function signatures are abreviated with
  the following characters:
    D = double
    I = long
    U = unsigned long
    V = void

  In each function's signature the first type specified the return type
  (which can't be void for obvious reasons). Thus:
    D_D_I represents  (double (*)( double, long )).
  Each signature is identified by an enum value in FUNC_SIGNATURE.
*/
enum FUNC_SIGNATURE
{
  D_D_D,     /* double (*)(double, double) */
  D_D,       /* double (*)(double) */
  D_V,       /* double (*)(void) */
  I_I,       /* long   (*)(long) */
  I_V,       /* long   (*)(void) */
  I_D,       /* long   (*)(double) */
};

/*
  Arrays describing the signatures.
  =================================

  Each type in a signature is represented by a lexer token
  (DOUBLE,INT or UNSIGNED).
  The first type is the function's return type - it must always be
  present. 0 marks the end of the signature.
*/
static const int D_D_D_desc[] = { DOUBLE, DOUBLE, DOUBLE, 0 };
static const int I_I_desc[]   = { INT, INT, 0 };
static const int I_D_desc[]   = { INT, DOUBLE, 0 };

/*
  Each entry in Signatures[] coresponds to a FUNC_SIGNATURE value and
  points to a description in the arrays above. To save space signatures
  with common right parts have been merged (similar to string pooling)
*/
static const int * const Signatures[] =
{
  D_D_D_desc,           // D_D_D
  D_D_D_desc + 1,       // D_D
  D_D_D_desc + 2,       // D_V
  I_I_desc,             // I_I
  I_I_desc + 1,         // I_V
  I_D_desc,             // I_D
};

// Array describing the functions available to the calculator.
// Each entry specifies the name, the signature and a pointer to a
// function whose declaration *must* corespond exactly to the
// signature.
//
// Most of the functions are defined in the standard C library but
// some of them are implemented in this module.
//
static struct
{
  const char * szName;
  enum FUNC_SIGNATURE desc;
  void (*func)(void);
} funcs[] =
{
  { "e",     D_V,   (void (*)(void))e },
  { "pi",    D_V,   (void (*)(void))pi },

  { "sin",   D_D,   (void (*)(void))sin },
  { "cos",   D_D,   (void (*)(void))cos },
  { "tan",   D_D,   (void (*)(void))tan },
  { "asin",  D_D,   (void (*)(void))asin },
  { "acos",  D_D,   (void (*)(void))acos },
  { "atan",  D_D,   (void (*)(void))atan },
  { "sinh",  D_D,   (void (*)(void))sinh },
  { "cosh",  D_D,   (void (*)(void))cosh },
  { "tanh",  D_D,   (void (*)(void))tanh },

  { "sqrt",  D_D,   (void (*)(void))sqrt },
  { "fabs",  D_D,   (void (*)(void))fabs },
  { "ceil",  D_D,   (void (*)(void))ceil },
  { "floor", D_D,   (void (*)(void))floor },
  { "abs",   I_I,   (void (*)(void))iabs },
  { "fmod",  D_D_D, (void (*)(void))fmod },
  { "rand",  I_V,   (void (*)(void))my_rand },
  { "random",I_I,   (void (*)(void))my_random },
  { "trunc", I_D,   (void (*)(void))my_trunc },
  { "round", I_D,   (void (*)(void))my_round },
  { "exp",   D_D,   (void (*)(void))exp },
  { "pow",   D_D_D, (void (*)(void))pow },
  { "log",   D_D_D, (void (*)(void))my_log },
  { "ln",    D_D,   (void (*)(void))log },
  { "lg",    D_D,   (void (*)(void))log10 },
  { "log10", D_D,   (void (*)(void))log10 },
  { "log2",  D_D,   (void (*)(void))my_log2},

  { "deg",   D_D,   (void (*)(void))deg },
  { "rad",   D_D,   (void (*)(void))rad },
};

int fcalc_FindFunc ( const char * szName )
{
  int funcIndex;

  for ( funcIndex = 0; funcIndex < _countof( funcs ); ++funcIndex )
  {
    if (strcmp( funcs[funcIndex].szName, szName ) == 0)
      return funcIndex;
  }

  return -1;
};

void fcalc_CallFunc ( Val * res, int funcIndex, Val * args, int argCount )
{
  int i;
  const int * pSign;

  // Check number of arguments: first count the arguments in the signature
  //
  pSign = Signatures[funcs[funcIndex].desc];

  for ( i = 0; pSign[i+1] != 0; ++i ) {};

  if (i != argCount) // check number of passed arguments
  {
    fcalc_Error( "Function '%s' requires %d arguments",
                 funcs[funcIndex].szName, i );
  }

  // Convert each agrument to its required type
  //
  for ( i = 0; pSign[i+1] != 0; ++i )
    fcalc_Convert( &args[i], pSign[i+1] );

  // Init the result value with the proper type
  //
  if (pSign[0] == DOUBLE)
    res->isFloat = TRUE;
  else
  {
    res->isFloat = FALSE;
    res->value.i.isSigned = pSign[0] == INT;
  }

  switch (funcs[funcIndex].desc)
  {
    case D_V:
      res->value.f =
        (*(double(*)(void))funcs[funcIndex].func)();
      break;

    case D_D:
      res->value.f =
        (*(double(*)(double))funcs[funcIndex].func)( args[0].value.f );
      break;

    case D_D_D:
      res->value.f =
        (*(double(*)(double,double))funcs[funcIndex].func)(
                        args[0].value.f,
        							  args[1].value.f );
      break;

    case I_I:
      res->value.i.v =
        (*(long (*)(long))funcs[funcIndex].func)( args[0].value.i.v );
      break;

    case I_V:
      res->value.i.v =
        (*(long (*)(void))funcs[funcIndex].func)();
      break;

    case I_D:
      res->value.i.v =
        (*(long (*)(double))funcs[funcIndex].func)( args[0].value.f );
      break;

    default:
      ASSERT( 0 );
      fcalc_Error( "Internal error: invalid function table" );
  }
};
