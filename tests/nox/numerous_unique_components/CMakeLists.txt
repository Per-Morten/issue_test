get_filename_component(TEST_NAME "${CMAKE_CURRENT_LIST_DIR}" NAME)
set(TEST_NAME ${PREFIX}_${TEST_NAME})

add_definitions(-DTRIVIAL_COMPONENT_COUNT=${NUC_TRIVIAL_COMPONENT_COUNT})
add_definitions(-DSEND_LOCAL_EVENT_PER_N_COMPONMENT=13)
add_definitions(-DSEND_GLOBAL_EVENT_PER_N_COMPONMENT=11)

project(${TEST_NAME})
 

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE TEMPLATES "src/*.tpp")
file(GLOB_RECURSE INLINES "src/*.ipp")
file(GLOB_RECURSE HEADERS "src/*.h")


set(ALL_SOURCES
    ${SOURCES}
    ${TEMPLATES}
    ${INLINES}
    ${HEADERS})

set(INCLUDE_DIRS src/ ${CMAKE_SOURCE_DIR}/ecs/src)


include_directories(${INCLUDE_DIRS})
add_executable (${TEST_NAME} ${ALL_SOURCES}) 
target_link_libraries(${TEST_NAME} ${NOX_ECS_LIBRARY} ${CMD_LIBRARY})
