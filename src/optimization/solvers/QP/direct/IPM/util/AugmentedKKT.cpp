/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {
namespace qp {
namespace direct {

// Form 
//
//    | Q + (x <> z)  A^T | | dx | = | -r_c - x <> r_mu |,
//    |      A         0  | | dy |   | -r_b             |
//
// where 
//
//    r_b  = A x - b,
//    r_c  = Q x + A^T y - z + c,
//    r_mu = x o z - tau e,
//
// and dz can be computed using
//
//   dz = - x <> (r_mu + z o dx)
//

template<typename Real>
void AugmentedKKT
( const Matrix<Real>& Q, const Matrix<Real>& A, 
  const Matrix<Real>& x, const Matrix<Real>& z,
        Matrix<Real>& J, bool onlyLower )
{
    DEBUG_ONLY(CSE cse("qp::direct::AugmentedKKT"))
    const Int m = A.Height();
    const Int n = A.Width();

    Zeros( J, m+n, m+n );
    const IR xInd(0,n), yInd(n,n+m);
    auto Jxx = J(xInd,xInd); auto Jxy = J(xInd,yInd);
    auto Jyx = J(yInd,xInd); auto Jyy = J(yInd,yInd);
    Matrix<Real> d( z );
    DiagonalSolve( LEFT, NORMAL, x, d );
    Diagonal( Jxx, d );
    Axpy( Real(1), Q, Jxx );
    Jyx = A;
    if( !onlyLower )
        Transpose( A, Jxy );
}

template<typename Real>
void AugmentedKKT
( const AbstractDistMatrix<Real>& Q,    const AbstractDistMatrix<Real>& A, 
  const AbstractDistMatrix<Real>& x,    const AbstractDistMatrix<Real>& z,
        AbstractDistMatrix<Real>& JPre, bool onlyLower )
{
    DEBUG_ONLY(CSE cse("qp::direct::AugmentedKKT"))
    const Int m = A.Height();
    const Int n = A.Width();

    auto JPtr = WriteProxy<Real,MC,MR>(&JPre);
    auto& J = *JPtr;

    Zeros( J, m+n, m+n );
    const IR xInd(0,n), yInd(n,n+m);
    auto Jxx = J(xInd,xInd); auto Jxy = J(xInd,yInd);
    auto Jyx = J(yInd,xInd); auto Jyy = J(yInd,yInd);
    DistMatrix<Real,STAR,STAR> d( z );
    DiagonalSolve( LEFT, NORMAL, x, d );
    Diagonal( Jxx, d.Matrix() );
    Axpy( Real(1), Q, Jxx );
    Jyx = A;
    if( !onlyLower )
        Transpose( A, Jxy );
}

template<typename Real>
void AugmentedKKT
( const SparseMatrix<Real>& Q, const SparseMatrix<Real>& A, 
  const Matrix<Real>& x,       const Matrix<Real>& z,
        SparseMatrix<Real>& J, bool onlyLower )
{
    DEBUG_ONLY(CSE cse("qp::direct::AugmentedKKT"))
    const Int m = A.Height();
    const Int n = A.Width();
    const Int numEntriesQ = Q.NumEntries();
    const Int numEntriesA = A.NumEntries();
    // Count the number of used entries of Q
    Int numUsedEntriesQ;
    if( onlyLower )
    {
        numUsedEntriesQ = 0;
        for( Int e=0; e<numEntriesQ; ++e )
            if( Q.Row(e) >= Q.Col(e) )
                ++numUsedEntriesQ;
    }
    else
        numUsedEntriesQ = numEntriesQ;

    Zeros( J, m+n, m+n );
    if( onlyLower )
        J.Reserve( numEntriesA + numUsedEntriesQ + n );
    else
        J.Reserve( 2*numEntriesA + numUsedEntriesQ + n ); 

    // x <> z updates
    for( Int j=0; j<n; ++j )
        J.QueueUpdate( j, j, z.Get(j,0)/x.Get(j,0) );
    // Q update
    for( Int e=0; e<numEntriesQ; ++e )
    {
        const Int i = Q.Row(e);
        const Int j = Q.Col(e);
        if( i >= j )
            J.QueueUpdate( i, j, Q.Value(e) );
        else if( !onlyLower )
            J.QueueUpdate( i, j, Q.Value(e) );
    }
    // A and A^T updates
    for( Int e=0; e<numEntriesA; ++e )
    {
        J.QueueUpdate( A.Row(e)+n, A.Col(e), A.Value(e) );
        if( !onlyLower )
            J.QueueUpdate( A.Col(e), A.Row(e)+n, A.Value(e) );
    }
    J.ProcessQueues();
}

template<typename Real>
void AugmentedKKT
( const DistSparseMatrix<Real>& Q, const DistSparseMatrix<Real>& A,
  const DistMultiVec<Real>& x,     const DistMultiVec<Real>& z,
        DistSparseMatrix<Real>& J, bool onlyLower )
{
    DEBUG_ONLY(CSE cse("qp::direct::AugmentedKKT"))
    const Int m = A.Height();
    const Int n = A.Width();

    mpi::Comm comm = A.Comm();
    const Int commSize = mpi::Size( comm );

    J.SetComm( comm );
    Zeros( J, m+n, m+n );

    // Compute the number of entries to send to each process
    // =====================================================
    vector<int> sendCounts(commSize,0);
    // For placing A into the bottom-left corner
    // -----------------------------------------
    for( Int e=0; e<A.NumLocalEntries(); ++e )
        ++sendCounts[ J.RowOwner(A.Row(e)+n) ];
    // For placing A^T into the top-right corner
    // -----------------------------------------
    DistSparseMatrix<Real> ATrans(comm);
    if( !onlyLower )
    {
        Transpose( A, ATrans );
        for( Int e=0; e<ATrans.NumLocalEntries(); ++e )
            ++sendCounts[ J.RowOwner(ATrans.Row(e)) ];
    }
    // For placing x <> z into the top-left corner
    // -------------------------------------------
    for( Int iLoc=0; iLoc<x.LocalHeight(); ++iLoc )
        ++sendCounts[ J.RowOwner( x.GlobalRow(iLoc) ) ];
    // For placing Q into the top-left corner
    for( Int e=0; e<Q.NumLocalEntries(); ++e )
    {
        const Int i = Q.Row(e);
        const Int j = Q.Col(e);
        if( i >= j || !onlyLower )
            ++sendCounts[ J.RowOwner(i) ]; 
    }
    vector<int> sendOffs;
    const int totalSend = Scan( sendCounts, sendOffs );

    // Pack the triplets
    // =================
    vector<Entry<Real>> sendBuf(totalSend);
    auto offs = sendOffs;
    // Pack A
    // ------
    for( Int e=0; e<A.NumLocalEntries(); ++e )
    {
        const Int i = A.Row(e) + n;
        const Int j = A.Col(e);
        sendBuf[offs[J.RowOwner(i)]++] = Entry<Real>{ i, j, A.Value(e) };
    }
    // Pack A^T
    // --------
    if( !onlyLower )
    {
        for( Int e=0; e<ATrans.NumLocalEntries(); ++e )
        {
            const Int i = ATrans.Row(e);
            const Int j = ATrans.Col(e) + n;
            const Real value = ATrans.Value(e);
            sendBuf[offs[J.RowOwner(i)]++] = Entry<Real>{ i, j, value };
        }
    }
    // Pack x <> z
    // -----------
    for( Int iLoc=0; iLoc<x.LocalHeight(); ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int j = i;
        const Real value = z.GetLocal(iLoc,0)/x.GetLocal(iLoc,0);
        sendBuf[offs[J.RowOwner(i)]++] = Entry<Real>{ i, j, value };
    }
    // Pack Q
    // ------
    for( Int e=0; e<Q.NumLocalEntries(); ++e )
    {
        const Int i = Q.Row(e);
        const Int j = Q.Col(e);
        if( i >= j || !onlyLower )
            sendBuf[offs[J.RowOwner(i)]++] = Entry<Real>{ i, j, Q.Value(e) };
    }

    // Exchange and unpack the triplets
    // ================================
    auto recvBuf = mpi::AllToAll( sendBuf, sendCounts, sendOffs, comm );
    J.Reserve( recvBuf.size() );
    for( auto& entry : recvBuf )
        J.QueueUpdate( entry );
    J.ProcessQueues();
}

template<typename Real>
void AugmentedKKTRHS
( const Matrix<Real>& x, 
  const Matrix<Real>& rc,  const Matrix<Real>& rb, 
  const Matrix<Real>& rmu,
        Matrix<Real>& d )
{
    DEBUG_ONLY(CSE cse("qp::direct::AugmentedKKTRHS"))
    const Int m = rb.Height();
    const Int n = rmu.Height();
    const IR xInd(0,n), yInd(n,n+m);
    Zeros( d, m+n, 1 );

    // dx := - (r_c + x <> r_mu)
    // =========================
    auto dx = d(xInd,ALL);
    dx = rmu;
    DiagonalSolve( LEFT, NORMAL, x, dx );
    Axpy( Real(1), rc, dx );
    Scale( Real(-1), dx );

    // dy := -r_b
    // ==========
    auto dy = d(yInd,ALL);
    dy = rb;
    Scale( Real(-1), dy );
}

template<typename Real>
void AugmentedKKTRHS
( const AbstractDistMatrix<Real>& xPre, 
  const AbstractDistMatrix<Real>& rc,   const AbstractDistMatrix<Real>& rb, 
  const AbstractDistMatrix<Real>& rmu,
        AbstractDistMatrix<Real>& dPre )
{
    DEBUG_ONLY(CSE cse("qp::direct::AugmentedKKTRHS"))

    ProxyCtrl ctrl;
    ctrl.colConstrain = true;
    ctrl.rowConstrain = true;
    ctrl.colAlign = 0;
    ctrl.rowAlign = 0;
    
    auto xPtr = ReadProxy<Real,MC,MR>(&xPre,ctrl);  auto& x = *xPtr;
    auto dPtr = WriteProxy<Real,MC,MR>(&dPre,ctrl); auto& d = *dPtr;

    const Int m = rb.Height();
    const Int n = rmu.Height();
    const IR xInd(0,n), yInd(n,n+m);
    Zeros( d, m+n, 1 );

    // dx := - (r_c + x <> r_mu)
    // =========================
    auto dx = d(xInd,ALL);
    dx = rmu;
    DiagonalSolve( LEFT, NORMAL, x, dx );
    Axpy( Real(1), rc, dx );
    Scale( Real(-1), dx );

    // dy := -r_b
    // ==========
    auto dy = d(yInd,ALL);
    dy = rb;
    Scale( Real(-1), dy );
}

template<typename Real>
void AugmentedKKTRHS
( const DistMultiVec<Real>& x,
  const DistMultiVec<Real>& rc,  const DistMultiVec<Real>& rb, 
  const DistMultiVec<Real>& rmu, 
        DistMultiVec<Real>& d )
{
    DEBUG_ONLY(CSE cse("qp::direct::FormAugmentedSystem"))
    const Int m = rb.Height();
    const Int n = x.Height();
    Zeros( d, m+n, 1 );
    mpi::Comm comm = x.Comm();
    const Int commSize = mpi::Size( comm );

    // Compute the number of entries to send/recv from each process
    // ============================================================
    vector<int> sendCounts(commSize,0);
    for( Int iLoc=0; iLoc<rc.LocalHeight(); ++iLoc )
        ++sendCounts[ d.RowOwner( rc.GlobalRow(iLoc) ) ];
    for( Int iLoc=0; iLoc<rb.LocalHeight(); ++iLoc )
        ++sendCounts[ d.RowOwner( rb.GlobalRow(iLoc)+n ) ];

    // Pack the doublets
    // =================
    vector<int> sendOffs;
    const int totalSend = Scan( sendCounts, sendOffs );
    vector<ValueInt<Real>> sendBuf(totalSend);
    auto offs = sendOffs;
    for( Int iLoc=0; iLoc<rc.LocalHeight(); ++iLoc )
    {
        const Int i = rc.GlobalRow(iLoc);
        const Real value = -rc.GetLocal(iLoc,0) -
                            rmu.GetLocal(iLoc,0)/x.GetLocal(iLoc,0);
        sendBuf[offs[d.RowOwner(i)]++] = ValueInt<Real>{ value, i };
    }
    for( Int iLoc=0; iLoc<rb.LocalHeight(); ++iLoc )
    {
        const Int i = rb.GlobalRow(iLoc) + n;
        const Real value = -rb.GetLocal(iLoc,0);
        sendBuf[offs[d.RowOwner(i)]++] = ValueInt<Real>{ value, i };
    }

    // Exchange and unpack the doublets
    // ================================
    auto recvBuf = mpi::AllToAll( sendBuf, sendCounts, sendOffs, comm );
    for( auto& entry : recvBuf )
        d.Update( entry.index, 0, entry.value );
}

template<typename Real>
void ExpandAugmentedSolution
( const Matrix<Real>& x,   const Matrix<Real>& z,
  const Matrix<Real>& rmu, const Matrix<Real>& d,
        Matrix<Real>& dx,        Matrix<Real>& dy, 
        Matrix<Real>& dz )
{
    DEBUG_ONLY(CSE cse("qp::direct::ExpandAugmentedSolution"))
    const Int n = rmu.Height();
    const Int m = d.Height() - n;

    // Extract dx and dy from [dx; dy]
    // ===============================
    dx.Resize( n, 1 );
    dy.Resize( m, 1 );
    const IR xInd(0,n), yInd(n,n+m);
    auto d_x = d(xInd,ALL);
    auto d_y = d(yInd,ALL);
    dx = d_x;
    dy = d_y;

    // dz := - x <> (r_mu + z o dx)
    // ============================
    dz = dx;
    DiagonalScale( LEFT, NORMAL, z, dz );
    Axpy( Real(1), rmu, dz );
    DiagonalSolve( LEFT, NORMAL, x, dz );
    Scale( Real(-1), dz );
}

template<typename Real>
void ExpandAugmentedSolution
( const AbstractDistMatrix<Real>& xPre,   const AbstractDistMatrix<Real>& zPre,
  const AbstractDistMatrix<Real>& rmuPre, const AbstractDistMatrix<Real>& dPre,
        AbstractDistMatrix<Real>& dxPre,        AbstractDistMatrix<Real>& dy, 
        AbstractDistMatrix<Real>& dzPre )
{
    DEBUG_ONLY(CSE cse("qp::direct::ExpandAugmentedSolution"))

    ProxyCtrl ctrl;
    ctrl.colConstrain = true;
    ctrl.rowConstrain = true;
    ctrl.colAlign = 0;
    ctrl.rowAlign = 0;

    auto xPtr = ReadProxy<Real,MC,MR>(&xPre,ctrl); auto& x = *xPtr;
    auto zPtr = ReadProxy<Real,MC,MR>(&zPre,ctrl); auto& z = *zPtr;

    auto rmuPtr = ReadProxy<Real,MC,MR>(&rmuPre); auto& rmu = *rmuPtr;
    auto dPtr   = ReadProxy<Real,MC,MR>(&dPre);   auto& d   = *dPtr;

    auto dxPtr = WriteProxy<Real,MC,MR>(&dxPre,ctrl); auto& dx = *dxPtr;
    auto dzPtr = WriteProxy<Real,MC,MR>(&dzPre,ctrl); auto& dz = *dzPtr;

    const Int n = rmu.Height();
    const Int m = d.Height() - n;

    // Extract dx and dy from [dx; dy]
    // ===============================
    dx.Resize( n, 1 );
    dy.Resize( m, 1 );
    const IR xInd(0,n), yInd(n,n+m);
    auto d_x = d(xInd,ALL);
    auto d_y = d(yInd,ALL);
    dx = d_x;
    Copy( d_y, dy );

    // dz := - x <> (r_mu + z o dx)
    // ============================
    dz = dx;
    DiagonalScale( LEFT, NORMAL, z, dz );
    Axpy( Real(1), rmu, dz );
    DiagonalSolve( LEFT, NORMAL, x, dz );
    Scale( Real(-1), dz );
}

template<typename Real>
void ExpandAugmentedSolution
( const DistMultiVec<Real>& x,   const DistMultiVec<Real>& z,
  const DistMultiVec<Real>& rmu, const DistMultiVec<Real>& d,
        DistMultiVec<Real>& dx,        DistMultiVec<Real>& dy, 
        DistMultiVec<Real>& dz )
{
    DEBUG_ONLY(CSE cse("qp::direct::ExpandAugmentedSolution"))
    const Int n = rmu.Height();
    const Int m = d.Height() - n;
    mpi::Comm comm = z.Comm();
    const Int commSize = mpi::Size(comm);

    // Extract dx and dy from [dx; dy]
    // ===============================
    dx.Resize( n, 1 );
    dy.Resize( m, 1 );
    // Compute the number of entries to send to each process
    // -----------------------------------------------------
    vector<int> sendCounts(commSize,0);
    for( Int iLoc=0; iLoc<d.LocalHeight(); ++iLoc )
    {
        const Int i = d.GlobalRow(iLoc);
        if( i < n )
            ++sendCounts[ dx.RowOwner(i) ];
        else
            ++sendCounts[ dy.RowOwner(i-n) ];
    }
    // Pack the entries and row indices of dx and dy
    // ---------------------------------------------
    vector<int> sendOffs;
    const int totalSend = Scan( sendCounts, sendOffs );
    vector<ValueInt<Real>> sendBuf(totalSend);
    auto offs = sendOffs;
    for( Int iLoc=0; iLoc<d.LocalHeight(); ++iLoc )
    {
        const Int i = d.GlobalRow(iLoc);
        if( i < n )
        {
            const int owner = dx.RowOwner(i); 
            sendBuf[offs[owner]++] = ValueInt<Real>{ d.GetLocal(iLoc,0), i };
        }
        else
        {
            const int owner = dy.RowOwner(i-n);
            sendBuf[offs[owner]++] = ValueInt<Real>{ d.GetLocal(iLoc,0), i };
        }
    }
    // Exchange and unpack the entries and indices
    // -------------------------------------------
    auto recvBuf = mpi::AllToAll( sendBuf, sendCounts, sendOffs, comm );
    for( auto& entry : recvBuf )
    {
        const Int i = entry.index;
        if( i < n )
            dx.Set( i, 0, entry.value );
        else
            dy.Set( i-n, 0, entry.value );
    }

    // dz := - x <> (r_mu + z o dx)
    // ============================
    dz = dx;
    DiagonalScale( LEFT, NORMAL, z, dz );
    Axpy( Real(1), rmu, dz );
    DiagonalSolve( LEFT, NORMAL, x, dz );
    Scale( Real(-1), dz );
}

#define PROTO(Real) \
  template void AugmentedKKT \
  ( const Matrix<Real>& Q, const Matrix<Real>& A, \
    const Matrix<Real>& x, const Matrix<Real>& z, \
    Matrix<Real>& J, bool onlyLower ); \
  template void AugmentedKKT \
  ( const AbstractDistMatrix<Real>& Q, const AbstractDistMatrix<Real>& A, \
    const AbstractDistMatrix<Real>& x, const AbstractDistMatrix<Real>& z, \
    AbstractDistMatrix<Real>& J, bool onlyLower ); \
  template void AugmentedKKT \
  ( const SparseMatrix<Real>& Q, const SparseMatrix<Real>& A, \
    const Matrix<Real>& x,       const Matrix<Real>& z, \
    SparseMatrix<Real>& J,       bool onlyLower ); \
  template void AugmentedKKT \
  ( const DistSparseMatrix<Real>& Q, const DistSparseMatrix<Real>& A, \
    const DistMultiVec<Real>& x,     const DistMultiVec<Real>& z, \
    DistSparseMatrix<Real>& J,       bool onlyLower ); \
  template void AugmentedKKTRHS \
  ( const Matrix<Real>& x, \
    const Matrix<Real>& rc, const Matrix<Real>& rb, \
    const Matrix<Real>& rmu, \
          Matrix<Real>& d ); \
  template void AugmentedKKTRHS \
  ( const AbstractDistMatrix<Real>& x, \
    const AbstractDistMatrix<Real>& rc,  const AbstractDistMatrix<Real>& rb, \
    const AbstractDistMatrix<Real>& rmu, \
          AbstractDistMatrix<Real>& d ); \
  template void AugmentedKKTRHS \
  ( const DistMultiVec<Real>& x, \
    const DistMultiVec<Real>& rc,  const DistMultiVec<Real>& rb, \
    const DistMultiVec<Real>& rmu, \
          DistMultiVec<Real>& d ); \
  template void ExpandAugmentedSolution \
  ( const Matrix<Real>& x,   const Matrix<Real>& z, \
    const Matrix<Real>& rmu, const Matrix<Real>& d, \
          Matrix<Real>& dx,        Matrix<Real>& dy, \
          Matrix<Real>& dz ); \
  template void ExpandAugmentedSolution \
  ( const AbstractDistMatrix<Real>& x,   const AbstractDistMatrix<Real>& z, \
    const AbstractDistMatrix<Real>& rmu, const AbstractDistMatrix<Real>& d, \
          AbstractDistMatrix<Real>& dx,        AbstractDistMatrix<Real>& dy, \
          AbstractDistMatrix<Real>& dz ); \
  template void ExpandAugmentedSolution \
  ( const DistMultiVec<Real>& x,   const DistMultiVec<Real>& z, \
    const DistMultiVec<Real>& rmu, const DistMultiVec<Real>& d, \
          DistMultiVec<Real>& dx,        DistMultiVec<Real>& dy, \
          DistMultiVec<Real>& dz );

#define EL_NO_INT_PROTO
#define EL_NO_COMPLEX_PROTO
#include "El/macros/Instantiate.h"

} // namespace direct
} // namespace qp
} // namespace El
