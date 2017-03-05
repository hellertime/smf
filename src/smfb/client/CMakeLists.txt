add_executable (smfb_client
  ${PROJECT_SOURCE_DIR}/src/smfb/client/main.cc
  )
target_link_libraries (smfbclient smfrpc cr)
install(TARGETS smfb_client DESTINATION bin)
