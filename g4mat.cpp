#include "pyne.h"
#include "nucname.h"

#include "G4SystemOfUnits.hh"
#include "G4Material.hh"

#include "DagSolidMaterial.hpp"

#define PYNE_DATA "local_nuc.h5"

int main(int argc, char* argv[])
{
  
  std::string filename = "atic_uwuw_zip.h5m";

  // load the pyne nuclear data
  pyne::NUC_DATA_PATH=filename;

  // new material library
  std::map<std::string,pyne::Material> material_library;

  material_library = load_materials(filename);
 
  std::map<int,G4Isotope*> g4_isotopes;
  g4_isotopes = get_g4isotopes(material_library);

  // generate an element for each isotopes, this is so we need only create 
  // one element for each nuclide, this makes making G4 materials much easier
  std::map<int,G4Element*> g4_elements;
  g4_elements = get_g4elements(g4_isotopes);

  // from the element map, now generate G4Materials
  std::map<std::string,G4Material*> g4_materials;
  g4_materials = get_g4materials( g4_elements, material_library );

  // can now take the map of materials and as each dag volume is discovered,
  // discover the properties and then assign the appropriate volume

  return 0;
}

std::map<std::string,pyne::Material> load_materials(std::string filepath)
{
  bool end = false;
  std::map<std::string,pyne::Material> material_library;
  int i;
  pyne::Material mat; // from file
  while( !end )
    {
      mat.from_hdf5(filepath,"/materials",++i);
      if ( material_library.count(mat.metadata["name"].asString()) )
	{
	   end = true;  
	}
      material_library[mat.metadata["name"].asString()]=mat;
    }
  
  for(std::map<std::string,pyne::Material>::const_iterator it = material_library.begin() ; it != material_library.end() ; ++it )
    {
      std::cout << it->first <<  std::endl;
    }

  return material_library;
}

std::map<int,G4Isotope*> get_g4isotopes(std::map<std::string, pyne::Material> material_library)
{
  std::map<int,G4Isotope*> g4isotopes;
  pyne::Material sum_mat;

  // parameters for new isotope
  std::string name; // name of the isotope
  int iz, n; // atomic number, nucleon number
  double a; // atomic mass

  // loop over the materials present and produce a single new material which contains all the isotopes
  for(std::map<std::string,pyne::Material>::const_iterator it = material_library.begin() ; it != material_library.end() ; ++it )
    {
      sum_mat = sum_mat + (it->second);
    }

  // loop through the comp map of this material
  pyne::comp_iter mat_it;
  
  for ( mat_it = sum_mat.comp.begin() ; mat_it != sum_mat.comp.end() ; ++mat_it )
    {
      int nuc_id = mat_it->first;
      G4Isotope* new_iso = new G4Isotope(name=pyne::nucname::name(nuc_id),iz=pyne::nucname::znum(nuc_id), n=pyne::nucname::anum(nuc_id),
					 a=(pyne::atomic_mass(nuc_id)*g/mole));
      g4isotopes[nuc_id]=new_iso;    
    }
   
  return g4isotopes;
}

/*
 * return a nucid-wise map of nucid vs G4Element
 */
std::map<int,G4Element*> get_g4elements(std::map<int,G4Isotope*> isotope_map)
{
  std::map<int,G4Element*> element_map;
  std::map<int,G4Isotope*>::iterator it;

  std::string name, symbol; // element name and symbol
  int ncomponents ; // number of constituent isotopes
  double abundance; //abundance of each isotope
  for ( it = isotope_map.begin() ; it != isotope_map.end() ; ++it )
    {
      G4Element* tmp = new G4Element(name = pyne::nucname::name(it->first), 
				       symbol = pyne::nucname::name(it->first), 
				       ncomponents = 1);
      tmp->AddIsotope(isotope_map[it->first], abundance = 100.0*perCent);
      element_map[it->first]=tmp;
    }

  return element_map;
}

/*
 * From element list now generate materials
 */
std::map<std::string,G4Material*> get_g4materials(std::map<int,G4Element*> element_map,
						  std::map<std::string, pyne::Material> material_library)
{
  std::map<std::string, G4Material*> material_map;
  std::map<std::string, pyne::Material>::iterator it;
  pyne::comp_iter mat_it;

  std::string name;
  int ncomponents;

  // loop over the PyNE material library instanciating as needed
  for ( it = material_library.begin() ; it != material_library.end() ; ++it )
    {
      // get the material object
      pyne::Material mat = it->second;

      int num_nucs = 0;
      for ( mat_it = mat.comp.begin() ; mat_it != mat.comp.end() ; ++mat_it )
	{
	  if( (mat_it->second) > 0.0 )
	    {
	      num_nucs++;
	    }
	}


      // create the g4 material
      G4Material * g4mat = new G4Material(name = mat.metadata["name"].asString(),
					  mat.density*g/(cm*cm*cm), 
					  ncomponents = num_nucs);
      // now iterate over the composiiton
      for ( mat_it = mat.comp.begin() ; mat_it != mat.comp.end() ; ++mat_it )
	{
	  if( (mat_it->second) > 0.0 ) // strip out abundance 0 nuclides
	    {
	      g4mat->AddElement(element_map[mat_it->first], mat_it->second);
	    }
	}
      material_map[mat.metadata["name"].asString()]=g4mat;
    }
  
  G4cout << *(G4Material::GetMaterialTable());

  return material_map;
}
