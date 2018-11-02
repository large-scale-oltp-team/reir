add_subdirectory(reir)

file(GLOB_RECURSE ENGINES reir/engine/*.cpp)
file(GLOB_RECURSE ENGINES_HEADERS reir/engine/*.hpp)
add_library(reir-engine SHARED ${ENGINES} ${ENGINES_HEADERS})
target_link_libraries(reir-engine foedus-core)

target_compile_features(reir-engine PRIVATE cxx_range_for cxx_auto_type)

install(TARGETS reir-engine
    EXPORT reir
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib/reir COMPONENT Development
    INCLUDES DESTINATION include/reir)

install(DIRECTORY ./
    DESTINATION include/reir
    COMPONENT Development
    FILES_MATCHING PATTERN "*.hpp")
