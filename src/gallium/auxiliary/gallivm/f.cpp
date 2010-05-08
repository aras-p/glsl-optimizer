/**************************************************************************
 *
 * (C) Copyright VMware, Inc 2010.
 * (C) Copyright John Maddock 2006.
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 **************************************************************************/


/*
 * This file allows to compute the minimax polynomial coefficients we use
 * for fast exp2/log2.
 *
 * How to use this source:
 *
 * - Download and abuild the NTL library from
 *   http://shoup.net/ntl/download.html
 *
 * - Download boost source code matching to your distro. 
 *
 * - Goto libs/math/minimax and replace f.cpp with this file.
 *
 * - Build as
 *
 *   g++ -o minimax -I /path/to/ntl/include main.cpp f.cpp /path/to/ntl/src/ntl.a -lboost_math_tr1
 *
 * - Run as 
 *
 *    ./minimax
 *
 * - For example, to compute exp2 5th order polynomial between [0, 1] do:
 *
 *    variant 1
 *    range 0 1
 *    order 5 0
 *    steps 200
 *    info
 *
 * - For more info see
 * http://www.boost.org/doc/libs/1_36_0/libs/math/doc/sf_and_dist/html/math_toolkit/toolkit/internals2/minimax.html
 */

#define L22
#include <boost/math/bindings/rr.hpp>
#include <boost/math/tools/polynomial.hpp>

#include <cmath>


boost::math::ntl::RR f(const boost::math::ntl::RR& x, int variant)
{
   static const boost::math::ntl::RR tiny = boost::math::tools::min_value<boost::math::ntl::RR>() * 64;
   
   switch(variant)
   {
   case 0:
      // log2(x)/(x - 1)
      return log(x)/log(2.0)/(x - 1.0);

   case 1:
      // exp2(x)
      return exp(x*log(2.0));
   }

   return 0;
}


void show_extra(
   const boost::math::tools::polynomial<boost::math::ntl::RR>& n, 
   const boost::math::tools::polynomial<boost::math::ntl::RR>& d, 
   const boost::math::ntl::RR& x_offset, 
   const boost::math::ntl::RR& y_offset, 
   int variant)
{
   switch(variant)
   {
   default:
      // do nothing here...
      ;
   }
}

