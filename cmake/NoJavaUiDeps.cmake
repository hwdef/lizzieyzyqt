if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT must point to the Qt source tree")
endif()

set(files)
file(GLOB_RECURSE source_files
    LIST_DIRECTORIES false
    "${ROOT}/src/*.cpp"
    "${ROOT}/src/*.h"
    "${ROOT}/src/*.hpp"
    "${ROOT}/src/*.qml"
    "${ROOT}/src/CMakeLists.txt"
    "${ROOT}/src/*/CMakeLists.txt"
    "${ROOT}/tests/*.cpp"
    "${ROOT}/tests/*.h"
    "${ROOT}/tests/*.hpp"
    "${ROOT}/tests/*.qml"
    "${ROOT}/tests/CMakeLists.txt"
    "${ROOT}/tests/*/CMakeLists.txt"
    "${ROOT}/cmake/*.cmake"
)
list(APPEND files "${ROOT}/CMakeLists.txt" ${source_files})
list(REMOVE_ITEM files "${ROOT}/cmake/NoJavaUiDeps.cmake")
list(REMOVE_ITEM files "${ROOT}/cmake/VerifyPackage.cmake")

set(forbidden_pattern "java[.]awt|javax[.]swing|Swing|AWT|Lizzie[.]")
foreach(file IN LISTS files)
    file(READ "${file}" content)
    if(content MATCHES "${forbidden_pattern}")
        message(FATAL_ERROR "Java UI dependency or legacy Lizzie global-state marker found in ${file}")
    endif()
endforeach()

message(STATUS "No Java UI dependency or legacy Lizzie global-state markers found under ${ROOT}")
