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
#include "ForceManII/FManII.hpp"
#include "ForceManII/Common.hpp"
#include "ForceManII/Util.hpp"
#include "ForceManII/InternalCoords/Distance.hpp"
#include "ForceManII/InternalCoords/Angle.hpp"
#include "ForceManII/InternalCoords/Torsion.hpp"
#include "ForceManII/InternalCoords/ImproperTorsion.hpp"
#include <iostream>
#include <algorithm>

namespace FManII {

CoordArray get_coords(const Vector& Carts,
                      const ConnData& Conns){
    const size_t NAtoms=Carts.size()/3;
    DEBUG_CHECK(NAtoms==Conns.size(),"Number of atoms differs among inputs");
    auto Sys=std::make_shared<Vector>(Carts);
    
    CoordArray FoundCoords;
    FoundCoords.emplace(IntCoord_t::BOND,std::move(make_unique<Distance>(Sys)));
    FoundCoords.emplace(IntCoord_t::ANGLE,std::move(make_unique<Angle>(Sys)));
    FoundCoords.emplace(IntCoord_t::TORSION,std::move(make_unique<Torsion>(Sys)));
    FoundCoords.emplace(IntCoord_t::IMPTORSION,std::move(make_unique<ImproperTorsion>(Sys)));
    FoundCoords.emplace(IntCoord_t::PAIR,std::move(make_unique<Distance>(Sys)));
    FoundCoords.emplace(IntCoord_t::PAIR14,std::move(make_unique<Distance>(Sys)));
    std::set<std::pair<size_t,size_t>> pair14,pair13,pair12;
    for(size_t AtomI=0;AtomI<NAtoms;++AtomI){
        for(size_t AtomJ : Conns[AtomI]){
            DEBUG_CHECK(AtomI!=AtomJ,"AtomI is bonded to itself. What??");
            for(size_t AtomK: Conns[AtomJ]){
                DEBUG_CHECK(AtomK!=AtomJ,"Atom J is bonded to itself. What??");
                if(AtomK==AtomI)continue;//Went backwards in the graph
                for(size_t AtomL : Conns[AtomK]){
                    DEBUG_CHECK(AtomL!=AtomK,"Atom K is bonded to itself");
                    if(AtomL==AtomJ)continue;//Went backwards
                    if(AtomL>AtomI){
                        pair14.insert(std::make_pair(AtomI,AtomL));
                    }
                    if(AtomK<AtomJ)continue;
                    FoundCoords[IntCoord_t::TORSION]->add_coord({AtomI,AtomJ,AtomK,AtomL});
                }//Close AtomL
                if(AtomK<AtomI)continue;//Avoid 2x counting angle
                pair13.insert(std::make_pair(AtomI,AtomK));
                FoundCoords[IntCoord_t::ANGLE]->add_coord({AtomI,AtomJ,AtomK});
                if(Conns[AtomJ].size()==3){
                    for(size_t AtomL: Conns[AtomJ]){
                        if(AtomL==AtomK||AtomL==AtomI||AtomL<AtomK)continue;
                        FoundCoords[IntCoord_t::IMPTORSION]->add_coord({AtomI,AtomJ,AtomK,AtomL});
                    }//Close AtomL imptorsion
                }//Close if imptorsion
            }//Close Atom K
            if(AtomJ<AtomI)continue; //Avoid 2x counting bond
            pair12.insert(std::make_pair(AtomI,AtomJ));
            FoundCoords[IntCoord_t::BOND]->add_coord({AtomI,AtomJ});
        }//Close Atom J     
    }//Close AtomI
    for(size_t AtomI=0;AtomI<NAtoms;++AtomI){
        for(size_t AtomJ=AtomI+1;AtomJ<NAtoms;++AtomJ){
            auto pair=std::make_pair(AtomI,AtomJ);
            if(pair12.count(pair))continue;
            const bool is13=pair13.count(pair),is14=pair14.count(pair);
            IntCoord_t ctype=IntCoord_t::PAIR;
            if(is13)ctype=IntCoord_t::PAIR13;
            else if(is14)ctype=IntCoord_t::PAIR14;
            FoundCoords[ctype]->add_coord({AtomI,AtomJ});
        }
    }
    return FoundCoords;
}

ParamSet assign_params(const CoordArray& coords,
                       const ForceField& ff,
                       const IVector& Types)
 {
    ParamSet ps;
    for(const auto& termi:ff.terms){//Loop over terms in force field
        const FFTerm_t term_type=termi.first;
        const Model_t model=term_type.first;
        const bool use_class=ff.paramtypes.at(term_type)==TypeTypes_t::CLASS;
        for(const auto& coordi:termi.second.coords){
            const InternalCoordinates& coord=*coords.at(coordi);
            for(IVector typei:coord.get_types()){
                IVector types;
                for(size_t t:typei){
                    const size_t ti=Types[t];
                    types.push_back(use_class?ff.type2class.at(ti):ti);
                }
                IVector Orderedts=(ff.orderrules.count(term_type)?
                                   ff.orderrules.at(term_type)(types):types);
                for(auto parami:termi.second.model().params){
                    auto prule=std::make_pair(model,parami);
                    if(ff.combrules.count(prule)){
                       const CombRule_t rule=ff.combrules.at(prule);
                       double val;
                        if(rule==CombRule_t::ARITHMETIC){
                            val=0.0;
                            for(auto ti:types)
                                val+=ff.params.at(term_type).at(parami).at({ti})[0];
                            val/=types.size();
                        }
                        else{
                            val=1.0;
                            for(auto ti:types)
                                val*=ff.params.at(term_type).at(parami).at({ti})[0];
                            if(rule==CombRule_t::GEOMETRIC)
                                val=std::pow(val,(1.0/types.size()));
                        }
                        ps[term_type][parami].push_back(val);
                    }
                    else{
                        const Vector& vs=
                                ff.params.at(term_type).at(parami).at(Orderedts);
                        if(term_type.first==Model_t::FOURIERSERIES)
                            for(size_t i=0;i<3;i++)
                                ps[term_type][parami].push_back(vs.size()>i?
                                                                    vs[i]:0.0);
                        else{
                            DEBUG_CHECK(vs.size()==1,"Wasn't expecting a vector");
                            ps[term_type][parami].push_back(vs[0]);
                        }
                    }
                }//End loop over parameters
            }//End loop over values of coordinate
        }//End loop over coordinates
    }//End loop over terms
    return ps;
}

std::map<FFTerm_t,Vector> deriv(size_t order,
                                const ForceField& ff,
                                const CoordArray& coords)
{
    std::map<FFTerm_t,Vector> rv;
//    for(const auto& i:ff.terms){
//        const FFTerm_t term_type=i.first;
//        const FFTerm& ffterm=i.second;
//        std::vector<Vector> cs,ps;
//        for(auto ci:ffterm.coords){
//            cs.push_back(coords.at(ci)->get_coords());
//            for(auto pi:ffterm.model().params)
//                ps.push_back(coords.at(ci)->get_params(pi));
//        }
//        Vector d=ffterm.deriv(order,ps,cs);
//        if(ff.scale_factors.count(term_type))
//            for(double& di:d)di*=ff.scale_factors.at(term_type);
//        rv.emplace(term_type,std::move(d));
//    }
    return rv;
}

HarmonicBond::HarmonicBond():
    FFTerm(std::make_shared<HarmonicOscillator>(),{IntCoord_t::BOND}){}

HarmonicAngle::HarmonicAngle():
    FFTerm(std::make_shared<HarmonicOscillator>(),{IntCoord_t::ANGLE}){}

FourierTorsion::FourierTorsion():
    FFTerm(std::make_shared<FourierSeries>(),{IntCoord_t::TORSION}){}

FourierImproperTorsion::FourierImproperTorsion():
    FFTerm(std::make_shared<FourierSeries>(),{IntCoord_t::IMPTORSION}){}

LJ14::LJ14():
    FFTerm(std::make_shared<LennardJones>(),{IntCoord_t::PAIR14}){}

LJPair::LJPair():
    FFTerm(std::make_shared<LennardJones>(),{IntCoord_t::PAIR}){}

Electrostatics14::Electrostatics14():
    FFTerm(std::make_shared<Electrostatics>(),{IntCoord_t::PAIR14}){}

ElectrostaticsPair::ElectrostaticsPair():
    FFTerm(std::make_shared<Electrostatics>(),{IntCoord_t::PAIR}){}

} //End namespace FManII
