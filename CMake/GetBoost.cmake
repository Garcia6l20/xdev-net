include(ExternalProject)

set(BOOST_VERSION 1.71.0)

if (WIN32)
    set(BOOST_CONFIGURE_COMMAND bootstrap.bat)
    set(BOOST_BUILD_COMMAND b2)
else()
    set(BOOST_CONFIGURE_COMMAND ./bootstrap.sh)
    set(BOOST_BUILD_COMMAND ./b2)
endif()

ExternalProject_Add(boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-${BOOST_VERSION}
    GIT_SUBMODULES
        libs/config
        libs/headers
        libs/atomic
        libs/chrono
        libs/coroutine
        libs/context
        libs/thread
        libs/system
        libs/asio
        libs/beast
        tools/build
        tools/boost_install
    GIT_SHALLOW YES
    CONFIGURE_COMMAND "${BOOST_CONFIGURE_COMMAND}"
    BUILD_COMMAND "${BOOST_BUILD_COMMAND}"
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE YES
)
ExternalProject_Get_property(boost SOURCE_DIR)
set(Boost_DIR ${SOURCE_DIR})

set(Boost_INCLUDE_DIR ${Boost_DIR})
set(Boost_LIBRARY_DIR ${Boost_DIR}/stage)
set(Boost_LIBRARIES boost_system boost_coroutine)
