
if(BUILD_TESTING)
  message(AUTHOR_WARNING "Building Tests.")
  add_subdirectory(tests)
  add_subdirectory(live-tests)
endif()
