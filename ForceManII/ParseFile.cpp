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
#include <iostream>
#include <algorithm>
#include <sstream>

using namespace std;

namespace FManII {

//Case-insensitive string compare
inline bool s_comp(const string& lhs, const string& rhs){
    if(lhs.size()!=rhs.size())return false;
    for(size_t i=0;i<lhs.size();++i)
        if (toupper(lhs[i]) != toupper(rhs[i]))return false;
    return true;
}

//Code factorization for setting the combination rule
inline void set_mean(pair<string,string> p,const string& token, ForceField& ff){
    if(s_comp(token,"GEOMETRIC"))ff.combrules[p]=FManII::geometric;
    else if(s_comp(token,"ARITHMETIC"))ff.combrules[p]=FManII::mean;
}

//Splits a line into strings based on whitespace
vector<string> tokenize(const string& line){
    istringstream iss(line);
    vector<string> tokens;
    while(iss){
        string token;
        iss>>token;
        if(token!="")
            tokens.push_back(token);
    }
    return tokens;
}

template<typename Fxn_t>
void generic_fill(ForceField&ff,
                  FFTerm_t ffterm,
                  const vector<string>& tokens,
                  size_t size,
                  Fxn_t fxn,
                  bool is_class,
                  const vector<pair<string,Vector>>& ps){
    ff.orderrules[ffterm]=fxn;
    if(!ff.paramtypes.count(ffterm))
        ff.paramtypes[ffterm]= is_class?TypeTypes_t::CLASS:TypeTypes_t::TYPE;
    if(!ff.terms.count(ffterm))
        ff.terms.emplace(ffterm,std::move(get_term(ffterm)));
    IVector vals;
    for(size_t i=1;i<=size;++i)vals.push_back(stoi(tokens[i]));
    vals=fxn(vals);
    for(size_t i=0;i<ps.size();++i)//Don't add incomplete parameters
        if(!ps[i].second.size())return;
    for(size_t i=0;i<ps.size();++i)
        ff.params.add_param(ffterm,ps[i].first,vals,ps[i].second);
}

//Fills in the improper torsion section of the FF
inline void parse_imp(const std::vector<std::string>& tokens,
                      ForceField& ff,
                      bool is_charmm,
                      double kcalmol2au,
                      double deg2rad){
    const std::array<int,4> temp_types={stoi(tokens[1]),stoi(tokens[2]),
                                      stoi(tokens[3]),stoi(tokens[4])};
    const size_t center=is_charmm?0:2;
    IVector types;
    for(size_t ti=0;ti<4;++ti)
        if(ti!=center)types.push_back(temp_types[ti]);
    types=imp_order({types[0],(size_t)temp_types[center],types[1],types[2]});
    FFTerm_t ffterm=Terms_t::FS_IMP;
    ff.orderrules[ffterm]=imp_order;
    ff.paramtypes[ffterm]=TypeTypes_t::CLASS;
    if(!ff.terms.count(ffterm))
        ff.terms.emplace(ffterm,std::move(get_term(ffterm)));
    const double v=stod(tokens[5])*kcalmol2au,
           phi=is_charmm?180.0*deg2rad:stod(tokens[6])*deg2rad;
    ff.params.add_param(ffterm,Param_t::amp,types,{v});
    ff.params.add_param(ffterm,Param_t::phi,types,{phi});
    ff.params.add_param(ffterm,Param_t::n,types,{stod(tokens[7])});
}

inline void parse_himp(const std::vector<std::string>& tokens,
                      ForceField& ff,
                      bool is_charmm,
                      double kcalmol2au,
                      double deg2rad){
    const std::array<int,4> temp_types={stoi(tokens[1]),stoi(tokens[2]),
                                      stoi(tokens[3]),stoi(tokens[4])};
    const size_t center=is_charmm?0:2;
    IVector types;
    for(size_t ti=0;ti<4;++ti)
        if(ti!=center)types.push_back(temp_types[ti]);
    types=imp_order({types[0],(size_t)temp_types[center],types[1],types[2]});
    FFTerm_t ffterm=Terms_t::HO_IMP;
    ff.orderrules[ffterm]=imp_order;
    ff.paramtypes[ffterm]=TypeTypes_t::CLASS;
    if(!ff.terms.count(ffterm))
        ff.terms.emplace(ffterm,std::move(get_term(ffterm)));
    //RMR I need the 0.5 to agree with Tinker, but do not fully understand why
    const double v=0.5*stod(tokens[5])*kcalmol2au,
           phi=is_charmm?180.0*deg2rad:stod(tokens[6])*deg2rad;
    ff.params.add_param(ffterm,Param_t::K,types,{v});
    ff.params.add_param(ffterm,Param_t::r0,types,{phi});
}

//Fills in the 6-12 section of the force field
inline void parse_lj(const std::vector<std::string>& tokens,
                     ForceField& ff,
                     double kcalmol2au,
                     double ang2au,
                     bool isRMin,
                     bool isradius,
                     bool is14){
    const std::vector<size_t> types({stoul(tokens[1])});
    const double scale=ang2au*(isradius?2.0:1.0)*(isRMin?1.0:std::pow(2.0,1.0/6.0));
    auto term=(is14?Terms_t::LJ14 : Terms_t::LJ);
    ff.params.add_param(term,Param_t::sigma,types,{stod(tokens[2])*scale});
    ff.params.add_param(term,Param_t::epsilon,types,{stod(tokens[3])*kcalmol2au});
    for(const auto& ci:{IntCoord_t::PAIR,IntCoord_t::PAIR14}){
        FFTerm_t ffterm=std::make_pair(Model_t::LENNARD_JONES,ci);
        if(!ff.paramtypes.count(ffterm))
            ff.paramtypes[ffterm]=TypeTypes_t::CLASS;
        if(!ff.terms.count(ffterm))
            ff.terms.emplace(ffterm,std::move(get_term(ffterm)));
    }

}

inline void parse_chg(const std::vector<std::string>& tokens,
                      ForceField& ff){
    const std::vector<size_t> types({stoul(tokens[1])});
    ff.params.add_param(Terms_t::CL,Param_t::q,types,{stod(tokens[2])});
    for(const auto& ci:{IntCoord_t::PAIR,IntCoord_t::PAIR14}){
        FFTerm_t ffterm=std::make_pair(Model_t::ELECTROSTATICS,ci);
        ff.paramtypes[ffterm]=TypeTypes_t::TYPE;
        if(!ff.terms.count(ffterm))
            ff.terms.emplace(ffterm,std::move(get_term(ffterm)));
    }
}

ForceField parse_file(std::istream&& file,
                      bool is_charmm,
                      double kcalmol2au,
                      double ang2au,
                      double deg2rad){
    using std::stoi;using std::stod;
    const double k2au=kcalmol2au/(ang2au*ang2au);
    std::string line;
    bool isRMin=true,isradius=true;
    ForceField ff;
    while(std::getline(file,line)){
        auto tokens=tokenize(line);
        if(tokens.size()<=1)continue;
        if(s_comp(tokens[0],"radiusrule"))
            set_mean({Model_t::LENNARD_JONES,Param_t::sigma},tokens[1],ff);
        else if(s_comp(tokens[0],"torsionunit"))
            ff.scale_factors[Terms_t::FS_TORSION]=stod(tokens[1]);
        else if(s_comp(tokens[0],"imptorunit"))
            ff.scale_factors[Terms_t::FS_IMP]=stod(tokens[1]);
        else if(s_comp(tokens[0],"vdwindex")){
            const auto& type=s_comp(tokens[1],"type")?TypeTypes_t::TYPE:TypeTypes_t::CLASS;
            ff.paramtypes[Terms_t::LJ]=type;
            ff.paramtypes[Terms_t::LJ14]=type;
        }
        else if(s_comp(tokens[0],"radiustype"))
            isRMin=s_comp(tokens[1],"R-MIN");
        else if(s_comp(tokens[0],"radiussize"))
            isradius=s_comp(tokens[1],"RADIUS");
        else if(s_comp(tokens[0],"epsilonrule"))
            set_mean({Model_t::LENNARD_JONES,Param_t::epsilon},tokens[1],ff);
        else if(s_comp(tokens[0],"vdw-14-scale"))
            ff.scale_factors[Terms_t::LJ14]=1.0/stod(tokens[1]);
        else if(s_comp(tokens[0],"chg-14-scale"))
            ff.scale_factors[Terms_t::CL14]=1.0/stod(tokens[1]);
        else if(s_comp(tokens[0],"atom"))
            ff.type2class[stoi(tokens[1])]=stoi(tokens[2]);
        else if(s_comp(tokens[0],"bond"))
            generic_fill(ff,Terms_t::HO_BOND,tokens,2,pair_order,
                true,
                {{Param_t::K,{2.0*k2au*stod(tokens[3])}},
                 {Param_t::r0,{ang2au*stod(tokens[4])}}});
        else if(s_comp(tokens[0],"angle"))
            generic_fill(ff,Terms_t::HO_ANGLE,tokens,3,
              angle_order,true,
            {{Param_t::K,{2.0*kcalmol2au*stod(tokens[4])}},
             {Param_t::r0,{deg2rad*stod(tokens[5])}}});
        else if(s_comp(tokens[0],"ureybrad"))
            generic_fill(ff,Terms_t::HO_PAIR13,tokens,3,
              angle_order,true,
            {{Param_t::K,{2.0*k2au*stod(tokens[4])}},
             {Param_t::r0,{ang2au*stod(tokens[5])}}});
        else if(s_comp(tokens[0],"imptors"))
            parse_imp(tokens,ff,is_charmm,kcalmol2au,deg2rad);
        else if(s_comp(tokens[0],"improper"))
            parse_himp(tokens,ff,is_charmm,kcalmol2au,deg2rad);
        else if(s_comp(tokens[0],"torsion"))
        {
            Vector v,phi,n;
            for(size_t ti=0;ti<3;++ti)
            {
                if(tokens.size()<5+3*(ti+1))break;
                v.push_back(stod(tokens[5+3*ti])*kcalmol2au);
                phi.push_back(stod(tokens[6+3*ti])*deg2rad);
                n.push_back(stod(tokens[7+3*ti]));
            }
            generic_fill(ff,Terms_t::FS_TORSION,tokens,4,
                   torsion_order,true,
                   {{Param_t::amp,v},{Param_t::phi,phi},{Param_t::n,n}});
        }
        else if(s_comp(tokens[0],"charge"))
            parse_chg(tokens,ff);
        else if(s_comp(tokens[0],"vdw"))
            parse_lj(tokens,ff,kcalmol2au,ang2au,isRMin,isradius,false);
        else if(s_comp(tokens[0],"vdw14"))
            parse_lj(tokens,ff,kcalmol2au,ang2au,isRMin,isradius,true);
    }
    ff.link_terms(Terms_t::LJ14,Terms_t::LJ);
    ff.link_terms(Terms_t::CL14,Terms_t::CL);
    for(const auto& pi:{IntCoord_t::PAIR,IntCoord_t::PAIR14}){
        auto ffterm=std::make_pair(Model_t::ELECTROSTATICS,pi);
        auto pterm=std::make_pair(Model_t::ELECTROSTATICS,Param_t::q);
        if(ff.terms.count(ffterm)){
            ff.combrules[pterm]=FManII::product;
        }
    }

    return ff;
}

}//End namespace
