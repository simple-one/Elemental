/*
   Copyright (c) 2009-2011, Jack Poulson
   All rights reserved.

   This file is part of Elemental.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of the owner nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/
#include <ctime>
#include "elemental.hpp"
#include "elemental/advanced_internal.hpp"
using namespace std;
using namespace elemental;
using namespace elemental::imports;

void Usage()
{
    cout << "Generates random matrix then solves for its LU factors.\n\n"
         << "  LU <r> <c> <m> <nb> <pivot?> <correctness?> <print?>\n\n"
         << "  r: number of process rows\n"
         << "  c: number of process cols\n"
         << "  m: height of matrix\n"
         << "  nb: algorithmic blocksize\n"
         << "  pivot: no partial pivoting iff 0\n"
         << "  test correctness?: false iff 0\n"
         << "  print matrices?: false iff 0\n" << endl;
}

template<typename F> // represents a real or complex field
void TestCorrectness
( bool pivoted, bool printMatrices, 
  const DistMatrix<F,MC,MR>& A,
  const DistMatrix<int,VC,STAR>& p,
  const DistMatrix<F,MC,MR>& AOrig )
{
    const Grid& g = A.Grid();
    const int m = AOrig.Height();

    if( g.VCRank() == 0 )
        cout << "Testing error..." << endl;

    // Generate random right-hand sides
    DistMatrix<F,MC,MR> X(m,100,g);
    DistMatrix<F,MC,MR> Y(g);
    X.SetToRandom();
    F oneNormOfX = advanced::Norm( X, ONE_NORM );
    F infNormOfX = advanced::Norm( X, INFINITY_NORM );
    F frobNormOfX = advanced::Norm( X, FROBENIUS_NORM );
    Y = X;

    if( pivoted )
    {
        // Apply the pivots to our random right-hand sides
        advanced::internal::ApplyRowPivots( Y, p );
    }

    // Solve against the (pivoted) right-hand sides
    basic::Trsm( LEFT, LOWER, NORMAL, UNIT, (F)1, A, Y );
    basic::Trsm( LEFT, UPPER, NORMAL, NON_UNIT, (F)1, A, Y );

    // Now investigate the residual, ||AOrig Y - X||_oo
    basic::Gemm( NORMAL, NORMAL, (F)-1, AOrig, Y, (F)1, X );
    F oneNormOfError = advanced::Norm( X, ONE_NORM );
    F infNormOfError = advanced::Norm( X, INFINITY_NORM );
    F frobNormOfError = advanced::Norm( X, FROBENIUS_NORM );
    F oneNormOfA = advanced::Norm( AOrig, ONE_NORM );
    F infNormOfA = advanced::Norm( AOrig, INFINITY_NORM );
    F frobNormOfA = advanced::Norm( AOrig, FROBENIUS_NORM );

    if( g.VCRank() == 0 )
    {
        cout << "||A||_1                  = " << Abs(oneNormOfA) << "\n"
             << "||A||_oo                 = " << Abs(infNormOfA) << "\n"
             << "||A||_F                  = " << Abs(frobNormOfA) << "\n"
             << "||X||_1                  = " << Abs(oneNormOfX) << "\n"
             << "||X||_oo                 = " << Abs(infNormOfX) << "\n"
             << "||X||_F                  = " << Abs(frobNormOfX) << "\n"
             << "||A U^-1 L^-1 X - X||_1  = " << Abs(oneNormOfError) << "\n"
             << "||A U^-1 L^-1 X - X||_oo = " << Abs(infNormOfError) << "\n"
             << "||A U^-1 L^-1 X - X||_F  = " << Abs(frobNormOfError) << endl;
    }
}

template<typename F> // represents a real or complex field
void TestLU
( bool pivot, bool testCorrectness, bool printMatrices, 
  int m, const Grid& g )
{
    double startTime, endTime, runTime, gFlops;
    DistMatrix<F,MC,MR> A(g);
    DistMatrix<F,MC,MR> ARef(g);
    DistMatrix<int,VC,STAR> p(g);

    A.ResizeTo( m, m );
    p.ResizeTo( m, 1 );

    A.SetToRandom();
    if( testCorrectness )
    {
        if( g.VCRank() == 0 )
        {
            cout << "  Making copy of original matrix...";
            cout.flush();
        }
        ARef = A;
        if( g.VCRank() == 0 )
            cout << "DONE" << endl;
    }
    if( printMatrices )
        A.Print("A");

    if( g.VCRank() == 0 )
    {
        cout << "  Starting LU factorization...";
        cout.flush();
    }
    mpi::Barrier( g.VCComm() );
    startTime = mpi::Time();
    if( pivot )
        advanced::LU( A, p );
    else
        advanced::LU( A );
    mpi::Barrier( g.VCComm() );
    endTime = mpi::Time();
    runTime = endTime - startTime;
    gFlops = advanced::internal::LUGFlops<F>( m, runTime );
    if( g.VCRank() == 0 )
    {
        cout << "DONE. " << endl
             << "  Time = " << runTime << " seconds. GFlops = " 
             << gFlops << endl;
    }
    if( printMatrices )
    {
        A.Print("A after factorization");
        if( pivot )
            p.Print("p after factorization");
    }
    if( testCorrectness )
        TestCorrectness( pivot, printMatrices, A, p, ARef );
}

int 
main( int argc, char* argv[] )
{
    Init( argc, argv );
    mpi::Comm comm = mpi::COMM_WORLD;
    const int rank = mpi::CommRank( comm );

    if( argc < 8 )
    {
        if( rank == 0 )
            Usage();
        Finalize();
        return 0;
    }

    try
    {
        int argNum = 0;
        const int r = atoi(argv[++argNum]);
        const int c = atoi(argv[++argNum]);
        const int m = atoi(argv[++argNum]);
        const int nb = atoi(argv[++argNum]);
        const bool pivot = atoi(argv[++argNum]);
        const bool testCorrectness = atoi(argv[++argNum]);
        const bool printMatrices = atoi(argv[++argNum]);
#ifndef RELEASE
        if( rank == 0 )
        {
            cout << "==========================================\n"
                 << " In debug mode! Performance will be poor! \n"
                 << "==========================================" << endl;
        }
#endif
        const Grid g( comm, r, c );
        SetBlocksize( nb );

        if( rank == 0 )
            cout << "Will test LU" 
                 << ( pivot ? " with partial pivoting" : " " ) << endl;

        if( rank == 0 )
        {
            cout << "---------------------\n"
                 << "Testing with doubles:\n"
                 << "---------------------" << endl;
        }
        TestLU<double>
        ( pivot, testCorrectness, printMatrices, m, g );

#ifndef WITHOUT_COMPLEX
        if( rank == 0 )
        {
            cout << "--------------------------------------\n"
                 << "Testing with double-precision complex:\n"
                 << "--------------------------------------" << endl;
        }
        TestLU<dcomplex>
        ( pivot, testCorrectness, printMatrices, m, g );
#endif
    }
    catch( exception& e )
    {
#ifndef RELEASE
        DumpCallStack();
#endif
        cerr << "Process " << rank << " caught error message:\n"
             << e.what() << endl;
    }   
    Finalize();
    return 0;
}
