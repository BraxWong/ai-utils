/**
 * ai-utils -- C++ Core utilities
 *
 * @file
 * @brief Declaration of template class Global.
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

/// @class Global
/// @brief Initialization order fiasco free global instances.
//
// Usage:
//
// Note: Below we assume that you want to instantiate global objects of type `class Foo`.
//
// 1) Call GlobalObjectManager::main_entered() at the _very_ top of `main()`.
//    This call signifies the end of the global constructors.
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
//      int main(int argc, void* argv[])
//      {
//        Debug(NAMESPACE_DEBUG::init());       // Takes care of calling  GlobalObjectManager::main_entered() when DEBUGGLOBAL is defined.
//        //...
//      }
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 2) Create a list of human readable integer constants, one for each
//    instance that you want to instantiate in the current application.
//    You may use any value except -1.  Other negative values are reserved.
//
//    For example:
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.h}
//      enum FooInstance {
//        red,
//        green,
//	  yellow
//      };
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 3) If you want to call the default constructor of `Foo`, use:
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.h}
//      using GlobalRedFoo = Global<Foo, red, GlobalConverterVoid>;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    If you want the constructor `Foo(int instance)` to be called,
//    where `instance` is the integer constant of that instance
//    (`red` in the last example), then use:
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.n}
//      using GlobalRedFoo = Global<Foo, red>;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    If you want other data to be passed to the constructor of
//    `Foo`, then you should define a `GlobalConverter' class yourself.
//    For example:
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.h}
//      class GlobalConverterString {
//      public:
//        string operator()(int instance);
//      };
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    Where `operator()` should convert the `instance` variable into
//    a string.
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.n}
//      using GlobalRedFoo = Global<Foo, red, GlobalConverterString>;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    This is especially useful for library classes since it allows
//    the set of instances to be extended later, independ of the library.
//
// 4) For each instance of `Foo`, instantiate a Global<> object.
//    For example:
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
//      namespace {
//      GlobalRedFoo    redDummy;
//      GlobalGreenFoo  greenDummy;
//      GlobalYellowFoo yellowDummy;
//      }
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    Note that there is no need to use a type alias.
//    You can do as well:
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
//      namespace {
//      Global<Foo, red,    GlobalConverterString> redDummy;
//      Global<Foo, green>                         greenDummy;
//      Global<Foo, yellow, GlobalConverterVoid>   yellowDummy;
//      }
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    The name of the dummy doesn't matter of course, as long as
//    it doesn't collide - you don't even need the anonymous namespace actually:
//    it's just handy to make sure that the dummy names won't collide.
//
// 5) Now access the instances as follows:
//    In constructors of other Global<> objects and in constructors of real global/static objects, use:
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
//      GlobalRedFoo::instantiate()
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    which returns a Foo& to the `red' instance.
//
//    For example:
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
//      class Bar {
//        int b;
//      public:
//        Bar() : b(Global<Color, blue>::instantiate().brightness()) { }
//      };
//
//      Bar bar;	// Instantiate a real global object of class `Bar'.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    Anywhere else (that is, in code called from `main()`) use `GlobalRedFoo::``#instance()`,
//    or `Global<Foo, red>::``#instance()` because the converter class doesn't matter in this case.
//
//
// If you want to check whether you did everything correctly, define `DEBUGGLOBAL`
// and it will tell you exactly what you did wrong, if anything.

#pragma once

#include "debug.h"
#if defined(DEBUGGLOBAL) && (!defined(CWDEBUG) || !CWDEBUG_ALLOC || !CWDEBUG_LOCATION)
#ifdef CWDEBUG
#error "Please configure with --disable-debug-global / -DEnableDebugGlobal:BOOL=OFF or (re)configure libcwd with --enable-alloc --enable-location / -DEnableLibcwdAlloc:BOOL=ON -DEnableLibcwdLocation:BOOL=ON (in cmake/gitache-configs/libcwd_r.cmake)."
#else
#error "You cannot define DEBUGGLOBAL without having CWDEBUG, CWDEBUG_ALLOC and CWDEBUG_LOCATION defined."
#endif
#endif
#ifdef DEBUGGLOBAL
#include "utils/macros.h"
#include <execinfo.h>   // For backtrace(3).
#endif
#include <new>

// Forward declarations.
template<class TYPE, int inst, class CONVERTER> class Global;
class GlobalObjectManager;
class GlobalConverterVoid;

/// @cond Doxygen_Suppress
// Private classes
namespace utils
{

namespace _internal_
{
  // *****************************************************
  // *                                                   *
  // *   NEVER USE ANYTHING FROM namespace _internal_ !  *
  // *                                                   *
  // *****************************************************

  // Base class for global objects
  class GlobalObject {
  friend class ::GlobalObjectManager;
  protected:
    virtual ~GlobalObject() = default;
#ifdef DEBUGGLOBAL
    virtual bool instantiated_from_constructor() const = 0;
    virtual void print_type_name(std::ostream&) const = 0;
    virtual void set_initialized_and_after_global_constructors() const = 0;
#endif
  };

  template<class TYPE, int inst>
  class GlobalBase {
  public:
    // Needed to calculate the size of Global<>::Instance
    // and to calculate the offset between Instance* and TYPE*.
    class InstanceDummy : public TYPE, public GlobalObject {
      friend class GlobalObject; // To suppress a warning
      virtual ~InstanceDummy() = default;
    };
  protected:
    static char instance_[/*sizeof(InstanceDummy)*/];
    static char initialized;
#ifdef DEBUGGLOBAL
    static bool initialized_and_after_global_constructors;
    static bool instantiated_from_constructor;
    static char const* instantiate_function_name;
    static void const* instantiate_return_address1;
public:
    static void set_initialized_and_after_global_constructors() { initialized_and_after_global_constructors = initialized; }
    static void set_instantiate_return_address0(void const* addr)
	{ instantiate_function_name = libcwd::pc_mangled_function_name((char const*)addr + libcwd::builtin_return_address_offset); }
    static void set_instantiate_return_address1(void const* addr) { instantiate_return_address1 = addr; }
#endif

  public:
    static inline TYPE& instance();

#ifdef DEBUGGLOBAL
  private:
    static void print_error_msg();
    static void check_call_to_instance();

  public:
    static bool gifc_()
	{
	  return instantiated_from_constructor;
	}
    static bool beingInstantiatedRightNow_()
	{
	  return (initialized == -1);
	}
#endif
  };

  template<class TYPE, int inst, class CONVERTER>
  class Instance : public TYPE, public GlobalObject {
  private:
    static CONVERTER parameter_converter;

  private:	// Make sure nobody instantiates Instance itself except for Global<TYPE, inst, CONVERTER>.
    friend class Global<TYPE, inst, CONVERTER>;
    Instance(int) : TYPE(parameter_converter(inst)) { }	// TYPE is private (compile error)? Look at NOTE2 at the bottom of this file.
    virtual ~Instance() = default;

#ifdef DEBUGGLOBAL
    virtual bool instantiated_from_constructor() const;
    virtual void print_type_name(std::ostream&) const;
    virtual void set_initialized_and_after_global_constructors() const;
#endif
  };

  template<class TYPE, int inst>
  class Instance<TYPE, inst, GlobalConverterVoid> : public TYPE, public ::utils::_internal_::GlobalObject {
  private:	// Make sure nobody instantiates Instance itself except for Global<TYPE, inst, GlobalConverterVoid>.
    friend class Global<TYPE, inst, GlobalConverterVoid>;
    Instance(int) { }		// TYPE is private (compile error)? Look at NOTE1 at the bottom of this file.
    virtual ~Instance() = default;      // "error: deleted function '~Instance' cannot override a non-deleted function" means:
                                        // * you forgot to add friendInstance to class TYPE (the final class) (ERROR1), or
                                        // * you are using Global instead of Singleton for singleton (ERROR2), or
                                        // * you are trying to use Global instead of Singleton in order to pass a parameter (ERROR2a), or

#ifdef DEBUGGLOBAL
    virtual bool instantiated_from_constructor() const;
    virtual void print_type_name(std::ostream&) const;
    virtual void set_initialized_and_after_global_constructors() const;
#endif
  };

#ifdef DEBUGGLOBAL
  template<class TYPE, int inst, class CONVERTER = int>
  class GlobalTypeName { };

  template<class TYPE, int inst, class CONVERTER = int>
  class GlobalInstanceTypeName { };
#endif

} // namespace _internal_
} // namespace utils
/// @endcond

//==========================================================================================================
// Declarations

/**---------------------------------------------------------------------------------------------------------
 *
 * @class GlobalConverterVoid
 *
 * To be used as third template parameter of Global<class TYPE, int inst, class CONVERTER> when
 * the default constructor of TYPE must be called on instantiation.
 */

class GlobalConverterVoid
{
public:
  /// Return `void` to be used for the construction of `TYPE`.
  void operator()(int) { }
};

/**---------------------------------------------------------------------------------------------------------
 *
 * @class GlobalConverterInt
 *
 * The default third template parameter of Global<class TYPE, int inst, class CONVERTER>.
 * Using this class causes TYPE to be created by calling TYPE(int inst).
 */

class GlobalConverterInt
{
public:
  /// Return an `int` to be used for the construction of `TYPE`.
  int operator()(int inst) { return inst; }
};

//----------------------------------------------------------------------------------------------------------
//
// template<class TYPE, int inst, class CONVERTER>
// class Global

template<class TYPE, int inst, class CONVERTER = GlobalConverterInt>
class Global : public ::utils::_internal_::GlobalBase<TYPE, inst>
{
  using Instance = typename ::utils::_internal_::Instance<TYPE, inst, CONVERTER>;
  using base_type = ::utils::_internal_::GlobalBase<TYPE, inst>;
public:
  Global();
  ~Global();
  /// Returns a reference to the underlaying instance. Initialized the object first if necessary.
  static inline TYPE& instantiate();
#ifdef DOXYGEN
  // This is really part of utils::_internal_::GlobalBase.
  /// A reference to the underlaying instance.
  static inline TYPE& instance();
#endif
private:
  static void initialize_instance_();
};

#ifdef DEBUGGLOBAL
template<class TYPE, int inst, class CONVERTER>
std::ostream& operator<<(std::ostream& os, typename ::utils::_internal_::GlobalTypeName<TYPE, inst, CONVERTER> const&)
{
  char const* p = libcwd::type_info_of<Global<TYPE, inst, CONVERTER> >().demangled_name();
  size_t len = strlen(p);
  if (!strcmp(", int>", p + len - 6))
  {
    os.write(p, len - 6);
    os << "[, CONVERTER]>";
  }
  else if (len > 21 && !strcmp(", GlobalConverterInt>", p + len - 21))
  {
    os.write(p, len - 21);
    os << '>';
  }
  else
    os << p;
  return os;
}

template<class TYPE, int inst, class CONVERTER>
std::ostream& operator<<(std::ostream& os, typename ::utils::_internal_::GlobalInstanceTypeName<TYPE, inst, CONVERTER> const&)
{
  ::utils::_internal_::GlobalTypeName<TYPE, inst, CONVERTER> name;
  return operator<<(os, name);
}
#endif

#include "GlobalObjectManager.h"

//==========================================================================================================
// Definitions

template<class TYPE, int inst, class CONVERTER>
inline TYPE& Global<TYPE, inst, CONVERTER>::instantiate()
{
#ifdef DEBUGGLOBAL
  utils::_internal_::GlobalBase<TYPE, inst>::instantiate_function_name =
      libcwd::pc_mangled_function_name((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset);
  PRAGMA_DIAGNOSTIC_PUSH_IGNORE_frame_address
  utils::_internal_::GlobalBase<TYPE, inst>::instantiate_return_address1 = __builtin_return_address(1);
  PRAGMA_DIAGNOSTIC_POP
#endif
  if (!base_type::initialized)
  {
    base_type::initialized = -1;			// Stop the next line from doing something if this is the GlobalObjectManager.
    Singleton<GlobalObjectManager>::instantiate();	// initialize_instance_() uses GlobalObjectManager
    initialize_instance_();
  }
  Instance* ptr = reinterpret_cast<Instance*>(base_type::instance_);
  return *static_cast<TYPE*>(ptr);
}

template<class TYPE, int inst, class CONVERTER>
Global<TYPE, inst, CONVERTER>::Global()
{
#ifdef DEBUGGLOBAL
  if (utils::_internal_::GlobalBase<TYPE, inst>::instantiated_from_constructor)
  {
    typename utils::_internal_::GlobalInstanceTypeName<TYPE, inst, CONVERTER> name;
    DoutFatal( dc::core,
	"The class `" << name << "' is defined more then once.\n"
	"          There should be one and only one code line reading:\n"
	"          static " << name << " dummy;" );
  }
  utils::_internal_::GlobalBase<TYPE, inst>::instantiated_from_constructor = true;
#endif
  if (!base_type::initialized)				// Only initialize when it wasn't initialized before
  {
    base_type::initialized = -1;			// Stop the next line from doing something if this is the GlobalObjectManager.
    Singleton<GlobalObjectManager>::instantiate();	// initialize_instance_() uses GlobalObjectManager
    initialize_instance_();
  }
#ifdef DEBUGGLOBAL
  else
    Singleton<GlobalObjectManager>::instantiate();	// We need to set instantiate_function_name/instantiate_return_address1 !
#endif
  Singleton<GlobalObjectManager>::instance().global_constructor_called();	// Update a counter
}

template<class TYPE, int inst, class CONVERTER>
Global<TYPE, inst, CONVERTER>::~Global()
{
  // Using instantiate() here instead of instance() to catch the case where someone wrote his own main().
  Singleton<GlobalObjectManager>::instantiate().global_destructor_called();
}

template<class TYPE, int inst, class CONVERTER>
void Global<TYPE, inst, CONVERTER>::initialize_instance_()
{
  base_type::initialized = -1;				// Stop Global<TYPE, inst>::Global() from calling us again.
  Instance* globalObject = new (base_type::instance_) Instance(inst);
  base_type::initialized = 1;
  Singleton<GlobalObjectManager>::instance().registerGlobalObject(globalObject);
}

namespace utils {
/// @cond Doxygen_Suppress
  namespace _internal_ {

    template<class TYPE, int inst, class CONVERTER>
    CONVERTER Instance<TYPE, inst, CONVERTER>::parameter_converter;

    template<class TYPE, int inst>
    char GlobalBase<TYPE, inst>::instance_[sizeof(InstanceDummy)] __attribute__((__aligned__));

    template<class TYPE, int inst>
    char GlobalBase<TYPE, inst>::initialized;

    template<class TYPE, int inst>
    inline TYPE& GlobalBase<TYPE, inst>::instance()
    {
#ifdef DEBUGGLOBAL
      if (!initialized_and_after_global_constructors)
	check_call_to_instance();
#endif
      InstanceDummy* ptr = reinterpret_cast<InstanceDummy*>(instance_);
      return *static_cast<TYPE*>(ptr);	// If `instance_' is not yet initialized, then define DEBUGGLOBAL to find out why.
    }

#ifdef DEBUGGLOBAL
    template<class TYPE, int inst>
    bool GlobalBase<TYPE, inst>::initialized_and_after_global_constructors = false;

    template<class TYPE, int inst>
    char const* GlobalBase<TYPE, inst>::instantiate_function_name = nullptr;

    template<class TYPE, int inst>
    void const* GlobalBase<TYPE, inst>::instantiate_return_address1 = nullptr;

    template<class TYPE, int inst>
    bool GlobalBase<TYPE, inst>::instantiated_from_constructor = false;

    template<class TYPE, int inst>
    void GlobalBase<TYPE, inst>::print_error_msg()
    {
      //
      // You should use `instantiate()' instead of `instance()' in constructors of Singleton<> and Global<> objects.
      //
      // Note that when `instance()' is called from some function foobar() which is called from such a constructor,
      // add a call to `instantiate()' in that constructor *before* the call to foobar(), don't change `instance()'
      // into `instantiate()' inside foobar().
      //
      // If `instance()' was called after main(), then you forgot to add a global object Global<> or Singleton<>
      // to make sure that this object is instantiated before main() is called.
      //
      if (GlobalObjectManager::is_after_global_constructors())
      {
        GlobalInstanceTypeName<TYPE, inst> name;
	initialized = -2;		// Stop endless loop (instance() below calling print_err_msg() again).
	DoutFatal( dc::core, "Missing global/static initialization of `" << name << "'.\n"
	    "          There should be one and only one code line reading:\n"
	    "          static " << name << " dummy;" );
      }
      else
	DoutFatal( dc::core,
	    "Using `instance()' in global constructor.  Use `instantiate()' inside the\n"
	    "          constructor instead, or add `instantiate()' to the constructor before calling\n"
	    "          the function that calls `instance()' when `instance()' wasn't called directly\n"
	    "          by the constructor." );

    }

    template<class TYPE, int inst>
    void GlobalBase<TYPE, inst>::check_call_to_instance()
    {
      if (!initialized)
	print_error_msg();

      // Do a backtrace looking for a common function from which instantiate() was called:
      void* prev_addr = nullptr;
      void* addresses[400];
      int depth = backtrace(addresses, sizeof(addresses) / sizeof(void*));
      int i = 0;
      while (i < depth)
      {
        void* addr = addresses[i];
	if (addr == instantiate_return_address1
	    && instantiate_function_name == libcwd::pc_mangled_function_name((char*)prev_addr + libcwd::builtin_return_address_offset))
	  break;
	prev_addr = addr;
	++i;
      }
      if (i == depth)
      {
	//
	// Calls to instantiate/instance should obey the following:
	//
	// libc function --> call to static/global constructor:		<-- instantiate_function_name
	//   ^                       {
	//   |                         call to instantiate()
// instantiate_return_address1         call to foobar1() --> call to foobar2() --> ... etc --> call to instance()
	//                           }
	//
        GlobalTypeName<TYPE, inst> name;
        PRAGMA_DIAGNOSTIC_PUSH_IGNORE_frame_address
        _Pragma("GCC diagnostic ignored \"-Winline\"")
	libcwd::location_ct loc(inst < 0 ? ((char*)__builtin_return_address(2) + libcwd::builtin_return_address_offset)
	                                : ((char*)__builtin_return_address(1) + libcwd::builtin_return_address_offset));
        PRAGMA_DIAGNOSTIC_POP
        DoutFatal(dc::core, loc << ": Calling " << name << "::instance() in (or indirectly from)\n"
            "          constructor of static or global object instead of (or without first) calling " << name << "::instantiate().");
      }
    }

    template<class TYPE, int inst, class CONVERTER>
    bool Instance<TYPE, inst, CONVERTER>::instantiated_from_constructor() const
    {
      return Global<TYPE, inst, CONVERTER>::gifc_();
    }

    template<class TYPE, int inst>
    bool Instance<TYPE, inst, GlobalConverterVoid>::instantiated_from_constructor() const
    {
      return Global<TYPE, inst, GlobalConverterVoid>::gifc_();
    }

    template<class TYPE, int inst, class CONVERTER>
    void Instance<TYPE, inst, CONVERTER>::print_type_name(std::ostream& os) const
    {
      typename utils::_internal_::GlobalInstanceTypeName<TYPE, inst, CONVERTER> name;
      os << name;
    }

    template<class TYPE, int inst>
    void Instance<TYPE, inst, GlobalConverterVoid>::print_type_name(std::ostream& os) const
    {
      typename utils::_internal_::GlobalInstanceTypeName<TYPE, inst, GlobalConverterVoid> name;
      os << name;
    }

    template<class TYPE, int inst, class CONVERTER>
    void Instance<TYPE, inst, CONVERTER>::set_initialized_and_after_global_constructors() const
    {
      Global<TYPE, inst, CONVERTER>::set_initialized_and_after_global_constructors();
    }

    template<class TYPE, int inst>
    void Instance<TYPE, inst, GlobalConverterVoid>::set_initialized_and_after_global_constructors() const
    {
      Global<TYPE, inst, GlobalConverterVoid>::set_initialized_and_after_global_constructors();
    }

#endif // DEBUGGLOBAL

  }	// namespace _internal_
/// @endcond
}	// namespace libcw

/*
// NOTE1

This note gives a diagnose for the problem:

YourProgram.cc: `YourClass::YourClass()' is private
utils/Global.h:107: within this context

Of course it is possible that `YourClass' is NOT a singleton and you just made its constructor
private by accident.  However, if `YourClass' is a singleton then its constructor should
indeed be private and the following should help to fix the problem:

The following compiler error (using egcs-2.95.1):

utils/Global.h: In method `utils::_internal_::Instance<Bar,-1,GlobalConverterVoid>::Instance(int)':
utils/Global.h:357:   instantiated from `Global<Bar,-1,GlobalConverterVoid>::initialize_instance_()'
utils/Global.h:342:   instantiated from `Global<Bar,-1,GlobalConverterVoid>::Global()'
Singleton_tst.cc:114:   instantiated from here
Singleton_tst.cc:77: `Bar::Bar()' is private
utils/Global.h:107: within this context

Means that you forgot to add a `friend_Instance' at the top of your singleton class `Bar'.

If instead you get this error:

utils/Global.h: In method `utils::_internal_::Instance<Bar,0,GlobalConverterVoid>::Instance(int)':
utils/Global.h:357:   instantiated from `Global<Bar,0,GlobalConverterVoid>::initialize_instance_()'
utils/Global.h:342:   instantiated from `Global<Bar,0,GlobalConverterVoid>::Global()'
Singleton_tst.cc:110:   instantiated from here
Singleton_tst.cc:77: `Bar::Bar()' is private
utils/Global.h:107: within this context

Where the `0' can be any positive integer.
Then instead you seem to be trying to instantiate `static Global<Bar, 0, GlobalConverterVoid>'.
Instead, use `static Singleton<Bar>' for singletons.

// NOTE2

This note gives a diagnose for the problem:

YourProgram.cc: `YourClass::YourClass()' is private
utils/Global.h:88: within this context

Of course it is possible that `YourClass' is NOT a singleton and you just made its constructor
private by accident.  However, if `YourClass' is a singleton then its constructor should
indeed be private and the following should help to fix the problem:

The following compiler error (using egcs-2.95.1):

utils/Global.h: In method `utils::_internal_::Instance<Bar,-1,YourGlobalConverter>::Instance(int)':
utils/Global.h:357:   instantiated from `Global<Bar,-1,YourGlobalConverter>::initialize_instance_()'
utils/Global.h:342:   instantiated from `Global<Bar,-1,YourGlobalConverter>::Global()'
Singleton_tst.cc:108:   instantiated from here
Singleton_tst.cc:90: `Bar::Bar(SomeType)' is private
utils/Global.h:94: within this context

Means you are trying to instantiate `Global<Bar,-1,YourGlobalConverter>' in an
attempt to pass a parameter to the singleton `Bar'.  This is however not possible.
A singleton can ONLY have a default constructor. You will have to use another
Global<> object to initialize a singleton (let the singleton read initialization
data from that other Global<> object).

*/
