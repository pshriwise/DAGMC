
#include "meshkit/MKCore.hpp"
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
  MEntVector curves; // handle for the curve we need to retrieve, is a vector
  CurveMesher * cm;   // handle for our MeshOp that we will create

  mk = new MKCore();
  mk->load_geometry("cyl.sat");

  mk->get_entities_by_dimension(1, curves);
  cm = (CurveMesher*) mk->construct_meshop("CurveMesher", curves);

  mk->setup();
  mk->execute();

  mk->save_mesh("cyl.h5m");

}

