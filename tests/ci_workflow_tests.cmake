if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

set(workflow_path "${ROOT}/.github/workflows/qt-cmake.yml")
if(NOT EXISTS "${workflow_path}")
    message(FATAL_ERROR "Qt CI workflow is missing: ${workflow_path}")
endif()

file(READ "${workflow_path}" workflow)

function(require_workflow_text pattern description)
    if(NOT workflow MATCHES "${pattern}")
        message(FATAL_ERROR "Qt CI workflow should ${description}")
    endif()
endfunction()

require_workflow_text("push:" "run for pushes")
require_workflow_text("pull_request:" "run for pull requests")
require_workflow_text("ubuntu-24\\.04" "build on Linux")
require_workflow_text("windows-2022" "build on Windows")
require_workflow_text("jurplel/install-qt-action@v4" "install Qt 6 through the CI setup")
require_workflow_text("cmake -S \\. -B build -G Ninja -DCMAKE_BUILD_TYPE=Release" "configure the root Qt project")
require_workflow_text("cmake --build build" "build the root Qt project")
require_workflow_text("ctest --test-dir build --parallel 2 --output-on-failure" "run the CTest suite")
require_workflow_text("cmake --install build --prefix" "smoke the install tree")
require_workflow_text("Verify installed diagnostics" "run installed diagnostics smoke")
require_workflow_text("Verify installed target acceptance report" "run installed target acceptance report smoke")
require_workflow_text(
    "Verify installed target acceptance record template"
    "run installed target acceptance record template smoke")
require_workflow_text("cmake -E env" "set diagnostics environment through CMake")
require_workflow_text("--target-acceptance-report" "run the target acceptance report CLI")
require_workflow_text("--target-acceptance-record-template" "run the target acceptance record template CLI")
require_workflow_text(
    "missing-qpa-for-installed-record-template-smoke"
    "prove installed target acceptance record template generation is QPA-independent")
require_workflow_text("cmake --build build --target package" "create a package")
require_workflow_text("build/LizzieYzyQt-\\*-Linux\\.tar\\.gz" "produce the platform-named Linux tar package")
require_workflow_text("build/LizzieYzyQt-\\*-Windows\\.zip" "produce the platform-named Windows zip package")
require_workflow_text("package-platform: \"Linux\"" "name the Linux package platform")
require_workflow_text("package-platform: \"Windows\"" "name the Windows package platform")
require_workflow_text("PACKAGE_EXPECT_PLATFORM" "verify platform-specific package filenames")
require_workflow_text("PACKAGE_REQUIRE_QT_DEPLOY" "verify Windows Qt deployment contents")
require_workflow_text("PACKAGE_RUN_VERSION_SMOKE=ON" "run the packaged executable smoke test")
require_workflow_text("PACKAGE_RUN_DIAGNOSTICS_SMOKE=ON" "run the packaged diagnostics smoke test")
require_workflow_text(
    "PACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE=ON"
    "run the packaged target acceptance record template smoke test")
require_workflow_text("PACKAGE_RUN_TARGET_ACCEPTANCE_REPORT_SMOKE=ON" "run the packaged target acceptance report smoke test")
require_workflow_text("PACKAGE_DIAGNOSTICS_QPA_PLATFORM" "configure diagnostics QPA platform per CI target")
require_workflow_text("PACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM" "configure target acceptance report QPA platform per CI target")
require_workflow_text("diagnostics-qpa-platform: \"offscreen\"" "run Linux diagnostics through offscreen QPA")
require_workflow_text("diagnostics-qpa-platform: \"windows\"" "run Windows diagnostics through the native QPA plugin")
require_workflow_text("package-diagnostics-smoke" "keep diagnostics smoke extraction separate")
require_workflow_text(
    "package-target-acceptance-record-template-smoke"
    "keep target acceptance record template smoke extraction separate")
require_workflow_text("package-target-acceptance-report-smoke" "keep target acceptance report smoke extraction separate")
require_workflow_text("cmake/VerifyPackage\\.cmake" "verify package contents")
require_workflow_text("actions/upload-artifact@v4" "upload package artifacts")

message(STATUS "ci_workflow_tests passed")
