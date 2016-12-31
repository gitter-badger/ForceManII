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

#include "ForceManII/InternalCoordinates.hpp"

///Namespace for all code associated with ForceManII
namespace FManII {

///Implements derivatives for distance between points.  
///See [Distance Class](@ref distance) for more detail.
class Distance: public InternalCoordinates {
public:
    Distance(cSharedVector Carts):
        InternalCoordinates(Carts){}
protected:
    Vector compute_value_(size_t deriv_i,const IVector& coord_i)const;
};


} //End namespace FManII

