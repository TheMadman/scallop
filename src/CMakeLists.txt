add_library(lexer lexer.c)
target_link_libraries(lexer csalt)
target_include_directories(lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(scallop scallop.c)
target_link_libraries(scallop lexer)
