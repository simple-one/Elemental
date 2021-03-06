/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.
 
   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

#include "./LowerMultiply/Forward.hpp"
#include "./LowerMultiply/Backward.hpp"

namespace El {
namespace ldl {

template<typename F>
void LowerMultiply
( Orientation orientation, const NodeInfo& info, 
  const Front<F>& front, MatrixNode<F>& X )
{
    DEBUG_ONLY(CSE cse("LowerMultiply"))
    if( orientation == NORMAL )
        LowerForwardMultiply( info, front, X );
    else
        LowerBackwardMultiply( info, front, X, orientation==ADJOINT );
}

template<typename F>
void LowerMultiply
( Orientation orientation, const DistNodeInfo& info, 
  const DistFront<F>& front, DistMultiVecNode<F>& X )
{
    DEBUG_ONLY(CSE cse("LowerMultiply"))
    if( orientation == NORMAL )
        LowerForwardMultiply( info, front, X );
    else
        LowerBackwardMultiply( info, front, X, orientation==ADJOINT );
}

template<typename F>
void LowerMultiply
( Orientation orientation, const DistNodeInfo& info, 
  const DistFront<F>& front, DistMatrixNode<F>& X )
{
    DEBUG_ONLY(CSE cse("LowerMultiply"))
    if( orientation == NORMAL )
        LowerForwardMultiply( info, front, X );
    else
        LowerBackwardMultiply( info, front, X, orientation==ADJOINT );
}

#define PROTO(F) \
  template void LowerMultiply \
  ( Orientation orientation, const NodeInfo& info, \
    const Front<F>& front, MatrixNode<F>& X ); \
  template void LowerMultiply \
  ( Orientation orientation, const DistNodeInfo& info, \
    const DistFront<F>& front, DistMultiVecNode<F>& X ); \
  template void LowerMultiply \
  ( Orientation orientation, const DistNodeInfo& info, \
    const DistFront<F>& front, DistMatrixNode<F>& X );

#define EL_NO_INT_PROTO
#include "El/macros/Instantiate.h"

} // namespace ldl
} // namespace El
