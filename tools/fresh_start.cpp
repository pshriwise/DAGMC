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

  // Load the geometry file
  mk->igeom_instance()->load("cyl_groups.sat");

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


  //create a map to hold relations of geom entities to meshsets
  std::map<iGeom::EntityHandle,iMesh::EntitySetHandle>entmap[5];

  // See how many entity sets exist after loading
  std::vector<iBase_EntityHandle> ents;
  mk->igeom_instance()->getEntities(root,iBase_ALL_TYPES, ents);
  std::cout << "Number of Entities: " << ents.size() << std::endl;

  //create iMesh tag for categories
  iBase_TagHandle category_tag;
  mk->imesh_instance()->createTag(CATEGORY_TAG_NAME,CATEGORY_TAG_SIZE,iBase_BYTES, category_tag);

  //create iMesh tag for geom_dim
  iBase_TagHandle geom_tag; 
  mk->imesh_instance()->createTag(GEOM_DIMENSION_TAG_NAME,1,iBase_INTEGER, geom_tag);

  //get the iGeom handle for the global_id
  iBase_TagHandle id_tag;
  mk->igeom_instance()->getTagHandle(GLOBAL_ID_TAG_NAME,id_tag);

  //create an id tag for the meshsets
  iBase_TagHandle mesh_id_tag;
  mk->imesh_instance()->createTag(GLOBAL_ID_TAG_NAME,1,iBase_INTEGER,mesh_id_tag);

  //get the vertices 
  ents.clear();
  mk->igeom_instance()->getEntities(root,iBase_VERTEX, ents);
  std::cout << "There are " << ents.size() << " vertices." << std::endl;

  //loop over all vertices
  for(unsigned int i=0; i<ents.size(); i++)
    {
      //create a new meshset
      iMesh::EntitySetHandle h;
      mk->imesh_instance()->createEntSet(false, h);

      //set the geom_dim tag
      mk->imesh_instance()->setEntSetIntData(h,geom_tag,0);

      //set the global id tag
      int id; 
      mk->igeom_instance()->getIntData(ents[i],id_tag,id);
      mk->imesh_instance()->setEntSetIntData(h,mesh_id_tag,id);
      
      //get the vertex coordinates
      double x,y,z;
      mk->igeom_instance()->getVtxCoord(ents[i],x,y,z);
      //create a new mesh vertex 
      iBase_EntityHandle vertex;
      mk->imesh_instance()->createVtx(x,y,z,vertex);

      // add this vertex to the new meshset
      mk->imesh_instance()->addEntToSet(vertex,h);
     
      //create the reference to this meshset in the map
      entmap[0][ents[i]]=h;

    }
  

  //now get the curves
  ents.clear();
  mk->igeom_instance()->getEntities(root,iBase_EDGE,ents);
  std::cout << "There are " << ents.size() << " ref edges." << std::endl;

  //loop through the reference edges
  for(unsigned int i=0; i<ents.size(); i++)
    {

      //create a new meshset
      iMesh::EntitySetHandle h;
      mk->imesh_instance()->createEntSet(false, h);

      //set the geom_dim tag
      mk->imesh_instance()->setEntSetIntData(h,geom_tag,1);

      //set the global id tag
      int id; 
      mk->igeom_instance()->getIntData(ents[i],id_tag,id);
      mk->imesh_instance()->setEntSetIntData(h,mesh_id_tag,id);

      //get the facets for this curve/ ref_edge
      std::vector<double> pnts;
      std::vector<int> conn;
      mk->igeom_instance()->getFacets(ents[i],faceting_tol,pnts,conn);
      std::cout << "Points returned from getFacets: " << pnts.size()/3 << std::endl;

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
      mk->imesh_instance()->addEntArrToSet(&verts[0],verts.size(),h);
      mk->imesh_instance()->addEntArrToSet(&edges[0],edges.size(),h);

      //create the reference to this meshset in the map
      entmap[1][ents[i]]=h;
    }

  //get all the faces
  ents.clear();
  mk->igeom_instance()->getEntities(root,iBase_FACE,ents);
  std::cout << "There are " << ents.size() << " faces." << std::endl;
 
  //loop over all the faces
  for(unsigned int i=0; i<ents.size(); i++)
    {
        //create a new meshset
      iMesh::EntitySetHandle h;
      mk->imesh_instance()->createEntSet(false, h);

      //set the geom_dim tag
      mk->imesh_instance()->setEntSetIntData(h,geom_tag,2);

      //set the global id tag
      int id; 
      mk->igeom_instance()->getIntData(ents[i],id_tag,id);
      mk->imesh_instance()->setEntSetIntData(h,mesh_id_tag,id);

      //get the facets for this surface/ ref_face
      std::vector<double> pnts;
      std::vector<int> conn;
      mk->igeom_instance()->getFacets(ents[i],faceting_tol,pnts,conn);

  
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
      //loop over the verts and create a triangle for every three verts
      
      for(unsigned int j=0; j<verts.size()-2; j+=3)
	{
	  //create a new triangle handle
          iBase_EntityHandle t; 
          mk->imesh_instance()->createEnt(iMesh_TRIANGLE, &verts[j],3,t);
	  tris.push_back(t);
        }
      std::cout << "Created " << tris.size() << " triangles" << std::endl;

      //add verticess and edges to the entity set
      mk->imesh_instance()->addEntArrToSet(&verts[0],verts.size(),h);
      mk->imesh_instance()->addEntArrToSet(&trisx[0],tris.size(),h);

      //create the reference to this meshset in the map
      entmap[2][ents[i]]=h;
    }


  //write the file
  mk->imesh_instance()->save(root,"cyl.h5m");

  

  std::cout << "Success!" << std::endl;
  return 0;
}


  /*
  //print out entity dimensions
  iBase_EntityType type;
  for(unsigned int i=0; i<ents.size() ;i ++)
    {
      mk->igeom_instance()->getEntType(ents[i],type);
      std::cout << "This entity is of type: " << type << std::endl;
      std::vector<iBase_TagHandle> tgs;
      mk->igeom_instance()->getAllTags(ents[i],tgs);
      std::cout<< "This entity has " << tgs.size() << " tags." << std::endl;

      if (type ==0)
	{ 
	  std::cout << "-------------------------" << std::endl;
	  std::cout << "VERTEX " << i << std::endl;
	  std::cout << "-------------------------" << std::endl;
          double x,y,z;
	  mk->igeom_instance()->getVtxCoord(ents[i],x,y,z);
	  std::cout << "Position of the vertex is: " << x << " " << y << " " << z <<  std::endl;
	  std::vector<iBase_TagHandle> tags;
          mk->igeom_instance()->getAllTags(ents[i],tags);
	  std::cout << "This vertex has " << tags.size() << " tags" << std::endl;
	  std::cout << "-------------------------" << std::endl;
	  std::cout << "Tag 1:" << std::endl;
	  std::cout << "-------------------------" << std::endl;
	  std::string name;
          mk->igeom_instance()->getTagName(tags[0],name);
	  std::cout << "Name: " << name << std::endl;
	  int name_value[NAME_TAG_SIZE];
          mk->igeom_instance()->getData(ents[i],tags[0], name_value);
	  std::cout << "Value: " << name_value[0] << std::endl;

  	  std::cout << "-------------------------" << std::endl;
	  std::cout << "Tag 2:" << std::endl;
	  std::cout << "-------------------------" << std::endl;
          mk->igeom_instance()->getTagName(tags[1],name);
	  std::cout << "Name: " << name << std::endl;
          int id;
          mk->igeom_instance()->getIntData(ents[i],tags[1],id);
	  std::cout << "Value: " << id << std::endl;

	}
    }
  */
