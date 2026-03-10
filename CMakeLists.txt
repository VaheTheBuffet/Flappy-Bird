cmake_minimum_required(VERSION 3.10)
project(Flappy-Bird)
find_package(SDL3 CONFIG QUIET)

if(NOT SDL3_FOUND)
    message(STATUS "SDL3 not found. Building it from source.")
    include(FetchContent)
    FetchContent_Declare(
            SDL
            GIT_REPOSITORY https://github.com/libsdl-org/SDL
            GIT_TAG        "release-3.4.0"
    )
    FetchContent_MakeAvailable(SDL)
else()
    find_package(SDL3 REQUIRED)
endif()

add_executable(Flappy-Bird
    main.c
)

target_link_libraries(Flappy-Bird
    SDL3::SDL3
)

add_custom_command(TARGET Flappy-Bird POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Flappy-Bird>/assets
)
