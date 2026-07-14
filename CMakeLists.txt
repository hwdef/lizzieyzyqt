cmake_minimum_required(VERSION 3.24)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR "LizzieYzyQt first-release targets are Linux and Windows; macOS is not supported by PLAN.md")
endif()

project(LizzieYzyQt VERSION 0.1.0 LANGUAGES CXX)

option(LIZZIE_BUILD_TESTS "Build LizzieYzy Qt tests" ON)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Quick QuickWidgets Qml)

add_subdirectory(src/core)
add_subdirectory(src/engine)
add_subdirectory(src/app)
add_subdirectory(src/ui)

add_executable(lizzieyzy_qt
    src/main.cpp
)

target_compile_definitions(lizzieyzy_qt
    PRIVATE
        LIZZIEYZY_QT_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(lizzieyzy_qt
    PRIVATE
        lizzie_ui
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        Qt6::Widgets
)

install(TARGETS lizzieyzy_qt
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

if(WIN32)
    qt_generate_deploy_app_script(
        TARGET lizzieyzy_qt
        OUTPUT_SCRIPT lizzieyzy_qt_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${lizzieyzy_qt_deploy_script})
endif()

install(
    FILES
        README.md
        PLAN.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

install(
    FILES
        docs/PlanCoverage.md
        docs/PlanRequirementAudit.md
        docs/Migration.md
        docs/Verification.md
        docs/TargetAcceptanceReport.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
)

if(LIZZIE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

set(CPACK_PACKAGE_NAME "LizzieYzyQt")
set(CPACK_PACKAGE_VENDOR "LizzieYzy")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Qt 6 application for LizzieYzy")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "LizzieYzyQt")
set(CPACK_VERBATIM_VARIABLES ON)
if(WIN32)
    set(LIZZIE_PACKAGE_PLATFORM "Windows")
    set(CPACK_GENERATOR "ZIP")
else()
    set(LIZZIE_PACKAGE_PLATFORM "Linux")
    set(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${LIZZIE_PACKAGE_PLATFORM}")
include(CPack)
