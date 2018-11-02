add_subdirectory(db)
add_subdirectory(exec)
# option(LLVM_PATH "path to llvm directory")

link_directories(${LLVM_LIBRARY_DIRS})
add_executable(reirc reirc.cpp)

target_include_directories(reirc PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/tanakh
        ${LLVM_INCLUDE_DIRS})

target_link_libraries(reirc PRIVATE
        reir-db
        reir-engine
        reir-exec
        foedus-core
        ${LLVM_LIBS}
        ${LLVM_SYSTEM_LIBS}
        )
#set_target_properties(reir PROPERTIES
#        COMPILE_FLAGS "-fno-rtti")

install(TARGETS reirc
    EXPORT reir
    RUNTIME DESTINATION bin COMPONENT Runtime)
