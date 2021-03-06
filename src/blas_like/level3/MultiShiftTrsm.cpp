/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

#include "./MultiShiftTrsm/LUN.hpp"
#include "./MultiShiftTrsm/LUT.hpp"

namespace El {

template<typename F>
void MultiShiftTrsm
( LeftOrRight side, UpperOrLower uplo, Orientation orientation,
  F alpha, Matrix<F>& U, const Matrix<F>& shifts, Matrix<F>& X )
{
    DEBUG_ONLY(CSE cse("MultiShiftTrsm"))
    if( side == LEFT && uplo == UPPER )
    {
        if( orientation == NORMAL )
            mstrsm::LUN( alpha, U, shifts, X );
        else
            mstrsm::LUT( orientation, alpha, U, shifts, X );
    }
    else
        LogicError("This option is not yet supported");
}

template<typename F>
void MultiShiftTrsm
( LeftOrRight side, UpperOrLower uplo, Orientation orientation,
  F alpha, const AbstractDistMatrix<F>& U, const AbstractDistMatrix<F>& shifts, 
  AbstractDistMatrix<F>& X )
{
    DEBUG_ONLY(CSE cse("MultiShiftTrsm"))
    if( side == LEFT && uplo == UPPER )
    {
        if( orientation == NORMAL )
            mstrsm::LUN( alpha, U, shifts, X );
        else
            mstrsm::LUT( orientation, alpha, U, shifts, X );
    }
    else
        LogicError("This option is not yet supported");
}

#define PROTO(F) \
  template void MultiShiftTrsm \
  ( LeftOrRight side, UpperOrLower uplo, Orientation orientation, \
    F alpha, Matrix<F>& U, const Matrix<F>& shifts, Matrix<F>& X ); \
  template void MultiShiftTrsm \
  ( LeftOrRight side, UpperOrLower uplo, Orientation orientation, \
    F alpha, const AbstractDistMatrix<F>& U, \
    const AbstractDistMatrix<F>& shifts, AbstractDistMatrix<F>& X );

#define EL_NO_INT_PROTO
#include "El/macros/Instantiate.h"

} // namespace El
