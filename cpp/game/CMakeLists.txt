set(GAME_SOURCES
  track.cpp
  command.cpp
)

set(GAME_HEADERS
  car.h
  lane.h
  position.h
  race.h
  track.h
  command.h
)

set(GAME_TESTS
  track_test.cpp
)

add_library(game ${GAME_SOURCES})

add_style_check_target(game "${GAME_SOURCES}" HWO)
add_style_check_target(game "${GAME_HEADERS}" HWO)

add_executable(game_tests ${GAME_TESTS})
target_link_libraries(game_tests game gtest gtest_main)
add_test(game_tests game_tests)