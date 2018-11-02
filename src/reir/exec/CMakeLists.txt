add_library(reir-exec STATIC
        ast_expression.hpp
        ast_statement.hpp
        compiler.cpp
        compiler_context.cpp
        db_interface.cpp
        executor.cpp
        leveldb_compiler.cpp
        parser.cpp
        ast_statement_parser.cpp
        ast_node_parser.cpp
        ast_node.hpp
        ast_node.cpp
        reir_context.cpp
        ast_expression_codegen.cpp
        ast_statement_codegen.cpp
        ast_expression_parser.cpp)

link_directories(${LLVM_LIBRARY_DIRS})
target_include_directories(reir-exec PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(reir-exec PUBLIC foedus-core)
target_link_libraries(reir-exec
  INTERFACE
    numa
    ${LLVM_LIBS}
    ${LLVM_SYSTEM_LIBS})

target_compile_features(reir-exec PUBLIC cxx_range_for cxx_auto_type)

install(TARGETS reir-exec
    EXPORT reir
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib/reir COMPONENT Development
    INCLUDES DESTINATION include/reir)
