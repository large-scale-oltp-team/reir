function(register_test filename)
  get_filename_component(testname ${filename} NAME_WE)
  add_executable(${testname} ${CMAKE_CURRENT_LIST_DIR}/${filename})
  if(BUILD_TESTS)
    add_test(
      NAME ${testname}
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${testname})
  else()
    # Let's create test target and exclude from full build.
    # This allows running specific errornous testcase while avoiding break on full build.
    set_target_properties(${testname} PROPERTIES EXCLUDE_FROM_ALL ON)
  endif()
  target_include_directories(${testname}
    PRIVATE
      ${LLVM_INCLUDE_DIRS}
      third_party/googletest/googletest/include)

  target_link_libraries(${testname}
    PRIVATE
      reir-engine
      gtest_main
      reir-engine
      reir-db
      reir-exec
      ${LLVM_LIBS}
      numa)
   message("registered test: ${testname}")
endfunction()

set(tests
  "attr_type_test.cpp"
  "attribute_test.cpp"
  "schema_test.cpp"
  "tokenizer_test.cpp"
  "cast_test.cpp"
  #"parser_test.cpp" // temporarily disabled for false positive memory leak
  "compiler_test.cpp"
  "ast_exec_test.cpp"
  "ast_expr_test.cpp"
  )

link_directories(${LLVM_LIBRARY_DIRS})
foreach(test ${tests})
  register_test(${test})
endforeach()

if (NOT TARGET gtest)
  add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)
  include_directories(third_party/googletest/include)
endif()

