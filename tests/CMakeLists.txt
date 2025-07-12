cmake_minimum_required(VERSION 3.25)

add_executable(vostok_camera_tests
    unit/camera/test_perspective_camera.cpp
    unit/camera/test_orthographic_camera.cpp
    unit/camera/test_frustum_camera.cpp
)

target_include_directories(vostok_camera_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(vostok_camera_tests
    PRIVATE
        vostok
        gtest
        gtest_main
        gmock
)

if(VOSTOK_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(vostok_camera_tests PRIVATE 
        --coverage
        -fprofile-arcs
        -ftest-coverage
        -O0
    )
    
    target_link_libraries(vostok_camera_tests PRIVATE 
        --coverage
        -lgcov
    )
endif()

target_compile_definitions(vostok_camera_tests
    PRIVATE
        VOSTOK_TESTING
)

include(GoogleTest)
gtest_discover_tests(vostok_camera_tests)