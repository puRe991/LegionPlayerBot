# Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# The extraction tools come from The Legion Preservation Project's
# LegionCore-7.3.5 (GPL-2.0), which builds them against a base target of its
# own. Provide that target here. The tools need C++17 (std::make_unique,
# std::optional); the core stays on C++11.
if (NOT TARGET trinity-core-interface)
  add_library(trinity-core-interface INTERFACE)
  target_compile_features(trinity-core-interface INTERFACE cxx_std_17)
  # GCC and Clang already default to C++17, so CMake adds no flag for the
  # feature above, and the core's -std=c++11 would win. Name it explicitly;
  # it comes after the global flags on the command line.
  target_compile_options(trinity-core-interface INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-std=c++17>)
  # Its DB2 loader marks classes for DLL export. The tools link everything
  # statically, and the core's Define.h has no such macro.
  target_compile_definitions(trinity-core-interface INTERFACE TC_COMMON_API=)
endif()

add_subdirectory(extractor_common)
add_subdirectory(map_extractor)
add_subdirectory(vmap4_assembler)
add_subdirectory(vmap4_extractor)
add_subdirectory(mmaps_generator)
add_subdirectory(web_api)
