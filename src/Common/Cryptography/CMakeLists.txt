add_lunar_library(
    NAME Common_Cryptography
    INCLUDE_PATH Common/Cryptography

    API_HEADER_FILES
        include/AES256.hpp

    PRIVATE_DEPENDENCIES
        LunarDB_Selenity
        OpenSSL::SSL
        OpenSSL::Crypto

    SOURCE_FILES
        src/AES256.cpp
)

if(LUNAR_ENABLE_TESTING AND LUNAR_BUILD_COMMON_CRYPTOGRAPHY_TESTS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/test/setup_unit_tests.cmake)
endif()
