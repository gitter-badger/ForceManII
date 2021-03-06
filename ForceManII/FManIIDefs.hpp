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
#pragma once

#include <map>
#include <vector>
#include <set>
#include <memory>

namespace FManII {

///These are the recognized types of parameters
namespace Param_t {
    constexpr auto K="K";///<A force constant
    constexpr auto r0="r0";///<The equilibrium value
    constexpr auto amp="amp";///<The amplitude for Fourier series
    constexpr auto phi="phi";///<The phase shift for Fourier series
    constexpr auto n="n";///<The periodicity for Fourier series
    constexpr auto q="q";///<The charge, in a.u., for point-charge, point-charge
    constexpr auto sigma="sigma";///<The minimum diameter of a 6-12 potential
    constexpr auto epsilon="epsilon";///<The well depth of a 6-12 potential
}

///These are the recognized types of IntCoords
namespace IntCoord_t {
    constexpr auto BOND="BOND";///<A bond
    constexpr auto PAIR13="PAIR13";///<A 1,3 pair
    constexpr auto PAIR14="PAIR14";///<A 1,4 pair
    constexpr auto PAIR="PAIR";///<A pair that is not a 1,2; 1,3; or 1,4 pair
    constexpr auto ANGLE="ANGLE";///<An angle
    constexpr auto TORSION="TORSION";///<A torsion
    constexpr auto IMPTORSION="IMPTORSION";///<An improper torsion angle
}

///These are the recognized types of models
namespace Model_t{
    constexpr auto HARMONICOSCILLATOR="HARMONICOSCILLATOR";
    constexpr auto FOURIERSERIES="FOURIERSERIES";
    constexpr auto ELECTROSTATICS="ELECTROSTATICS";
    constexpr auto LENNARD_JONES="LENNARD_JONES";
}

///Flag for using the atom type or the atom class
namespace TypeTypes_t{
    constexpr auto TYPE="TYPE";///<The term uses the atom type
    constexpr auto CLASS="CLASS";///<The term uses the atom class
}

///For convenience these are most of the term types
namespace Terms_t{
    constexpr auto HO_BOND=std::make_pair(Model_t::HARMONICOSCILLATOR,
                                          IntCoord_t::BOND);
    constexpr auto HO_ANGLE=std::make_pair(Model_t::HARMONICOSCILLATOR,
                                           IntCoord_t::ANGLE);
    constexpr auto HO_PAIR13=std::make_pair(Model_t::HARMONICOSCILLATOR,
                                            IntCoord_t::PAIR13);
    constexpr auto HO_IMP=std::make_pair(Model_t::HARMONICOSCILLATOR,
                                            IntCoord_t::IMPTORSION);
    constexpr auto FS_TORSION=std::make_pair(Model_t::FOURIERSERIES,
                                             IntCoord_t::TORSION);
    constexpr auto FS_IMP=std::make_pair(Model_t::FOURIERSERIES,
                                         IntCoord_t::IMPTORSION);
    constexpr auto CL14=std::make_pair(Model_t::ELECTROSTATICS,
                                       IntCoord_t::PAIR14);
    constexpr auto CL=std::make_pair(Model_t::ELECTROSTATICS,
                                     IntCoord_t::PAIR);
    constexpr auto LJ14=std::make_pair(Model_t::LENNARD_JONES,
                                       IntCoord_t::PAIR14);
    constexpr auto LJ=std::make_pair(Model_t::LENNARD_JONES,
                                     IntCoord_t::PAIR);
}

///Type of a FFTerm
using FFTerm_t=std::pair<std::string,std::string>;


///Type of a quantity we are treating as a mathematical vector
using Vector=std::vector<double>;

///Shared pointer to a mathematical vector
using SharedVector=std::shared_ptr<Vector>;

///Shared pointer to a const mathematical vector
using cSharedVector=std::shared_ptr<const Vector>;

///Array of unsigned long integers
using IVector=std::vector<size_t>;

///Structure to hold the details of the molecular system
struct Molecule{
    ///The internal coordinates arranged by type
    std::map<std::string,Vector> coords;

    ///The Cartesian coordinates of the system
    cSharedVector carts;

    ///A list such that element i is the NAtoms associated with the i-th coord
    std::map<std::string,std::vector<IVector>> atom_numbers;
};

///Array such that element i is a vector of the atoms bonded to atom i
using ConnData=std::vector<std::set<size_t>>;

///Map from a ff term to its set of parameters
using ParamSet=std::map<FFTerm_t,std::map<std::string,Vector>>;

///An array of the requested derivatives sorted by force field term type
using DerivType=std::map<FFTerm_t,Vector>;


}//end namespace
