# Can also be configured independently to test selection without optional UI dependencies.
cmake_minimum_required(VERSION 3.22)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(flameshot-selection-tests LANGUAGES CXX)
    enable_testing()
endif()
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
set(CMAKE_AUTOMOC ON)
add_executable(selection-tests
    selectionwidget_test.cpp
    ../src/widgets/capture/selectiongeometry.cpp
    ../src/widgets/capture/selectionwidget.cpp
    ../src/widgets/capture/selectionwidget.h
    ../src/utils/globalvalues.cpp)
target_include_directories(selection-tests PRIVATE ../src)
target_compile_features(selection-tests PRIVATE cxx_std_20)
target_compile_definitions(selection-tests PRIVATE APP_VERSION="test" FLAMESHOT_GIT_HASH="test")
target_link_libraries(selection-tests PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME selection-aspect-ratio COMMAND selection-tests)
set_tests_properties(selection-aspect-ratio PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
