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
#include "elemental/dist_matrix.hpp"
using namespace std;
using namespace elemental;
using namespace elemental::imports;
using namespace elemental::utilities;

// Template conventions:
//   G: general datatype
//
//   T: any ring, e.g., the (Gaussian) integers and the real/complex numbers
//   Z: representation of a real ring, e.g., the integers or real numbers
//   std::complex<Z>: representation of a complex ring, e.g. Gaussian integers
//                    or complex numbers
//
//   F: representation of real or complex number
//   R: representation of real number
//   std::complex<R>: representation of complex number

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::Print
( ostream& os, const string msg ) const
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::Print");
#endif
    const elemental::Grid& g = this->Grid();
    if( g.VCRank() == 0 && msg != "" )
        os << msg << endl;

    const int height      = this->Height();
    const int width       = this->Width();
    const int localHeight = this->LocalHeight();
    const int c           = g.Width();
    const int colShift    = this->ColShift();

    if( height == 0 || width == 0 )
    {
#ifndef RELEASE
        PopCallStack();
#endif
        return;
    }

    // Only one process row needs to participate
    if( g.MCRank() == 0 )
    {
        vector<T> sendBuf(height*width,0);
        const T* thisLocalBuffer = this->LockedLocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
            for( int j=0; j<width; ++j )
                sendBuf[(colShift+iLocal*c)+j*height] = 
                    thisLocalBuffer[iLocal+j*thisLDim];

        // If we are the root, allocate a receive buffer
        vector<T> recvBuf;
        if( g.MRRank() == 0 )
            recvBuf.resize( height*width );

        // Sum the contributions and send to the root
        mpi::Reduce
        ( &sendBuf[0], &recvBuf[0], height*width, mpi::SUM, 0, g.MRComm() );

        if( g.MRRank() == 0 )
        {
            // Print the data
            for( int i=0; i<height; ++i )
            {
                for( int j=0; j<width; ++j )
                    os << recvBuf[i+j*height] << " ";
                os << "\n";
            }
            os << endl;
        }
    }
    mpi::Barrier( g.VCComm() );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::Print( const string msg ) const
{
    Print( cout, msg );
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::Align
( int colAlignment )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::Align");
    this->AssertFreeColAlignment();
#endif
    this->AlignCols( colAlignment );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignCols
( int colAlignment )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::AlignCols");
    this->AssertFreeColAlignment();
#endif
    const elemental::Grid& g = this->Grid();
#ifndef RELEASE
    if( colAlignment < 0 || colAlignment >= g.Width() )
        throw runtime_error( "Invalid column alignment for [MR,* ]" );
#endif
    this->_colAlignment = colAlignment;
    this->_colShift = Shift( g.MRRank(), colAlignment, g.Width() );
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignWith
( const DistMatrixBase<T,MR,MC>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::AlignWith([MR,MC])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    this->_colAlignment = A.ColAlignment();
    this->_colShift = A.ColShift();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignWith
( const DistMatrixBase<T,MR,STAR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::AlignWith([MR,* ])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    this->_colAlignment = A.ColAlignment();
    this->_colShift = A.ColShift();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignWith
( const DistMatrixBase<T,MC,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::AlignWith([MC,MR])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    this->_colAlignment = A.RowAlignment();
    this->_colShift = A.RowShift();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignWith
( const DistMatrixBase<T,STAR,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::AlignWith([* ,MR])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    this->_colAlignment = A.RowAlignment();
    this->_colShift = A.RowShift();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignWith
( const DistMatrixBase<T,VR,STAR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::AlignWith([VR,* ])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    const elemental::Grid& g = this->Grid();
    this->_colAlignment = A.ColAlignment();
    this->_colShift = 
        Shift( g.MRRank(), this->ColAlignment(), g.Width() );
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignWith
( const DistMatrixBase<T,STAR,VR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::AlignWith([* ,VR])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    const elemental::Grid& g = this->Grid();
    this->_colAlignment = A.RowAlignment();
    this->_colShift = 
        Shift( g.MRRank(), this->ColAlignment(), g.Width() );
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignColsWith
( const DistMatrixBase<T,MR,MC>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignColsWith
( const DistMatrixBase<T,MR,STAR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignColsWith
( const DistMatrixBase<T,MC,MR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignColsWith
( const DistMatrixBase<T,STAR,MR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignColsWith
( const DistMatrixBase<T,VR,STAR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AlignColsWith
( const DistMatrixBase<T,STAR,VR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::View
( DistMatrixBase<T,MR,STAR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::View");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
#endif
    this->_height = A.Height();
    this->_width = A.Width();
    this->_colAlignment = A.ColAlignment();
    this->_colShift = A.ColShift();
    this->_localMatrix.View( A.LocalMatrix() );
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::LockedView
( const DistMatrixBase<T,MR,STAR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::LockedView");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
#endif
    this->_height = A.Height();
    this->_width = A.Width();
    this->_colAlignment = A.ColAlignment();
    this->_colShift = A.ColShift();
    this->_localMatrix.LockedView( A.LockedLocalMatrix() );
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::View
( DistMatrixBase<T,MR,STAR>& A,
  int i, int j, int height, int width )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::View");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
    this->AssertValidSubmatrix( A, i, j, height, width );
#endif
    this->_height = height;
    this->_width = width;
    {
        const elemental::Grid& g = this->Grid();
        const int c   = g.Width();
        const int col = g.MRRank();

        this->_colAlignment = (A.ColAlignment()+i) % c;
        this->_colShift = Shift( col, this->ColAlignment(), c );

        const int localHeightBefore = LocalLength( i, A.ColShift(), c );
        const int localHeight = LocalLength( height, this->ColShift(), c );

        this->_localMatrix.View
        ( A.LocalMatrix(), localHeightBefore, j, localHeight, width );
    }
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::LockedView
( const DistMatrixBase<T,MR,STAR>& A,
  int i, int j, int height, int width )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::LockedView");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
    this->AssertValidSubmatrix( A, i, j, height, width );
#endif
    this->_height = height;
    this->_width = width;
    {
        const elemental::Grid& g = this->Grid();
        const int c   = g.Width();
        const int col = g.MRRank();

        this->_colAlignment = (A.ColAlignment()+i) % c;
        this->_colShift = Shift( col, this->ColAlignment(), c );

        const int localHeightBefore = LocalLength( i, A.ColShift(), c );
        const int localHeight = LocalLength( height, this->ColShift(), c );

        this->_localMatrix.LockedView
        ( A.LockedLocalMatrix(), localHeightBefore, j, localHeight, width );
    }
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::View1x2
( DistMatrixBase<T,MR,STAR>& AL, DistMatrixBase<T,MR,STAR>& AR )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::View1x2");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AL );
    this->AssertSameGrid( AR );
    this->AssertConforming1x2( AL, AR );
#endif
    this->_height = AL.Height();
    this->_width = AL.Width() + AR.Width();
    this->_colAlignment = AL.ColAlignment();
    this->_colShift = AL.ColShift();
    this->_localMatrix.View1x2( AL.LocalMatrix(), AR.LocalMatrix() );
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::LockedView1x2
( const DistMatrixBase<T,MR,STAR>& AL, const DistMatrixBase<T,MR,STAR>& AR )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::LockedView1x2");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AL );
    this->AssertSameGrid( AR );
    this->AssertConforming1x2( AL, AR );
#endif
    this->_height = AL.Height();
    this->_width = AL.Width() + AR.Width();
    this->_colAlignment = AL.ColAlignment();
    this->_colShift = AL.ColShift();
    this->_localMatrix.LockedView1x2
    ( AL.LockedLocalMatrix(), AR.LockedLocalMatrix() );
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::View2x1
( DistMatrixBase<T,MR,STAR>& AT,
  DistMatrixBase<T,MR,STAR>& AB )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::View2x1");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AT );
    this->AssertSameGrid( AB );
    this->AssertConforming2x1( AT, AB );
#endif
    this->_height = AT.Height() + AB.Height();
    this->_width = AT.Width();
    this->_colAlignment = AT.ColAlignment();
    this->_colShift = AT.ColShift();
    this->_localMatrix.View2x1
    ( AT.LocalMatrix(),
      AB.LocalMatrix() );
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::LockedView2x1
( const DistMatrixBase<T,MR,STAR>& AT,
  const DistMatrixBase<T,MR,STAR>& AB )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::LockedView2x1");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AT );
    this->AssertSameGrid( AB );
    this->AssertConforming2x1( AT, AB );
#endif
    this->_height = AT.Height() + AB.Height();
    this->_width = AT.Width();
    this->_colAlignment = AT.ColAlignment();
    this->_colShift = AT.ColShift();
    this->_localMatrix.LockedView2x1
    ( AT.LockedLocalMatrix(),
      AB.LockedLocalMatrix() );
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::View2x2
( DistMatrixBase<T,MR,STAR>& ATL,
  DistMatrixBase<T,MR,STAR>& ATR,
  DistMatrixBase<T,MR,STAR>& ABL,
  DistMatrixBase<T,MR,STAR>& ABR )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::View2x2");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( ATL );
    this->AssertSameGrid( ATR );
    this->AssertSameGrid( ABL );
    this->AssertSameGrid( ABR );
    this->AssertConforming2x2( ATL, ATR, ABL, ABR );
#endif
    this->_height = ATL.Height() + ABL.Height();
    this->_width = ATL.Width() + ATR.Width();
    this->_colAlignment = ATL.ColAlignment();
    this->_colShift = ATL.ColShift();
    this->_localMatrix.View2x2
    ( ATL.LocalMatrix(), ATR.LocalMatrix(),
      ABL.LocalMatrix(), ABR.LocalMatrix() );
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::LockedView2x2
( const DistMatrixBase<T,MR,STAR>& ATL,
  const DistMatrixBase<T,MR,STAR>& ATR,
  const DistMatrixBase<T,MR,STAR>& ABL,
  const DistMatrixBase<T,MR,STAR>& ABR )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::LockedView2x2");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( ATL );
    this->AssertSameGrid( ATR );
    this->AssertSameGrid( ABL );
    this->AssertSameGrid( ABR );
    this->AssertConforming2x2( ATL, ATR, ABL, ABR );
#endif
    this->_height = ATL.Height() + ABL.Height();
    this->_width = ATL.Width() + ATR.Width();
    this->_colAlignment = ATL.ColAlignment();
    this->_colShift = ATL.ColShift();
    this->_localMatrix.LockedView2x2
    ( ATL.LockedLocalMatrix(), ATR.LockedLocalMatrix(),
      ABL.LockedLocalMatrix(), ABR.LockedLocalMatrix() );
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::ResizeTo
( int height, int width )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::ResizeTo");
    this->AssertNotLockedView();
    if( height < 0 || width < 0 )
        throw logic_error( "Height and width must be non-negative." );
#endif
    this->_height = height;
    this->_width = width;
    this->_localMatrix.ResizeTo
    ( LocalLength(height,this->ColShift(),this->Grid().Width()), width );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
T
elemental::DistMatrixBase<T,MR,STAR>::Get
( int i, int j ) const
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::Get");
    this->AssertValidEntry( i, j );
#endif
    // We will determine the owner column of entry (i,j) and broadcast from that
    // columns within each process row
    const elemental::Grid& g = this->Grid();
    const int ownerCol = (i + this->ColAlignment()) % g.Width();

    T u;
    if( g.MRRank() == ownerCol )
    {
        const int iLoc = (i-this->ColShift()) / g.Width();
        u = this->GetLocalEntry(iLoc,j);
    }
    mpi::Broadcast( &u, 1, ownerCol, g.MRComm() );

#ifndef RELEASE
    PopCallStack();
#endif
    return u;
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::Set
( int i, int j, T u )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::Set");
    this->AssertValidEntry( i, j );
#endif
    const elemental::Grid& g = this->Grid();
    const int ownerCol = (i + this->ColAlignment()) % g.Width();

    if( g.MRRank() == ownerCol )
    {
        const int iLoc = (i-this->ColShift()) / g.Width();
        this->SetLocalEntry(iLoc,j,u);
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::Update
( int i, int j, T u )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::Update");
    this->AssertValidEntry( i, j );
#endif
    const elemental::Grid& g = this->Grid();
    const int ownerCol = (i + this->ColAlignment()) % g.Width();

    if( g.MRRank() == ownerCol )
    {
        const int iLoc = (i-this->ColShift()) / g.Width();
        this->UpdateLocalEntry(iLoc,j,u);
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

//
// Utility functions, e.g., SetToIdentity and MakeTrapezoidal
//

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::MakeTrapezoidal
( Side side, Shape shape, int offset )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::MakeTrapezoidal");
    this->AssertNotLockedView();
#endif
    const int height = this->Height();    
    const int width = this->Width();
    const int localHeight = this->LocalHeight();
    const int c = this->Grid().Width();
    const int colShift = this->ColShift();

    if( shape == LOWER )
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            int lastZeroRow = ( side==LEFT ? j-offset-1
                                           : j-offset+height-width-1 );
            if( lastZeroRow >= 0 )
            {
                int boundary = min( lastZeroRow+1, height );
                int numZeroRows = RawLocalLength( boundary, colShift, c );
                T* thisCol = &thisLocalBuffer[j*thisLDim];
                memset( thisCol, 0, numZeroRows*sizeof(T) );
            }
        }
    }
    else
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            int firstZeroRow = ( side==LEFT ? max(j-offset+1,0)
                                            : max(j-offset+height-width+1,0) );
            int numNonzeroRows = RawLocalLength(firstZeroRow,colShift,c);
            if( numNonzeroRows < localHeight )
            {
                T* thisCol = &thisLocalBuffer[numNonzeroRows+j*thisLDim];
                memset( thisCol, 0, (localHeight-numNonzeroRows)*sizeof(T) );
            }
        }
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::ScaleTrapezoidal
( T alpha, Side side, Shape shape, int offset )
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::ScaleTrapezoidal");
    this->AssertNotLockedView();
#endif
    const int height = this->Height();    
    const int width = this->Width();
    const int localHeight = this->LocalHeight();
    const int c = this->Grid().Width();
    const int colShift = this->ColShift();

    if( shape == UPPER )
    {
        T* thisLocalBuffer =this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            int lastRow = ( side==LEFT ? j-offset : j-offset+height-width );
            int boundary = min( lastRow+1, height );
            int numRows = RawLocalLength( boundary, colShift, c );
            T* thisCol = &thisLocalBuffer[j*thisLDim];
            for( int iLocal=0; iLocal<numRows; ++iLocal )
                thisCol[iLocal] *= alpha;
        }
    }
    else
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            int firstRow = ( side==LEFT ? max(j-offset,0)
                                        : max(j-offset+height-width,0) );
            int numZeroRows = RawLocalLength( firstRow, colShift, c );
            T* thisCol = &thisLocalBuffer[numZeroRows+j*thisLDim];
            for( int iLocal=0; iLocal<(localHeight-numZeroRows); ++iLocal )
                thisCol[iLocal] *= alpha;
        }
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::SetToIdentity()
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::SetToIdentity");
    this->AssertNotLockedView();
#endif
    const int width = this->Width();
    const int localHeight = this->LocalHeight();
    const int c = this->Grid().Width();
    const int colShift = this->ColShift();

    this->SetToZero();

    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for( int iLocal=0; iLocal<localHeight; ++iLocal )
    {
        const int i = colShift + iLocal*c;
        if( i < width )
            thisLocalBuffer[iLocal+i*thisLDim] = 1;
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::SetToRandom()
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::SetToRandom");
    this->AssertNotLockedView();
#endif
    const elemental::Grid& g = this->Grid();
    const int width = this->Width();
    const int localHeight = this->LocalHeight();
    const int bufSize = localHeight*width;

    this->_auxMemory.Require( bufSize );

    // Create random matrix on process row 0, then broadcast
    T* buffer = this->_auxMemory.Buffer();
    if( g.MCRank() == 0 )
    {
        for( int j=0; j<width; ++j )
            for( int i=0; i<localHeight; ++i )
                buffer[i+j*localHeight] = SampleUnitBall<T>();
    }
    mpi::Broadcast( buffer, bufSize, 0, g.MCComm() );

    // Unpack
    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for COLLAPSE(2)
#endif
    for( int j=0; j<width; ++j )
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
            thisLocalBuffer[iLocal+j*thisLDim] = buffer[iLocal+j*localHeight];

    this->_auxMemory.Release();
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::SumOverCol()
{
#ifndef RELEASE
    PushCallStack("[MR,* ]::SumOverCol");
    this->AssertNotLockedView();
#endif
    const elemental::Grid& g = this->Grid();
    
    const int width = this->Width();
    const int localHeight = this->LocalHeight();
    const int localSize = max( localHeight*width, mpi::MIN_COLL_MSG );

    this->_auxMemory.Require( 2*localSize );
    T* buffer = this->_auxMemory.Buffer();
    T* sendBuf = &buffer[0];
    T* recvBuf = &buffer[localSize];

    // Pack
    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for( int j=0; j<width; ++j )
    {
        const T* thisCol = &thisLocalBuffer[j*thisLDim];
        T* sendBufCol = &sendBuf[j*localHeight];
        memcpy( sendBufCol, thisCol, localHeight*sizeof(T) );
    }

    // AllReduce sum
    mpi::AllReduce
    ( sendBuf, recvBuf, localSize, mpi::SUM, g.MCComm() );

    // Unpack
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for( int j=0; j<width; ++j )
    {
        const T* recvBufCol = &recvBuf[j*localHeight];
        T* thisCol = &thisLocalBuffer[j*thisLDim];
        memcpy( thisCol, recvBufCol, localHeight*sizeof(T) );
    }
    this->_auxMemory.Release();
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::AdjointFrom
( const DistMatrixBase<T,MC,MR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ]::AdjointFrom");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSizeAsTranspose( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->_colAlignment = A.RowAlignment();
            this->_colShift = 
                Shift( g.MRRank(), this->ColAlignment(), g.Width() );
        }
        this->ResizeTo( A.Width(), A.Height() );
    }

    if( this->ColAlignment() == A.RowAlignment() )
    {
        const int r = g.Height();

        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localHeightOfA = A.LocalHeight();
        const int maxLocalWidth = MaxLocalLength(width,r);

        const int portionSize = 
            max(localHeight*maxLocalWidth,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (r+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* originalData = &buffer[0];
        T* gatheredData = &buffer[portionSize];

        // Pack 
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int jLocal=0; jLocal<localHeightOfA; ++jLocal )
            for( int iLocal=0; iLocal<localHeight; ++iLocal )
                originalData[iLocal+jLocal*localHeight] = 
                    Conj( ALocalBuffer[jLocal+iLocal*ALDim] );

        // Communicate
        mpi::AllGather
        ( originalData, portionSize,
          gatheredData, portionSize, g.MCComm() );

        // Unpack
        const int colAlignmentOfA = A.ColAlignment();
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &gatheredData[k*portionSize];

            const int rowShift = RawShift( k, colAlignmentOfA, r );
            const int localWidth = RawLocalLength( width, rowShift, r );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*localHeight];
                T* thisCol = &thisLocalBuffer[(rowShift+jLocal*r)*thisLDim];
                memcpy( thisCol, dataCol, localHeight*sizeof(T) );
            }
        }
        this->_auxMemory.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [MR,* ]::AdjointFrom" << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int col = g.MRRank();

        const int colAlignment = this->ColAlignment();
        const int rowAlignmentOfA = A.RowAlignment();
        const int sendCol = (col+c+colAlignment-rowAlignmentOfA) % c;
        const int recvCol = (col+c+rowAlignmentOfA-colAlignment) % c;

        const int height = this->Height();
        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localHeightOfA = A.LocalHeight();
        const int localWidthOfA = A.LocalWidth();
        const int maxLocalHeight = MaxLocalLength(height,c);
        const int maxLocalWidth = MaxLocalLength(width,r);

        const int portionSize = 
            max(maxLocalHeight*maxLocalWidth,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (r+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[portionSize];

        // Pack the currently owned local data of A into the second buffer
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int jLocal=0; jLocal<localHeightOfA; ++jLocal )
            for( int iLocal=0; iLocal<localWidthOfA; ++iLocal )
                secondBuffer[iLocal+jLocal*localWidthOfA] = 
                    Conj( ALocalBuffer[jLocal+iLocal*ALDim] );

        // Perform the SendRecv: puts the new data into the first buffer
        mpi::SendRecv
        ( secondBuffer, portionSize, sendCol, 0,
          firstBuffer,  portionSize, recvCol, mpi::ANY_TAG, g.MRComm() );

        // Use the output of the SendRecv as input to the AllGather
        mpi::AllGather
        ( firstBuffer,  portionSize,
          secondBuffer, portionSize, g.MCComm() );

        // Unpack the contents of each member of the process col
        const int colAlignmentOfA = A.ColAlignment();
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const int rowShift = RawShift( k, colAlignmentOfA, r );
            const int localWidth = RawLocalLength( width, rowShift, r );
#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*localHeight];
                T* thisCol = &thisLocalBuffer[(rowShift+jLocal*r)*thisLDim];
                memcpy( thisCol, dataCol, localHeight*sizeof(T) );
            }
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MR,STAR>::TransposeFrom
( const DistMatrixBase<T,MC,MR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ]::TransposeFrom");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSizeAsTranspose( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->_colAlignment = A.RowAlignment();
            this->_colShift = 
                Shift( g.MRRank(), this->ColAlignment(), g.Width() );
        }
        this->ResizeTo( A.Width(), A.Height() );
    }

    if( this->ColAlignment() == A.RowAlignment() )
    {
        const int r = g.Height();

        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localHeightOfA = A.LocalHeight();
        const int maxLocalWidth = MaxLocalLength(width,r);

        const int portionSize = 
            max(localHeight*maxLocalWidth,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (r+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* originalData = &buffer[0];
        T* gatheredData = &buffer[portionSize];

        // Pack 
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int jLocal=0; jLocal<localHeightOfA; ++jLocal )
            for( int iLocal=0; iLocal<localHeight; ++iLocal )
                originalData[iLocal+jLocal*localHeight] = 
                    ALocalBuffer[jLocal+iLocal*ALDim];

        // Communicate
        mpi::AllGather
        ( originalData, portionSize,
          gatheredData, portionSize, g.MCComm() );

        // Unpack
        const int colAlignmentOfA = A.ColAlignment();
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &gatheredData[k*portionSize];

            const int rowShift = RawShift( k, colAlignmentOfA, r );
            const int localWidth = RawLocalLength( width, rowShift, r );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*localHeight];
                T* thisCol = &thisLocalBuffer[(rowShift+jLocal*r)*thisLDim];
                memcpy( thisCol, dataCol, localHeight*sizeof(T) );
            }
        }
        this->_auxMemory.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [MR,* ]::TransposeFrom" << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int col = g.MRRank();

        const int colAlignment = this->ColAlignment();
        const int rowAlignmentOfA = A.RowAlignment();
        const int sendCol = (col+c+colAlignment-rowAlignmentOfA) % c;
        const int recvCol = (col+c+rowAlignmentOfA-colAlignment) % c;

        const int height = this->Height();
        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localHeightOfA = A.LocalHeight();
        const int localWidthOfA = A.LocalWidth();
        const int maxLocalHeight = MaxLocalLength(height,c);
        const int maxLocalWidth = MaxLocalLength(width,r);

        const int portionSize = 
            max(maxLocalHeight*maxLocalWidth,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (r+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[portionSize];

        // Pack the currently owned local data of A into the second buffer
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int jLocal=0; jLocal<localHeightOfA; ++jLocal )
            for( int iLocal=0; iLocal<localWidthOfA; ++iLocal )
                secondBuffer[iLocal+jLocal*localWidthOfA] = 
                    ALocalBuffer[jLocal+iLocal*ALDim];

        // Perform the SendRecv: puts the new data into the first buffer
        mpi::SendRecv
        ( secondBuffer, portionSize, sendCol, 0,
          firstBuffer,  portionSize, recvCol, mpi::ANY_TAG, g.MRComm() );

        // Use the output of the SendRecv as input to the AllGather
        mpi::AllGather
        ( firstBuffer,  portionSize,
          secondBuffer, portionSize, g.MCComm() );

        // Unpack the contents of each member of the process col
        const int colAlignmentOfA = A.ColAlignment();
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const int rowShift = RawShift( k, colAlignmentOfA, r );
            const int localWidth = RawLocalLength( width, rowShift, r );
#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*localHeight];
                T* thisCol = &thisLocalBuffer[(rowShift+jLocal*r)*thisLDim];
                memcpy( thisCol, dataCol, localHeight*sizeof(T) );
            }
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,MC,MR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [MC,MR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,VC,STAR> > A_VC_STAR
    ( new DistMatrix<T,VC,STAR>(g) );
    *A_VC_STAR = A;

    auto_ptr< DistMatrix<T,VR,STAR> > A_VR_STAR
    ( new DistMatrix<T,VR,STAR>(true,this->ColAlignment(),g) );
    *A_VR_STAR = *A_VC_STAR;
    delete A_VC_STAR.release(); // lowers memory highwater

    *this = *A_VR_STAR;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,MC,STAR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [MC,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( A.Width() == 1 )
    {
        if( !this->Viewing() )
            this->ResizeTo( A.Height(), 1 );

        const int r = g.Height();
        const int c = g.Width();
        const int p = g.Size();
        const int myCol = g.MRRank();
        const int rankCM = g.VCRank();
        const int rankRM = g.VRRank();
        const int colAlignment = this->ColAlignment();
        const int colShift = this->ColShift();
        const int colAlignmentOfA = A.ColAlignment();
        const int colShiftOfA = A.ColShift();

        const int height = this->Height();
        const int maxLocalVectorHeight = MaxLocalLength(height,p);
        const int portionSize = max(maxLocalVectorHeight,mpi::MIN_COLL_MSG);

        const int colShiftVR = Shift(rankRM,colAlignment,p);
        const int colShiftVCOfA = Shift(rankCM,colAlignmentOfA,p);
        const int sendRankRM = (rankRM+(p+colShiftVCOfA-colShiftVR)) % p;
        const int recvRankCM = (rankCM+(p+colShiftVR-colShiftVCOfA)) % p;
        const int recvRankRM = (recvRankCM/r)+c*(recvRankCM%r);

        this->_auxMemory.Require( (r+1)*portionSize );
        T* buffer = this->_auxMemory.Buffer();
        T* sendBuf = &buffer[0];
        T* recvBuf = &buffer[r*portionSize];

        // A[VC,* ] <- A[MC,* ]
        {
            const int shift = Shift(rankCM,colAlignmentOfA,p);
            const int offset = (shift-colShiftOfA) / r;
            const int thisLocalHeight = LocalLength(height,shift,p);

            const T* ALocalBuffer = A.LockedLocalBuffer();
#ifdef _OPENMP
            #pragma omp parallel for
#endif
            for( int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                sendBuf[iLocal] = ALocalBuffer[offset+iLocal*c];
        }

        // A[VR,* ] <- A[VC,* ]
        mpi::SendRecv
        ( sendBuf, portionSize, sendRankRM, 0,
          recvBuf, portionSize, recvRankRM, mpi::ANY_TAG, g.VRComm() );

        // A[MR,* ] <- A[VR,* ]
        mpi::AllGather
        ( recvBuf, portionSize,
          sendBuf, portionSize, g.MCComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &sendBuf[k*portionSize];

            const int shift = RawShift(myCol+c*k,colAlignment,p);
            const int offset = (shift-colShift) / c;
            const int thisLocalHeight = RawLocalLength(height,shift,p);

            for( int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                thisLocalBuffer[offset+iLocal*r] = data[iLocal];
        }
        this->_auxMemory.Release();
    }
    else
    {
        auto_ptr< DistMatrix<T,VC,STAR> > A_VC_STAR
        ( new DistMatrix<T,VC,STAR>(g) );
        *A_VC_STAR = A;

        auto_ptr< DistMatrix<T,VR,STAR> > A_VR_STAR
        ( new DistMatrix<T,VR,STAR>(true,this->ColAlignment(),g) );
        *A_VR_STAR = *A_VC_STAR;
        delete A_VC_STAR.release(); // lowers memory highwater

        *this = *A_VR_STAR;
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,STAR,MR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [* ,MR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,MC,MR> > A_MC_MR
    ( new DistMatrix<T,MC,MR>(g) );
    *A_MC_MR   = A;

    auto_ptr< DistMatrix<T,VC,STAR> > A_VC_STAR
    ( new DistMatrix<T,VC,STAR>(g) );
    *A_VC_STAR = *A_MC_MR;
    delete A_MC_MR.release(); // lowers memory highwater

    auto_ptr< DistMatrix<T,VR,STAR> > A_VR_STAR
    ( new DistMatrix<T,VR,STAR>(true,this->ColAlignment(),g) );
    *A_VR_STAR = *A_VC_STAR;
    delete A_VC_STAR.release(); // lowers memory highwater

    *this = *A_VR_STAR;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,MD,STAR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ] = [MD,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    throw logic_error( "[MR,* ] = [MD,* ] not yet implemented." );
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,STAR,MD>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [* ,MD]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    throw logic_error( "[MR,* ] = [* ,MD] not yet implemented." );
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,MR,MC>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [MR,MC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->_colAlignment = A.ColAlignment();
            this->_colShift = 
                Shift( g.MRRank(), this->ColAlignment(), g.Width() );
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->ColAlignment() == A.ColAlignment() )
    {
        const int r = g.Height();

        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localWidthOfA = A.LocalWidth();
        const int maxLocalWidth = MaxLocalLength(width,r);

        const int portionSize = 
            max(localHeight*maxLocalWidth,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (r+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* originalData = &buffer[0];
        T* gatheredData = &buffer[portionSize];

        // Pack 
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidthOfA; ++jLocal )
        {
            const T* ACol = &ALocalBuffer[jLocal*ALDim];
            T* originalDataCol = &originalData[jLocal*localHeight];
            memcpy( originalDataCol, ACol, localHeight*sizeof(T) );
        }

        // Communicate
        mpi::AllGather
        ( originalData, portionSize,
          gatheredData, portionSize, g.MCComm() );

        // Unpack
        const int rowAlignmentOfA = A.RowAlignment();
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &gatheredData[k*portionSize];

            const int rowShift = RawShift( k, rowAlignmentOfA, r );
            const int localWidth = RawLocalLength( width, rowShift, r );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*localHeight];
                T* thisCol = &thisLocalBuffer[(rowShift+jLocal*r)*thisLDim];
                memcpy( thisCol, dataCol, localHeight*sizeof(T) );
            }
        }
        this->_auxMemory.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [MR,* ] <- [MR,MC]." << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int col = g.MRRank();

        const int colAlignment = this->ColAlignment();
        const int colAlignmentOfA = A.ColAlignment();
        const int sendCol = (col+c+colAlignment-colAlignmentOfA) % c;
        const int recvCol = (col+c+colAlignmentOfA-colAlignment) % c;

        const int height = this->Height();
        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localHeightOfA = A.LocalHeight();
        const int localWidthOfA = A.LocalWidth();
        const int maxLocalHeight = MaxLocalLength(height,c);
        const int maxLocalWidth = MaxLocalLength(width,r);

        const int portionSize = 
            max(maxLocalHeight*maxLocalWidth,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (r+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[portionSize];

        // Pack the currently owned local data of A into the second buffer
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidthOfA; ++jLocal )
        {
            const T* ACol = &ALocalBuffer[jLocal*ALDim];
            T* secondBufferCol = &secondBuffer[jLocal*localHeightOfA];
            memcpy( secondBufferCol, ACol, localHeightOfA*sizeof(T) );
        }

        // Perform the SendRecv: puts the new data into the first buffer
        mpi::SendRecv
        ( secondBuffer, portionSize, sendCol, 0,
          firstBuffer,  portionSize, recvCol, mpi::ANY_TAG, g.MRComm() );

        // Use the output of the SendRecv as input to the AllGather
        mpi::AllGather
        ( firstBuffer,  portionSize,
          secondBuffer, portionSize, g.MCComm() );

        // Unpack the contents of each member of the process col
        const int rowAlignmentOfA = A.RowAlignment();
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const int rowShift = RawShift( k, rowAlignmentOfA, r );
            const int localWidth = RawLocalLength( width, rowShift, r );
#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*localHeight];
                T* thisCol = &thisLocalBuffer[(rowShift+jLocal*r)*thisLDim];
                memcpy( thisCol, dataCol, localHeight*sizeof(T) );
            }
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,MR,STAR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [MR,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->_colAlignment = A.ColAlignment();
            this->_colShift = A.ColShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->ColAlignment() == A.ColAlignment() )
    {
        this->_localMatrix = A.LockedLocalMatrix();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [MR,* ] <- [MR,* ]." << endl;
#endif
        const int rank = g.MRRank();
        const int c = g.Width();

        const int colAlignment = this->ColAlignment();
        const int colAlignmentOfA = A.ColAlignment();

        const int sendRank = (rank+c+colAlignment-colAlignmentOfA) % c;
        const int recvRank = (rank+c+colAlignmentOfA-colAlignment) % c;

        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localHeightOfA = A.LocalHeight();

        const int sendSize = localHeightOfA * width;
        const int recvSize = localHeight * width;

        this->_auxMemory.Require( sendSize + recvSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* ACol = &ALocalBuffer[j*ALDim];
            T* sendBufferCol = &sendBuffer[j*localHeightOfA];
            memcpy( sendBufferCol, ACol, localHeightOfA*sizeof(T) );
        }

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendRank, 0,
          recvBuffer, recvSize, recvRank, mpi::ANY_TAG, g.MRComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* recvBufferCol = &recvBuffer[j*localHeight];
            T* thisCol = &thisLocalBuffer[j*thisLDim];
            memcpy( thisCol, recvBufferCol, localHeight*sizeof(T) );
        }

        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,STAR,MC>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [* ,MC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,MR,MC> A_MR_MC(g);

    A_MR_MC = A;
    *this   = A_MR_MC;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,VC,STAR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [VC,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,VR,STAR> A_VR_STAR(true,this->ColAlignment(),g);

    A_VR_STAR = A;
    *this     = A_VR_STAR;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,STAR,VC>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [* ,VC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,MR,MC> A_MR_MC(g);

    A_MR_MC = A;
    *this   = A_MR_MC;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,VR,STAR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [VR,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
#ifdef CACHE_WARNINGS
    if( A.Width() != 1 && g.VCRank() == 0 )
    {
        cerr << 
          "[MR,* ] <- [VR,* ] potentially causes a large amount of cache-"
          "thrashing. If possible avoid it by performing the redistribution "
          "with a (conjugate)-transpose: " << endl <<
          "  [* ,MR].(Conjugate)TransposeFrom([VR,* ])" << endl;
    }
#endif
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->_colAlignment = A.ColAlignment() % g.Width();
            this->_colShift = 
                Shift( g.MRRank(), this->ColAlignment(), g.Width() );
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->ColAlignment() == A.ColAlignment() % g.Width() )
    {
        const int r = g.Height();
        const int c = g.Width();
        const int p = r * c;
        const int col = g.MRRank();

        const int height = this->Height();
        const int width = this->Width();
        const int localHeightOfA = A.LocalHeight();
        const int maxLocalHeightOfA = MaxLocalLength(height,p);

        const int portionSize = 
            max(maxLocalHeightOfA*width,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (r+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* originalData = &buffer[0];
        T* gatheredData = &buffer[portionSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* ACol = &ALocalBuffer[j*ALDim];
            T* originalDataCol = &originalData[j*localHeightOfA];
            memcpy( originalDataCol, ACol, localHeightOfA*sizeof(T) );
        }

        // Communicate
        mpi::AllGather
        ( originalData, portionSize,
          gatheredData, portionSize, g.MCComm() );

        // Unpack
        const int colShift = this->ColShift();
        const int colAlignmentOfA = A.ColAlignment();
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &gatheredData[k*portionSize];

            const int colShiftOfA = RawShift( col+c*k, colAlignmentOfA, p );
            const int colOffset = (colShiftOfA-colShift) / c;
            const int localHeight = RawLocalLength( height, colShiftOfA, p );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for COLLAPSE(2)
#endif
            for( int j=0; j<width; ++j )
                for( int iLocal=0; iLocal<localHeight; ++iLocal )
                    thisLocalBuffer[(colOffset+iLocal*r)+j*thisLDim] = 
                        data[iLocal+j*localHeight];
        }
        this->_auxMemory.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [MR,* ] <- [VR,* ]." << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int p = g.Size();
        const int col = g.MRRank();
        const int rank = g.VRRank();

        // Perform the SendRecv to make A have the same colAlignment
        const int colAlignment = this->ColAlignment();
        const int colAlignmentOfA = A.ColAlignment();
        const int colShift = this->ColShift();

        const int sendRank = (rank+p+colAlignment-colAlignmentOfA) % p;
        const int recvRank = (rank+p+colAlignmentOfA-colAlignment) % p;

        const int height = this->Height();
        const int width = this->Width();
        const int localHeightOfA = A.LocalHeight();
        const int maxLocalHeightOfA = MaxLocalLength(height,p);

        const int portionSize = 
            max(maxLocalHeightOfA*width,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (r+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[portionSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* ACol = &ALocalBuffer[j*ALDim];
            T* secondBufferCol = &secondBuffer[j*localHeightOfA];
            memcpy( secondBufferCol, ACol, localHeightOfA*sizeof(T) );
        }

        // Perform the SendRecv: puts the new data into the first buffer
        mpi::SendRecv
        ( secondBuffer, portionSize, sendRank, 0,
          firstBuffer,  portionSize, recvRank, mpi::ANY_TAG, g.VRComm() );

        // Use the SendRecv as input to the AllGather
        mpi::AllGather
        ( firstBuffer,  portionSize,
          secondBuffer, portionSize, g.MCComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const int colShiftOfA = RawShift( col+c*k, colAlignment, p );
            const int colOffset = (colShiftOfA-colShift) / c;
            const int localHeight = RawLocalLength( height, colShiftOfA, p );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for COLLAPSE(2)
#endif
            for( int j=0; j<width; ++j )
                for( int iLocal=0; iLocal<localHeight; ++iLocal )
                    thisLocalBuffer[(colOffset+iLocal*r)+j*thisLDim] = 
                        data[iLocal+j*localHeight];
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,STAR,VR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[MR,* ] = [* ,VR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,STAR,VC> > A_STAR_VC
    ( new DistMatrix<T,STAR,VC>(g) );
    *A_STAR_VC = A;

    auto_ptr< DistMatrix<T,MR,MC> > A_MR_MC
    ( new DistMatrix<T,MR,MC>(true,false,this->ColAlignment(),0,g) );
    *A_MR_MC = *A_STAR_VC;
    delete A_STAR_VC.release(); // lowers memory highwater

    *this = *A_MR_MC;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MR,STAR>&
elemental::DistMatrixBase<T,MR,STAR>::operator=
( const DistMatrixBase<T,STAR,STAR>& A )
{
#ifndef RELEASE
    PushCallStack("[MR,* ] = [* ,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );

    const int c = g.Width();
    const int colShift = this->ColShift();

    const int width = this->Width();
    const int localHeight = this->LocalHeight();

    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
    const T* ALocalBuffer = A.LockedLocalBuffer();
    const int ALDim = A.LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for COLLAPSE(2)
#endif
    for( int j=0; j<width; ++j )
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
            thisLocalBuffer[iLocal+j*thisLDim] = 
                ALocalBuffer[(colShift+iLocal*c)+j*ALDim];
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template class elemental::DistMatrixBase<int,   MR,STAR>;
template class elemental::DistMatrixBase<float, MR,STAR>;
template class elemental::DistMatrixBase<double,MR,STAR>;
#ifndef WITHOUT_COMPLEX
template class elemental::DistMatrixBase<scomplex,MR,STAR>;
template class elemental::DistMatrixBase<dcomplex,MR,STAR>;
#endif
