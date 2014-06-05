
#include "meshkit/MKCore.hpp"
#include "meshkit/SolidSurfaceMesher.hpp"
#include "meshkit/SolidCurveMesher.hpp"
#include "meshkit/SizingFunction.hpp"
#include "meshkit/ModelEnt.hpp"
#include "meshkit/Matrix.hpp"

#include "MBTagConventions.hpp"

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
  MKCore * mk;       // handle for the instance of MeshKit
  MEntVector surfs; // handle for the curve we need to retrieve, is a vector
  SolidSurfaceMesher * ssm;   // handle for our MeshOp that we will create

  mk = new MKCore();
  mk->load_geometry("cyl_grps.sat");

  mk->get_entities_by_dimension(2, surfs);
  ssm = (SolidSurfaceMesher*) mk->construct_meshop("SolidSurfaceMesher", surfs);
  ssm ->set_mesh_params(1e-3);

  mk->setup();
  mk->execute();

  markup_mesh(mk);

  mk->save_mesh("cyl_grps.h5m");

}

void markup_mesh(MKCore *mk)
{

  //create iMesh tag for categories
  iBase_TagHandle category_tag;
  mk->imesh_instance()->createTag(CATEGORY_TAG_NAME,CATEGORY_TAG_SIZE,iBase_BYTES, category_tag);

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

  //loop over the group sets
  for(unsigned int i = 0; i < gsets.size(); i++)
    {

      //get the mesh EntitySetHandle associated with this iGeom group set
      iMesh::EntitySetHandle msh;
      iRel::Error err = mk->group_set_pair()->getSetSetRelation(gsets[i], false, msh);

      //now apply the category tag to this entity set
      mk->imesh_instance()->setEntSetData(msh, category_tag, &geom_categories[4]);

    }
}
