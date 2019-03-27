set(BENCH_LIST
    benchmark-multi-threaded
    benchmark-single-threaded
    lf-bench-mth
    lf-slist-test
    vec-deque-iterate)

foreach(NAME IN LISTS BENCH_LIST)
    add_executable(${NAME} ${NAME}.cpp)
    target_link_libraries(${NAME} benchmark atomic)
endforeach()
 

