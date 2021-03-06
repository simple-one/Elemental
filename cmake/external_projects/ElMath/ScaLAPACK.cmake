#
#  Copyright 2009-2015, Jack Poulson
#  All rights reserved.
#
#  This file is part of Elemental and is under the BSD 2-Clause License,
#  which can be found in the LICENSE file in the root directory, or at
#  http://opensource.org/licenses/BSD-2-Clause
#
include(ExternalProject)
include(ElCheckFunctionExists)
include(ElLibraryName)

# If MATH_LIBS_AT_CONFIG exists, see if it supports ScaLAPACK
if(MATH_LIBS_AT_CONFIG)
  set(CMAKE_REQUIRED_FLAGS "${MPI_C_COMPILE_FLAGS}")
  set(CMAKE_REQUIRED_LINKER_FLAGS "${MPI_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
  set(CMAKE_REQUIRED_INCLUDES ${MPI_C_INCLUDE_PATH})
  set(CMAKE_REQUIRED_LIBRARIES ${MATH_LIBS_AT_CONFIG} ${MPI_C_LIBRARIES})
  El_check_function_exists(dtrsm   EL_HAVE_DTRSM)
  El_check_function_exists(dtrsm_  EL_HAVE_DTRSM_POST)
  if(EL_HAVE_DTRSM OR EL_HAVE_DTRSM_POST)
    set(HAVE_BLAS TRUE)
  else()
    message(WARNING "Could not find BLAS in ${MATH_LIBS_AT_CONFIG}")
  endif()
  El_check_function_exists(dsytrd  EL_HAVE_DSYTRD)
  El_check_function_exists(dsytrd_ EL_HAVE_DSYTRD_POST)
  if(EL_HAVE_DSYTRD OR EL_HAVE_DSYTRD_POST)
    set(HAVE_LAPACK TRUE)
  else()
    message(WARNING "Could not find LAPACK in ${MATH_LIBS_AT_CONFIG}")
  endif()
  if(EL_SCALAPACK_SUFFIX)
    El_check_function_exists(pdsyngst${EL_SCALAPACK_SUFFIX} EL_HAVE_PDSYNGST)
    El_check_function_exists(Csys2blacs_handle EL_HAVE_CSYS2BLACS)
    if(NOT EL_HAVE_PDSYNGST OR NOT EL_HAVE_CSYS2BLACS)
      message(STATUS "Did not detect ScaLAPACK")
    elseif(HAVE_BLAS AND HAVE_LAPACK)
      set(MATH_LIBS_HAS_SCALAPACK TRUE)
      set(EL_HAVE_SCALAPACK_SUFFIX TRUE)
    endif()
  else()
    # NOTE: pdsyngst was chosen because MKL ScaLAPACK only defines pdsyngst_,
    #       but not pdsyngst, despite defining both pdpotrf and pdpotrf_. 
    El_check_function_exists(pdsyngst  EL_HAVE_PDSYNGST)
    El_check_function_exists(pdsyngst_ EL_HAVE_PDSYNGST_POST)
    El_check_function_exists(Csys2blacs_handle EL_HAVE_CSYS2BLACS)
    if(HAVE_BLAS AND HAVE_LAPACK AND EL_HAVE_PDSYNGST AND EL_HAVE_CSYS2BLACS)
      set(EL_SCALAPACK_SUFFIX)
      set(MATH_LIBS_HAS_SCALAPACK TRUE)
    elseif(HAVE_BLAS AND HAVE_LAPACK AND 
           EL_HAVE_PDSYNGST_POST AND EL_HAVE_CSYS2BLACS)
      set(EL_SCALAPACK_SUFFIX _)
      set(EL_HAVE_SCALAPACK_SUFFIX TRUE)
      set(MATH_LIBS_HAS_SCALAPACK TRUE)
    else()
      message(STATUS "Did not detect ScaLAPACK")
    endif()
  endif()
  if(MATH_LIBS_HAS_SCALAPACK)
    El_check_function_exists(pdlaqr0${EL_SCALAPACK_SUFFIX} EL_HAVE_PDLAQR0)
    El_check_function_exists(pdlaqr1${EL_SCALAPACK_SUFFIX} EL_HAVE_PDLAQR1)
    if(NOT EL_HAVE_PDLAQR0 OR NOT EL_HAVE_PDLAQR1)
      message(STATUS "MATH_LIBS_AT_CONFIG=${MATH_LIBS_AT_CONFIG} supports some of ScaLAPACK, but not PDLAQR{0,1}.")
      set(MATH_LIBS_HAS_SCALAPACK FALSE)
    endif()
  endif()
  set(CMAKE_REQUIRED_FLAGS)
  set(CMAKE_REQUIRED_LINKER_FLAGS)
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
endif()

if(NOT MATH_LIBS_HAS_SCALAPACK AND NOT EL_BUILD_SCALAPACK)
  find_library(ScaLAPACK NAMES scalapack PATHS ${MATH_PATHS})
  if(ScaLAPACK)
    get_filename_component(ScaLAPACK_DIR ScaLAPACK DIRECTORY)
    if(EXISTS "${ScaLAPACK_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}blacs${CMAKE_SHARED_LIBRARY_SUFFIX}" OR 
       EXISTS "${ScaLAPACK_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}blacs${CMAKE_STATIC_LIBRARY_SUFFIX}")
      message(WARNING "Found blacs library in ${ScaLAPACK_DIR}; ScaLAPACK appears to be too old to link to")
    else()
      set(CMAKE_REQUIRED_FLAGS "${MPI_C_COMPILE_FLAGS}")
      set(CMAKE_REQUIRED_LINKER_FLAGS "${MPI_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
      set(CMAKE_REQUIRED_INCLUDES ${MPI_C_INCLUDE_PATH})
      set(CMAKE_REQUIRED_LIBRARIES 
        ${ScaLAPACK} ${MATH_LIBS_AT_CONFIG} ${MPI_C_LIBRARIES})
      El_check_function_exists(dtrsm   EL_HAVE_DTRSM)
      El_check_function_exists(dtrsm_  EL_HAVE_DTRSM_POST)
      if(EL_HAVE_DTRSM OR EL_HAVE_DTRSM_POST)
        set(HAVE_BLAS TRUE)
      else()
        message(STATUS "Cannot link BLAS with found ScaLAPACK and MATH_LIBS")
      endif()
      El_check_function_exists(dsytrd  EL_HAVE_DSYTRD)
      El_check_function_exists(dsytrd_ EL_HAVE_DSYTRD_POST)
      if(EL_HAVE_DSYTRD OR EL_HAVE_DSYTRD_POST)
        set(HAVE_LAPACK TRUE)
      else()
        message(STATUS "Cannot link LAPACK with found ScaLAPACK and MATH_LIBS")
      endif()
      if(EL_SCALAPACK_SUFFIX)
        El_check_function_exists(pdsyngst${EL_SCALAPACK_SUFFIX} EL_HAVE_PDSYNGST)
        El_check_function_exists(Csys2blacs_handle EL_HAVE_CSYS2BLACS)
        if(NOT EL_HAVE_PDSYNGST OR NOT EL_HAVE_CSYS2BLACS)
          message(WARNING "Could not find pdsyngst${EL_SCALAPACK_SUFFIX} and Csys2blacs_handle")
        elseif(HAVE_BLAS AND HAVE_LAPACK)
          set(USE_FOUND_SCALAPACK TRUE)
          set(EL_HAVE_SCALAPACK_SUFFIX TRUE)
        endif()
      else()
        # pdsyngst was chosen because MKL ScaLAPACK only defines pdsyngst_,
        # but not pdsyngst, despite defining both pdpotrf and pdpotrf_. 
        El_check_function_exists(pdsyngst  EL_HAVE_PDSYNGST)
        El_check_function_exists(pdsyngst_ EL_HAVE_PDSYNGST_POST)
        El_check_function_exists(Csys2blacs_handle EL_HAVE_CSYS2BLACS)
        if(HAVE_BLAS AND HAVE_LAPACK AND 
           EL_HAVE_PDSYNGST AND EL_HAVE_CSYS2BLACS)
          set(EL_SCALAPACK_SUFFIX)
          set(USE_FOUND_SCALAPACK TRUE)
        elseif(HAVE_BLAS AND HAVE_LAPACK AND 
               EL_HAVE_PDSYNGST_POST AND EL_HAVE_CSYS2BLACS)
          set(EL_SCALAPACK_SUFFIX _)
          set(EL_HAVE_SCALAPACK_SUFFIX TRUE)
          set(USE_FOUND_SCALAPACK TRUE)
        else()
          message(STATUS "Could not link ScaLAPACK")
        endif()
      endif()
      if(USE_FOUND_SCALAPACK)
        El_check_function_exists(pdlaqr0${EL_SCALAPACK_SUFFIX} EL_HAVE_PDLAQR0)
        El_check_function_exists(pdlaqr1${EL_SCALAPACK_SUFFIX} EL_HAVE_PDLAQR1)
        if(NOT EL_HAVE_PDLAQR0 OR NOT EL_HAVE_PDLAQR1)
          message(STATUS "${ScaLAPACK} supports some of ScaLAPACK, but not PDLAQR{0,1}.")
          set(USE_FOUND_SCALAPACK FALSE)
        endif()
      endif()
      set(CMAKE_REQUIRED_FLAGS)
      set(CMAKE_REQUIRED_LINKER_FLAGS)
      set(CMAKE_REQUIRED_INCLUDES)
      set(CMAKE_REQUIRED_LIBRARIES)
    endif()
  endif()
endif()

if(MATH_LIBS_HAS_SCALAPACK)
  set(SCALAPACK_LIBS ${MATH_LIBS_AT_CONFIG})
  set(SCALAPACK_LIBS_AT_CONFIG ${MATH_LIBS_AT_CONFIG})
  set(EL_BUILT_SCALAPACK FALSE)
  set(EL_HAVE_SCALAPACK TRUE)
elseif(USE_FOUND_SCALAPACK)
  set(SCALAPACK_LIBS ${ScaLAPACK} ${MATH_LIBS_AT_CONFIG})
  set(SCALAPACK_LIBS_AT_CONFIG ${ScaLAPACK} ${MATH_LIBS_AT_CONFIG})
  set(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${SCALAPACK_LIBS})
  set(EL_HAVE_SCALAPACK TRUE)
elseif(EL_HAVE_F90_INTERFACE AND EL_HAVE_MPI_FORTRAN)
  if(NOT DEFINED SCALAPACK_URL)
    set(SCALAPACK_URL https://github.com/poulson/scalapack.git)
  endif()
  message(STATUS "Will pull ScaLAPACK from ${SCALAPACK_URL}")

  set(SCALAPACK_SOURCE_DIR ${PROJECT_BINARY_DIR}/download/scalapack/source)
  set(SCALAPACK_BINARY_DIR ${PROJECT_BINARY_DIR}/download/scalapack/build)

  if(MATH_LIBS_AT_CONFIG)
    set(LAPACK_COMMAND -D LAPACK_LIBRARIES=${MATH_LIBS_AT_CONFIG})
  elseif(APPLE)
    if(EL_PREFER_BLIS_LAPACK)
      set(LAPACK_COMMAND -D BUILD_BLIS_LAPACK=ON)
    elseif(EL_PREFER_OPENBLAS)
      set(LAPACK_COMMAND -D BUILD_OPENBLAS=ON)
    else()
      set(LAPACK_COMMAND -D FORCE_APPLE_MATH=ON)
    endif()
  else()
    if(EL_PREFER_BLIS_LAPACK)
      set(LAPACK_COMMAND -D BUILD_BLIS_LAPACK=ON)
    else()
      set(LAPACK_COMMAND -D BUILD_OPENBLAS=ON)
    endif()
  endif()

  # Convert various MPI lists (delimted with ';') to use a '^^' delimiter
  # (following the advice from
  # http://www.kitware.com/media/html/BuildingExternalProjectsWithCMake2.8.html)
  # due to running into build problems within ScaLAPACK's linking of OpenMPI 
  # Fortran libraries.
  string(REPLACE ";" "^^" MPI_C_INCSTRING "${MPI_C_INCLUDE_PATH}")
  string(REPLACE ";" "^^" MPI_Fortran_INCSTRING "${MPI_Fortran_INCLUDE_PATH}")
  string(REPLACE ";" "^^" MPI_C_LIBSTRING "${MPI_C_LIBS}")
  string(REPLACE ";" "^^" MPI_Fortran_LIBSTRING "${MPI_Fortran_LIBS}")
  ExternalProject_Add(project_scalapack
    PREFIX ${CMAKE_INSTALL_PREFIX}
    GIT_REPOSITORY ${SCALAPACK_URL}
    STAMP_DIR  ${SCALAPACK_BINARY_DIR}/stamp
    SOURCE_DIR ${SCALAPACK_SOURCE_DIR}
    BINARY_DIR ${SCALAPACK_BINARY_DIR}
    TMP_DIR    ${SCALAPACK_BINARY_DIR}/tmp
    UPDATE_COMMAND ""
    LIST_SEPARATOR ^^
    CMAKE_ARGS 
      -D CMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -D CMAKE_Fortran_COMPILER=${CMAKE_Fortran_COMPILER}
      -D CMAKE_C_FLAGS=${CMAKE_C_FLAGS}
      -D CMAKE_Fortran_FLAGS=${CMAKE_Fortran_FLAGS}
      -D MPI_C_INCLUDE_PATH:STRING=${MPI_C_INCSTRING}
      -D MPI_Fortran_INCLUDE_PATH:STRING=${MPI_Fortran_INCSTRING}
      -D MPI_C_COMPILE_FLAGS=${MPI_C_COMPILE_FLAGS}
      -D MPI_Fortran_COMPILE_FLAGS=${MPI_Fortran_COMPILE_FLAGS}
      -D MPI_C_LIBRARIES:STRING=${MPI_C_LIBSTRING}
      -D MPI_Fortran_LIBRARIES:STRING=${MPI_Fortran_LIBSTRING}
      -D MPI_LINK_FLAGS=${MPI_LINK_FLAGS}
      ${LAPACK_COMMAND}
      -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
      -D BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
      -D CMAKE_MACOSX_RPATH=${CMAKE_MACOSX_RPATH}
      -D CMAKE_SKIP_BUILD_RPATH=${CMAKE_SKIP_BUILD_RPATH}
      -D CMAKE_BUILD_WITH_INSTALL_RPATH=${CMAKE_BUILD_WITH_INSTALL_RPATH}
      -D CMAKE_INSTALL_RPATH_USE_LINK_PATH=${CMAKE_INSTALL_RPATH_USE_LINK_PATH} 
      -D CMAKE_INSTALL_RPATH=${CMAKE_INSTALL_RPATH}
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  )

  # Extract the source and install directories
  ExternalProject_Get_Property(project_scalapack source_dir install_dir)

  # Add targets for libscalapack (either shared or static)
  add_library(libscalapack ${LIBRARY_TYPE} IMPORTED)
  El_library_name(scalapack_name scalapack)
  set(SCALAPACK_LIB ${install_dir}/lib/${scalapack_name})
  set_property(TARGET libscalapack PROPERTY IMPORTED_LOCATION ${SCALAPACK_LIB})
  if(WIN32)
    add_library(libscalapack-F ${LIBRARY_TYPE} IMPORTED)
    El_library_name(scalapack-F_name scalapack-F)
    set(SCALAPACKF_LIB ${install_dir}/lib${scalapack_name})
    set_property(TARGET libscalapack-F PROPERTY 
      IMPORTED_LOCATION ${SCALAPACKF_LIB})
  endif()

  if(MATH_LIBS_AT_CONFIG)
    if(WIN32)
      set(SCALAPACK_LIBS 
        ${SCALAPACK_LIB} ${SCALAPACKF_LIB} ${MATH_LIBS_AT_CONFIG})
    else()
      set(SCALAPACK_LIBS ${SCALAPACK_LIB} ${MATH_LIBS_AT_CONFIG})
    endif()
    set(SCALAPACK_LIBS_AT_CONFIG ${MATH_LIBS_AT_CONFIG})
  elseif(EL_PREFER_BLIS_LAPACK)
    add_library(liblapack ${LIBRARY_TYPE} IMPORTED)
    El_library_name(lapack_name lapack)
    set(LAPACK_LIB ${install_dir}/lib/${lapack_name})
    set_property(TARGET liblapack PROPERTY IMPORTED_LOCATION ${LAPACK_LIB})

    add_library(libblis ${LIBRARY_TYPE} IMPORTED)
    El_library_name(blis_name blis)
    set(BLIS_LIB ${install_dir}/lib/${blis_name})
    set_property(TARGET libblis PROPERTY IMPORTED_LOCATION ${BLIS_LIB})

    set(EL_BUILT_BLIS_LAPACK TRUE)
    if(WIN32)
      set(SCALAPACK_LIBS ${SCALAPACK_LIB} ${SCALAPACKF_LIB} 
        ${LAPACK_LIB} ${BLIS_LIB})
    else()
      set(SCALAPACK_LIBS ${SCALAPACK_LIB} ${LAPACK_LIB} ${BLIS_LIB})
    endif()
  else()
    add_library(libopenblas ${LIBRARY_TYPE} IMPORTED)
    El_library_name(openblas_name openblas)
    set(OPENBLAS_LIB ${install_dir}/lib/${openblas_name})
    set_property(TARGET libopenblas PROPERTY IMPORTED_LOCATION ${OPENBLAS_LIB})

    set(EL_BUILT_OPENBLAS TRUE)
    if(WIN32)
      set(SCALAPACK_LIBS ${SCALAPACK_LIB} ${SCALAPACKF_LIB} ${OPENBLAS_LIB})
    else()
      set(SCALAPACK_LIBS ${SCALAPACK_LIB} ${OPENBLAS_LIB})
    endif()
  endif()

  set(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${SCALAPACK_LIBS})
  set(EL_BUILT_SCALAPACK TRUE)
  set(EL_HAVE_SCALAPACK TRUE)
else()
  set(EL_BUILT_SCALAPACK FALSE)
  set(EL_HAVE_SCALAPACK FALSE)
endif()
