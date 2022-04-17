/**
 * ai-utils -- C++ Core utilities
 *
 * @file
 * @brief Implementation of class GlobalObjectManager.
 *
 * @Copyright (C) 2014, 2016  Carlo Wood.
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

#include "sys.h"
#include "GlobalObjectManager.h"
#include "debug.h"

#ifdef CWDEBUG
#include <libcwd/cwprint.h>
#endif

using namespace utils::_internal_;

#ifdef DEBUGGLOBAL
bool GlobalObjectManager::after_global_constructors = false;
#endif

/// @cond Doxygen_Suppress
void GlobalObjectManager::registerGlobalObject(GlobalObject* globalObject)
{
  globalObjects.push_back(globalObject);
}

void GlobalObjectManager::deleteGlobalObjects()
{
  bool done;
  do
  {
    GlobalObject* globalObject = globalObjects.back();
    globalObjects.pop_back();
    done = globalObjects.empty();
    if (!done)				// Don't call the destructor of GlobalObjectManager itself! (last one is self)
      globalObject->~GlobalObject();
  }
  while(!done);
}
/// @endcond

#ifdef DEBUGGLOBAL
void GlobalObjectManager::main_entered()
{
  Singleton<GlobalObjectManager>::instantiate();
  for (globalObjects_type::const_iterator i(Singleton<GlobalObjectManager>::instance().globalObjects.begin());
       i != Singleton<GlobalObjectManager>::instance().globalObjects.end();
       ++i)
  {
    if (!(*i)->instantiated_from_constructor())
    {
      DoutFatal( dc::core,
	  "Missing global/static initialization of `" << cwprint_using(*(*i), &GlobalObject::print_type_name) << "'.\n"
	  "          There should be one and only one code line reading:\n"
	  "          static " << cwprint_using(*(*i), &GlobalObject::print_type_name) << " dummy;" );
    }
    (*i)->set_initialized_and_after_global_constructors();
  }
  Singleton<GlobalObjectManager>::instance().after_global_constructors = true;
}
#endif

namespace {

SingletonInstance<GlobalObjectManager> GlobalObjectManager_instance __attribute__ ((unused));

} // namespace
