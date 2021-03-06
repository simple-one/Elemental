/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"
using namespace std;
using namespace El;

template<typename F>
void MakeQuasiTriangular( UpperOrLower uplo, DistMatrix<F>& A )
{
    DEBUG_ONLY(CallStackEntry cse("MakeQuasiTriangular"))
    const Int n = A.Height();
    if( n < 3 )
        return;
    if( uplo == LOWER )
    {
        MakeTrapezoidal( LOWER, A, 1 );
        auto dSup = GetDiagonal(A,+1);
        DistMatrix<F,STAR,STAR> dSup_STAR_STAR( dSup );
        for( Int j=0; j<n-2; ++j )
        {
            const F thisSup = dSup_STAR_STAR.Get(j,  0);
            const F nextSup = dSup_STAR_STAR.Get(j+1,0);
            if( thisSup != F(0) && nextSup != F(0) )
            {
                A.Set(j+1,j+2,0);
                dSup_STAR_STAR.Set(j+1,0,0);
            }
        }
    }
    else
    {
        MakeTrapezoidal( UPPER, A, -1 );
        auto dSub = GetDiagonal(A,-1);
        DistMatrix<F,STAR,STAR> dSub_STAR_STAR( dSub );
        for( Int j=0; j<n-2; ++j )
        {
            const F thisSub = dSub_STAR_STAR.Get(j,  0);
            const F nextSub = dSub_STAR_STAR.Get(j+1,0);
            if( thisSub != F(0) && nextSub != F(0) )
            {
                A.Set(j+2,j+1,0);
                dSub_STAR_STAR.Set(j+1,0,0);
            }
        }
    }
}

template<typename F> 
void TestMultiShiftQuasiTrsm
( bool print,
  LeftOrRight side, UpperOrLower uplo, Orientation orientation, 
  Int m, Int n, F alpha, const Grid& g )
{
    typedef Base<F> Real;
    DistMatrix<F> H(g), X(g);
    DistMatrix<F,VR,STAR> shifts(g);

    if( side == LEFT )
    {
        HermitianUniformSpectrum( H, m, 1, 10 );
        Uniform( shifts, n, 1, F(0), Real(0.5) );
    }
    else
    {
        HermitianUniformSpectrum( H, n, 1, 10 );
        Uniform( shifts, m, 1, F(0), Real(0.5) );
    }
    MakeQuasiTriangular( uplo, H );

    auto modShifts(shifts);
    if( orientation == ADJOINT )
        Conjugate( modShifts );

    Uniform( X, m, n );
    DistMatrix<F> Y(g);
    Zeros( Y, m, n );
    if( side == LEFT )
    {
        Gemm( orientation, NORMAL, F(1)/alpha, H, X, F(1), Y );
        for( Int j=0; j<n; ++j )
        {
            auto x = LockedView( X, 0, j, m, 1 );
            auto y =       View( Y, 0, j, m, 1 );
            Axpy( -modShifts.Get(j,0)/alpha, x, y );
        }
    }
    else
    {
        Gemm( NORMAL, orientation, F(1)/alpha, X, H, F(1), Y );
        for( Int i=0; i<m; ++i )
        {
            auto x = LockedView( X, i, 0, 1, n );
            auto y =       View( Y, i, 0, 1, n );
            Axpy( -modShifts.Get(i,0)/alpha, x, y );
        }
    }

    if( print )
    {
        Print( H, "H" );
        Print( shifts, "shifts" );
        Print( X, "X" );
        Print( Y, "Y" );
    }
    if( g.Rank() == 0 )
    {
        cout << "  Starting MultiShiftQuasiTrsm...";
        cout.flush();
    }
    mpi::Barrier( g.Comm() );
    const double startTime = mpi::Time();
    MultiShiftQuasiTrsm( side, uplo, orientation, alpha, H, shifts, Y );
    mpi::Barrier( g.Comm() );
    const double runTime = mpi::Time() - startTime;
    const double realGFlops = 
        ( side==LEFT ? double(m)*double(m)*double(n)
                     : double(m)*double(n)*double(n) ) /(1.e9*runTime);
    const double gFlops = ( IsComplex<F>::val ? 4*realGFlops : realGFlops );
    if( g.Rank() == 0 )
    {
        cout << "DONE. \n"
             << "  Time = " << runTime << " seconds. GFlops ~= " << gFlops 
             << endl;
    }
    if( print )
        Print( Y, "Y after solve" );
    Axpy( F(-1), X, Y );
    const auto HFrob = FrobeniusNorm( H );
    const auto XFrob = FrobeniusNorm( X );
    const auto EFrob = FrobeniusNorm( Y );
    if( g.Rank() == 0 )
    {
        cout << "|| H ||_F = " << HFrob << "\n"
             << "|| X ||_F = " << XFrob << "\n"
             << "|| E ||_F = " << EFrob << "\n" << std::endl;
    }
}

int 
main( int argc, char* argv[] )
{
    Initialize( argc, argv );
    mpi::Comm comm = mpi::COMM_WORLD;
    const Int commRank = mpi::Rank( comm );
    const Int commSize = mpi::Size( comm );

    try
    {
        Int r = Input("--r","height of process grid",0);
        const bool colMajor = Input("--colMajor","column-major ordering?",true);
        const char sideChar = Input("--side","side to solve from: L/R",'L');
        const char uploChar = Input
            ("--uplo","lower or upper quasi-triangular: L/U",'L');
        const char transChar = Input
            ("--trans","orientation of quasi-triangular matrix: N/T/C",'N');
        const Int m = Input("--m","height of result",100);
        const Int n = Input("--n","width of result",100);
        const Int nb = Input("--nb","algorithmic blocksize",96);
        const bool print = Input("--print","print matrices?",false);
        ProcessInput();
        PrintInputReport();

        if( r == 0 )
            r = Grid::FindFactor( commSize );
        const GridOrder order = ( colMajor ? COLUMN_MAJOR : ROW_MAJOR );
        const Grid g( comm, r, order );
        const LeftOrRight side = CharToLeftOrRight( sideChar );
        const UpperOrLower uplo = CharToUpperOrLower( uploChar );
        const Orientation orientation = CharToOrientation( transChar );
        SetBlocksize( nb );

        ComplainIfDebug();
        if( commRank == 0 )
            cout << "Will test MultiShiftQuasiTrsm" 
                 << sideChar << uploChar << transChar << endl;

        if( commRank == 0 )
            cout << "Testing with doubles:" << endl;
        TestMultiShiftQuasiTrsm<double>
        ( print, side, uplo, orientation, m, n, 3., g );

        if( commRank == 0 )
            cout << "Testing with double-precision complex:" << endl;
        TestMultiShiftQuasiTrsm<Complex<double>>
        ( print, side, uplo, orientation, m, n, Complex<double>(3), g );
    }
    catch( exception& e ) { ReportException(e); }

    Finalize();
    return 0;
}
