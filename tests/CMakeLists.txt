function(testcase target source)
	add_executable(${target}_test ${source})
	target_link_libraries(${target}_test csalt lexer)
	add_test(NAME ${target} COMMAND ${target}_test)
endfunction(testcase)

testcase(scallop_lexer scallop_lexer.c)
