if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT must point to the project source tree")
endif()

set(files
    "${ROOT}/.clang-format"
    "${ROOT}/CMakeLists.txt"
    "${ROOT}/README.md"
)

foreach(pattern IN ITEMS
    "${ROOT}/.github/workflows/*.yml"
    "${ROOT}/.github/workflows/*.yaml"
    "${ROOT}/cmake/*.cmake"
    "${ROOT}/docs/*.md"
    "${ROOT}/src/*/*.cpp"
    "${ROOT}/src/*/*.h"
    "${ROOT}/src/*/CMakeLists.txt"
    "${ROOT}/tests/*.cpp"
    "${ROOT}/tests/*.h"
    "${ROOT}/tests/CMakeLists.txt"
)
    file(GLOB_RECURSE matched_files LIST_DIRECTORIES false "${pattern}")
    list(APPEND files ${matched_files})
endforeach()

list(REMOVE_DUPLICATES files)

foreach(file IN LISTS files)
    if(NOT EXISTS "${file}")
        continue()
    endif()
    file(READ "${file}" content)
    string(REPLACE "\r\n" "\n" content "${content}")
    if(content MATCHES "[ \t]\n")
        message(FATAL_ERROR "Trailing whitespace found in ${file}")
    endif()
    if(content MATCHES "[ \t]$")
        message(FATAL_ERROR "Trailing whitespace at end of file found in ${file}")
    endif()
    if(content MATCHES "\t")
        message(FATAL_ERROR "Tab character found in ${file}; use spaces for indentation")
    endif()
endforeach()

message(STATUS "Style check passed for ${ROOT}")
