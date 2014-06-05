cmake_minimum_required(VERSION 2.8)

#project name
project(dagmc_preproc)


#set itaps paths
#iGeom
set(IGEOM_INC ${CGM_DIR}/include)
set(IGEOM_LIB ${CGM_DIR}/lib)
#iMesh/moab
set(MOAB_INC ${MOAB_DIR}/include)
set(MOAB_LIB ${MOAB_DIR}/lib)
#iRel
set(IREL_INC ${LASSO_DIR}/include)
set(IREL_LIB ${LASSO_DIR}/lib)

#set meshkit lib paths
set(MESHKIT_INC ${MK_DIR}/include)
set(MESHKIT_LIB ${MK_DIR}/lib)

#set include flags
include_directories(${IGEOM_INC} ${MOAB_INC} ${IREL_INC} ${MESHKIT_INC})

#set lib flags
set(LIBRARIES ${IGEOM_LIB}/libiGeom.so ${MOAB_LIB}/libiMesh.so ${IREL_LIB}/libiRel.so ${MESHKIT_LIB}/libMeshKit.so ${MOAB_LIB}/libMOAB.so)

set(CMAKE_INSTALL_PREFIX .)
add_executable(dagmc_preproc dagmc_preproc.cpp)
target_link_libraries(dagmc_preproc ${LIBRARIES})

install(TARGETS dagmc_preproc DESTINATION .)

