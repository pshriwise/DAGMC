// MCNP5/dagmc/TallyEvent.cpp

#include "moab/CartVect.hpp"

#include "TallyEvent.hpp"

//---------------------------------------------------------------------------//
// CONSTRUCTOR
//---------------------------------------------------------------------------//
TallyEvent::TallyEvent(): event_type(NONE){}

//---------------------------------------------------------------------------//
// PUBLIC INTERFACE
//---------------------------------------------------------------------------//
// Create a new Tally with the implementation that calls this method
Tally *createTally(std::multimap<std::string, std::string>& options, 
                   unsigned int tally_id,
                   const std::vector<double>& energy_bin_bounds,
                   bool total_energy_bin)
{
	Tally *ret;
        TallyInput input; 

        input.options  = options;
	input.tally_id = tally_id;
        input.energy_bin_bounds = energy_bin_bounds;
        input.total_energy_bin  = total_energy_bin; 
        
        // Tally::create_tally is not static:  this does not compile
        ret = Tally::create_tally(input);
        return ret;
}

// Add a Tally  
void TallyEvent::addTally(int tally_index, Tally *obs)
{
        observers.insert(std::pair<int, Tally *>(tally_index, obs));   
}

// Remove a Tally - Observer pattern best practise
void TallyEvent::removeTally(int tally_index, Tally *obs)
{
        std::map<int, Tally *>::iterator it;	
 	it = observers.find(tally_index);
	observers.erase(it);
}

void TallyEvent::update_tallies()
{
       std::map<int, Tally*>::iterator map_it;
       for (map_it = observers.begin(); map_it != observers.end(); ++map_it)
       {
           Tally *tally = map_it->second;
	   tally->update();
       }
}

void TallyEvent::end_history()
{
       std::map<int, Tally*>::iterator map_it;
       for (map_it = observers.begin(); map_it != observers.end(); ++map_it)
       {
           Tally *tally = map_it->second;
	   tally->end_history();
       }
}

void TallyEvent::write_data()
{
       std::map<int, Tally*>::iterator map_it;
       for (map_it = observers.begin(); map_it != observers.end(); ++map_it)
       {
           Tally *tally = map_it->second;
	   tally->write_data();
       }
}

void TallyEvent::set_event(const moab::CartVect& position, 
                           const moab::CartVect& direction,
                           double track_length, double total_cross_section, 
                           double particle_energy, double particle_weight, 
                           EventType type)
{
    /// Set the particle state object
    particle.position            = position;
    particle.direction           = direction;
    particle.track_length        = track_length;
    particle.total_cross_section = total_cross_section;
    particle.energy              = particle_energy;
    particle.weight              = particle_weight;
 
    event_type = type;
}

//---------------------------------------------------------------------------//
void TallyEvent::set_tally_multiplier(double value)
{
    tally_multiplier = value;
    return;
}
//---------------------------------------------------------------------------//
// TALLY EVENT ACCESS METHODS
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
double TallyEvent::get_tally_multiplier() const
{
    return tally_multiplier;
}
//---------------------------------------------------------------------------//
double TallyEvent::get_weighting_factor() const
{
    return tally_multiplier * particle.weight;
}
//---------------------------------------------------------------------------//
TallyEvent::EventType TallyEvent::get_event_type() const
{
    return event_type;
}
//---------------------------------------------------------------------------//

// end of MCNP5/dagmc/TallyEvent.cpp