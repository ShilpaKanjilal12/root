// @(#)root/pyroot:$Name:  $:$Id: Converters.cxx,v 1.16 2005/09/09 05:19:10 brun Exp $
// Author: Wim Lavrijsen, Jan 2005

// Bindings
#include "PyROOT.h"
#include "Converters.h"
#include "ObjectProxy.h"
#include "PyBufferFactory.h"
#include "Utility.h"
#include "RootWrapper.h"

// ROOT
#include "TClass.h"
#include "TClassEdit.h"

// CINT
#include "Api.h"

// Standard
#include <string.h>
#include <utility>


//- data ______________________________________________________________________
PyROOT::ConvFactories_t PyROOT::gConvFactories;

//- base converter implementation ---------------------------------------------
PyObject* PyROOT::TConverter::FromMemory( void* )
{
   PyErr_SetString( PyExc_TypeError, "unknown type can not be converted from memory" );
   return 0;
}

//_____________________________________________________________________________
Bool_t PyROOT::TConverter::ToMemory( PyObject*, void* )
{
   PyErr_SetString( PyExc_TypeError, "unknown type can not be converted to memory" );
   return kFALSE;
}


//- helper macro's ------------------------------------------------------------
#define PYROOT_IMPLEMENT_BASIC_CONVERTER( name, type, stype, F1, F2 )         \
PyObject* PyROOT::T##name##Converter::FromMemory( void* address )             \
{                                                                             \
   return F1( (stype)*((type*)address) );                                     \
}                                                                             \
                                                                              \
Bool_t PyROOT::T##name##Converter::ToMemory( PyObject* value, void* address ) \
{                                                                             \
   type s = (type)F2( value );                                                \
   if ( PyErr_Occurred() )                                                    \
      return kFALSE;                                                          \
   *((type*)address) = (type)s;                                               \
   return kTRUE;                                                              \
}

#define PYROOT_IMPLEMENT_BASIC_REF_CONVERTER( name )                          \
PyObject* PyROOT::T##name##Converter::FromMemory( void* )                     \
{                                                                             \
   return 0;                                                                  \
}                                                                             \
                                                                              \
Bool_t PyROOT::T##name##Converter::ToMemory( PyObject*, void* )               \
{                                                                             \
   return kFALSE;                                                             \
}


//_____________________________________________________________________________
#define PYROOT_IMPLEMENT_BASIC_CHAR_CONVERTER( name, type, low, high )        \
Bool_t PyROOT::T##name##Converter::SetArg( PyObject* pyobject, G__CallFunc* func )\
{                                                                             \
   if ( PyString_Check( pyobject ) ) {                                        \
      if ( PyString_GET_SIZE( pyobject ) == 1 )                               \
         func->SetArg( (Long_t)PyString_AS_STRING( pyobject )[0] );           \
      else                                                                    \
         PyErr_Format( PyExc_TypeError,                                       \
            #type" expected, got string of size %d", PyString_GET_SIZE( pyobject ) );\
   } else {                                                                  \
      Long_t l = PyLong_AsLong( pyobject );                                  \
      if ( PyErr_Occurred() )                                                \
         return kFALSE;                                                      \
      if ( ! ( low <= l && l <= high ) ) {                                   \
         PyErr_SetString( PyExc_ValueError, "integer to character: value out of range" );\
         return kFALSE;                                                      \
      }                                                                      \
      func->SetArg( l );                                                     \
   }                                                                         \
   return kTRUE;                                                             \
}                                                                            \
                                                                             \
PyObject* PyROOT::T##name##Converter::FromMemory( void* address )            \
{                                                                            \
   type buf[2]; buf[1] = (type)'\0';                                         \
   buf[0] = *((type*)address);                                               \
   return PyString_FromString( (char*)buf );                                 \
}                                                                            \
                                                                             \
Bool_t PyROOT::T##name##Converter::ToMemory( PyObject* value, void* address )\
{                                                                            \
   const char* buf = PyString_AsString( value );                             \
   if ( PyErr_Occurred() )                                                   \
      return kFALSE;                                                         \
                                                                             \
   int len = strlen( buf );                                                  \
   if ( len != 1 ) {                                                         \
      PyErr_Format( PyExc_TypeError, #type" expected, got string of size %d", len );\
      return kFALSE;                                                         \
   }                                                                         \
                                                                             \
   *((type*)address) = (type)buf[0];                                         \
   return kTRUE;                                                             \
}


//- converters for built-ins --------------------------------------------------
Bool_t PyROOT::TLongConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   func->SetArg( PyLong_AsLong( pyobject ) );
   if ( PyErr_Occurred() )
      return kFALSE;
   return kTRUE;
}

PYROOT_IMPLEMENT_BASIC_CONVERTER( Long, Long_t, Long_t, PyLong_FromLong, PyLong_AsLong )

//____________________________________________________________________________
Bool_t PyROOT::TLongRefConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   if ( ! PyInt_CheckExact( pyobject ) )
      return kFALSE;

   func->SetArgRef( ((PyIntObject*)pyobject)->ob_ival );
   return kTRUE;
}

PYROOT_IMPLEMENT_BASIC_REF_CONVERTER( LongRef )

//____________________________________________________________________________
Bool_t PyROOT::TBoolConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   Long_t l = PyLong_AsLong( pyobject );
   if ( PyErr_Occurred() )
      return kFALSE;

   if ( ! ( l == 0 || l == 1 ) ) {
      PyErr_SetString( PyExc_TypeError, "boolean value should be bool, or integer 1 or 0" );
      return kFALSE;
   }

   func->SetArg( l );
   return kTRUE;
}

PYROOT_IMPLEMENT_BASIC_CONVERTER( Bool, Bool_t, Long_t, PyInt_FromLong, PyInt_AsLong )

//____________________________________________________________________________
PYROOT_IMPLEMENT_BASIC_CHAR_CONVERTER( Char,  Char_t, -128, 127 )
PYROOT_IMPLEMENT_BASIC_CHAR_CONVERTER( UChar, UChar_t,   0, 255 )

//____________________________________________________________________________
PYROOT_IMPLEMENT_BASIC_CONVERTER( Short,  Short_t,  Long_t, PyInt_FromLong,  PyInt_AsLong )
PYROOT_IMPLEMENT_BASIC_CONVERTER( UShort, UShort_t, Long_t, PyInt_FromLong,  PyInt_AsLong )
PYROOT_IMPLEMENT_BASIC_CONVERTER( Int,    Int_t,    Long_t, PyInt_FromLong,  PyInt_AsLong )
PYROOT_IMPLEMENT_BASIC_CONVERTER( UInt,   UInt_t,   Long_t, PyInt_FromLong,  PyInt_AsLong )

PyObject* PyROOT::TULongConverter::FromMemory( void* address )
{
   return PyLong_FromUnsignedLong( *((ULong_t*)address) );
}

Bool_t PyROOT::TULongConverter::ToMemory( PyObject* value, void* address )
{
   ULong_t s = (ULong_t)PyLong_AsUnsignedLong( value );
   if ( PyErr_Occurred() ) {
      if ( PyInt_Check( value ) ) {    // shouldn't be ... bug in python?
         PyErr_Clear();
         s = (ULong_t)PyInt_AS_LONG( value ); 
      } else
         return kFALSE;
   }

   *((ULong_t*)address) = s;
   return kTRUE;
}

//____________________________________________________________________________
Bool_t PyROOT::TDoubleConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   func->SetArg( PyFloat_AsDouble( pyobject ) );
   if ( PyErr_Occurred() )
      return kFALSE;
   return kTRUE;
}

PYROOT_IMPLEMENT_BASIC_CONVERTER( Double, Double_t, Double_t, PyFloat_FromDouble, PyFloat_AsDouble )
PYROOT_IMPLEMENT_BASIC_CONVERTER( Float,  Float_t,  Double_t, PyFloat_FromDouble, PyFloat_AsDouble )

//____________________________________________________________________________
Bool_t PyROOT::TDoubleRefConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   if ( ! PyFloat_CheckExact( pyobject ) )
      return kFALSE;

   func->SetArgRef( ((PyFloatObject*)pyobject)->ob_fval );
   return kTRUE;
}

PYROOT_IMPLEMENT_BASIC_REF_CONVERTER( DoubleRef )

//____________________________________________________________________________
Bool_t PyROOT::TVoidConverter::SetArg( PyObject*, G__CallFunc* )
{
   PyErr_SetString( PyExc_SystemError, "void/unknown arguments can\'t be set" );
   return kFALSE;
}

//____________________________________________________________________________
Bool_t PyROOT::TLongLongConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   func->SetArg( PyLong_AsLongLong( pyobject ) );
   if ( PyErr_Occurred() )
      return kFALSE;
   return kTRUE;
}

PyObject* PyROOT::TLongLongConverter::FromMemory( void* address )
{
   return PyLong_FromLongLong( *(Long64_t*)address );
}

Bool_t PyROOT::TLongLongConverter::ToMemory( PyObject* value, void* address )
{
   Long64_t ll = PyLong_AsLongLong( value );
   if ( PyErr_Occurred() )
      return kFALSE;
   *((Long64_t*)address) = ll;
   return kTRUE;
}

//____________________________________________________________________________
Bool_t PyROOT::TCStringConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
// construct a new string and copy it in new memory
   const char* s = PyString_AsString( pyobject );
   if ( PyErr_Occurred() )
      return kFALSE;
   fBuffer = s;

// set the value and declare success
   func->SetArg( reinterpret_cast< Long_t >( fBuffer.c_str() ) );
   return kTRUE;
}

PyObject* PyROOT::TCStringConverter::FromMemory( void* address ) {
   if ( address )
      return PyString_FromString( *(char**)address );
   return PyString_FromString( const_cast< char* >( "" ) );
}

Bool_t PyROOT::TCStringConverter::ToMemory( PyObject* value, void* address ) {
   const char* s = PyString_AsString( value );
   if ( PyErr_Occurred() )
      return kFALSE;

   strcpy( *(char**)address, s );
   return kTRUE;
}


//- pointer/array conversions -------------------------------------------------
namespace {

   inline Bool_t CArraySetArg( PyObject* pyobject, G__CallFunc* func, char tc, int size )
   {
      void* buf = 0;
      int buflen = PyROOT::Utility::GetBuffer( pyobject, tc, size, buf );
      if ( ! buf || buflen == 0 )
         return kFALSE;

      func->SetArg( (Long_t)buf );
      return kTRUE;
   }

} // unnamed namespace

Bool_t PyROOT::TVoidArrayConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
// just convert pointer if it is a ROOT object
   if ( ObjectProxy_Check( pyobject ) ) {
   // depending on memory policy, some objects are no longer owned when passed to C++
      if ( ! fKeepControl )
         ((ObjectProxy*)pyobject)->Release();

   // set pointer (may be null) and declare success
      func->SetArg( reinterpret_cast< Long_t >( ((ObjectProxy*)pyobject)->GetObject() ) );
      return kTRUE;
   }

// special case: NULL pointer
   if ( pyobject == gNullObject ) {
      func->SetArg( 0l );
      return kTRUE;
   }

// special case: opaque CObject from somewhere
   if ( PyCObject_Check( pyobject ) ) {
      func->SetArg( (Long_t)PyCObject_AsVoidPtr( pyobject ) );
      return kTRUE;
   }

// final try: attempt to get buffer
   void* buf = 0;
   int buflen = Utility::GetBuffer( pyobject, '*', 1, buf, kFALSE );

// ok if buffer exists (can't perform any useful size checks)
   if ( buf && buflen != 0 ) {
      func->SetArg( (Long_t)buf );
      return kTRUE;
   }

// give up
   return kFALSE;
}

//____________________________________________________________________________
PyObject* PyROOT::TVoidArrayConverter::FromMemory( void* address )
{
   return PyLong_FromLong( (Long_t)address );
}

//____________________________________________________________________________
Bool_t PyROOT::TVoidArrayConverter::ToMemory( PyObject* value, void* address )
{
// just convert pointer if it is a ROOT object
   if ( ObjectProxy_Check( value ) ) {
   // depending on memory policy, some objects are no longer owned when passed to C++
      if ( ! fKeepControl )
         ((ObjectProxy*)value)->Release();

   // set pointer (may be null) and declare success
      *(void**)address = ((ObjectProxy*)value)->GetObject();
      return kTRUE;
   }

// special case: NULL pointer
   if ( value == gNullObject ) {
      *(void**)address = 0;
      return kTRUE;
   }

   void* buf = 0;
   int buflen = Utility::GetBuffer( value, '*', 1, buf, kFALSE );
   if ( ! buf || buflen == 0 )
      return kFALSE;

   *(void**)address = buf;
   return kTRUE;
}

//____________________________________________________________________________
#define PYROOT_IMPLEMENT_ARRAY_CONVERTER( name, type, code )                 \
Bool_t PyROOT::T##name##ArrayConverter::SetArg( PyObject* pyobject, G__CallFunc* func )\
{                                                                            \
   return CArraySetArg( pyobject, func, code, sizeof(type) );                \
}                                                                            \
                                                                             \
PyObject* PyROOT::T##name##ArrayConverter::FromMemory( void* address )       \
{                                                                            \
   return BufFac_t::Instance()->PyBuffer_FromMemory( *(type**)address, fSize );\
}                                                                            \
                                                                             \
Bool_t PyROOT::T##name##ArrayConverter::ToMemory( PyObject* value, void* address )\
{                                                                            \
   void* buf = 0;                                                            \
   int buflen = Utility::GetBuffer( value, code, sizeof(type), buf );        \
   if ( ! buf || buflen == 0 )                                               \
      return kFALSE;                                                         \
   if ( 0 <= fSize ) {                                                       \
      if ( fSize < buflen/(int)sizeof(type) ) {                              \
         PyErr_SetString( PyExc_ValueError, "buffer too large for value" );  \
         return kFALSE;                                                      \
      }                                                                      \
      memcpy( *(type**)address, buf, 0 < buflen ? buflen : sizeof(type) );   \
   } else                                                                    \
      *(type**)address = (type*)buf;                                         \
   return kTRUE;                                                             \
}

//____________________________________________________________________________
PYROOT_IMPLEMENT_ARRAY_CONVERTER( Short,  Short_t,  'h' )
PYROOT_IMPLEMENT_ARRAY_CONVERTER( UShort, UShort_t, 'H' )
PYROOT_IMPLEMENT_ARRAY_CONVERTER( Int,    Int_t,    'i' )
PYROOT_IMPLEMENT_ARRAY_CONVERTER( UInt,   UInt_t,   'I' )
PYROOT_IMPLEMENT_ARRAY_CONVERTER( Long,   Long_t,   'l' )
PYROOT_IMPLEMENT_ARRAY_CONVERTER( ULong,  ULong_t,  'L' )
PYROOT_IMPLEMENT_ARRAY_CONVERTER( Float,  Float_t,  'f' )
PYROOT_IMPLEMENT_ARRAY_CONVERTER( Double, Double_t, 'd' )

//____________________________________________________________________________
Bool_t PyROOT::TLongLongArrayConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   PyObject* pytc = PyObject_GetAttrString( pyobject, const_cast< char* >( "typecode" ) );
   if ( pytc != 0 ) {              // iow, this array has a known type, but there's no
      Py_DECREF( pytc );           // such thing for long long in module array
      return kFALSE;
   }
   
   return TVoidArrayConverter::SetArg( pyobject, func );
}


//- converters for special cases ----------------------------------------------
#define PYROOT_IMPLEMENT_STRING_AS_PRIMITIVE_CONVERTER( name, strtype, DF1 )  \
Bool_t PyROOT::T##name##Converter::SetArg( PyObject* pyobject, G__CallFunc* func )\
{                                                                             \
   const char* s = PyString_AsString( pyobject );                             \
   if ( PyErr_Occurred() )                                                    \
      return kFALSE;                                                          \
   fBuffer = s;                                                               \
   func->SetArg( reinterpret_cast< Long_t >( &fBuffer ) );                    \
   return kTRUE;                                                              \
}                                                                             \
                                                                              \
PyObject* PyROOT::T##name##Converter::FromMemory( void* address ) {           \
   if ( address )                                                             \
      return PyString_FromString( ((strtype*)address)->DF1() );               \
   return PyString_FromString( const_cast< char* >( "" ) );                   \
}                                                                             \
                                                                              \
Bool_t PyROOT::T##name##Converter::ToMemory( PyObject* value, void* address ) \
{                                                                             \
   const char* buf = PyString_AsString( value );                              \
   if ( PyErr_Occurred() )                                                    \
      return kFALSE;                                                          \
                                                                              \
   *((strtype*)address) = buf;                                                \
   return kTRUE;                                                              \
}


PYROOT_IMPLEMENT_STRING_AS_PRIMITIVE_CONVERTER( TString,   TString,     Data )
PYROOT_IMPLEMENT_STRING_AS_PRIMITIVE_CONVERTER( STLString, std::string, c_str )

//____________________________________________________________________________
Bool_t PyROOT::TRootObjectConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   if ( ! ObjectProxy_Check( pyobject ) ) {
      if ( pyobject == gNullObject ) {   // allow NULL pointer as a special case
          func->SetArg( 0l );
          return kTRUE;
      }

   // not a PyROOT object (TODO: handle SWIG etc.)
      return kFALSE;
   }

   ObjectProxy* pyobj = (ObjectProxy*)pyobject;
   if ( pyobj->ObjectIsA()->GetBaseClass( fClass.GetClass() ) ) {
   // depending on memory policy, some objects need releasing when passed into functions
      if ( ! KeepControl() )
         ((ObjectProxy*)pyobject)->Release();

   // calculate offset between formal and actual arguments
      void* obj = pyobj->GetObject();
      G__ClassInfo* clFormalInfo = fClass->GetClassInfo();
      G__ClassInfo* clActualInfo = pyobj->ObjectIsA()->GetClassInfo();
      Long_t offset = 0;
      if ( clFormalInfo && clActualInfo )
         offset = G__isanybase( clFormalInfo->Tagnum(), clActualInfo->Tagnum(), (Long_t)obj );

   // set pointer (may be null) and declare success
      func->SetArg( reinterpret_cast< Long_t >( obj ) + offset );
      return kTRUE;

   } else if ( ! fClass.GetClass()->GetClassInfo() ) {
   // assume "user knows best" to allow anonymous pointer passing
      func->SetArg( reinterpret_cast< Long_t >( pyobj->GetObject() ) );
      return kTRUE;
   }

   return kFALSE;
}

//____________________________________________________________________________
PyObject* PyROOT::TRootObjectConverter::FromMemory( void* address )
{
   return BindRootObject( address, fClass, kFALSE );
}

//____________________________________________________________________________
Bool_t PyROOT::TRootObjectConverter::ToMemory( PyObject* value, void* address )
{
   if ( ! ObjectProxy_Check( value ) ) {
      if ( value == gNullObject ) {   // allow NULL pointer as a special case
          *(Long_t**)address = 0l;
          return kTRUE;
      }

   // not a PyROOT object (TODO: handle SWIG etc.)
      return kFALSE;
   }

   if ( ((ObjectProxy*)value)->ObjectIsA()->GetBaseClass( fClass.GetClass() ) ) {
   // depending on memory policy, some objects need releasing when passed into functions
      if ( ! KeepControl() )
         ((ObjectProxy*)value)->Release();

   // TODO: fix this, as this is sooo wrong ...
      memcpy( (void*)address, ((ObjectProxy*)value)->GetObject(), fClass->Size() );
      return kTRUE;
   }

   return kFALSE;
}

//____________________________________________________________________________
Bool_t PyROOT::TRootObjectPtrConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   if ( ! ObjectProxy_Check( pyobject ) )
      return kFALSE;              // not a PyROOT object (TODO: handle SWIG etc.)

   if ( ((ObjectProxy*)pyobject)->ObjectIsA()->GetBaseClass( fClass.GetClass() ) ) {
   // depending on memory policy, some objects need releasing when passed into functions
      if ( ! KeepControl() )
         ((ObjectProxy*)pyobject)->Release();

   // set pointer (may be null) and declare success
      func->SetArg( reinterpret_cast< Long_t >( &((ObjectProxy*)pyobject)->fObject ) );
      return kTRUE;
   }

   return kFALSE;
}

//____________________________________________________________________________
PyObject* PyROOT::TRootObjectPtrConverter::FromMemory( void* address )
{
   return BindRootObject( address, fClass, kTRUE );
}

//____________________________________________________________________________
Bool_t PyROOT::TRootObjectPtrConverter::ToMemory( PyObject* value, void* address )
{
   if ( ! ObjectProxy_Check( value ) )
      return kFALSE;              // not a PyROOT object (TODO: handle SWIG etc.)

   if ( ((ObjectProxy*)value)->ObjectIsA()->GetBaseClass( fClass.GetClass() ) ) {
   // depending on memory policy, some objects need releasing when passed into functions
      if ( ! KeepControl() )
         ((ObjectProxy*)value)->Release();

   // set pointer (may be null) and declare success
      *(void**)address = ((ObjectProxy*)value)->GetObject();
      return kTRUE;
   }

   return kFALSE;
}

//____________________________________________________________________________
Bool_t PyROOT::TVoidPtrRefConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   if ( ObjectProxy_Check( pyobject ) ) {
      func->SetArgRef( reinterpret_cast< Long_t& >( ((ObjectProxy*)pyobject)->fObject ) );
      return kTRUE;
   }

   return kFALSE;
}

//____________________________________________________________________________
Bool_t PyROOT::TVoidPtrPtrConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
   if ( ObjectProxy_Check( pyobject ) ) {
   // this is a ROOT object, take and set its address
      func->SetArg( reinterpret_cast< Long_t >( &((ObjectProxy*)pyobject)->fObject ) );
      return kTRUE;
   }

// buffer objects are allowed under "user knows best"
   void* buf = 0;
   int buflen = Utility::GetBuffer( pyobject, '*', 1, buf, kFALSE );

// ok if buffer exists (can't perform any useful size checks)
   if ( buf && buflen != 0 ) {
      func->SetArg( (Long_t)buf );
      return kTRUE;
   }

   return kFALSE;
}

//____________________________________________________________________________
Bool_t PyROOT::TPyObjectConverter::SetArg( PyObject* pyobject, G__CallFunc* func )
{
// by definition: set and declare success
   func->SetArg( reinterpret_cast< Long_t >( pyobject ) );
   return kTRUE;
}

PyObject* PyROOT::TPyObjectConverter::FromMemory( void* address )
{
   PyObject* pyobject = *((PyObject**)address);

   if ( ! pyobject ) {
      Py_INCREF( Py_None );
      return Py_None;
   }

   Py_INCREF( pyobject );
   return pyobject;
}

Bool_t PyROOT::TPyObjectConverter::ToMemory( PyObject* value, void* address )
{
   Py_INCREF( value );
   *((PyObject**)address) = value;
   return kTRUE;
}


//- factories -----------------------------------------------------------------
PyROOT::TConverter* PyROOT::CreateConverter( const std::string& fullType, Long_t user )
{
// The matching of the fulltype to a converter factory goes in five steps:
//   1) full, exact match
//   2) match of decorated, unqualified type
//   3) accept const ref as by value
//   4) accept ref as pointer
//   5) generalized cases (covers basically all ROOT classes)
//
// If all fails, void is used, which will generate a run-time warning when used.

// resolve typedefs etc.
   G__TypeInfo ti( fullType.c_str() );
   std::string resolvedType = ti.TrueName();

// an exactly matching converter is preferred
   ConvFactories_t::iterator h = gConvFactories.find( resolvedType );
   if ( h != gConvFactories.end() )
      return (h->second)( user );

//-- nothing? ok, collect information about the type and possible qualifiers/decorators
   const std::string& cpd = Utility::Compound( resolvedType );
   std::string realType   = TClassEdit::ShortType( resolvedType.c_str(), 1 );

// accept unqualified type (as python does not know about qualifiers)
   h = gConvFactories.find( realType + cpd );
   if ( h != gConvFactories.end() )
      return (h->second)( user );

//-- nothing? collect qualifier information
   Bool_t isConst = resolvedType.find( "const" ) != std::string::npos;

// accept const <type>& as converter by value (as python copies most types)
   if ( isConst && cpd == "&" ) {
      h = gConvFactories.find( realType );
      if ( h != gConvFactories.end() )
         return (h->second)( user );
   }

//-- still nothing? try pointer instead of ref, if ref
   if ( cpd == "&" ) {
      h = gConvFactories.find( realType + "*" );
      if ( h != gConvFactories.end() )
         return (h->second)( user );
   }

//-- still nothing? use a generalized converter
   Bool_t control = kTRUE;
   if ( Utility::gMemoryPolicy == Utility::kHeuristics )
      control = cpd == "&" || isConst;

// converters for known/ROOT classes and default (void*)
   TConverter* result = 0;
   if ( TClass* klass = gROOT->GetClass( realType.c_str() ) ) {
      if ( cpd == "**" || cpd == "*&" || cpd == "&*" )
         result = new TRootObjectPtrConverter( klass, control );
      else if ( cpd == "*" || cpd == "&" )
         result = new TRootObjectConverter( klass, control );
      else if ( cpd == "" )               // by value
         result = new TRootObjectConverter( klass, kTRUE );

   } else if ( ti.Property() & G__BIT_ISENUM ) {
   // special case (CINT): represent enums as unsigned integers
      if ( cpd == "&" )
         h = gConvFactories.find( "long&" );
      else
         h = gConvFactories.find( "UInt_t" );
   }

   if ( ! result && h != gConvFactories.end() )
   // converter factory available, use it to create converter
      result = (h->second)( user );
   else if ( ! result ) {
      if ( cpd != "" )
         result = new TVoidArrayConverter();       // "user knows best"
      else
         result = new TVoidConverter();            // fails on use
   }

   return result;
}

//____________________________________________________________________________
#define PYROOT_BASIC_CONVERTER_FACTORY( name )                               \
TConverter* Create##name##Converter( Long_t )                                \
{                                                                            \
   return new T##name##Converter();                                          \
}

#define PYROOT_ARRAY_CONVERTER_FACTORY( name )                               \
TConverter* Create##name##Converter( Long_t user )                           \
{                                                                            \
   return new T##name##Converter( (Int_t)user );                             \
}

//____________________________________________________________________________
namespace {

   using namespace PyROOT;

// use macro rather than template for portability ...
   PYROOT_BASIC_CONVERTER_FACTORY( Bool )
   PYROOT_BASIC_CONVERTER_FACTORY( Char )
   PYROOT_BASIC_CONVERTER_FACTORY( UChar )
   PYROOT_BASIC_CONVERTER_FACTORY( Short )
   PYROOT_BASIC_CONVERTER_FACTORY( UShort )
   PYROOT_BASIC_CONVERTER_FACTORY( Int )
   PYROOT_BASIC_CONVERTER_FACTORY( UInt )
   PYROOT_BASIC_CONVERTER_FACTORY( Long )
   PYROOT_BASIC_CONVERTER_FACTORY( LongRef )
   PYROOT_BASIC_CONVERTER_FACTORY( ULong )
   PYROOT_BASIC_CONVERTER_FACTORY( Float )
   PYROOT_BASIC_CONVERTER_FACTORY( Double )
   PYROOT_BASIC_CONVERTER_FACTORY( DoubleRef )
   PYROOT_BASIC_CONVERTER_FACTORY( Void )
   PYROOT_BASIC_CONVERTER_FACTORY( LongLong )
   PYROOT_BASIC_CONVERTER_FACTORY( CString )
   PYROOT_ARRAY_CONVERTER_FACTORY( ShortArray )
   PYROOT_ARRAY_CONVERTER_FACTORY( UShortArray )
   PYROOT_ARRAY_CONVERTER_FACTORY( IntArray )
   PYROOT_ARRAY_CONVERTER_FACTORY( UIntArray )
   PYROOT_ARRAY_CONVERTER_FACTORY( LongArray )
   PYROOT_ARRAY_CONVERTER_FACTORY( ULongArray )
   PYROOT_ARRAY_CONVERTER_FACTORY( FloatArray )
   PYROOT_ARRAY_CONVERTER_FACTORY( DoubleArray )
   PYROOT_BASIC_CONVERTER_FACTORY( VoidArray )
   PYROOT_BASIC_CONVERTER_FACTORY( LongLongArray )
   PYROOT_BASIC_CONVERTER_FACTORY( TString )
   PYROOT_BASIC_CONVERTER_FACTORY( STLString )
   PYROOT_BASIC_CONVERTER_FACTORY( VoidPtrRef )
   PYROOT_BASIC_CONVERTER_FACTORY( VoidPtrPtr )
   PYROOT_BASIC_CONVERTER_FACTORY( PyObject )

// converter factories for ROOT types
   typedef std::pair< const char*, ConverterFactory_t > NFp_t;

   NFp_t factories_[] = {
   // factories for built-ins
      NFp_t( "bool",               &CreateBoolConverter               ),
      NFp_t( "char",               &CreateCharConverter               ),
      NFp_t( "unsigned char",      &CreateUCharConverter              ),
      NFp_t( "short",              &CreateShortConverter              ),
      NFp_t( "unsigned short",     &CreateUShortConverter             ),
      NFp_t( "int",                &CreateIntConverter                ),
      NFp_t( "int&",               &CreateLongRefConverter            ),
      NFp_t( "unsigned int",       &CreateUIntConverter               ),
      NFp_t( "UInt_t", /* enum */  &CreateUIntConverter               ),
      NFp_t( "long",               &CreateLongConverter               ),
      NFp_t( "long&",              &CreateLongRefConverter            ),
      NFp_t( "unsigned long",      &CreateULongConverter              ),
      NFp_t( "long long",          &CreateLongLongConverter           ),
      NFp_t( "float",              &CreateFloatConverter              ),
      NFp_t( "double",             &CreateDoubleConverter             ),
      NFp_t( "double&",            &CreateDoubleRefConverter          ),
      NFp_t( "void",               &CreateVoidConverter               ),

   // pointer/array factories
      NFp_t( "short*",             &CreateShortArrayConverter         ),
      NFp_t( "unsigned short*",    &CreateUShortArrayConverter        ),
      NFp_t( "int*",               &CreateIntArrayConverter           ),
      NFp_t( "unsigned int*",      &CreateUIntArrayConverter          ),
      NFp_t( "long*",              &CreateLongArrayConverter          ),
      NFp_t( "unsigned long*",     &CreateULongArrayConverter         ),
      NFp_t( "float*",             &CreateFloatArrayConverter         ),
      NFp_t( "double*",            &CreateDoubleArrayConverter        ),
      NFp_t( "long long*",         &CreateLongLongArrayConverter      ),
      NFp_t( "void*",              &CreateVoidArrayConverter          ),

   // factories for special cases
      NFp_t( "const char*",        &CreateCStringConverter            ),
      NFp_t( "char*",              &CreateCStringConverter            ),
      NFp_t( "TString",            &CreateTStringConverter            ),
      NFp_t( "TString&",           &CreateTStringConverter            ),
      NFp_t( "std::string",        &CreateSTLStringConverter          ),
      NFp_t( "string",             &CreateSTLStringConverter          ),
      NFp_t( "std::string&",       &CreateSTLStringConverter          ),
      NFp_t( "string&",            &CreateSTLStringConverter          ),
      NFp_t( "void*&",             &CreateVoidPtrRefConverter         ),
      NFp_t( "void**",             &CreateVoidPtrPtrConverter         ),
      NFp_t( "PyObject*",          &CreatePyObjectConverter           ),
      NFp_t( "_object*",           &CreatePyObjectConverter           )
   };

   struct InitConvFactories_t {
   public:
      InitConvFactories_t()
      {
         int nf = sizeof( factories_ ) / sizeof( factories_[ 0 ] );
         for ( int i = 0; i < nf; ++i ) {
            gConvFactories[ factories_[ i ].first ] = factories_[ i ].second;
         }
      }
   } initConvFactories_;

} // unnamed namespace
