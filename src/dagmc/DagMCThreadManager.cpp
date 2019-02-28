#include <iostream>
#include "DagMCThreadManager.hpp"

namespace DagMC {

// constructor
  DagThreadManager::DagThreadManager(moab::Interface* MBI, int num_threads,
				     const double p_overlap_tolerance,
				     const double p_numerical_precision) {
    //
    moab_instance_created = false;
    gtt_instance_created = false;

    // set the tolerances

    // if the moab pointer is not null point to it
    if (MBI != NULL) {
      MOAB = MBI;
    } else {
      // make new moab instance to share all DAGMC data
      MOAB = new moab::Core();
      moab_instance_created = true;
    }

    // make a new GTT for Thread manager
    gtt_instance_created = true;
    GTT = new moab::GeomTopoTool(MOAB,false);

    // number of threads
    set_num_threads(num_threads);

    // set storage for master
    dagmc_instances.push_back(new moab::DagMC(GTT));
    dagmc_rayhistories.push_back(new DagMCRayState());

    // setup threads
    setup_child_threads();
    defaultFacetingTolerance = 0.001;
    numerical_precision = p_numerical_precision;
    overlap_tolerance = p_overlap_tolerance;
  }

  DagThreadManager::DagThreadManager(moab::GeomTopoTool *gtt, int num_threads,
				     const double p_overlap_tolerance, const double p_numerical_precision) {
        //
    moab_instance_created = false;
    gtt_instance_created = false;

    // set the tolerances
    // if the moab pointer is not null point to it
    if (gtt != NULL) {
      MOAB = gtt->get_moab_instance();
      GTT = gtt;
    } else {
      // make new moab instance to share all DAGMC data
      MOAB = new moab::Core();
      moab_instance_created = true;
      // make a new GTT for Thread manager
      gtt_instance_created = true;
      GTT = new moab::GeomTopoTool(MOAB,false);
    }

    // number of threads
    set_num_threads(num_threads);

    // set storage for master
    dagmc_instances.push_back(new moab::DagMC(GTT));
    dagmc_rayhistories.push_back(new DagMC::DagMCRayState());

    // setup threads
    setup_child_threads();
    defaultFacetingTolerance = 0.001;

    numerical_precision = p_numerical_precision;
    overlap_tolerance = p_overlap_tolerance;
  }

  // destructor
  DagThreadManager::~DagThreadManager() {
    for (int i = 0 ; i < num_threads ; i++) {
      // delete each dagmc instance
      delete dagmc_instances[i];
      delete dagmc_rayhistories[i];
    }
    if(gtt_instance_created) delete GTT;
    if(moab_instance_created) delete MOAB;
  }

  // set the number of threads
  void DagThreadManager::set_num_threads(const int thread_count) {
    num_threads = thread_count;
  }

  void DagThreadManager::setup_child_threads() {
    // loop over the number of threads and make a new DAGMC instance for each
    for (int i = 1 ; i < num_threads ; i++) {
      // push back a vector of DAGMC instances
      dagmc_instances.push_back(new moab::DagMC(GTT));
      // collection of ray histories for each thread
      dagmc_rayhistories.push_back(new DagMCRayState());
    }
  }

  // note this should only be called by 1 thread
  moab::ErrorCode DagThreadManager::load_file(const char* cfile) {
    moab::ErrorCode rval = moab::MB_FAILURE;
    rval = dagmc_instances[0]->load_file(cfile);
    return rval;
  }

  // this should only be called by 1 thread
  moab::ErrorCode DagThreadManager::init_OBBTree() {
    moab::ErrorCode rval = moab::MB_FAILURE;
    rval = dagmc_instances[0]->init_OBBTree();
    initialise_child_threads();
    return rval;
  }

  // initalise threads with the loaded data
  void DagThreadManager::initialise_child_threads() {
    for (int i = 1 ; i < num_threads ; i++) {
      moab::ErrorCode rval;
      //std::cout << i << std::endl;
      rval = get_dagmc_instance(i)->load_existing_contents();
      rval = get_dagmc_instance(i)->init_OBBTree();
    }
  }

  // threaded ray fire
  moab::ErrorCode DagThreadManager::ray_fire(const moab::EntityHandle volume,
					       const double ray_start[3],
					       const double ray_dir[3],
					       moab::EntityHandle &next_surf,
					       double &next_surf_dist,
					       const int thread_idx,
					       const bool use_ray_history,
					       const double dist_limit,
					       const int ray_orientation,
					       moab::OrientedBoxTreeTool::TrvStats* stats){
    moab::ErrorCode rval = moab::MB_FAILURE;

    moab::DagMC::RayHistory* history = (use_ray_history) ? &(dagmc_rayhistories[thread_idx]->history) : (NULL);
    rval = dagmc_instances[thread_idx]->ray_fire(volume,ray_start,ray_dir,next_surf,
                                                 next_surf_dist,history,dist_limit,
                                                 ray_orientation,stats);

    return rval;
  }

  // thread safe point_in_volume
  moab::ErrorCode DagThreadManager::point_in_volume(const moab::EntityHandle volume,
				  const double xyz[3],
				  int &result,
				  const int thread_id,
				  const double* uvw,
				  const bool use_ray_history) {

    moab::ErrorCode rval = moab::MB_FAILURE;

    moab::DagMC::RayHistory* history = (use_ray_history) ? &(dagmc_rayhistories[thread_id]->history) : (NULL);
    rval = dagmc_instances[thread_id]->point_in_volume(volume,xyz,result,uvw,history);

    return rval;
  }

  // thread safe point_in_volume
  moab::ErrorCode DagThreadManager::point_in_volume_slow(const moab::EntityHandle volume,
				  const double xyz[3],
				  int &result,
				  const int thread_id) {

    moab::ErrorCode rval = moab::MB_FAILURE;

    rval = dagmc_instances[thread_id]->point_in_volume_slow(volume,xyz,result);

    return rval;
  }

  // thread safe point_in_volume
  moab::ErrorCode DagThreadManager::closest_to_location(const moab::EntityHandle volume,
				  const double coords[3],
				  double &result,
                                  moab::EntityHandle *surface,
				  const int thread_id) {

    moab::ErrorCode rval = moab::MB_FAILURE;

    rval = dagmc_instances[thread_id]->closest_to_location(volume,coords,result,surface);

    return rval;
  }

  // thread save test volume boundary
  moab::ErrorCode DagThreadManager::test_volume_boundary(const moab::EntityHandle volume,
				       const moab::EntityHandle surface,
				       const double xyz[3],
				       const double uvw[3],
				       int &result,
				       const int thread_idx,
				       const bool use_ray_history) {

    moab::ErrorCode rval = moab::MB_SUCCESS;

    moab::DagMC::RayHistory* history = (use_ray_history) ? &(dagmc_rayhistories[thread_idx]->history) : (NULL);
    rval = dagmc_instances[thread_idx]->test_volume_boundary(volume,surface,xyz,uvw,result,history);
  }

  // thread safe get_angle
  moab::ErrorCode DagThreadManager::get_angle(moab::EntityHandle surf, const double xyz[3],
			    double angle[3], const int thread_idx, const bool use_ray_history) {
    moab::ErrorCode rval = moab::MB_FAILURE;
    moab::DagMC::RayHistory* history = (use_ray_history) ? &(dagmc_rayhistories[thread_idx]->history) : (NULL);
    rval = dagmc_instances[thread_idx]->get_angle(surf,xyz,angle,history);

    return rval;
  }

  // thread safe reset history
  moab::ErrorCode DagThreadManager::reset_history(const int thread_idx) {

    dagmc_rayhistories[thread_idx]->history.reset();

    return moab::MB_SUCCESS;
  }

  // thread safe reset history
  moab::ErrorCode DagThreadManager::rollback_history(const int thread_idx) {

    dagmc_rayhistories[thread_idx]->history.rollback_last_intersection();

    return moab::MB_SUCCESS;
  }

} // end of namespace
