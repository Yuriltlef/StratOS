cmake_minimum_required(VERSION 3.15)

add_executable(StaticArrayTest01
    "${CMAKE_CURRENT_SOURCE_DIR}/src/StaticArrayTest01.cpp"
    )

target_link_libraries(StaticArrayTest01 PRIVATE
    doctest
    MUSSTL
    )

add_executable(RingBufferTest01
    "${CMAKE_CURRENT_SOURCE_DIR}/src/RingBufferTest01.cpp"
)

target_link_libraries(RingBufferTest01 PRIVATE
    doctest
    MUSSTL
    )

add_executable(BitMapTest01
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BitMapTest01.cpp"
)

target_link_libraries(BitMapTest01 PRIVATE
    doctest
    MUSSTL
    )

add_executable(IntrusiveListTest01
    "${CMAKE_CURRENT_SOURCE_DIR}/src/IntrusiveListTest01.cpp"
)

target_link_libraries(IntrusiveListTest01 PRIVATE
    doctest
    MUSSTL
    )

add_executable(PriorityQueueTest01
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PriorityQueueTest01.cpp"
)

target_link_libraries(PriorityQueueTest01 PRIVATE
    doctest
    MUSSTL
    )


add_test(NAME StaticArrayTest01 COMMAND StaticArrayTest01)
add_test(NAME RingBufferTest01 COMMAND RingBufferTest01)
add_test(NAME BitMapTest01 COMMAND BitMapTest01)
add_test(NAME IntrusiveListTest01 COMMAND IntrusiveListTest01)
add_test(NAME PriorityQueueTest01 COMMAND PriorityQueueTest01)