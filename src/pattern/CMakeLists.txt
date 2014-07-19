file(GLOB HEADER_FILES *.h)
add_library(patterns STATIC FastFlip.cpp randomPlayTest.cpp Patterns.cpp Stable.cpp ${HEADER_FILES})

add_executable(randomPlayTest randomPlayMain.cpp)
target_link_libraries(randomPlayTest patterns)
target_link_libraries(randomPlayTest core)
target_link_libraries(randomPlayTest n64)
target_link_libraries(randomPlayTest odk)

add_test(randomTest randomPlayTest)
