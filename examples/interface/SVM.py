#
#  Copyright (c) 2009-2015, Jack Poulson
#  All rights reserved.
#
#  This file is part of Elemental and is under the BSD 2-Clause License, 
#  which can be found in the LICENSE file in the root directory, or at 
#  http://opensource.org/licenses/BSD-2-Clause
#
import El, time

# TODO: Add noise so that the soft margin is actually put to use.
#       Perhaps a weighted coin should be flipped based upon the distance to the
#       hyperplane.

n0 = 50
n1 = 50
numLambdas = 4
startLambda = 1
endLambda = 10
display = True
worldRank = El.mpi.WorldRank()
worldSize = El.mpi.WorldSize()

# Stack two 2D finite-difference matrices on top of each other
# and make the last column dense
def StackedFD2D(N0,N1):
  A = El.DistSparseMatrix()
  height = 2*N0*N1
  width = N0*N1
  A.Resize(height,width)
  localHeight = A.LocalHeight()
  A.Reserve(6*localHeight)
  for sLoc in xrange(localHeight):
    s = A.GlobalRow(sLoc)
    if s < N0*N1:
      x0 = s % N0
      x1 = s / N0
      A.QueueUpdate( sLoc, s, 11 )
      if x0 > 0:
        A.QueueUpdate( sLoc, s-1, -10 )
      if x0+1 < N0:
        A.QueueUpdate( sLoc, s+1, 20 )
      if x1 > 0:
        A.QueueUpdate( sLoc, s-N0, -30 )
      if x1+1 < N1:
        A.QueueUpdate( sLoc, s+N0, 40 )
    else:
      sRel = s-N0*N1
      x0 = sRel % N0
      x1 = sRel / N0
      A.QueueUpdate( sLoc, sRel, -20 )
      if x0 > 0:
        A.QueueUpdate( sLoc, sRel-1, -1 )
      if x0+1 < N0:
        A.QueueUpdate( sLoc, sRel+1, -2 )
      if x1 > 0:
        A.QueueUpdate( sLoc, sRel-N0, -3 )
      if x1+1 < N1:
        A.QueueUpdate( sLoc, sRel+N0, 3 )

    # The dense last column
    A.QueueUpdate( sLoc, width-1, -10/height );

  A.ProcessQueues()
  return A

# Define a random (affine) hyperplane
wGen = El.DistMultiVec()
El.Gaussian( wGen, n0*n1, 1 )
wGenNorm = El.FrobeniusNorm( wGen )
El.Scale( 1./wGenNorm, wGen )
# TODO: Add support for mpi::Broadcast and randomly generate this
offset = 0.3147

A = StackedFD2D(n0,n1)

# Label the points based upon their location relative to the hyperplane
d = El.DistMultiVec()
El.Ones( d, 2*n0*n1, 1 )
El.SparseMultiply( El.NORMAL, 1., A, wGen, -offset, d )
El.EntrywiseMap( d, lambda alpha : 1. if alpha > 0 else -1. )

if display:
  El.Display( wGen, "wGen" )
  if worldRank == 0:
    print "offset =", offset
  El.Display( A, "A" )
  El.Display( d, "d" )

ctrl = El.SVMCtrl_d()
ctrl.ipmCtrl.mehrotraCtrl.progress = True

for j in xrange(0,numLambdas):
  lambd = startLambda + j*(endLambda-startLambda)/(numLambdas-1.)
  if worldRank == 0:
    print "lambda =", lambd

  # TODO: Explicitly return w, beta, and z
  startSVM = time.clock()
  x = El.SVM( A, d, lambd, ctrl )
  endSVM = time.clock()
  if worldRank == 0:
    print "SVM time: ", endSVM-startSVM

  if display:
    El.Display( x, "[w;beta;z]" )

# Require the user to press a button before the figures are closed
El.Finalize()
if worldSize == 1:
  raw_input('Press Enter to exit')
