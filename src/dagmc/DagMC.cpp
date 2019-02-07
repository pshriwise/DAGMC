#include "DagMC.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <algorithm>
#include <set>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <math.h>
#ifndef M_PI  /* windows */
# define M_PI 3.14159265358979323846
#endif

#define MB_OBB_TREE_TAG_NAME "OBB_TREE"
#define FACETING_TOL_TAG_NAME "FACETING_TOL"

namespace moab {

/* Tolerance Summary

   Facet Tolerance:
   Maximum distance between continuous solid model surface and faceted surface.
     Performance:  increasing tolerance increased performance (fewer triangles)
     Robustness:   should not be affected
     Knowledge:    user must understand how coarser faceting influences accuracy
                   of results
*/

const bool counting = false; /* controls counts of ray casts and pt_in_vols */

// Empty synonym map for DagMC::parse_metadata()
const std::map<std::string, std::string> DagMC::no_synonyms;

// DagMC Constructor
DagMC::DagMC(Interface* mb_impl, double overlap_tolerance, double p_numerical_precision) {
  moab_instance_created = false;
  // if we arent handed a moab instance create one
  if (NULL == mb_impl) {
    mb_impl = new moab::Core();
    moab_instance_created = true;
  }

  // set the internal moab pointer
  MBI = mb_impl;

  // make new GeomTopoTool and GeomQueryTool
  GTT = new moab::GeomTopoTool(MBI, false);
  GQT = new moab::GeomQueryTool(GTT, overlap_tolerance, p_numerical_precision);

  // This is the correct place to uniquely define default values for the dagmc settings
  defaultFacetingTolerance = .001;
}

// Destructor
DagMC::~DagMC() {
  // delete the GeomTopoTool and GeomQueryTool
  delete GTT;
  delete GQT;

  // if we created the moab instance
  // clear it
  if (moab_instance_created) {
    MBI->delete_mesh();
    delete MBI;
  }
}

// get the float verision of dagmc version string
float DagMC::version(std::string* version_string) {
  if (NULL != version_string)
    *version_string = std::string("DagMC version ") + std::string(DAGMC_VERSION_STRING);
  return DAGMC_VERSION;
}

unsigned int DagMC::interface_revision() {
  unsigned int result = 0;
  const char* interface_string = DAGMC_INTERFACE_REVISION;
  if (strlen(interface_string) >= 5) {
    // start looking for the revision number after "$Rev: "
    result = strtol(interface_string + 5, NULL, 10);
  }
  return result;
}

/* SECTION I: Geometry Initialization and problem setup */

// the standard DAGMC load file method
ErrorCode DagMC::load_file(const char* cfile) {
  ErrorCode rval;
  std::cout << "Loading file " << cfile << std::endl;
  // load options
  char options[120] = {0};
  char file_ext[4] = "" ; // file extension

  // get the last 4 chars of file .i.e .h5m .sat etc
  memcpy(file_ext, &cfile[strlen(cfile) - 4], 4);

  EntityHandle file_set;
  rval = MBI->create_meshset(MESHSET_SET, file_set);
  if (MB_SUCCESS != rval)
    return rval;

  rval = MBI->load_file(cfile, &file_set, options, NULL, 0, 0);

  if (MB_UNHANDLED_OPTION == rval) {
    // Some options were unhandled; this is common for loading h5m files.
    // Print a warning if an option was unhandled for a file that does not end in '.h5m'
    std::string filename(cfile);
    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".h5m") {
      std::cerr << "DagMC warning: unhandled file loading options." << std::endl;
    }
  } else if (MB_SUCCESS != rval) {
    std::cerr << "DagMC Couldn't read file " << cfile << std::endl;
    std::string message;
    if (MB_SUCCESS == MBI->get_last_error(message) && !message.empty())
      std::cerr << "Error message: " << message << std::endl;

    return rval;
  }

  return finish_loading();
}

// helper function to load the existing contents of a MOAB instance into DAGMC
ErrorCode DagMC::load_existing_contents() {
  return finish_loading();
}

// setup the implicit compliment
ErrorCode DagMC::setup_impl_compl() {
  // If it doesn't already exist, create implicit complement
  // Create data structures for implicit complement
  ErrorCode rval = GTT->setup_implicit_complement();
  if (MB_SUCCESS != rval) {
    std::cerr << "Failed to find or create implicit complement handle." << std::endl;
    return rval;
  }
  return MB_SUCCESS;
}

ErrorCode DagMC::create_graveyard() {
  ErrorCode rval;

  EntityHandle graveyard_vol;
  rval = create_containing_volume(graveyard_vol);
  MB_CHK_SET_ERR(rval, "Failed to create the containing volume");

  Tag category_tag;
  rval = MBI->tag_get_handle(CATEGORY_TAG_NAME, CATEGORY_TAG_SIZE,
                                 MB_TYPE_OPAQUE, category_tag, MB_TAG_SPARSE|MB_TAG_CREAT);
  MB_CHK_SET_ERR(rval, "Could not get the category tag");

  // find the graveyard group
  Range graveyard_grps;
  Tag tags[2] = {category_tag, name_tag()};
  static const char cat_val[CATEGORY_TAG_SIZE] = "Group\0";
  static const char name_val[NAME_TAG_SIZE] = "mat:Graveyard\0";
  const void* vals[2] = {&cat_val, &name_val};
  rval = MBI->get_entities_by_type_and_tag(0, MBENTITYSET, &tags[0], &vals[0], 2, graveyard_grps);
  if (MB_ENTITY_NOT_FOUND != rval) {
    MB_CHK_SET_ERR(rval, "Failed to get graveyard group");
  }

  EntityHandle graveyard_group;
  if (graveyard_grps.size() > 0) {
    graveyard_group = graveyard_grps[0];
  } else {
    // otherwise create the group and add the vol
    rval = MBI->create_meshset(0, graveyard_group);
    MB_CHK_SET_ERR(rval, "Failed to create a new meshset for a new graveyard");

    rval = GTT->add_geo_set(graveyard_group, 4);
    MB_CHK_SET_ERR(rval, "Failed to add the graveyard group to the GTT");

    rval = MBI->tag_set_data(category_tag, &graveyard_group, 1, cat_val);
    MB_CHK_SET_ERR(rval, "Failed to set the graveyard category tag name");

    rval = MBI->tag_set_data(name_tag(), &graveyard_group, 1, name_val);
    MB_CHK_SET_ERR(rval, "Failed to set the graveyard metadata name");
  }

  // add the graveyard_vol to the graveyard group
  rval = MBI->add_entities(graveyard_group, &graveyard_vol, 1);
  MB_CHK_SET_ERR(rval, "Failed to add the new graveyard volume to the graveyard_group");

  if (GTT->have_obb_tree()) {
    rval = GTT->construct_obb_tree(graveyard_vol);
    MB_CHK_SET_ERR(rval, "Failed to build graveyard OBB tree");
  }

  // now handle implicit complement, need to rebuild if it exists
  EntityHandle ic;
  if (MB_ENTITY_NOT_FOUND != GTT->get_implicit_complement(ic)) {
    EntityHandle ic_tree_root;
    bool rebuild_ic_tree = MB_INDEX_OUT_OF_RANGE == GTT->get_root(ic, ic_tree_root);

    // delete IC
    rval = GTT->delete_implicit_complement();
    MB_CHK_SET_ERR(rval, "Failed to delete the implicit complement");

    // re-create IC
    rval = setup_impl_compl();
    MB_CHK_SET_ERR(rval, "Failed to re-create the implicit complement when adding containing volume");


    // if needed, delete IC tree
    if (rebuild_ic_tree) {

      // get the new IC
      rval = GTT->get_implicit_complement(ic);
      MB_CHK_SET_ERR(rval, "Failed to get the new implicit complement when adding containing volume");

      rval = GTT->construct_obb_tree(ic);
      MB_CHK_SET_ERR(rval, "Failed to re-create the implicity complement OBB tree when adding containing volume");

    } // implicit complement handling

  }

  return MB_SUCCESS;
}


ErrorCode DagMC::create_containing_volume(EntityHandle& containing_vol) {
  ErrorCode rval;

  // determine the extents of the model
  Range current_volumes;
  rval = GTT->get_gsets_by_dimension(3, current_volumes);
  MB_CHK_SET_ERR(rval, "Failed to get current model volumes when adding containing volume");

  Range::iterator it;
  double model_max[3] = {-1.e37, -1.e37, -1.e37};
  double model_min[3] = { 1.e37,  1.e37,  1.e37};

  for(it = current_volumes.begin(); it != current_volumes.end(); it++) {
    double vol_max[3], vol_min[3];
    rval = getobb(*it, vol_min, vol_max);
    MB_CHK_SET_ERR(rval, "Failed to get OBB for volume: " << get_entity_id(*it));

    // update min
    model_min[0] = vol_min[0] < model_min[0] ? vol_min[0] : model_min[0];
    model_min[1] = vol_min[1] < model_min[1] ? vol_min[1] : model_min[1];
    model_min[2] = vol_min[2] < model_min[2] ? vol_min[2] : model_min[2];

    // update max
    model_max[0] = vol_max[0] > model_max[0] ? vol_max[0] : model_max[0];
    model_max[1] = vol_max[1] > model_max[1] ? vol_max[1] : model_max[1];
    model_max[2] = vol_max[2] > model_max[2] ? vol_max[2] : model_max[2];
  }

  // bump the vertices away from the origin a bit
  double bump_val = 10.0;
  model_min[0] -= bump_val; model_min[1] -= bump_val; model_min[2] -= bump_val;
  model_max[0] += bump_val; model_max[1] += bump_val; model_max[2] += bump_val;

  // create inner surface (normals inward)
  EntityHandle inner_surf;
  rval = create_box_surface(model_min, model_max, inner_surf, false);
  MB_CHK_SET_ERR(rval, "Failed to create the inner box surface of the containing volume");

  // bump them some more for the outer surface
  model_min[0] -= bump_val; model_min[1] -= bump_val; model_min[2] -= bump_val;
  model_max[0] += bump_val; model_max[1] += bump_val; model_max[2] += bump_val;

  // create outer surface (normals outward)
  EntityHandle outer_surf;
  rval = create_box_surface(model_min, model_max, outer_surf);
  MB_CHK_SET_ERR(rval, "Failed to create the outer box surface of the containing volume");

  // create volume set
  EntityHandle volume;
  rval = MBI->create_meshset(0, volume);
  MB_CHK_SET_ERR(rval, "Failed to create containing volume meshset");

  // create parent/child relationships
  rval = MBI->add_parent_child(volume, inner_surf);
  MB_CHK_SET_ERR(rval, "Failed to create parent-child relationship for inner containing surface");
  rval = MBI->add_parent_child(volume, outer_surf);
  MB_CHK_SET_ERR(rval, "Failed to create parent-child relationship for outer containing surface");

  rval = GTT->add_geo_set(volume, 3);
  MB_CHK_SET_ERR(rval, "Failed to add containing volume to the model");

  // set sense relationships (both forward)
  rval = GTT->set_sense(inner_surf, volume, 1);
  MB_CHK_SET_ERR(rval, "Failed to set inner surface sense for the containing volume");
  rval = GTT->set_sense(outer_surf, volume, 1);
  MB_CHK_SET_ERR(rval, "Failed to set inner surface sense for the containing volume");

  containing_vol = volume;

  return MB_SUCCESS;

}

ErrorCode DagMC::create_box_surface(double box_min[3],
                                    double box_max[3],
                                    EntityHandle& surface,
                                    bool normals_out) {
  ErrorCode rval;
  // check for degenerate coordinates
  if( box_min[0] >= box_max[0] || box_min[1] >= box_max[1] || box_min[2] >= box_max[2] ) {
    std::cerr << "Invalid box coordinates for creating a new surface: \n";
    std::cerr << "Box min corner: " << box_min[0] << " " << box_min[1] << " " << box_min[2] << "\n";
    std::cerr << "Box max corner: " << box_max[0] << " " << box_max[1] << " " << box_max[2] << "\n";
    return MB_FAILURE;
  }

  // create vertices for each corner
  EntityHandle inner_verts[8];
  double vert_coords[3];
  // bottom (in Z)
  vert_coords[0] = box_min[0]; vert_coords[1] = box_min[1]; vert_coords[2] = box_min[2];
  rval = MBI->create_vertex(vert_coords, inner_verts[0]);
  MB_CHK_SET_ERR(rval, "Unable to create vertex for containing volume");
  vert_coords[0] = box_max[0]; vert_coords[1] = box_min[1]; vert_coords[2] = box_min[2];
  rval = MBI->create_vertex(vert_coords, inner_verts[1]);
  MB_CHK_SET_ERR(rval, "Unable to create vertex for containing volume");
  vert_coords[0] = box_max[0]; vert_coords[1] = box_max[1]; vert_coords[2] = box_min[2];
  rval = MBI->create_vertex(vert_coords, inner_verts[2]);
  MB_CHK_SET_ERR(rval, "Unable to create vertex for containing volume");
  vert_coords[0] = box_min[0]; vert_coords[1] = box_max[1]; vert_coords[2] = box_min[2];
  rval = MBI->create_vertex(vert_coords, inner_verts[3]);
  MB_CHK_SET_ERR(rval, "Unable to create vertex for containing volume");
  // top (in Z)
  vert_coords[0] = box_min[0]; vert_coords[1] = box_min[1]; vert_coords[2] = box_max[2];
  rval = MBI->create_vertex(vert_coords, inner_verts[4]);
  MB_CHK_SET_ERR(rval, "Unable to create vertex for containing volume");
  vert_coords[0] = box_max[0]; vert_coords[1] = box_min[1]; vert_coords[2] = box_max[2];
  rval = MBI->create_vertex(vert_coords, inner_verts[5]);
  MB_CHK_SET_ERR(rval, "Unable to create vertex for containing volume");
  vert_coords[0] = box_max[0]; vert_coords[1] = box_max[1]; vert_coords[2] = box_max[2];
  rval = MBI->create_vertex(vert_coords, inner_verts[6]);
  MB_CHK_SET_ERR(rval, "Unable to create vertex for containing volume");
  vert_coords[0] = box_min[0]; vert_coords[1] = box_max[1]; vert_coords[2] = box_max[2];
  rval = MBI->create_vertex(vert_coords, inner_verts[7]);
  MB_CHK_SET_ERR(rval, "Unable to create vertex for containing volume");

  // create inner triangles
  EntityHandle inner_tris[12];
  int conn[12][3] = {{0, 3, 1}, {2, 1, 3},
                     {1, 2, 5}, {6, 5, 2},
                     {0, 1, 4}, {5, 4, 1},
                     {3, 0, 7}, {4, 7, 0},
                     {2, 3, 6}, {7, 6, 3},
                     {4, 5, 7}, {6, 7, 5}};

  std::vector<EntityHandle> tris;
  for (int i = 0; i < 12; i++) {
    EntityHandle connectivity[3];
    if (normals_out) {
      connectivity[0] = inner_verts[conn[i][0]];
      connectivity[1] = inner_verts[conn[i][1]];
      connectivity[2] = inner_verts[conn[i][2]];
    } else {
      connectivity[0] = inner_verts[conn[i][0]];
      connectivity[1] = inner_verts[conn[i][2]];
      connectivity[2] = inner_verts[conn[i][1]];
    }
    EntityHandle new_tri;

    rval = MBI->create_element(MBTRI, connectivity, 3, new_tri);
    MB_CHK_SET_ERR(rval, "Failed to create new triangle for containing volume");

    tris.push_back(new_tri);
  }

  // create a new meshset for the surface
  EntityHandle surf_set;
  rval = MBI->create_meshset(0, surf_set);
  MB_CHK_SET_ERR(rval, "Failed to create surface set for containing volume");

  // add triangles to set
  rval = MBI->add_entities(surf_set, &(tris[0]), tris.size());
  MB_CHK_SET_ERR(rval, "Failed to add triangles to surface for containing volume");

  rval = GTT->add_geo_set(surf_set, 2);
  MB_CHK_SET_ERR(rval, "Failed to add box surface to model");

  surface = surf_set;

  return MB_SUCCESS;
}

// gets the entity sets tagged with geomtag 2 and 3
// surfaces and volumes respectively
ErrorCode DagMC::setup_geometry(Range& surfs, Range& vols) {
  ErrorCode rval;

  // get all surfaces
  rval = GTT->get_gsets_by_dimension(2, surfs);
  MB_CHK_SET_ERR(rval, "Could not get surfaces from GTT");

  // get all volumes
  rval = GTT->get_gsets_by_dimension(3, vols);
  MB_CHK_SET_ERR(rval, "Could not get volumes from GTT");

  return MB_SUCCESS;
}

// sets up the obb tree for the problem
ErrorCode DagMC::setup_obbs() {
  ErrorCode rval;

  // If we havent got an OBB Tree, build one.
  if (!GTT->have_obb_tree()) {
    std::cout << "Building OBB Tree..." << std::endl;
    rval = GTT->construct_obb_trees();
    MB_CHK_SET_ERR(rval, "Failed to build obb trees");
  }

  return MB_SUCCESS;
}

// setups of the indices for the problem, builds a list of
ErrorCode DagMC::setup_indices() {
  Range surfs, vols;
  ErrorCode rval = setup_geometry(surfs, vols);

  // build the various index vectors used for efficiency
  rval = build_indices(surfs, vols);
  MB_CHK_SET_ERR(rval, "Failed to build surface/volume indices");
  return MB_SUCCESS;
}

// initialise the obb tree
ErrorCode DagMC::init_OBBTree() {
  ErrorCode rval;

  // find all geometry sets
  rval = GTT->find_geomsets();
  MB_CHK_SET_ERR(rval, "GeomTopoTool could not find the geometry sets");

  // implicit compliment
  // EntityHandle implicit_complement;
  //  rval = GTT->get_implicit_complement(implicit_complement, true);
  rval = setup_impl_compl();
  MB_CHK_SET_ERR(rval, "Failed to setup the implicit compliment");

  // build obbs
  rval = setup_obbs();
  MB_CHK_SET_ERR(rval, "Failed to setup the OBBs");

  // create graveyard
  rval = create_graveyard();
  MB_CHK_SET_ERR(rval, "Failed to create graveyard volume");

  // setup indices
  rval = setup_indices();
  MB_CHK_SET_ERR(rval, "Failed to setup problem indices");

  return MB_SUCCESS;
}

// helper function to finish setting up required tags.
ErrorCode DagMC::finish_loading() {
  ErrorCode rval;

  nameTag = get_tag(NAME_TAG_NAME, NAME_TAG_SIZE, MB_TAG_SPARSE, MB_TYPE_OPAQUE, NULL, false);

  facetingTolTag = get_tag(FACETING_TOL_TAG_NAME, 1, MB_TAG_SPARSE, MB_TYPE_DOUBLE);

  // search for a tag that has the faceting tolerance
  Range tagged_sets;
  double facet_tol_tagvalue = 0;
  bool other_set_tagged = false, root_tagged = false;

  // get list of entity sets that are tagged with faceting tolerance
  // (possibly empty set)
  rval = MBI->get_entities_by_type_and_tag(0, MBENTITYSET, &facetingTolTag,
                                           NULL, 1, tagged_sets);
  // if NOT empty set
  if (MB_SUCCESS == rval && !tagged_sets.empty()) {
    rval = MBI->tag_get_data(facetingTolTag, &(*tagged_sets.begin()), 1, &facet_tol_tagvalue);
    if (MB_SUCCESS != rval)
      return rval;
    other_set_tagged = true;
  } else if (MB_SUCCESS == rval) {
    // check to see if interface is tagged
    EntityHandle root = 0;
    rval = MBI->tag_get_data(facetingTolTag, &root, 1, &facet_tol_tagvalue);
    if (MB_SUCCESS == rval)
      root_tagged = true;
    else
      rval = MB_SUCCESS;
  }

  if ((root_tagged || other_set_tagged) && facet_tol_tagvalue > 0) {
    facetingTolerance = facet_tol_tagvalue;
  }

  // initialize GQT
  std::cout << "Initializing the GeomQueryTool..." << std::endl;
  rval = GTT->find_geomsets();
  MB_CHK_SET_ERR(rval, "Failed to find the geometry sets");

  std::cout << "Using faceting tolerance: " << facetingTolerance << std::endl;

  return MB_SUCCESS;
}


/* SECTION II: Fundamental Geometry Operations/Queries */

ErrorCode DagMC::ray_fire(const EntityHandle volume, const double point[3],
                          const double dir[3], EntityHandle& next_surf,
                          double& next_surf_dist,
                          RayHistory* history,
                          double user_dist_limit, int ray_orientation,
                          OrientedBoxTreeTool::TrvStats* stats) {
  ErrorCode rval = GQT->ray_fire(volume, point, dir, next_surf, next_surf_dist,
                                 history, user_dist_limit, ray_orientation,
                                 stats);
  return rval;
}

ErrorCode DagMC::point_in_volume(const EntityHandle volume, const double xyz[3],
                                 int& result, const double* uvw,
                                 const RayHistory* history) {
  ErrorCode rval = GQT->point_in_volume(volume, xyz, result, uvw, history);
  return rval;
}

ErrorCode DagMC::test_volume_boundary(const EntityHandle volume,
                                      const EntityHandle surface,
                                      const double xyz[3], const double uvw[3],
                                      int& result,
                                      const RayHistory* history) {
  ErrorCode rval = GQT->test_volume_boundary(volume, surface, xyz, uvw, result,
                                             history);
  return rval;
}

// use spherical area test to determine inside/outside of a polyhedron.
ErrorCode DagMC::point_in_volume_slow(EntityHandle volume, const double xyz[3],
                                      int& result) {
  ErrorCode rval = GQT->point_in_volume_slow(volume, xyz, result);
  return rval;
}

// detemine distance to nearest surface
ErrorCode DagMC::closest_to_location(EntityHandle volume,
                                     const double coords[3], double& result,
                                     EntityHandle* surface) {
  ErrorCode rval = GQT->closest_to_location(volume, coords, result, surface);
  return rval;
}

// calculate volume of polyhedron
ErrorCode DagMC::measure_volume(EntityHandle volume, double& result) {
  ErrorCode rval = GQT->measure_volume(volume, result);
  return rval;
}

// sum area of elements in surface
ErrorCode DagMC::measure_area(EntityHandle surface, double& result) {
  ErrorCode rval = GQT->measure_area(surface, result);
  return rval;
}

// get sense of surface(s) wrt volume
ErrorCode DagMC::surface_sense(EntityHandle volume, int num_surfaces,
                               const EntityHandle* surfaces, int* senses_out) {
  ErrorCode rval = GTT->get_surface_senses(volume, num_surfaces, surfaces,
                                           senses_out);
  return rval;
}

// get sense of surface(s) wrt volume
ErrorCode DagMC::surface_sense(EntityHandle volume, EntityHandle surface,
                               int& sense_out) {
  ErrorCode rval = GTT->get_sense(surface, volume, sense_out);
  return rval;
}

ErrorCode DagMC::get_angle(EntityHandle surf, const double in_pt[3],
                           double angle[3],
                           const RayHistory* history) {
  ErrorCode rval = GQT->get_normal(surf, in_pt, angle, history);
  return rval;
}

ErrorCode DagMC::next_vol(EntityHandle surface, EntityHandle old_volume,
                          EntityHandle& new_volume) {
  ErrorCode rval = GTT->next_vol(surface, old_volume, new_volume);
  return rval;
}

/* SECTION III */

EntityHandle DagMC::entity_by_id(int dimension, int id) {
  return GTT->entity_by_id(dimension, id);
}

int DagMC::id_by_index(int dimension, int index) {
  EntityHandle h = entity_by_index(dimension, index);
  if (!h)
    return 0;

  int result = 0;
  MBI->tag_get_data(GTT->get_gid_tag(), &h, 1, &result);
  return result;
}

int DagMC::get_entity_id(EntityHandle this_ent) {
  return GTT->global_id(this_ent);
}

ErrorCode DagMC::build_indices(Range& surfs, Range& vols) {
  ErrorCode rval = MB_SUCCESS;

  // surf/vol offsets are just first handles
  setOffset = std::min(*surfs.begin(), *vols.begin());

  // max
  EntityHandle tmp_offset = std::max(surfs.back(), vols.back());

  // set size
  entIndices.resize(tmp_offset - setOffset + 1);

  // store surf/vol handles lists (surf/vol by index) and
  // index by handle lists
  surf_handles().resize(surfs.size() + 1);
  std::vector<EntityHandle>::iterator iter = surf_handles().begin();
  *(iter++) = 0;
  std::copy(surfs.begin(), surfs.end(), iter);
  int idx = 1;
  for (Range::iterator rit = surfs.begin(); rit != surfs.end(); ++rit)
    entIndices[*rit - setOffset] = idx++;

  vol_handles().resize(vols.size() + 1);
  iter = vol_handles().begin();
  *(iter++) = 0;
  std::copy(vols.begin(), vols.end(), iter);
  idx = 1;
  for (Range::iterator rit = vols.begin(); rit != vols.end(); ++rit)
    entIndices[*rit - setOffset] = idx++;

  // get group handles
  Tag category_tag = get_tag(CATEGORY_TAG_NAME, CATEGORY_TAG_SIZE,
                             MB_TAG_SPARSE, MB_TYPE_OPAQUE);
  char group_category[CATEGORY_TAG_SIZE];
  std::fill(group_category, group_category + CATEGORY_TAG_SIZE, '\0');
  sprintf(group_category, "%s", "Group");
  const void* const group_val[] = {&group_category};
  Range groups;
  rval = MBI->get_entities_by_type_and_tag(0, MBENTITYSET, &category_tag,
                                           group_val, 1, groups);
  if (MB_SUCCESS != rval)
    return rval;
  group_handles().resize(groups.size() + 1);
  group_handles()[0] = 0;
  std::copy(groups.begin(), groups.end(), &group_handles()[1]);

  return MB_SUCCESS;
}



/* SECTION IV */

void DagMC::set_overlap_thickness(double new_thickness) {
  GQT->set_overlap_thickness(new_thickness);
}

void DagMC::set_numerical_precision(double new_precision) {
  GQT->set_numerical_precision(new_precision);
}

ErrorCode DagMC::write_mesh(const char* ffile,
                            const int flen) {
  ErrorCode rval;

  // write out a mesh file if requested
  if (ffile && 0 < flen) {
    rval = MBI->write_mesh(ffile);
    if (MB_SUCCESS != rval) {
      std::cerr << "Failed to write mesh to " << ffile << "." << std::endl;
      return rval;
    }
  }

  return MB_SUCCESS;
}

/* SECTION V: Metadata handling */

ErrorCode DagMC::get_group_name(EntityHandle group_set, std::string& name) {
  ErrorCode rval;
  const void* v = NULL;
  int ignored;
  rval = MBI->tag_get_by_ptr(name_tag(), &group_set, 1, &v, &ignored);
  if (MB_SUCCESS != rval)
    return rval;
  name = static_cast<const char*>(v);
  return MB_SUCCESS;
}

ErrorCode DagMC::parse_group_name(EntityHandle group_set, prop_map& result, const char* delimiters) {
  ErrorCode rval;
  std::string group_name;
  rval = get_group_name(group_set, group_name);
  if (rval != MB_SUCCESS)
    return rval;

  std::vector< std::string > group_tokens;
  tokenize(group_name, group_tokens, delimiters);

  // iterate over all the keyword positions
  // keywords are even indices, their values (optional) are odd indices
  for (unsigned int i = 0; i < group_tokens.size(); i += 2) {
    std::string groupkey = group_tokens[i];
    std::string groupval;
    if (i < group_tokens.size() - 1)
      groupval = group_tokens[i + 1];
    result[groupkey] = groupval;
  }
  return MB_SUCCESS;
}

ErrorCode DagMC::detect_available_props(std::vector<std::string>& keywords_list, const char* delimiters) {
  ErrorCode rval;
  std::set< std::string > keywords;
  for (std::vector<EntityHandle>::const_iterator grp = group_handles().begin();
       grp != group_handles().end(); ++grp) {
    std::map< std::string, std::string > properties;
    rval = parse_group_name(*grp, properties, delimiters);
    if (rval == MB_TAG_NOT_FOUND)
      continue;
    else if (rval != MB_SUCCESS)
      return rval;

    for (prop_map::iterator i = properties.begin();
         i != properties.end(); ++i) {
      keywords.insert((*i).first);
    }
  }
  keywords_list.assign(keywords.begin(), keywords.end());
  return MB_SUCCESS;
}

ErrorCode DagMC::append_packed_string(Tag tag, EntityHandle eh,
                                      std::string& new_string) {
  // When properties have multiple values, the values are tagged in a single character array
  // with the different values separated by null characters
  ErrorCode rval;
  const void* p;
  const char* str;
  int len;
  rval = MBI->tag_get_by_ptr(tag, &eh, 1, &p, &len);
  if (rval == MB_TAG_NOT_FOUND) {
    // This is the first entry, and can be set directly
    p = new_string.c_str();
    return MBI->tag_clear_data(tag, &eh, 1, p, new_string.length() + 1);
  } else if (rval != MB_SUCCESS)
    return rval;
  else {
    str = static_cast<const char*>(p);
  }

  // append a new value for the property to the existing property string
  unsigned int tail_len = new_string.length() + 1;
  char* new_packed_string = new char[ len + tail_len ];
  memcpy(new_packed_string, str, len);
  memcpy(new_packed_string + len, new_string.c_str(), tail_len);

  int new_len = len + tail_len;
  p = new_packed_string;
  rval = MBI->tag_set_by_ptr(tag, &eh, 1, &p, &new_len);
  delete[] new_packed_string;
  return rval;
}

ErrorCode DagMC::unpack_packed_string(Tag tag, EntityHandle eh,
                                      std::vector< std::string >& values) {
  ErrorCode rval;
  const void* p;
  const char* str;
  int len;
  rval = MBI->tag_get_by_ptr(tag, &eh, 1, &p, &len);
  if (rval != MB_SUCCESS)
    return rval;
  str = static_cast<const char*>(p);
  int idx = 0;
  while (idx < len) {
    std::string item(str + idx);
    values.push_back(item);
    idx += item.length() + 1;
  }
  return MB_SUCCESS;
}

ErrorCode DagMC::parse_properties(const std::vector<std::string>& keywords,
                                  const std::map<std::string, std::string>& keyword_synonyms,
                                  const char* delimiters) {
  ErrorCode rval;

  // master keyword map, mapping user-set words in cubit to canonical property names
  std::map< std::string, std::string > keyword_map(keyword_synonyms);

  for (std::vector<std::string>::const_iterator i = keywords.begin();
       i != keywords.end(); ++i) {
    keyword_map[*i] = *i;
  }

  // the set of all canonical property names
  std::set< std::string > prop_names;
  for (prop_map::iterator i = keyword_map.begin();
       i != keyword_map.end(); ++i) {
    prop_names.insert((*i).second);
  }

  // set up DagMC's property tags based on what's been requested
  for (std::set<std::string>::iterator i = prop_names.begin();
       i != prop_names.end(); ++i) {
    std::string tagname("DAGMCPROP_");
    tagname += (*i);

    Tag new_tag;
    rval = MBI->tag_get_handle(tagname.c_str(), 0, MB_TYPE_OPAQUE, new_tag,
                               MB_TAG_SPARSE | MB_TAG_VARLEN | MB_TAG_CREAT);
    if (MB_SUCCESS != rval)
      return rval;
    property_tagmap[(*i)] = new_tag;
  }

  // now that the keywords and tags are ready, iterate over all the actual geometry groups
  for (std::vector<EntityHandle>::iterator grp = group_handles().begin();
       grp != group_handles().end(); ++grp) {

    prop_map properties;
    rval = parse_group_name(*grp, properties, delimiters);
    if (rval == MB_TAG_NOT_FOUND)
      continue;
    else if (rval != MB_SUCCESS)
      return rval;

    Range grp_sets;
    rval = MBI->get_entities_by_type(*grp, MBENTITYSET, grp_sets);
    if (MB_SUCCESS != rval)
      return rval;
    if (grp_sets.size() == 0)
      continue;

    for (prop_map::iterator i = properties.begin();
         i != properties.end(); ++i) {
      std::string groupkey = (*i).first;
      std::string groupval = (*i).second;

      if (property_tagmap.find(groupkey) != property_tagmap.end()) {
        Tag proptag = property_tagmap[groupkey];
        const unsigned int groupsize = grp_sets.size();
        for (unsigned int j = 0; j < groupsize; ++j) {
          rval = append_packed_string(proptag, grp_sets[j], groupval);
        }
      }
    }
  }
  return MB_SUCCESS;
}

ErrorCode DagMC::prop_value(EntityHandle eh, const std::string& prop, std::string& value) {
  ErrorCode rval;

  std::map<std::string, Tag>::iterator it = property_tagmap.find(prop);
  if (it == property_tagmap.end()) {
    return MB_TAG_NOT_FOUND;
  }

  Tag proptag = (*it).second;
  const void* data;
  int ignored;

  rval = MBI->tag_get_by_ptr(proptag, &eh, 1, &data, &ignored);
  if (rval != MB_SUCCESS)
    return rval;
  value = static_cast<const char*>(data);
  return MB_SUCCESS;
}

ErrorCode DagMC::prop_values(EntityHandle eh, const std::string& prop,
                             std::vector< std::string >& values) {

  std::map<std::string, Tag>::iterator it = property_tagmap.find(prop);
  if (it == property_tagmap.end()) {
    return MB_TAG_NOT_FOUND;
  }
  Tag proptag = (*it).second;

  return unpack_packed_string(proptag, eh, values);

}

bool DagMC::has_prop(EntityHandle eh, const std::string& prop) {
  ErrorCode rval;

  std::map<std::string, Tag>::iterator it = property_tagmap.find(prop);
  if (it == property_tagmap.end()) {
    return false;
  }

  Tag proptag = (*it).second;
  const void* data;
  int ignored;

  rval = MBI->tag_get_by_ptr(proptag, &eh, 1, &data, &ignored);
  return (rval == MB_SUCCESS);

}


ErrorCode DagMC::get_all_prop_values(const std::string& prop, std::vector<std::string>& return_list) {
  ErrorCode rval;
  std::map<std::string, Tag>::iterator it = property_tagmap.find(prop);
  if (it == property_tagmap.end()) {
    return MB_TAG_NOT_FOUND;
  }
  Tag proptag = (*it).second;
  Range all_ents;

  rval = MBI->get_entities_by_type_and_tag(0, MBENTITYSET, &proptag, NULL, 1, all_ents);
  if (MB_SUCCESS != rval)
    return rval;

  std::set<std::string> unique_values;
  for (Range::iterator i = all_ents.begin(); i != all_ents.end(); ++i) {
    std::vector<std::string> values;
    rval = prop_values(*i, prop, values);
    if (MB_SUCCESS != rval)
      return rval;
    unique_values.insert(values.begin(), values.end());
  }

  return_list.assign(unique_values.begin(), unique_values.end());
  return MB_SUCCESS;
}

ErrorCode DagMC::entities_by_property(const std::string& prop, std::vector<EntityHandle>& return_list,
                                      int dimension, const std::string* value) {
  ErrorCode rval;
  std::map<std::string, Tag>::iterator it = property_tagmap.find(prop);
  if (it == property_tagmap.end()) {
    return MB_TAG_NOT_FOUND;
  }
  Tag proptag = (*it).second;
  Range all_ents;

  // Note that we cannot specify values for proptag here-- the passed value,
  // if it exists, may be only a subset of the packed string representation
  // of this tag.
  Tag tags[2] = {proptag, GTT->get_geom_tag()};
  void* vals[2] = {NULL, (dimension != 0) ? &dimension : NULL };
  rval = MBI->get_entities_by_type_and_tag(0, MBENTITYSET, tags, vals, 2, all_ents);
  if (MB_SUCCESS != rval)
    return rval;

  std::set<EntityHandle> handles;
  for (Range::iterator i = all_ents.begin(); i != all_ents.end(); ++i) {
    std::vector<std::string> values;
    rval = prop_values(*i, prop, values);
    if (MB_SUCCESS != rval)
      return rval;
    if (value) {
      if (std::find(values.begin(), values.end(), *value) != values.end()) {
        handles.insert(*i);
      }
    } else {
      handles.insert(*i);
    }
  }

  return_list.assign(handles.begin(), handles.end());
  return MB_SUCCESS;
}

bool DagMC::is_implicit_complement(EntityHandle volume) {
  return GTT->is_implicit_complement(volume);
}

void DagMC::tokenize(const std::string& str,
                     std::vector<std::string>& tokens,
                     const char* delimiters) const {
  std::string::size_type last = str.find_first_not_of(delimiters, 0);
  std::string::size_type pos  = str.find_first_of(delimiters, last);
  if (std::string::npos == pos)
    tokens.push_back(str);
  else
    while (std::string::npos != pos && std::string::npos != last) {
      tokens.push_back(str.substr(last, pos - last));
      last = str.find_first_not_of(delimiters, pos);
      pos  = str.find_first_of(delimiters, last);
      if (std::string::npos == pos)
        pos = str.size();
    }
}

Tag DagMC::get_tag(const char* name, int size, TagType store,
                   DataType type, const void* def_value,
                   bool create_if_missing) {
  Tag retval = 0;
  unsigned flags = store | MB_TAG_CREAT;
  // NOTE: this function seems to be broken in that create_if_missing has
  // the opposite meaning from what its name implies.  However, changing the
  // behavior causes tests to fail, so I'm leaving the existing behavior
  // in place.  -- j.kraftcheck.
  if (!create_if_missing)
    flags |= MB_TAG_EXCL;
  ErrorCode result = MBI->tag_get_handle(name, size, type, retval, flags, def_value);
  if (create_if_missing && MB_SUCCESS != result)
    std::cerr << "Couldn't find nor create tag named " << name << std::endl;

  return retval;
}


} // namespace moab
