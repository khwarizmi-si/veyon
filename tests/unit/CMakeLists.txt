add_executable(license-test LicenseTest.cpp)
target_link_libraries(license-test PRIVATE veyon-core Qt${QT_MAJOR_VERSION}::Test)
target_compile_definitions(license-test PRIVATE LICENSE_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
target_compile_definitions(license-test PRIVATE VEYON_ID_TS_PATH="${CMAKE_SOURCE_DIR}/translations/veyon_id.ts")
set_default_target_properties(license-test)
# <QTest> pulls in Qt 5.15's qsizepolicy.h, whose inline enum arithmetic is deprecated under
# C++20, and NO_SYSTEM_FROM_IMPORTED makes -Werror apply to Qt's own headers
target_compile_options(license-test PRIVATE "-Wno-deprecated-enum-enum-conversion")
add_test(NAME license-test COMMAND license-test)
