if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT must point to the Qt source tree")
endif()

file(GLOB_RECURSE source_files
    LIST_DIRECTORIES false
    "${ROOT}/src/*.cpp"
    "${ROOT}/src/*.h"
    "${ROOT}/src/*.hpp"
    "${ROOT}/src/*.qml"
    "${ROOT}/src/CMakeLists.txt"
    "${ROOT}/src/*/CMakeLists.txt"
)

set(out_of_scope_path_pattern
    "(^|/)[^/]*(remote[-_]?ssh|ssh[-_]?engine|remote[-_]?engine|fox|online[-_]?board|online[-_]?sgf|screen[-_]?board[-_]?sync|screen[-_]?sync|distributed[-_]?training|katago[-_]?training|training[-_]?worker|gnugo|leelaz|leela[-_]?zero|pachi|fuego)[^/]*($|/)")
set(out_of_scope_declaration_pattern
    "(class|struct|namespace|enum class|void|bool|auto|qaction|qmenu)[^\n]*(remote[-_ ]?ssh|ssh[-_ ]?engine|remote[-_ ]?engine|fox|online[-_ ]?board|online[-_ ]?sgf|screen[-_ ]?board[-_ ]?sync|screen[-_ ]?sync|distributed[-_ ]?training|katago[-_ ]?training|training[-_ ]?worker|gnugo|leelaz|leela[-_ ]?zero|pachi|fuego)")

foreach(file IN LISTS source_files)
    file(RELATIVE_PATH relative_file "${ROOT}" "${file}")
    string(REPLACE "\\" "/" normalized_file "${relative_file}")
    string(TOLOWER "${normalized_file}" lowercase_file)
    if(lowercase_file MATCHES "${out_of_scope_path_pattern}")
        message(FATAL_ERROR "First-release out-of-scope implementation path found in ${file}")
    endif()

    file(READ "${file}" content)
    string(TOLOWER "${content}" lowercase_content)
    if(lowercase_content MATCHES "${out_of_scope_declaration_pattern}")
        message(FATAL_ERROR "First-release out-of-scope implementation marker found in ${file}")
    endif()
endforeach()

message(STATUS "No first-release out-of-scope implementation markers found under ${ROOT}/src")
