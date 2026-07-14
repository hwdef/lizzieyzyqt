add_library(lizzie_core STATIC
    BoardPosition.cpp
    GameModel.cpp
    Sgf.cpp
    Types.cpp
)

target_include_directories(lizzie_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(lizzie_core PUBLIC cxx_std_20)
