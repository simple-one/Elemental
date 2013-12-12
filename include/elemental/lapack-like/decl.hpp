/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef ELEM_LAPACK_DECL_HPP
#define ELEM_LAPACK_DECL_HPP

namespace elem {

namespace hermitian_gen_definite_eig_type_wrapper {
enum HermitianGenDefiniteEigType
{
    AXBX=1,
    ABX=2,
    BAX=3
};
}
using namespace hermitian_gen_definite_eig_type_wrapper;

namespace hermitian_tridiag_approach_wrapper {
enum HermitianTridiagApproach
{
    HERMITIAN_TRIDIAG_NORMAL, // Keep the current grid
    HERMITIAN_TRIDIAG_SQUARE, // Drop to a square process grid
    HERMITIAN_TRIDIAG_DEFAULT // Square grid algorithm only if already square
};
}
using namespace hermitian_tridiag_approach_wrapper;

namespace ldl_pivot_type_wrapper {
enum LDLPivotType
{
    BUNCH_KAUFMAN_A=0,
    BUNCH_KAUFMAN_C=1,
    BUNCH_KAUFMAN_D=2,
    BUNCH_KAUFMAN_BOUNDED=3,
    BUNCH_PARLETT=4
};
}
using namespace ldl_pivot_type_wrapper;

struct LDLPivot
{
    Int nb;
    Int from[2];
};

// Throws an error if PMRRR was not built along with Elemental
inline void EnsurePMRRR()
{
#ifndef HAVE_PMRRR
    RuntimeError("PMRRR is required for this routine");
#endif
}

// Compute the eigenvalues of a Hermitian matrix
template<typename F>
void HermitianEig
( UpperOrLower uplo, Matrix<F>& A, Matrix<BASE(F)>& w, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, DistMatrix<F,STAR,STAR>& A, 
  DistMatrix<BASE(F),STAR,STAR>& w, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, DistMatrix<F>& A, DistMatrix<BASE(F),VR,STAR>& w, 
  SortType sort=UNSORTED );

// Compute the full eigenvalue decomposition of a Hermitian matrix
template<typename F>
void HermitianEig
( UpperOrLower uplo, Matrix<F>& A, Matrix<BASE(F)>& w, Matrix<F>& Z, 
  SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, DistMatrix<F,STAR,STAR>& A, 
  DistMatrix<BASE(F),STAR,STAR>& w, DistMatrix<F,STAR,STAR>& Z, 
  SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, 
  DistMatrix<F>& A, DistMatrix<BASE(F),VR,STAR>& w, DistMatrix<F>& paddedZ,
  SortType sort=UNSORTED );

// Compute the eigenvalues of a Hermitian matrix within a selected range
template<typename F>
void HermitianEig
( UpperOrLower uplo, Matrix<F>& A, Matrix<BASE(F)>& w,
  Int lowerBound, Int upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, Matrix<F>& A, Matrix<BASE(F)>& w,
  BASE(F) lowerBound, BASE(F) upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, 
  DistMatrix<F,STAR,STAR>& A, 
  DistMatrix<BASE(F),STAR,STAR>& w,
  Int lowerBound, Int upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, 
  DistMatrix<F,STAR,STAR>& A, 
  DistMatrix<BASE(F),STAR,STAR>& w,
  BASE(F) lowerBound, BASE(F) upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, DistMatrix<F>& A, DistMatrix<BASE(F),VR,STAR>& w, 
  Int lowerBound, Int upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, DistMatrix<F>& A, DistMatrix<BASE(F),VR,STAR>& w,
  BASE(F) lowerBound, BASE(F) upperBound, SortType sort=UNSORTED );

// Compute a selected set of eigenpairs of a Hermitian matrix
template<typename F>
void HermitianEig
( UpperOrLower uplo, 
  Matrix<F>& A, Matrix<BASE(F)>& w, Matrix<F>& Z,
  Int lowerBound, Int upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, 
  Matrix<F>& A, Matrix<BASE(F)>& w, Matrix<F>& Z,
  BASE(F) lowerBound, BASE(F) upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo,
  DistMatrix<F,STAR,STAR>& A, 
  DistMatrix<BASE(F),STAR,STAR>& w, 
  DistMatrix<F,STAR,STAR>& Z,
  Int lowerBound, Int upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo,
  DistMatrix<F,STAR,STAR>& A, 
  DistMatrix<BASE(F),STAR,STAR>& w, 
  DistMatrix<F,STAR,STAR>& Z,
  BASE(F) lowerBound, BASE(F) upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, 
  DistMatrix<F>& A, DistMatrix<BASE(F),VR,STAR>& w, DistMatrix<F>& paddedZ,
  Int lowerBound, Int upperBound, SortType sort=UNSORTED );
template<typename F>
void HermitianEig
( UpperOrLower uplo, 
  DistMatrix<F>& A, DistMatrix<BASE(F),VR,STAR>& w, DistMatrix<F>& paddedZ,
  BASE(F) lowerBound, BASE(F) upperBound, SortType sort=UNSORTED );

template<typename F>
void HermitianTridiag( UpperOrLower uplo, Matrix<F>& A );
template<typename F>
void HermitianTridiag( UpperOrLower uplo, DistMatrix<F>& A );

template<typename F>
void HermitianTridiag( UpperOrLower uplo, Matrix<F>& A, Matrix<F>& t );
template<typename F>
void HermitianTridiag
( UpperOrLower uplo, DistMatrix<F>& A, DistMatrix<F,STAR,STAR>& t );

void SetHermitianTridiagApproach( HermitianTridiagApproach approach );
HermitianTridiagApproach GetHermitianTridiagApproach();

// If dropping down to a square grid, the two simplest approaches are to take 
// the first r^2 processes from the original grid (for an r x r grid) and to
// either order them column-major or row-major to form the square grid.
void SetHermitianTridiagGridOrder( GridOrder order );
GridOrder GetHermitianTridiagGridOrder();

} // namespace elem

#endif // ifndef ELEM_LAPACK_DECL_HPP