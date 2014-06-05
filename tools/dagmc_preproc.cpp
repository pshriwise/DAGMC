
#include "meshkit/MKCore.hpp"
#include "meshkit/SolidSurfaceMesher.hpp"
#include "meshkit/SolidCurveMesher.hpp"
#include "meshkit/SizingFunction.hpp"
#include "meshkit/ModelEnt.hpp"
#include "meshkit/Matrix.hpp"

#include "DagMC.hpp"

#include "MBTagConventions.hpp"
#include "moab/ProgOptions.hpp"

using namespace MeshKit;

const bool save_mesh = true;

#ifdef HAVE_ACIS
std::string extension = ".sat";
#elif HAVE_OCC
std::string extension = ".stp";
#endif

//function for adding DAGMC-specific tags to the mesh
void markup_mesh(MKCore *mk);

int main(int argc, char **argv) 
{

  ProgOptions po("dagmc_preproc: a tool for preprocessing CAD files for DAGMC analysis."); 

  std::string input_file; 
  std::string output_file = "dagmc_preproc_out.h5m";

  //add progrom options 
  po.addOpt<double>("ftol,f","Faceting distance tolerance", po.add_cancel_opt );
  po.addOpt<std::string>("outmesh,o", "Specify output file name (default "+output_file+")", &output_file); 
  po.addRequiredArg<std::string>("input_file", "Path to input file for preprocessing", &input_file); 
  
  po.parseCommandLine( argc, argv );


  MKCore * mk;       // handle for the instance of MeshKit
  MEntVector surfs; // handle for the curve we need to retrieve, is a vector
  SolidSurfaceMesher * ssm;   // handle for our MeshOp that we will create
  
  //set facet_tol 
  double facet_tol = 1e-04;
  po.getOpt( "ftol", &facet_tol);

  // this is typically the default value
  // should it be an option??
  double geom_resabs = 1e-6;

  mk = new MKCore();
  mk->load_geometry(input_file.c_str());

  mk->get_entities_by_dimension(2, surfs);
  ssm = (SolidSurfaceMesher*) mk->construct_meshop("SolidSurfaceMesher", surfs);
  ssm ->set_mesh_params(facet_tol, geom_resabs);

  mk->setup();
  mk->execute();

  //apply DAGMC tags to mesh
  markup_mesh(mk);

  //create faceting_tol_tag
  iBase_TagHandle facet_tol_tag; 
  mk->imesh_instance()->createTag("FACETING_TOL",1,iBase_DOUBLE, facet_tol_tag);

  //create the reabsorption tag 
  iBase_TagHandle geom_resabs_tag;
  mk->imesh_instance()->createTag("GEOMETRY_RESABS",1,iBase_DOUBLE, geom_resabs_tag);

  //create the file set
  iBase_EntitySetHandle file_set;
  mk->imesh_instance()->createEntSet(false,file_set);
  mk->imesh_instance()->setEntSetDblData(file_set,facet_tol_tag,facet_tol);
  mk->imesh_instance()->setEntSetDblData(file_set,geom_resabs_tag,geom_resabs);

  mk->save_mesh(output_file.c_str());

}

void markup_mesh(MKCore *mk)
{

  //create iMesh tag for categories
  iBase_TagHandle category_tag;
  mk->imesh_instance()->createTag(CATEGORY_TAG_NAME, CATEGORY_TAG_SIZE, iBase_BYTES, category_tag);

  //establish the geom categories
  char geom_categories[][CATEGORY_TAG_SIZE] = 
              {"Vertex\0", "Curve\0", "Surface\0", "Volume\0", "Group\0"};


  MEntVector ent_list;
  MEntVector::iterator eit;
  for(unsigned int dim = 0; dim < 4; dim++)
    {
      //clear out entity list 
      ent_list.clear();
      //get entities of dim
      mk->get_entities_by_dimension(dim,ent_list); 

      //tag each entity with the appropriate category tag value
      for(unsigned int i = 0; i< ent_list.size(); i++)
	{
	  //get the ModelEnt's iMesh handle 
          ModelEnt ent = *(ent_list[i]); 
	  iMesh::EntitySetHandle msh = IBSH(ent_list[i]->mesh_handle());
          
          //set the tag value
          mk->imesh_instance()->setEntSetData(msh, category_tag, &geom_categories[dim]);

	}
    }

  //do the same for groups... in the most annoying way possible 
  
  //get all igeom EntSets (groups)
  std::vector<iGeom::EntitySetHandle> gsets; 
  mk->igeom_instance()->getEntSets(mk->igeom_instance()->getRootSet(),-1,gsets);

  //create a tag for iGeom group names
  iGeom::TagHandle gname_tag;
  mk->igeom_instance()->createTag(NAME_TAG_NAME,NAME_TAG_SIZE,iBase_BYTES,gname_tag); 

  //create a tag for iMesh group names
  iMesh::TagHandle mname_tag; 
  mk->imesh_instance()->createTag(NAME_TAG_NAME,NAME_TAG_SIZE,iBase_BYTES,mname_tag); 


  //loop over the group sets
  for(unsigned int i = 0; i < gsets.size(); i++)
    {

      //get the mesh EntitySetHandle associated with this iGeom group set
      iMesh::EntitySetHandle msh;
      iRel::Error err = mk->group_set_pair()->getSetSetRelation(gsets[i], false, msh);

      //now apply the category tag to this entity set
      mk->imesh_instance()->setEntSetData(msh, category_tag, &geom_categories[4]);

      //print out the group names
      char grp_name[NAME_TAG_SIZE];
      mk->igeom_instance()->getEntSetData(gsets[i],gname_tag,grp_name);

      std::cout << "Group Name: " << grp_name << std::endl; 

      //set the mesh group set with this tag name
      mk->imesh_instance()->setEntSetData(msh,mname_tag,&grp_name);

    }
}
