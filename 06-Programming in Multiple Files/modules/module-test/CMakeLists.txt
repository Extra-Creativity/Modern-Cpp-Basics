add_executable(module-simple simple/main.cpp)
target_sources(module-simple
    PUBLIC FILE_SET CXX_MODULES 
    FILES simple/Person.mpp simple/Customer.mpp)
target_compile_features(module-simple PRIVATE cxx_std_23)

file(GLOB split_impl_src split-impl/*.cpp)
add_executable(module-split-impl ${split_impl_src})
target_sources(module-split-impl
    PUBLIC FILE_SET CXX_MODULES 
    FILES split-impl/Person.mpp)
target_compile_features(module-simple PRIVATE cxx_std_23)

file(GLOB partition_src partition/*.cpp)
file(GLOB partition_modules partition/*.mpp)
add_executable(module-partition ${partition_src})
target_sources(module-partition
    PUBLIC FILE_SET CXX_MODULES 
    FILES ${partition_modules})
target_compile_features(module-partition PRIVATE cxx_std_23)

file(GLOB global_frag_src global-fragment/*.cpp)
add_executable(module-global-frag ${global_frag_src})
target_sources(module-global-frag
    PUBLIC FILE_SET CXX_MODULES 
    FILES global-fragment/Person.mpp
    # PUBLIC FILE_SET HEADERS FILES global-fragment/Old.h
    )
target_compile_features(module-global-frag PRIVATE cxx_std_23)

add_executable(module-private private-fragment/main.cpp)
target_sources(module-private
    PUBLIC FILE_SET CXX_MODULES 
    FILES private-fragment/Person.mpp)
target_compile_features(module-private PRIVATE cxx_std_23)
# Header unit isn't supported yet in cmake, so we don't try it.