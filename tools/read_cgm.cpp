
#include "meshkit/MKCore.hpp"
#include "meshkit/SolidSurfaceMesher.hpp"
#include "meshkit/CurveMesher.hpp"
#include "meshkit/SizingFunction.hpp"
#include "meshkit/ModelEnt.hpp"
#include "meshkit/Matrix.hpp"


using namespace MeshKit;

const bool save_mesh = true;

#ifdef HAVE_ACIS
std::string extension = ".sat";
#elif HAVE_OCC
std::string extension = ".stp";
#endif


int main(int argc, char **argv) 
{
  MKCore * mk;       // handle for the instance of MeshKit
  MEntVector curves, surfs; // handle for the curve we need to retrieve, is a vector
  SolidSurfaceMesher * ssm;   // handle for our MeshOp that we will create
  CurveMesher *cm;

  mk = new MKCore();
  mk->load_geometry("cyl.sat");

  mk->get_entities_by_dimension(1, curves);
  cm = (CurveMesher*) mk->construct_meshop("CurveMesher", curves);
  cm ->set_mesh_params(1e-3);
  mk->get_entities_by_dimension(2, surfs);
  ssm = (SolidSurfaceMesher*) mk->construct_meshop("SolidSurfaceMesher", surfs);
  ssm ->set_mesh_params(1e-3);

  mk->setup();
  mk->execute();

  mk->save_mesh("cyl.h5m");

}

