
set(files
        "attr_type.cpp"
        "attribute.cpp"
        "schema.cpp"
        "metadata.cpp"
        "value.cpp"
        )

add_library(reir-db SHARED ${files})
target_link_libraries(reir-db PUBLIC leveldb)
target_compile_features(reir-db
        PRIVATE cxx_range_for cxx_auto_type)

install(TARGETS reir-db
        EXPORT reir
        LIBRARY DESTINATION lib COMPONENT Runtime
        ARCHIVE DESTINATION lib/reir COMPONENT Development
        INCLUDES DESTINATION include/reir)
