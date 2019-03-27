set(UNIT_TEST_LIST slab slab2)

add_library(tests-main OBJECT tests-main.cpp)
 
foreach(NAME IN LISTS UNIT_TEST_LIST)
    #list(APPEND UNIT_TEST_SOURCE_LIST ${NAME}.test.cpp)

    set(TARGET_NAME ${NAME})
     
    add_executable(${TARGET_NAME} ${NAME}.test.cpp $<TARGET_OBJECTS:tests-main>)
     
    add_test(
      NAME ${TARGET_NAME}
      COMMAND ${TARGET_NAME} -o report.xml -r junit)
endforeach()
 
