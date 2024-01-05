#include <math.h>
#include <stdlib.h>

#include <iostream>
#include <limits>
#include <vector>

#include "DagMC.hpp"
#include "MBTagConventions.hpp"
#include "moab/Core.hpp"
#include "moab/Interface.hpp"

#define CHKERR \
  if (MB_SUCCESS != rval) return rval

using namespace moab;

int main(int argc, char* argv[]) {
  ErrorCode rval;

  if (argc != 5 && argc != 8) {
    std::cerr << "Usage: " << argv[0] << " <mesh_filename> "
              << "<xxx> <yyy> <zzz> [<uuu> <vvv> <www>]" << std::endl;
    return 1;
  }

  char* filename = argv[1];
  double xxx = atof(argv[2]);
  double yyy = atof(argv[3]);
  double zzz = atof(argv[4]);
  double uuu = 0;
  double vvv = 0;
  double www = 0;

  if (argc > 5) {
    uuu = atof(argv[5]);
    vvv = atof(argv[6]);
    www = atof(argv[7]);
  }

  std::cout << "Searching for volume containing:" << std::endl
            << "(x,y,z) = (" << xxx << "," << yyy << "," << zzz << ")"
            << std::endl
            << " of geometry " << filename
            << std::endl;

  DagMC dagmc{};
  rval = dagmc.load_file(filename);
  if (MB_SUCCESS != rval) {
    std::cerr << "Failed to load file." << std::endl;
    return 2;
  }
  rval = dagmc.init_OBBTree();
  if (MB_SUCCESS != rval) {
    std::cerr << "Failed to initialize DagMC." << std::endl;
    return 2;
  }

  double xyz[3] = {xxx, yyy, zzz};
  double uvw[3] = {uuu, vvv, www};
  int inside {0};

  int volume {-1};
  for (int vol = 1; vol <= dagmc.num_entities(3); ++vol) {
    EntityHandle volHandle = dagmc.entity_by_index(3, vol);
    rval = dagmc.point_in_volume(volHandle, xyz, inside);
    if (inside != 0) {
      volume = vol;
      break;
    }
  }

  if (volume >= 0) {
    int volume_id = dagmc.id_by_index(3, volume);
    std::cout << "Point is inside volume " << volume_id << std::endl;
  } else {
    std::cout << "Point is outside all volumes" << std::endl;
  }

}
