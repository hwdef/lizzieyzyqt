add_executable(lizzie_core_tests
    core_tests.cpp
)

target_link_libraries(lizzie_core_tests
    PRIVATE
        lizzie_core
)

add_test(NAME lizzie_core_tests COMMAND lizzie_core_tests)

add_test(
    NAME lizzie_style_check
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/StyleCheck.cmake
)

add_test(
    NAME lizzie_no_java_ui_dependencies
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/NoJavaUiDeps.cmake
)

add_test(
    NAME lizzie_no_direct_ui_gtp_commands
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/NoDirectUiGtpCommands.cmake
)

add_test(
    NAME lizzie_layer_boundary_check
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/LayerBoundaryCheck.cmake
)

add_test(
    NAME lizzie_first_release_scope_check
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/FirstReleaseScopeCheck.cmake
)

add_test(
    NAME lizzie_localization_text_check
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/LocalizationTextCheck.cmake
)

add_test(
    NAME lizzie_platform_cmake_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT=${PROJECT_SOURCE_DIR}
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/platform_cmake_tests.cmake
)

add_test(
    NAME lizzie_ci_workflow_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/ci_workflow_tests.cmake
)

add_test(
    NAME lizzie_no_java_ui_deps_script_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT=${PROJECT_SOURCE_DIR}
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/no_java_ui_deps_script_tests.cmake
)

add_test(
    NAME lizzie_no_direct_ui_gtp_commands_script_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT=${PROJECT_SOURCE_DIR}
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/no_direct_ui_gtp_commands_script_tests.cmake
)

add_test(
    NAME lizzie_layer_boundary_check_script_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT=${PROJECT_SOURCE_DIR}
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/layer_boundary_check_script_tests.cmake
)

add_test(
    NAME lizzie_first_release_scope_check_script_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT=${PROJECT_SOURCE_DIR}
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/first_release_scope_check_script_tests.cmake
)

add_test(
    NAME lizzie_localization_text_check_script_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT=${PROJECT_SOURCE_DIR}
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/localization_text_check_tests.cmake
)

add_test(
    NAME lizzie_verify_package_script_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT=${PROJECT_SOURCE_DIR}
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_package_script_tests.cmake
)

add_test(NAME lizzie_cli_version COMMAND lizzieyzy_qt --version)
set_tests_properties(
    lizzie_cli_version
    PROPERTIES PASS_REGULAR_EXPRESSION "LizzieYzy Qt ${PROJECT_VERSION}"
)

add_test(
    NAME lizzie_cli_argument_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DAPP_EXECUTABLE=$<TARGET_FILE:lizzieyzy_qt>
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_argument_tests.cmake
)

add_test(NAME lizzie_cli_diagnostics COMMAND lizzieyzy_qt --diagnostics)
set_tests_properties(
    lizzie_cli_diagnostics
    PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        PASS_REGULAR_EXPRESSION "qt\\.quick\\.graphicsApi: "
)

add_test(NAME lizzie_cli_target_acceptance_record_template COMMAND lizzieyzy_qt --target-acceptance-record-template)
set_tests_properties(
    lizzie_cli_target_acceptance_record_template
    PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=missing-qpa-for-template-test"
        PASS_REGULAR_EXPRESSION "^\\[metadata\\]"
)

set(cli_diagnostics_model "${CMAKE_CURRENT_BINARY_DIR}/diagnostics-model.bin.gz")
set(cli_diagnostics_analysis_config "${CMAKE_CURRENT_BINARY_DIR}/diagnostics-analysis.cfg")
set(cli_diagnostics_gtp_config "${CMAKE_CURRENT_BINARY_DIR}/diagnostics-gtp.cfg")
file(WRITE "${cli_diagnostics_model}" "diagnostics model fixture\n")
file(WRITE "${cli_diagnostics_analysis_config}" "diagnostics analysis config fixture\n")
file(WRITE "${cli_diagnostics_gtp_config}" "diagnostics gtp config fixture\n")
add_test(
    NAME lizzie_cli_diagnostics_katago_env
    COMMAND
        ${CMAKE_COMMAND} -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=$<TARGET_FILE:lizzieyzy_qt>"
        "LIZZIE_KATAGO_MODEL=${cli_diagnostics_model}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${cli_diagnostics_analysis_config}"
        "LIZZIE_KATAGO_GTP_CONFIG=${cli_diagnostics_gtp_config}"
        $<TARGET_FILE:lizzieyzy_qt> --diagnostics
)
set_tests_properties(
    lizzie_cli_diagnostics_katago_env
    PROPERTIES PASS_REGULAR_EXPRESSION "katago\\.env\\.executable\\.executable: true"
)

add_test(
    NAME lizzie_cli_diagnostics_fields
    COMMAND
        ${CMAKE_COMMAND}
        -DAPP_EXECUTABLE=$<TARGET_FILE:lizzieyzy_qt>
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_diagnostics_tests.cmake
)

add_test(
    NAME lizzie_cli_diagnostics_readiness
    COMMAND
        ${CMAKE_COMMAND}
        -DAPP_EXECUTABLE=$<TARGET_FILE:lizzieyzy_qt>
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_diagnostics_readiness_tests.cmake
)

add_executable(lizzie_target_acceptance_image_fixture
    target_acceptance_image_fixture.cpp
)

target_link_libraries(lizzie_target_acceptance_image_fixture
    PRIVATE
        Qt6::Core
        Qt6::Gui
)

add_test(
    NAME lizzie_cli_target_acceptance_report
    COMMAND
        ${CMAKE_COMMAND}
        -DAPP_EXECUTABLE=$<TARGET_FILE:lizzieyzy_qt>
        -DIMAGE_FIXTURE_WRITER=$<TARGET_FILE:lizzie_target_acceptance_image_fixture>
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_target_acceptance_report_tests.cmake
)

add_executable(lizzie_engine_protocol_tests
    engine_protocol_tests.cpp
)

target_link_libraries(lizzie_engine_protocol_tests
    PRIVATE
        lizzie_engine
)

add_test(NAME lizzie_engine_protocol_tests COMMAND lizzie_engine_protocol_tests)

add_executable(lizzie_realtime_analysis_tests
    realtime_analysis_tests.cpp
)

target_link_libraries(lizzie_realtime_analysis_tests
    PRIVATE
        lizzie_engine
)

add_test(NAME lizzie_realtime_analysis_tests COMMAND lizzie_realtime_analysis_tests)

add_executable(lizzie_analysis_json_tests
    analysis_json_tests.cpp
)

target_link_libraries(lizzie_analysis_json_tests
    PRIVATE
        lizzie_engine
)

add_test(NAME lizzie_analysis_json_tests COMMAND lizzie_analysis_json_tests)

add_executable(lizzie_batch_analysis_tests
    batch_analysis_tests.cpp
)

target_link_libraries(lizzie_batch_analysis_tests
    PRIVATE
        lizzie_engine
)

add_test(NAME lizzie_batch_analysis_tests COMMAND lizzie_batch_analysis_tests)

add_executable(lizzie_position_sync_tests
    position_sync_tests.cpp
)

target_link_libraries(lizzie_position_sync_tests
    PRIVATE
        lizzie_engine
)

add_test(NAME lizzie_position_sync_tests COMMAND lizzie_position_sync_tests)

add_executable(lizzie_performance_tests
    performance_tests.cpp
)

target_link_libraries(lizzie_performance_tests
    PRIVATE
        lizzie_engine
)

add_test(NAME lizzie_performance_tests COMMAND lizzie_performance_tests)

add_executable(lizzie_analysis_graph_widget_tests
    analysis_graph_widget_tests.cpp
)

target_link_libraries(lizzie_analysis_graph_widget_tests
    PRIVATE
        lizzie_ui
        Qt6::Widgets
)

add_test(NAME lizzie_analysis_graph_widget_tests COMMAND lizzie_analysis_graph_widget_tests)
set_tests_properties(lizzie_analysis_graph_widget_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_test(NAME lizzie_analysis_graph_widget_tests_scale1_5 COMMAND lizzie_analysis_graph_widget_tests)
set_tests_properties(
    lizzie_analysis_graph_widget_tests_scale1_5
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=1.5"
)
add_test(NAME lizzie_analysis_graph_widget_tests_scale2 COMMAND lizzie_analysis_graph_widget_tests)
set_tests_properties(
    lizzie_analysis_graph_widget_tests_scale2
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=2"
)

add_executable(lizzie_board_quick_item_tests
    board_quick_item_tests.cpp
)

target_link_libraries(lizzie_board_quick_item_tests
    PRIVATE
        lizzie_ui
        Qt6::Widgets
)

add_test(NAME lizzie_board_quick_item_tests COMMAND lizzie_board_quick_item_tests)
set_tests_properties(lizzie_board_quick_item_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_test(NAME lizzie_board_quick_item_tests_scale1_5 COMMAND lizzie_board_quick_item_tests)
set_tests_properties(
    lizzie_board_quick_item_tests_scale1_5
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=1.5"
)
add_test(NAME lizzie_board_quick_item_tests_scale2 COMMAND lizzie_board_quick_item_tests)
set_tests_properties(
    lizzie_board_quick_item_tests_scale2
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=2"
)

add_executable(lizzie_ui_smoke_tests
    ui_smoke_tests.cpp
)

target_link_libraries(lizzie_ui_smoke_tests
    PRIVATE
        lizzie_ui
        Qt6::Widgets
)

add_test(NAME lizzie_ui_smoke_tests COMMAND lizzie_ui_smoke_tests $<TARGET_FILE:fake_katago> $<TARGET_FILE:fake_analysis_engine>)
set_tests_properties(lizzie_ui_smoke_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_test(NAME lizzie_ui_smoke_tests_scale1_5 COMMAND lizzie_ui_smoke_tests $<TARGET_FILE:fake_katago> $<TARGET_FILE:fake_analysis_engine>)
set_tests_properties(
    lizzie_ui_smoke_tests_scale1_5
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=1.5"
)
add_test(NAME lizzie_ui_smoke_tests_scale2 COMMAND lizzie_ui_smoke_tests $<TARGET_FILE:fake_katago> $<TARGET_FILE:fake_analysis_engine>)
set_tests_properties(
    lizzie_ui_smoke_tests_scale2
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=2"
)

add_executable(lizzie_settings_dialog_tests
    settings_dialog_tests.cpp
)

target_link_libraries(lizzie_settings_dialog_tests
    PRIVATE
        lizzie_ui
        Qt6::Widgets
)

add_test(NAME lizzie_settings_dialog_tests COMMAND lizzie_settings_dialog_tests)
set_tests_properties(lizzie_settings_dialog_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(lizzie_settings_apply_tests
    settings_apply_tests.cpp
)

target_link_libraries(lizzie_settings_apply_tests
    PRIVATE
        lizzie_app
        Qt6::Gui
)

add_test(NAME lizzie_settings_apply_tests COMMAND lizzie_settings_apply_tests)

add_executable(lizzie_engine_automation_tests
    engine_automation_tests.cpp
)

target_link_libraries(lizzie_engine_automation_tests
    PRIVATE
        lizzie_app
)

add_test(NAME lizzie_engine_automation_tests COMMAND lizzie_engine_automation_tests)

add_executable(lizzie_file_write_tests
    file_write_tests.cpp
)

target_link_libraries(lizzie_file_write_tests
    PRIVATE
        lizzie_app
        Qt6::Core
)

add_test(NAME lizzie_file_write_tests COMMAND lizzie_file_write_tests)

add_executable(lizzie_app_session_settings_tests
    app_session_settings_tests.cpp
)

target_link_libraries(lizzie_app_session_settings_tests
    PRIVATE
        lizzie_app
        Qt6::Core
)

add_test(NAME lizzie_app_session_settings_tests COMMAND lizzie_app_session_settings_tests)

add_executable(lizzie_app_settings_model_tests
    app_settings_model_tests.cpp
)

target_link_libraries(lizzie_app_settings_model_tests
    PRIVATE
        lizzie_app
        Qt6::Gui
)

add_test(NAME lizzie_app_settings_model_tests COMMAND lizzie_app_settings_model_tests)

add_executable(lizzie_gtp_console_widget_tests
    gtp_console_widget_tests.cpp
)

target_link_libraries(lizzie_gtp_console_widget_tests
    PRIVATE
        lizzie_ui
        Qt6::Widgets
)

add_test(NAME lizzie_gtp_console_widget_tests COMMAND lizzie_gtp_console_widget_tests)
set_tests_properties(lizzie_gtp_console_widget_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(lizzie_legacy_config_import_tests
    legacy_config_import_tests.cpp
)

target_link_libraries(lizzie_legacy_config_import_tests
    PRIVATE
        lizzie_app
        Qt6::Gui
)

add_test(NAME lizzie_legacy_config_import_tests COMMAND lizzie_legacy_config_import_tests)
set_tests_properties(lizzie_legacy_config_import_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(fake_katago
    fake_katago.cpp
)

add_executable(fake_analysis_engine
    fake_analysis_engine.cpp
)

add_dependencies(lizzie_ui_smoke_tests fake_katago fake_analysis_engine)

add_executable(lizzie_katago_process_fixture_tests
    katago_process_fixture_tests.cpp
)
add_dependencies(lizzie_katago_process_fixture_tests fake_katago)

target_link_libraries(lizzie_katago_process_fixture_tests
    PRIVATE
        lizzie_engine
        Qt6::Core
)

add_test(
    NAME lizzie_katago_process_fixture_tests
    COMMAND lizzie_katago_process_fixture_tests $<TARGET_FILE:fake_katago>
)

add_executable(lizzie_analysis_process_fixture_tests
    analysis_process_fixture_tests.cpp
)
add_dependencies(lizzie_analysis_process_fixture_tests fake_analysis_engine)

target_link_libraries(lizzie_analysis_process_fixture_tests
    PRIVATE
        lizzie_engine
        Qt6::Core
)

add_test(
    NAME lizzie_analysis_process_fixture_tests
    COMMAND lizzie_analysis_process_fixture_tests $<TARGET_FILE:fake_analysis_engine>
)

add_executable(lizzie_engine_worker_tests
    engine_worker_tests.cpp
)
add_dependencies(lizzie_engine_worker_tests fake_katago fake_analysis_engine)

target_link_libraries(lizzie_engine_worker_tests
    PRIVATE
        lizzie_engine
        Qt6::Core
)

add_test(
    NAME lizzie_engine_worker_tests
    COMMAND lizzie_engine_worker_tests $<TARGET_FILE:fake_katago> $<TARGET_FILE:fake_analysis_engine>
)

add_executable(lizzie_katago_integration_tests
    katago_integration_tests.cpp
)

target_link_libraries(lizzie_katago_integration_tests
    PRIVATE
        lizzie_engine
        Qt6::Core
)

add_test(NAME lizzie_katago_integration_tests COMMAND lizzie_katago_integration_tests)
set_tests_properties(
    lizzie_katago_integration_tests
    PROPERTIES SKIP_REGULAR_EXPRESSION "katago_integration_tests skipped"
)

add_test(
    NAME lizzie_katago_integration_preflight_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DKATAGO_INTEGRATION_TEST=$<TARGET_FILE:lizzie_katago_integration_tests>
        -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/katago_integration_preflight_tests.cmake
)
