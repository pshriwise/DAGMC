#include "DagMC.hpp"
#include "moab/OrientedBoxTreeTool.hpp"
#include "moab/Core.hpp"

#ifndef THREAD_MANAGER_HPP
#define THREAD_MANAGER_HPP 1

namespace DagMC {

// Struct for storage of ray state
class DagMCRayState {
 public:
  // set some default values via constructor
  DagMCRayState() {
    last_uvw[0] = 0.;
    last_uvw[1] = 0.;
    last_uvw[2] = 0.;
    visited_surface = false;
    use_dist_limit = false;
    double dist_limit = 0.0;
    last_nps = 0;
  }
  ~DagMCRayState() {};

 public:
  double last_uvw[3];
  moab::DagMC::RayHistory history;
  std::vector<moab::DagMC::RayHistory> history_bank;
  std::vector<moab::DagMC::RayHistory> pblcm_history_stack;
  bool visited_surface;
  bool use_dist_limit;
  double dist_limit;
  int last_nps;

};

class DagThreadManager {
 public:
  DagThreadManager(moab::Interface* MBI = NULL, int num_threads = 1, 
                   const double overlap_tolerance = 0.,
                   const double p_numerical_precision = 1.e-3);

  DagThreadManager(moab::GeomTopoTool *gtt, int num_threads = 1,
                   const double overlap_tolerance = 0., 
                   const double p_numerical_precision = 1.e-3);

  ~DagThreadManager();

  // setup the dagmc state for the threads
  void setup_child_threads();

  void initialise_child_threads();

  void set_num_threads(const int thread_count);

  moab::ErrorCode load_file(const char* cfile);

  moab::ErrorCode init_OBBTree();

  moab::ErrorCode ray_fire(const moab::EntityHandle volume,
					       const double ray_start[3],
					       const double ray_dir[3],
					       moab::EntityHandle &next_surf,
					       double &next_surf_dist,
					       const int thread_idx = 0,
					       bool use_ray_history = false,
					       double dist_limit = 0.,
					       int ray_orientation = 1,
					       moab::OrientedBoxTreeTool::TrvStats* stats = NULL);

  moab::ErrorCode  point_in_volume(const moab::EntityHandle volume,
				  const double xyz[3],
				  int &result,
				  const int thread_id = 0,
				  const double* uvw = NULL,
				  bool use_history = false);

  moab::ErrorCode point_in_volume_slow(const moab::EntityHandle volume,
				  const double xyz[3],
				  int &result,
				  const int thread_id = 0);        

  moab::ErrorCode closest_to_location(const moab::EntityHandle volume,
				  const double coords[3],
				  double &result,
				  moab::EntityHandle *surface,
				  const int thread_id = 0);

  moab::ErrorCode test_volume_boundary(const moab::EntityHandle volume,
				       const moab::EntityHandle surface,
				       const double xyz[3],
				       const double uvw[3],
				       int &result,
				       const int thread_idx = 0,
				       const bool use_history = false);

  moab::ErrorCode get_angle(const moab::EntityHandle surf, 
                            const double xyz[3],
			                      double angle[3], 
                            const int thread_idx, 
                            const bool use_history);

  moab::ErrorCode reset_history(const int thread_idx);

  moab::ErrorCode rollback_history(const int thread_idx);

  private:
  // get the dagmc instance for a given thread
  inline moab::DagMC* get_dagmc_instance(const int thread_id) {
    return dagmc_instances[thread_id];
  }

  // get the dagmc_raystate for a given thread
  inline DagMCRayState* get_dagmc_raystate(int thread_id) {
    return dagmc_rayhistories[thread_id];
  }

 private:
  int num_threads; ///< Number of threads to be held by the manager
  std::vector<moab::DagMC*> dagmc_instances; ///< vector of dagmc instances
  std::vector<DagMCRayState*> dagmc_rayhistories; ///< vector to the associated ray history
  moab::Interface* MOAB; ///< moab pointer
  moab::GeomTopoTool *GTT; ///< GTT Pointer
  double overlap_tolerance; ///< overlap tolerance
  double numerical_precision; ///< numerical precision
  double defaultFacetingTolerance;

  bool moab_instance_created;
  bool gtt_instance_created;
};

} // end of namespace
#endif
