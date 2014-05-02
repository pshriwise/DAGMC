cmake_minimum_required(VERSION 2.8)

#project name
project(read_cgm)


#set itaps paths
#iGeom
set(IGEOM_INC ${CGM_DIR}/include)
set(IGEOM_LIB ${CGM_DIR}/lib)
#iMesh
set(IMESH_INC ${MOAB_DIR}/include)
set(IMESH_LIB ${MOAB_DIR}/lib)
#iRel
set(IREL_INC ${LASSO_DIR}/include)
set(IREL_LIB ${LASSO_DIR}/lib)

#set meshkit lib paths
set(MESHKIT_INC ${MK_DIR}/include)
set(MESHKIT_LIB ${MK_DIR}/lib)

#set include flags
include_directories(${IGEOM_INC} ${IMESH_INC} ${IREL_INC} ${MESHKIT_INC})

#set lib flags
set(LIBRARIES ${IGEOM_LIB}/libiGeom.so ${IMESH_LIB}/libiMesh.so ${IREL_LIB}/libiRel.so ${MESHKIT_LIB}/libMeshKit.so)

set(CMAKE_INSTALL_PREFIX .)
add_executable(read_cgm fresh_start.cpp)
target_link_libraries(read_cgm ${LIBRARIES})

install(TARGETS read_cgm DESTINATION .)

