# Run pirate_unit_tests and fail if branch coverage is not above PIRATE_COVERAGE_FAIL_UNDER.
# GCC/Clang: gcovr --fail-under-branch. MSVC: OpenCppCoverage Cobertura XML if on PATH.

if(NOT PIRATE_UNIT_TEST_EXE)
  message(FATAL_ERROR "PIRATE_UNIT_TEST_EXE is required")
endif()
if(NOT PIRATE_COVERAGE_FAIL_UNDER)
  set(PIRATE_COVERAGE_FAIL_UNDER 80)
endif()

execute_process(
    COMMAND "${PIRATE_UNIT_TEST_EXE}"
    RESULT_VARIABLE _test_rc)
if(NOT _test_rc EQUAL 0)
  message(FATAL_ERROR "pirate_unit_tests failed with ${_test_rc}")
endif()

find_program(_gcovr gcovr)
find_program(_occ NAMES OpenCppCoverage OpenCppCoverage.exe)

if(_gcovr)
  execute_process(
      COMMAND "${_gcovr}"
              --root "${PIRATE_SOURCE_DIR}"
              --filter "${PIRATE_SOURCE_DIR}/src/pirate"
              --branches
              --exclude-throw-branches
              --fail-under-branch ${PIRATE_COVERAGE_FAIL_UNDER}
      WORKING_DIRECTORY "${PIRATE_BINARY_DIR}"
      RESULT_VARIABLE _cov_rc)
  if(NOT _cov_rc EQUAL 0)
    message(FATAL_ERROR
        "Branch coverage is not above ${PIRATE_COVERAGE_FAIL_UNDER}% (gcovr exit ${_cov_rc})")
  endif()
elseif(_occ)
  set(_xml "${PIRATE_BINARY_DIR}/coverage.xml")
  execute_process(
      COMMAND "${_occ}"
              --sources "${PIRATE_SOURCE_DIR}/src/pirate"
              --export_type "cobertura:${_xml}"
              -- "${PIRATE_UNIT_TEST_EXE}"
      WORKING_DIRECTORY "${PIRATE_BINARY_DIR}"
      RESULT_VARIABLE _cov_rc)
  if(NOT _cov_rc EQUAL 0)
    message(FATAL_ERROR "OpenCppCoverage failed")
  endif()
  file(READ "${_xml}" _xml_text)
  string(REGEX MATCH "branch-rate=\"1(\\.0*)?\"" _full "${_xml_text}")
  if(_full)
    set(_pct 100)
  else()
    string(REGEX MATCH "branch-rate=\"0\\.([0-9][0-9])" _frac "${_xml_text}")
    if(NOT CMAKE_MATCH_1)
      message(FATAL_ERROR "Could not parse branch-rate from ${_xml}")
    endif()
    set(_pct ${CMAKE_MATCH_1})
  endif()
  message(STATUS "Branch coverage: ${_pct}% (required > ${PIRATE_COVERAGE_FAIL_UNDER}%)")
  if(_pct LESS_EQUAL PIRATE_COVERAGE_FAIL_UNDER)
    message(FATAL_ERROR "Branch coverage ${_pct}% is not above ${PIRATE_COVERAGE_FAIL_UNDER}%")
  endif()
else()
  message(WARNING
      "No gcovr or OpenCppCoverage found; tests passed but branch coverage was not measured. "
      "Install gcovr (GCC/Clang with -DPIRATE_ENABLE_COVERAGE=ON) or OpenCppCoverage (MSVC).")
endif()
