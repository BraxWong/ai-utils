/**
 * ai-utils -- C++ Core utilities
 *
 * @file
 * @brief Definition of log2 and ceil_log2.
 *
 * @Copyright (C) 2018  Carlo Wood.
 *
 * pub   dsa3072/C155A4EEE4E527A2 2018-08-16 Carlo Wood (CarloWood on Libera) <carlo@alinoe.com>
 * fingerprint: 8020 B266 6305 EE2F D53E  6827 C155 A4EE E4E5 27A2
 *
 * This file is part of ai-utils.
 *
 * ai-utils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ai-utils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ai-utils.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "clz.h"
#include "BuiltinArg.h"

namespace utils {

// Function utils::log2(n)
//
// Returns, if
//
//   n == 0:    -1 (this is used by ceil_log2)
//   n > 0:     floor(log2(n)) (the index of the most significant set bit, aka 1 << log2(n) == n iff n is a power of 2).
//
template<typename T>
constexpr int log2(T n)
{
  return n == 0 ? -1 : 8 * sizeof(typename BuiltinArg<T>::type) - 1 - clz(static_cast<typename BuiltinArg<T>::type>(n));
}

// Function utils::ceil_log2(n)
//
// Returns ceil(log2(n)).
// Undefined if n == 0;
//
template<typename T>
constexpr int ceil_log2(T n)
{
  return 1 + log2(static_cast<T>(n - 1));
}

} // namespace utils
