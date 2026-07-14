if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT must point to the Qt source tree")
endif()

set(files)
file(GLOB_RECURSE source_files
    LIST_DIRECTORIES false
    "${ROOT}/src/*.cpp"
    "${ROOT}/src/*.h"
    "${ROOT}/src/*.hpp"
    "${ROOT}/tests/*.cpp"
    "${ROOT}/tests/*.h"
    "${ROOT}/tests/*.hpp"
)
list(APPEND files ${source_files})
list(REMOVE_DUPLICATES files)

set(forbidden_chinese_english_config_pattern "需要[^\n\"]*(analysis config|GTP config)")
foreach(file IN LISTS files)
    file(READ "${file}" content)
    if(content MATCHES "${forbidden_chinese_english_config_pattern}")
        message(FATAL_ERROR "Chinese diagnostic contains untranslated config wording in ${file}")
    endif()
endforeach()

message(STATUS "Localization text check passed for ${ROOT}")
