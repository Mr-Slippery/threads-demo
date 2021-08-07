include(CTest)
add_executable(compute_test compute_test.cpp)
target_link_libraries(compute_test
  compute
  gtest gtest_main
  Threads::Threads
)
add_test(NAME compute_test COMMAND compute_test)