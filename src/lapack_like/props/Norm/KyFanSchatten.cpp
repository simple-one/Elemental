/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

template<typename F> 
Base<F> KyFanSchattenNorm( const Matrix<F>& A, Int k, Base<F> p )
{
    DEBUG_ONLY(CSE cse("KyFanSchattenNorm"))
    if( k < 1 || k > Min(A.Height(),A.Width()) )
        LogicError("Invalid index of KyFanSchatten norm");

    typedef Base<F> Real;
    Matrix<F> B( A );
    Matrix<Real> s;
    SVD( B, s );

    Real sum = 0;
    for( Int j=k-1; j>=0; --j )
        sum += Pow( s.Get(j,0), p );
    return Pow( sum, 1/p );
}

template<typename F>
Base<F> HermitianKyFanSchattenNorm
( UpperOrLower uplo, const Matrix<F>& A, Int k, Base<F> p )
{
    DEBUG_ONLY(CSE cse("HermitianKyFanSchattenNorm"))
    if( k < 1 || k > Min(A.Height(),A.Width()) )
        LogicError("Invalid index of KyFanSchatten norm");

    typedef Base<F> Real;
    Matrix<F> B( A );
    Matrix<Real> s;
    HermitianSVD( uplo, B, s );

    Real sum = 0;
    for( Int j=k-1; j>=0; --j )
        sum += Pow( s.Get(j,0), p );
    return Pow( sum, 1/p );
}

template<typename F>
Base<F> SymmetricKyFanSchattenNorm
( UpperOrLower uplo, const Matrix<F>& A, Int k, Base<F> p )
{
    DEBUG_ONLY(CSE cse("SymmetricKyFanSchattenNorm"))
    if( k < 1 || k > Min(A.Height(),A.Width()) )
        LogicError("Invalid index of KyFanSchatten norm");

    typedef Base<F> Real;
    Matrix<F> B( A );
    Matrix<Real> s;
    MakeSymmetric( uplo, B );
    SVD( B, s );

    Real sum = 0;
    for( Int j=k-1; j>=0; --j )
        sum += Pow( s.Get(j,0), p );
    return Pow( sum, 1/p );
}

template<typename F> 
Base<F> KyFanSchattenNorm( const AbstractDistMatrix<F>& A, Int k, Base<F> p )
{
    DEBUG_ONLY(CSE cse("KyFanSchattenNorm"))
    if( k < 1 || k > Min(A.Height(),A.Width()) )
        LogicError("Invalid index of KyFanSchatten norm");

    typedef Base<F> Real;
    DistMatrix<F> B( A );
    DistMatrix<Real,VR,STAR> s( A.Grid() );
    SVD( B, s );

    Real localSum = 0;
    auto sTop = s( IR(0,k), ALL );
    const Int localHeight = sTop.LocalHeight();
    for( Int j=localHeight-1; j>=0; --j )
        localSum += Pow( sTop.GetLocal(j,0), p );
    const Real sum = mpi::AllReduce( localSum, sTop.ColComm() );
    return Pow( sum, 1/p );
}

template<typename F>
Base<F> HermitianKyFanSchattenNorm
( UpperOrLower uplo, const AbstractDistMatrix<F>& A, Int k, Base<F> p )
{
    DEBUG_ONLY(CSE cse("HermitianKyFanSchattenNorm"))
    if( k < 1 || k > Min(A.Height(),A.Width()) )
        LogicError("Invalid index of KyFanSchatten norm");

    typedef Base<F> Real;
    DistMatrix<F> B( A );
    DistMatrix<Real,VR,STAR> s( A.Grid() );
    HermitianSVD( uplo, B, s );

    Real localSum = 0;
    auto sTop = s( IR(0,k), ALL );
    const Int localHeight = sTop.LocalHeight();
    for( Int j=localHeight-1; j>=0; --j )
        localSum += Pow( sTop.GetLocal(j,0), p );
    const Real sum = mpi::AllReduce( localSum, sTop.ColComm() );
    return Pow( sum, 1/p );
}

template<typename F>
Base<F> SymmetricKyFanSchattenNorm
( UpperOrLower uplo, const AbstractDistMatrix<F>& A, Int k, Base<F> p )
{
    DEBUG_ONLY(CSE cse("SymmetricKyFanSchattenNorm"))
    if( k < 1 || k > Min(A.Height(),A.Width()) )
        LogicError("Invalid index of KyFanSchatten norm");

    typedef Base<F> Real;
    DistMatrix<F> B( A );
    DistMatrix<Real,VR,STAR> s( A.Grid() );
    MakeSymmetric( uplo, B );
    SVD( B, s );

    Real localSum = 0;
    auto sTop = s( IR(0,k), ALL );
    const Int localHeight = sTop.LocalHeight();
    for( Int j=localHeight-1; j>=0; --j )
        localSum += Pow( sTop.GetLocal(j,0), p );
    const Real sum = mpi::AllReduce( localSum, sTop.ColComm() );
    return Pow( sum, 1/p );
}

#define PROTO(F) \
  template Base<F> KyFanSchattenNorm \
  ( const Matrix<F>& A, Int k, Base<F> p ); \
  template Base<F> KyFanSchattenNorm \
  ( const AbstractDistMatrix<F>& A, Int k, Base<F> p ); \
  template Base<F> HermitianKyFanSchattenNorm \
  ( UpperOrLower uplo, const Matrix<F>& A, Int k, Base<F> p ); \
  template Base<F> HermitianKyFanSchattenNorm \
  ( UpperOrLower uplo, const AbstractDistMatrix<F>& A, Int k, Base<F> p ); \
  template Base<F> SymmetricKyFanSchattenNorm \
  ( UpperOrLower uplo, const Matrix<F>& A, Int k, Base<F> p ); \
  template Base<F> SymmetricKyFanSchattenNorm \
  ( UpperOrLower uplo, const AbstractDistMatrix<F>& A, Int k, Base<F> p );

#define EL_NO_INT_PROTO
#include "El/macros/Instantiate.h"

} // namespace El
