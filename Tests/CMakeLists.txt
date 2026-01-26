
add_subdirectory(Subjects/)
add_subdirectory(Unit/)
add_subdirectory(Regression/)

add_test(NAME TestSuite COMMAND IACore_Test_Suite)
