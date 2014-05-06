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

//ADD SENSE TAG NAMES
const char GEOM_SENSE_2_TAG_NAME[] = "GEOM_SENSE_2";
const char GEOM_SENSE_N_ENTS_TAG_NAME[] = "GEOM_SENSE_N_ENTS";
const char GEOM_SENSE_N_SENSES_TAG_NAME[] = "GEOM_SENSE_N_SENSES"; 

using namespace MeshKit;


void create_entity_sets( MKCore *mk_iface, iBase_EntitySetHandle root_set, std::map<iGeom::EntityHandle,iMesh::EntitySetHandle> (&entmap)[5])
{
  //establish the geom categories
  char geom_categories[][CATEGORY_TAG_SIZE] = 
              {"Vertex\0", "Curve\0", "Surface\0", "Volume\0", "Group\0"};

  //establish the types of entities to get for each dim 
  iBase_EntityType types[4]={iBase_VERTEX,iBase_EDGE,iBase_FACE,iBase_REGION};

  //create iMesh tag for categories
  iBase_TagHandle category_tag;
  mk_iface->imesh_instance()->createTag(CATEGORY_TAG_NAME,CATEGORY_TAG_SIZE,iBase_BYTES, category_tag);

  //create iMesh tag for geom_dim
  iBase_TagHandle geom_tag; 
  mk_iface->imesh_instance()->createTag(GEOM_DIMENSION_TAG_NAME,1,iBase_INTEGER, geom_tag);

  //get the iGeom handle for the global_id
  iBase_TagHandle id_tag;
  mk_iface->igeom_instance()->getTagHandle(GLOBAL_ID_TAG_NAME,id_tag);

  //create an id tag for the meshsets
  iBase_TagHandle mesh_id_tag;
  mk_iface->imesh_instance()->createTag(GLOBAL_ID_TAG_NAME,1,iBase_INTEGER,mesh_id_tag);


  std::vector<iBase_EntityHandle> ent_list;
  for(unsigned int dim=0; dim<4; dim++)
    {
      //remove previous Entities from list
      ent_list.clear();
      //get the geom entities for this dimension
      mk_iface->igeom_instance()->getEntities(root_set,types[dim],ent_list);

      //loop over entities in this dimension
      for(unsigned int i=0; i<ent_list.size(); i++)
	{

          //create a new meshset
          iMesh::EntitySetHandle h;
          mk_iface->imesh_instance()->createEntSet(false, h);

          //ADD TAGS

          //set the geom_dim tag
          mk_iface->imesh_instance()->setEntSetIntData(h,geom_tag,dim);

          //set the global id tag
          int id; 
          mk_iface->igeom_instance()->getIntData(ent_list[i],id_tag,id);
          mk_iface->imesh_instance()->setEntSetIntData(h,mesh_id_tag,id);

          //set the category tag
          mk_iface->imesh_instance()->setEntSetData(h, category_tag, &geom_categories[dim]);

          //create the reference to this meshset in the map
          entmap[dim][ent_list[i]]=h;


	}


    }
}

void store_surface_senses(MKCore *mk_iface, std::map<iGeom::EntityHandle, iMesh::EntitySetHandle> (&entmap)[5])
{

  //surface to volume tag
  iMesh::TagHandle sense2Tag;
  mk_iface->imesh_instance()->createTag(GEOM_SENSE_2_TAG_NAME,2,iBase_ENTITY_SET_HANDLE, sense2Tag);

  std::map<iGeom::EntityHandle,iMesh::EntitySetHandle>::iterator map_it; 

  for(map_it = entmap[2].begin(); map_it != entmap[2].end(); ++map_it)
    {

      //the geometry entity
      iGeom::EntityHandle gh = map_it->first;
      
      //the corresponding meshset
      iMesh::EntitySetHandle msh = map_it->second;

      //get the surface's volume handle 
      std::vector<iGeom::EntityHandle> vols; 
      mk_iface->igeom_instance()->getEntAdj(gh,iBase_REGION,vols);
      std::cout << "Number of adjacent vols: " << vols.size() << std::endl;
      

      //create structure for setting the meshset sense info
      iMesh::EntitySetHandle sense_data[2]={ 0, 0};

      //loop over volumes
      for(unsigned int i = 0; i<vols.size(); i++)
	{
	  //get the sense of the volume
          int sense;
          mk_iface->igeom_instance()->getEntNrmlSense(gh,vols[i],sense);
          
          //check that the forward sense isn't already set
          if (1 == sense && 0 != sense_data[0])
	    std::cout << "Multiple volumes found for forward sense!!" << std::endl;

          //set the forward entity to the mesh set handle for this geom entity
          if(1 == sense && 0 == sense_data[0]) sense_data[0]=entmap[3][vols[i]];

          //check that the reverse sense isn't already set
          if (-1 == sense && 0 != sense_data[1])
	  std::cout << "Multiple volumes found for forward sense!!" << std::endl;

          // set the reverse entity to the mesh set handle for this geom entity
          if(-1 == sense && 0 == sense_data[1]) sense_data[1]=entmap[3][vols[i]];
	}

      //now set the meshset tag info
      mk_iface->imesh_instance()->setEntSetData(msh,sense2Tag,sense_data);

    }


}

void create_topology( MKCore *mk_iface, std::map<iGeom::EntityHandle,iMesh::EntitySetHandle> (&entmap)[5])
{

  //create iterator for the map
  std::map<iGeom::EntityHandle,iMesh::EntitySetHandle>::iterator map_it;

  //establish the types of entities to get for each dim 
  iBase_EntityType types[4]={iBase_VERTEX,iBase_EDGE,iBase_FACE,iBase_REGION};

  for(unsigned int dim = 3; dim>0; dim--)
    {
      //CREATE SET-SET RELATIONS
      for(map_it=entmap[dim].begin(); map_it!=entmap[dim].end(); ++map_it)
        {

          // get the meshset for the this dimension
          iBase_EntitySetHandle dim_set= map_it->second;

          // get child entitties of the current dim entitiy
          std::vector<iBase_EntityHandle> lower_ents;
          mk_iface->igeom_instance()->getEntAdj(map_it->first,types[dim-1],lower_ents);

          for(unsigned int i=0; i<lower_ents.size(); i++)
            {
              //add the surface meshset to the vol meshset
              mk_iface->imesh_instance()->addPrntChld(dim_set,entmap[dim-1][lower_ents[i]]);
            }
    
        }
    }
}

void store_curve_senses( MKCore *mk_iface, std::map<iGeom::EntityHandle,iMesh::EntitySetHandle> (&entmap)[5])
{

 
  std::map<iGeom::EntityHandle,iMesh::EntitySetHandle>::iterator map_it;

  //loop over all of the curves
  for(map_it = entmap[1].begin(); map_it != entmap[1].end(); ++map_it)
    {
      //the geometry entity
      iGeom::EntityHandle gh = map_it->first;
 
      //the corresponding meshset
      iMesh::EntitySetHandle msh = map_it->second;

      //get the all surfaces adjacent to the curve
      std::vector<iGeom::EntityHandle> surfs; 
      mk_iface->igeom_instance()->getEntAdj(gh,iBase_FACE,surfs);
      std::cout << "Number of adjacent surfaces: " << surfs.size() << std::endl;
 
      //This requires two tags
      //One for the EntitySets
      iBase_TagHandle senseNEnts;
      mk_iface->imesh_instance()->createTag(GEOM_SENSE_N_ENTS_TAG_NAME,surfs.size(),iBase_ENTITY_SET_HANDLE, senseNEnts);

      //One for the senses
      iBase_TagHandle senseNSenses;
      mk_iface->imesh_instance()->createTag(GEOM_SENSE_N_SENSES_TAG_NAME,surfs.size(),iBase_INTEGER,senseNSenses);


      //vector to keep track of the senses with 
      std::vector<int> senses;
      std::vector<iMesh::EntitySetHandle> meshsets;

      //loop over the surfaces
      for(unsigned int i=0; i<surfs.size(); i++)
	{

          //get the curve to surface sense
          int sense; 
          mk_iface->igeom_instance()->getEgFcSense(gh,surfs[i],sense);
          senses.push_back(sense);

          //populate the meshset vector 
          meshsets.push_back(entmap[2][surfs[i]]);
	}

      mk_iface->imesh_instance()->setEntSetData(msh,senseNSenses,&senses[0]);
 
      mk_iface->imesh_instance()->setEntSetData(msh,senseNEnts,&meshsets[0]);
   
      meshsets.clear();
      senses.clear();

    }


}


 int main(int argc, char **argv)
{

  MKCore *mk; // handle for the instance of MeshKit
  mk = new MKCore(); // start up MK

  // Load the geometry file
  mk->igeom_instance()->load("cylcube.sat");

  //get the igeom rootset
  iBase_EntitySetHandle root = mk->igeom_instance()->getRootSet();
  //create faceting_tol_tag
  iBase_TagHandle facet_tol_tag; 
  mk->imesh_instance()->createTag("FACETING_TOL",1,iBase_DOUBLE, facet_tol_tag);
  double faceting_tol = 1e-3;

  //create the reabsorption tag 
  iBase_TagHandle geom_resabs_tag;
  mk->imesh_instance()->createTag("GEOMETRY_RESABS",1,iBase_DOUBLE, geom_resabs_tag);
  double geom_resabs = 1e-6;

  //create the file set
  iBase_EntitySetHandle file_set;
  mk->imesh_instance()->createEntSet(false,file_set);
  mk->imesh_instance()->setEntSetDblData(file_set,facet_tol_tag,faceting_tol);
  mk->imesh_instance()->setEntSetDblData(file_set,geom_resabs_tag,geom_resabs);

  //create a map to hold relations of geom entities to meshsets
  std::map<iGeom::EntityHandle,iMesh::EntitySetHandle>entmap[5];

  // See how many entity sets exist after loading
  std::vector<iBase_EntityHandle> ents;
  mk->igeom_instance()->getEntities(root,iBase_ALL_TYPES, ents);
  std::cout << "Number of Entities: " << ents.size() << std::endl;

  //create iMesh tag for categories
  iBase_TagHandle category_tag;
  mk->imesh_instance()->createTag(CATEGORY_TAG_NAME,CATEGORY_TAG_SIZE,iBase_BYTES, category_tag);

  //establish the geom categories
  char geom_categories[][CATEGORY_TAG_SIZE] = 
              {"Vertex\0", "Curve\0", "Surface\0", "Volume\0", "Group\0"};


  //create iMesh tag for geom_dim
  iBase_TagHandle geom_tag; 
  mk->imesh_instance()->createTag(GEOM_DIMENSION_TAG_NAME,1,iBase_INTEGER, geom_tag);

  //get the iGeom handle for the global_id
  iBase_TagHandle id_tag;
  mk->igeom_instance()->getTagHandle(GLOBAL_ID_TAG_NAME,id_tag);

  //create an id tag for the meshsets
  iBase_TagHandle mesh_id_tag;
  mk->imesh_instance()->createTag(GLOBAL_ID_TAG_NAME,1,iBase_INTEGER,mesh_id_tag);

  create_entity_sets(mk, root, entmap);

  //create a map iterator for the ent_sets
  std::map<iGeom::EntityHandle,iMesh::EntitySetHandle>::iterator map_it;

  //loop over the vertex entities
  for(map_it = entmap[0].begin(); map_it != entmap[0].end(); ++map_it)
    {
      //get the geom entity handle from the map
      iBase_EntityHandle gh = map_it->first;
 
      //get the mesh set handle from the map
      iBase_EntitySetHandle msh=map_it->second;

      //get the vertex coordinates
      double x,y,z;

      mk->igeom_instance()->getVtxCoord(gh,x,y,z);
      //create a new mesh vertex 
      iBase_EntityHandle vertex;
      mk->imesh_instance()->createVtx(x,y,z,vertex);

      // add this vertex to the new meshset
      mk->imesh_instance()->addEntToSet(vertex,msh);
     
    }
  
  //loop through the reference edges
  for(map_it=entmap[1].begin(); map_it!=entmap[1].end(); ++map_it)
    {

        //get the geom entity handle from the map
      iBase_EntityHandle gh = map_it->first;
 
      //get the mesh set handle from the map
      iBase_EntitySetHandle msh=map_it->second;

      //get the facets for this curve/ ref_edge
      std::vector<double> pnts;
      std::vector<int> conn;
      mk->igeom_instance()->getFacets(gh,faceting_tol,pnts,conn);
      std::cout << "Points returned from getFacets: " << pnts.size()/3 << std::endl;
      std::cout << "Facets returned from getFacets: " << conn.size() << std::endl;
      //create vector for keeping track of the vertices
      std::vector<iBase_EntityHandle> verts;

      // loop over the facet points
      for(unsigned int j=0; j<pnts.size(); j+=3)
	{


	  //create a new vertex handle 
          iBase_EntityHandle v;
          mk->imesh_instance()->createVtx(pnts[j],pnts[j+1],pnts[j+2],v);
          verts.push_back(v);

	}
       
      //vector to contain the edges
      std::vector<iBase_EntityHandle> edges;
      //loop over vertices and create edges
      for (unsigned int j=0; j<verts.size()-1; j++)
	{
	  //create edges
          iBase_EntityHandle e; 
          mk->imesh_instance()->createEnt(iMesh_LINE_SEGMENT, &verts[j],2,e);
	  edges.push_back(e);
	}
 
      //add verticess and edges to the entity set
      mk->imesh_instance()->addEntArrToSet(&verts[0],verts.size(),msh);
      mk->imesh_instance()->addEntArrToSet(&edges[0],edges.size(),msh);

    }
 
  //loop over all the faces
  for(map_it=entmap[2].begin(); map_it!=entmap[2].end(); ++map_it)
    {
        //get the geom entity handle from the map
      iBase_EntityHandle gh = map_it->first;
 
      //get the mesh set handle from the map
      iBase_EntitySetHandle msh=map_it->second;

      //get the facets for this surface/ ref_face
      std::vector<double> pnts;
      std::vector<int> conn;
      mk->igeom_instance()->getFacets(gh,faceting_tol,pnts,conn);
      std::cout << "Triangles returned from getFacets: " << pnts.size()/3 << std::endl;
      std::cout << "Facets returned from getFacets: " << conn.size()/3 << std::endl;
  
      //create vector for keeping track of the vertices
      std::vector<iBase_EntityHandle> verts;

      // loop over the facet points
      for(unsigned int j=0; j<pnts.size(); j+=3)
	{

	  //create a new vertex handle 
          iBase_EntityHandle v;
          mk->imesh_instance()->createVtx(pnts[j],pnts[j+1],pnts[j+2],v);
          verts.push_back(v);
	}
      
      //vector for keeping track of the triangles
      std::vector<iBase_EntityHandle> tris;

      //loop over the connectivity
      for(unsigned int j=0; j<conn.size()-2; j+=3)
	{
          //get the appropriate points for a triangle and add them to a vector
	  std::vector<iBase_EntityHandle> tri_verts; 
          tri_verts.push_back(verts[conn[j]]);
          tri_verts.push_back(verts[conn[j+1]]);
          tri_verts.push_back(verts[conn[j+2]]);

	  //create a new triangle handle
          iBase_EntityHandle t; 
          mk->imesh_instance()->createEnt(iMesh_TRIANGLE, &tri_verts[0],3,t);
          tri_verts.clear();
	  tris.push_back(t);

        }
      std::cout << "Created " << tris.size() << " triangles" << std::endl;

      //add verticess and edges to the entity set
      mk->imesh_instance()->addEntArrToSet(&verts[0],verts.size(),msh);
      mk->imesh_instance()->addEntArrToSet(&tris[0],tris.size(),msh);
    }


  //CREATE Geom ENT to Mesh ENTSET RELATIONS
  create_topology(mk, entmap);

  //Get and store the surface to volume sense data
  store_surface_senses(mk, entmap);


  //Get and store the curve to surface sense data
  store_curve_senses(mk, entmap);

  //check how many curves are in the file
  std::vector<iBase_EntityHandle> all_faces;
  mk->igeom_instance()->getEntities(mk->igeom_instance()->getRootSet(),iBase_FACE,all_faces);
  std::cout << "There are " << all_faces.size() << " faces in the model." << std::endl;

  std::vector<iBase_EntityHandle> adj_faces;
  map_it = entmap[3].begin();
  mk->igeom_instance()->getEntAdj(map_it->first, iBase_FACE, adj_faces);
  std::cout << adj_faces.size() << " faces are adjacent to this volume." << std::endl;

  //write the file
  mk->imesh_instance()->save(root,"cylcube.h5m");

  std::cout << "Success!" << std::endl;
  return 0;
}


