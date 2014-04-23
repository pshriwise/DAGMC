// Simple file for loading geometry entities using meshkit

#include "meshkit/MKCore.hpp"
#include "meshkit/iGeom.hpp"
#include "MBTagConventions.hpp"
#include "meshkit/ModelEnt.hpp"
#include "meshkit/EdgeMesher.hpp"
#include "meshkit/TriangleMesher.hpp"
#include "meshkit/VertexMesher.hpp"
#include "meshkit/SizingFunction.hpp" 
#include "meshkit/iMesh.hpp"


using namespace MeshKit;


 int main(int argc, char **argv)
{


  MKCore *mk; // handle for the instance of MeshKit
  mk = new MKCore(); // start up MK

  moab::Interface *mb = mk->moab_instance(); // create moab instance 
  moab::ErrorCode rval; //establish moab error code


  iBase_ErrorType err;
  // attempt a geometry load

  //load a file that contains a single cube
  mk->load_geometry("cyl.sat",NULL,0,0,0,true,true);

  //get the curves
  MEntVector curves;
  mk->get_entities_by_dimension(1,curves);

  
  //create edge mesher
  EdgeMesher *em = (EdgeMesher*) mk->construct_meshop("EdgeMesher",curves);

  //create the sizing function for the facets
  SizingFunction sf(mk,-1,5);

  for(unsigned int i = 0; i < curves.size() ; i++)
    {
     curves[i]->sizing_function_index(sf.core_index());
    }

  //Mesh the curves
  mk->setup_and_execute();
  
  //create a new tag in moab for categories
  //moab::Tag category_tag;
  //rval = mb->tag_get_handle( CATEGORY_TAG_NAME, CATEGORY_TAG_SIZE, moab::MB_TYPE_OPAQUE, category_tag,  moab::MB_TAG_SPARSE|moab::MB_TAG_CREAT);
  //if (rval!=moab::MB_SUCCESS) return rval;

  //create iMesh tag for categories
  iBase_TagHandle category_tag;
  mk->imesh_instance()->createTag(CATEGORY_TAG_NAME,CATEGORY_TAG_SIZE,iBase_BYTES, category_tag);

  //get geom_tag handle
  iBase_TagHandle geom_tag; 
  mk->imesh_instance()->getTagHandle(GEOM_DIMENSION_TAG_NAME,geom_tag);

  //create faceting_tol_tag
  iBase_TagHandle facet_tol_tag; 
  mk->imesh_instance()->createTag("FACETING_TOL",1,iBase_DOUBLE, facet_tol_tag);
  double faceting_tol = 1e-3;

  iBase_EntitySetHandle file_set;
  mk->imesh_instance()->createEntSet(false,file_set);
  mk->imesh_instance()->setEntSetDblData(file_set,facet_tol_tag,faceting_tol);
 
  iBase_TagHandle geom_resabs_tag;
  mk->imesh_instance()->createTag("GEOMETRY_RESABS",1,iBase_DOUBLE, geom_resabs_tag);
  double geom_resabs = 1e-6;

  mk->imesh_instance()->setEntSetDblData(file_set,geom_resabs_tag,geom_resabs);

  std::vector<iBase_EntityHandle> all_ents;
  mk->imesh_instance()->getEntities(mk->imesh_instance()->getRootSet(),iBase_ALL_TYPES,iMesh_ALL_TOPOLOGIES,all_ents);

  mk->imesh_instance()->addEntArrToSet(&all_ents[0],all_ents.size(),file_set);


  //create iGeom tag
  iGeom::TagHandle cat_geom_tag;
  mk->igeom_instance()->createTag("CATEGORY",1,iBase_BYTES, cat_geom_tag);
 
  //establish the geom categories
  char geom_categories[][CATEGORY_TAG_SIZE] = 
              {"Vertex\0", "Curve\0", "Surface\0", "Volume\0", "Group\0"};


  //loop through all entities by dim
  MEntVector ent_list;
  MEntVector::iterator ei;
  for(int dim=0; dim<4; dim++)
    {
       ent_list.clear();
       mk->get_entities_by_dimension(dim,ent_list);
       if (dim ==4) std::cout<<ent_list.size()<<std::endl;
       for(unsigned int i=0; i<ent_list.size(); ++i)
         {
           //iMesh::EntitySetHandle h;
           ModelEnt ent = *(ent_list[i]);
	   iGeom::EntityHandle h = ent_list[i]->geom_handle();
           const void *dum = &geom_categories[dim];



           //iBase_EntityHandle mesh_set;
           //mk->irel_instance()->getEntSetRelation(h,false,mesh_set);
	   //std::cout<<ent_list[i]->iRelPairIndex()<<std::endl;
           
           	   
           //void *data = &geom_categories[dim];

           
           //rval = mb->tag_set_data(category_tag, &h, 1, &geom_categories[dim]);
           //if(rval!=moab::MB_SUCCESS) return rval;

	   //mk->igeom_instance()->setData(h, cat_geom_tag, &geom_categories[dim]);
         }
    }


  
  //try getting all the entities through igeom
  std::vector<iBase_EntitySetHandle> ent_set_handles;
  mk->imesh_instance()->getEntSets(mk->imesh_instance()->getRootSet(),1,ent_set_handles);



  for(unsigned int i=0; i<ent_set_handles.size(); i++)
    {
      int dim;
      mk->imesh_instance()->getEntSetIntData(ent_set_handles[i],geom_tag,dim); 
      std::cout << "Dimension of the EntSet: " << dim << std::endl;
      
      mk->imesh_instance()->setEntSetData(ent_set_handles[i],category_tag, &geom_categories[dim]);
    }


  std::vector<iBase_EntityHandle> ent_handles;
  mk->imesh_instance()->getEntities(mk->imesh_instance()->getRootSet(), iBase_VERTEX, iMesh_ALL_TOPOLOGIES, ent_handles);






  //print the types of the entity sets

  std::cout << "Number of entity sets " << ent_set_handles.size() << std::endl;
  std::cout << "Number of entities " << ent_handles.size() << std::endl;



  mk->save_mesh("cyl.h5m");
  std::cout << "Success!" << std::endl;  
  return 0;  
}
