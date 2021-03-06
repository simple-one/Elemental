/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"
#include "./NNLS/ADMM.hpp"
#include "./NNLS/IPM.hpp"

namespace El {

// Solve each problem 
//
//   min || A x - b ||_2 
//   s.t. x >= 0
//
// by transforming it into the explicit QP
//
//   min (1/2) x^T (A^T A) x + (-A^T b)^T x
//   s.t. x >= 0.
//
// Note that the matrix A^T A is cached amongst all instances
// (and this caching is the reason NNLS supports X and B as matrices).
//

template<typename Real>
void NNLS
( const Matrix<Real>& A, const Matrix<Real>& B, 
        Matrix<Real>& X, 
  const NNLSCtrl<Real>& ctrl )
{
    DEBUG_ONLY(CSE cse("NNLS"))
    if( IsComplex<Real>::val ) 
        LogicError("The datatype was assumed to be real");
    if( ctrl.useIPM )
        nnls::IPM( A, B, X, ctrl.ipmCtrl );
    else
        nnls::ADMM( A, B, X, ctrl.admmCtrl );
}

template<typename Real>
void NNLS
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& B, 
        AbstractDistMatrix<Real>& X,
  const NNLSCtrl<Real>& ctrl )
{
    DEBUG_ONLY(CSE cse("NNLS"))
    if( IsComplex<Real>::val ) 
        LogicError("The datatype was assumed to be real");
    if( ctrl.useIPM )
        nnls::IPM( A, B, X, ctrl.ipmCtrl );
    else
        nnls::ADMM( A, B, X, ctrl.admmCtrl );
}

template<typename Real>
void NNLS
( const SparseMatrix<Real>& A, const Matrix<Real>& B, 
        Matrix<Real>& X, 
  const NNLSCtrl<Real>& ctrl )
{
    DEBUG_ONLY(CSE cse("NNLS"))
    if( IsComplex<Real>::val ) 
        LogicError("The datatype was assumed to be real");
    if( !ctrl.useIPM )
        LogicError("ADMM NNLS not yet supported for sparse matrices");
    nnls::IPM( A, B, X, ctrl.ipmCtrl );
}

template<typename Real>
void NNLS
( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& B, 
        DistMultiVec<Real>& X, 
  const NNLSCtrl<Real>& ctrl )
{
    DEBUG_ONLY(CSE cse("NNLS"))
    if( IsComplex<Real>::val ) 
        LogicError("The datatype was assumed to be real");
    if( !ctrl.useIPM )
        LogicError("ADMM NNLS not yet supported for sparse matrices");
    nnls::IPM( A, B, X, ctrl.ipmCtrl );
}

#define PROTO(Real) \
  template void NNLS \
  ( const Matrix<Real>& A, const Matrix<Real>& B, \
          Matrix<Real>& X, \
    const NNLSCtrl<Real>& ctrl ); \
  template void NNLS \
  ( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& B, \
          AbstractDistMatrix<Real>& X, \
    const NNLSCtrl<Real>& ctrl ); \
  template void NNLS \
  ( const SparseMatrix<Real>& A, const Matrix<Real>& B, \
          Matrix<Real>& X, \
    const NNLSCtrl<Real>& ctrl ); \
  template void NNLS \
  ( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& B, \
          DistMultiVec<Real>& X, \
    const NNLSCtrl<Real>& ctrl );

#define EL_NO_INT_PROTO
#define EL_NO_COMPLEX_PROTO
#include "El/macros/Instantiate.h"

} // namespace El
