if(WITH_FUZZERS)
	add_subdirectory(libfuzzer)
endif()

if(WITH_TESTS)
	add_subdirectory(unit)
endif()
