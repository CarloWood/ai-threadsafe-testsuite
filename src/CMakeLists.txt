include( AICxxProject )

add_executable( threadsafe_test threadsafe_test.cxx )
target_link_libraries( threadsafe_test PRIVATE ${AICXX_OBJECTS_LIST} )

add_executable( AIReadWriteMutex_test AIReadWriteMutex_test.cxx )
target_link_libraries( AIReadWriteMutex_test PRIVATE ${AICXX_OBJECTS_LIST} )

add_executable( condition_variable_test condition_variable_test.cxx )
target_link_libraries( condition_variable_test PRIVATE ${AICXX_OBJECTS_LIST} )

if ( TARGET MoodyCamel::microbench )
  add_executable( AIReadWriteSpinLock_test AIReadWriteSpinLock_test.cxx )
  target_link_libraries( AIReadWriteSpinLock_test PRIVATE ${AICXX_OBJECTS_LIST} MoodyCamel::microbench )
endif ()
