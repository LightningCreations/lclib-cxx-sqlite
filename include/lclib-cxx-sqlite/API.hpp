/*
 * API.hpp
 *
 *  Created on: Oct 10, 2018
 *      Author: connor
 */

#ifndef __INCLUDE_API_HPP__2018_10_10_20_34_12
#define __INCLUDE_API_HPP__2018_10_10_20_34_12
#include <lclib-cxx/Config.hpp>

#ifdef _LIBLC_CXX_SQLITE_BUILD
LCLIBEXPORT void init();
#else
LCLIBIMPORT void init();
#endif


#endif /* __INCLUDE_API_HPP__2018_10_10_20_34_12 */
