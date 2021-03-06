/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

// W = | A^H  L |, where K and L are Hermitian.
//     | K   -A |
//
// The solution, X, to the equation
//   X K X - A^H X - X A = L
// is returned, as well as the number of Newton iterations for computing sgn(W).
//
// See Chapter 2 of Nicholas J. Higham's "Functions of Matrices"

template<typename F>
void Ricatti( Matrix<F>& W, Matrix<F>& X, SignCtrl<Base<F>> ctrl )
{
    DEBUG_ONLY(CSE cse("Ricatti"))
    Sign( W, ctrl );
    const Int n = W.Height()/2;
    Matrix<F> WTL, WTR,
              WBL, WBR;
    PartitionDownDiagonal
    ( W, WTL, WTR,
         WBL, WBR, n );

    // (ML, MR) = sgn(W) - I
    ShiftDiagonal( W, F(-1) );

    // Solve for X in ML X = -MR
    Matrix<F> ML, MR;
    PartitionRight( W, ML, MR, n );
    Scale( F(-1), MR );
    ls::Overwrite( NORMAL, ML, MR, X );
}

template<typename F>
void Ricatti
( AbstractDistMatrix<F>& WPre, AbstractDistMatrix<F>& X, 
  SignCtrl<Base<F>> ctrl )
{
    DEBUG_ONLY(CSE cse("Ricatti"))

    auto WPtr = ReadProxy<F,MC,MR>( &WPre );
    auto& W = *WPtr;

    const Grid& g = W.Grid();
    Sign( W, ctrl );
    const Int n = W.Height()/2;
    DistMatrix<F> WTL(g), WTR(g),
                  WBL(g), WBR(g);
    PartitionDownDiagonal
    ( W, WTL, WTR,
         WBL, WBR, n );

    // (ML, MR) = sgn(W) - I
    ShiftDiagonal( W, F(-1) );

    // Solve for X in ML X = -MR
    DistMatrix<F> ML(g), MR(g);
    PartitionRight( W, ML, MR, n );
    Scale( F(-1), MR );
    ls::Overwrite( NORMAL, ML, MR, X );
}

template<typename F>
void Ricatti
( UpperOrLower uplo, 
  const Matrix<F>& A, const Matrix<F>& K, const Matrix<F>& L, Matrix<F>& X,
  SignCtrl<Base<F>> ctrl )
{
    DEBUG_ONLY(
        CSE cse("Sylvester");
        if( A.Height() != A.Width() )
            LogicError("A must be square");
        if( K.Height() != K.Width() )
            LogicError("K must be square");
        if( L.Height() != L.Width() )
            LogicError("L must be square");
        if( A.Height() != K.Height() || A.Height() != L.Height() )
            LogicError("A, K, and L must be the same size");
    )
    const Int n = A.Height();
    Matrix<F> W, WTL, WTR,
                 WBL, WBR;
    Zeros( W, 2*n, 2*n );
    PartitionDownDiagonal
    ( W, WTL, WTR,
         WBL, WBR, n );

    Adjoint( A, WTL );
    WBR = A; Scale( F(-1), WBR );
    WBL = K; MakeHermitian( uplo, WBL );
    WTR = L; MakeHermitian( uplo, WTR );

    Ricatti( W, X, ctrl );
}

template<typename F>
void Ricatti
( UpperOrLower uplo, 
  const AbstractDistMatrix<F>& A, const AbstractDistMatrix<F>& K, 
  const AbstractDistMatrix<F>& L,       AbstractDistMatrix<F>& X, 
  SignCtrl<Base<F>> ctrl )
{
    DEBUG_ONLY(
        CSE cse("Sylvester");
        if( A.Height() != A.Width() )
            LogicError("A must be square");
        if( K.Height() != K.Width() )
            LogicError("K must be square");
        if( L.Height() != L.Width() )
            LogicError("L must be square");
        if( A.Height() != K.Height() || A.Height() != L.Height() )
            LogicError("A, K, and L must be the same size");
        AssertSameGrids( A, K, L );
    )
    const Grid& g = A.Grid();
    const Int n = A.Height();
    DistMatrix<F> W(g), WTL(g), WTR(g),
                        WBL(g), WBR(g);
    Zeros( W, 2*n, 2*n );
    PartitionDownDiagonal
    ( W, WTL, WTR,
         WBL, WBR, n );

    Adjoint( A, WTL );
    WBR = A; Scale( F(-1), WBR );
    WBL = K; MakeHermitian( uplo, WBL );
    WTR = L; MakeHermitian( uplo, WTR );

    Ricatti( W, X, ctrl );
}

#define PROTO(F) \
  template void Ricatti \
  ( Matrix<F>& W, Matrix<F>& X, SignCtrl<Base<F>> ctrl ); \
  template void Ricatti \
  ( AbstractDistMatrix<F>& W, AbstractDistMatrix<F>& X, \
    SignCtrl<Base<F>> ctrl ); \
  template void Ricatti \
  ( UpperOrLower uplo, \
    const Matrix<F>& A, const Matrix<F>& K, const Matrix<F>& L, \
          Matrix<F>& X, SignCtrl<Base<F>> ctrl ); \
  template void Ricatti \
  ( UpperOrLower uplo, \
    const AbstractDistMatrix<F>& A, const AbstractDistMatrix<F>& K, \
    const AbstractDistMatrix<F>& L,       AbstractDistMatrix<F>& X, \
    SignCtrl<Base<F>> ctrl );

#define EL_NO_INT_PROTO
#include "El/macros/Instantiate.h"

} // namespace El
