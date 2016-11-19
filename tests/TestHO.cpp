/*
 * Copyright (C) 2016 Ryan M. Richard <ryanmrichard1 at gmail.com>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 */

#include <ForceManII/FFTerm.hpp>
#include <ForceManII/HarmonicOscillator.hpp>
#include "TestMacros.hpp"
#include "ubiquitin.hpp"
#include <cmath>

int main(int argc, char** argv){
    test_header("Testing Harmonic Oscillator force-field term");
    FManII::HarmonicOscillator HO;
    
#ifndef NDEBUG
std::vector<double> a(3,0.0),b(2,0.0),c;
TEST_THROW(c=HO.deriv(a,b),"bond.size()==ks.size()");
#endif

     FManII::CoordArray coords=FManII::get_coords(ubiquitin,ubiquitin_FF_types,
            ubiquitin_FF_params,ubiquitin_conns);
     const std::vector<double>& bonds=coords[FManII::BOND]->values();
     const std::vector<double>& bond_k=coords[FManII::BOND]->params(FManII::K);
     const std::vector<double>& angles=coords[FManII::ANGLE]->values();
     const std::vector<double>& angle_k=coords[FManII::ANGLE]->params(FManII::K);
     
    //Energy check
    std::vector<double> Energy=HO.deriv(bonds,bond_k);
    test_value(Energy[0],ubiquitinbond_e,1e-5,"Bond Energy");
    
    Energy=HO.deriv(angles,angle_k);
    test_value(Energy[0],ubiquitinangle_e,1e-5,"Angle Energy");
    
    test_footer();
    return 0;
} //End main

