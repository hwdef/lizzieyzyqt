if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(work_dir "${TEST_BINARY_DIR}/platform-cmake")
file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${work_dir}")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        -S "${ROOT}"
        -B "${work_dir}/darwin"
        -DCMAKE_SYSTEM_NAME=Darwin
        -DLIZZIE_BUILD_TESTS=OFF
    RESULT_VARIABLE darwin_result
    OUTPUT_VARIABLE darwin_output
    ERROR_VARIABLE darwin_error
)
if(darwin_result EQUAL 0)
    message(FATAL_ERROR "Darwin CMake configuration should fail for the first Qt release")
endif()
set(darwin_combined_output "${darwin_output}${darwin_error}")
if(NOT darwin_combined_output MATCHES "Linux and Windows" OR
    NOT darwin_combined_output MATCHES "macOS is not" OR
    NOT darwin_combined_output MATCHES "PLAN\\.md")
    message(FATAL_ERROR "Darwin failure should explain the PLAN.md platform scope: ${darwin_output}${darwin_error}")
endif()

file(READ "${ROOT}/src/main.cpp" main_cpp)
file(READ "${ROOT}/CMakeLists.txt" cmake_lists)
if(NOT cmake_lists MATCHES "LIZZIE_PACKAGE_PLATFORM \"Windows\"" OR
    NOT cmake_lists MATCHES "LIZZIE_PACKAGE_PLATFORM \"Linux\"" OR
    NOT cmake_lists MATCHES "CPACK_PACKAGE_FILE_NAME")
    message(FATAL_ERROR "CPack packages should use deterministic Linux/Windows platform-specific filenames")
endif()

if(NOT main_cpp MATCHES "platformName\\.startsWith\\(\"wayland\"\\)" OR
    NOT main_cpp MATCHES "appendUnique\\(baseNames, \"qwayland\"\\)" OR
    NOT main_cpp MATCHES "appendUnique\\(baseNames, \"qwayland-generic\"\\)" OR
    NOT main_cpp MATCHES "appendUnique\\(baseNames, \"qwayland-egl\"\\)")
    message(
        FATAL_ERROR
        "Wayland diagnostics should check the common qwayland platform plugin before legacy-specific names")
endif()

if(NOT main_cpp MATCHES "platformName\\.startsWith\\(\"windows\"\\)" OR
    NOT main_cpp MATCHES "commonWindowsPlatformPluginBaseNames" OR
    NOT main_cpp MATCHES "appendUnique\\(baseNames, \"qwindows\"\\)" OR
    NOT main_cpp MATCHES "qt\\.platformPlugin\\.commonWindows")
    message(
        FATAL_ERROR
        "Windows diagnostics should always check the common qwindows platform plugin")
endif()

message(STATUS "platform_cmake_tests passed")
