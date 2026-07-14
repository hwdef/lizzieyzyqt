if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT must point to the Qt source tree")
endif()

file(GLOB_RECURSE ui_files
    LIST_DIRECTORIES false
    "${ROOT}/src/ui/*.cpp"
    "${ROOT}/src/ui/*.h"
    "${ROOT}/src/ui/*.hpp"
    "${ROOT}/src/ui/*.qml"
)

set(presentation_files ${ui_files})
if(EXISTS "${ROOT}/src/main.cpp")
    list(APPEND presentation_files "${ROOT}/src/main.cpp")
endif()
list(REMOVE_DUPLICATES presentation_files)

set(forbidden_patterns
    "GtpCommand[ \t\r\n]*[{]"
    "([.][ \t\r\n]*|->[ \t\r\n]*)sendCommand[ \t\r\n]*[(]"
)

foreach(file IN LISTS presentation_files)
    file(READ "${file}" content)
    foreach(pattern IN LISTS forbidden_patterns)
        if(content MATCHES "${pattern}")
            message(FATAL_ERROR "Direct presentation GTP command construction or send found in ${file}")
        endif()
    endforeach()
endforeach()

message(STATUS "No direct presentation GTP command construction found under ${ROOT}/src/ui and ${ROOT}/src/main.cpp")
