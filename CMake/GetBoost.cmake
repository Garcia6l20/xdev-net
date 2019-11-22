include(ExternalProject)

set(BOOST_VERSION 1.71.0)

set(B2_OPTIONS
    --with-system
    --with-coroutine
)

if (WIN32)
    set(BOOST_CONFIGURE_COMMAND bootstrap.bat)
    set(BOOST_BUILD_COMMAND b2 ${B2_OPTIONS})
    # as I use to say: windows suck...
    set(BOOST_EXTRA_SUBMODULES
        libs/predef
        libs/winapi
        libs/date_time
        libs/throw_exception
    )
else()
    set(BOOST_CONFIGURE_COMMAND ./bootstrap.sh)
    set(BOOST_BUILD_COMMAND ./b2 ${B2_OPTIONS})
endif()

ExternalProject_Add(boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-${BOOST_VERSION}
    GIT_SUBMODULES
        libs/assert
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
        ${BOOST_EXTRA_SUBMODULES}
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
