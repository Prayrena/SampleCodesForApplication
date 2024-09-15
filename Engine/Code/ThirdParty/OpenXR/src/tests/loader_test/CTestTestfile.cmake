# CMake generated Testfile for 
# Source directory: C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test
# Build directory: D:/HelloWorldTest/src/tests/loader_test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(loader_test "D:/HelloWorldTest/src/tests/loader_test/Debug/loader_test.exe")
  set_tests_properties(loader_test PROPERTIES  WORKING_DIRECTORY "D:/HelloWorldTest/src/tests/loader_test/Debug" _BACKTRACE_TRIPLES "C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test/CMakeLists.txt;71;add_test;C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(loader_test "D:/HelloWorldTest/src/tests/loader_test/Release/loader_test.exe")
  set_tests_properties(loader_test PROPERTIES  WORKING_DIRECTORY "D:/HelloWorldTest/src/tests/loader_test/Release" _BACKTRACE_TRIPLES "C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test/CMakeLists.txt;71;add_test;C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(loader_test "D:/HelloWorldTest/src/tests/loader_test/MinSizeRel/loader_test.exe")
  set_tests_properties(loader_test PROPERTIES  WORKING_DIRECTORY "D:/HelloWorldTest/src/tests/loader_test/MinSizeRel" _BACKTRACE_TRIPLES "C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test/CMakeLists.txt;71;add_test;C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(loader_test "D:/HelloWorldTest/src/tests/loader_test/RelWithDebInfo/loader_test.exe")
  set_tests_properties(loader_test PROPERTIES  WORKING_DIRECTORY "D:/HelloWorldTest/src/tests/loader_test/RelWithDebInfo" _BACKTRACE_TRIPLES "C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test/CMakeLists.txt;71;add_test;C:/Users/Chengxiang Li/Desktop/OpenXR-SDK-Source-release-1.1.37/src/tests/loader_test/CMakeLists.txt;0;")
else()
  add_test(loader_test NOT_AVAILABLE)
endif()
subdirs("test_layers")
subdirs("test_runtimes")
