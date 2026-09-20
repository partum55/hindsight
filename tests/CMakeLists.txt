include(FetchContent)

FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(doctest)

add_executable(hindsight_tests test_cli.cpp)
target_link_libraries(hindsight_tests PRIVATE hindsight_core doctest::doctest)

add_test(NAME hindsight_tests COMMAND hindsight_tests)
