add_subdirectory(parser)
add_subdirectory(synthesis)

include_directories(${PARSER_INCLUDE_PATH} ${SYNTHESIS_INCLUDE_PATH} ${EXT_INCLUDE_PATH})

add_executable(rims4ltlf rims.cpp)
add_executable(rims4ltlfBenchs rimsBenchs.cpp)
add_executable(MaxSyft maxsyft.cpp)
target_link_libraries(rims4ltlf ${PARSER_LIB_NAME} ${SYNTHESIS_LIB_NAME})
target_link_libraries(rims4ltlfBenchs ${PARSER_LIB_NAME} ${SYNTHESIS_LIB_NAME})
target_link_libraries(MaxSyft ${PARSER_LIB_NAME} ${SYNTHESIS_LIB_NAME})

INSTALL (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*")
