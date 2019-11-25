include(ExternalProject)

set(BOOST_VERSION 1.71.0)

set(BOOST_MODULES system coroutine date_time)
foreach(_module ${BOOST_MODULES})
    if (WIN32)
        list(APPEND Boost_LIBRARIES libboost_${_module}-vc142-mt-x64-1_71.lib)
    else()
        list(APPEND Boost_LIBRARIES boost_${_module})
    endif()
    list(APPEND BOOST_OPTIONS_WITH --with-${_module})
endforeach()

set(B2_OPTIONS ${BOOST_OPTIONS_WITH})

if (WIN32)
    set(BOOST_CONFIGURE_COMMAND bootstrap.bat)
    set(BOOST_BUILD_COMMAND b2 ${B2_OPTIONS})
else()
    set(BOOST_CONFIGURE_COMMAND ./bootstrap.sh)
    set(BOOST_BUILD_COMMAND ./b2 ${B2_OPTIONS})
endif()

ExternalProject_Add(boost EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-${BOOST_VERSION}
    GIT_SHALLOW YES
    CONFIGURE_COMMAND "${BOOST_CONFIGURE_COMMAND}"
    BUILD_COMMAND "${BOOST_BUILD_COMMAND}"
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE YES
)
ExternalProject_Get_property(boost SOURCE_DIR)
set(Boost_DIR ${SOURCE_DIR})
set(Boost_ROOT ${SOURCE_DIR})

set(Boost_INCLUDE_DIRS ${Boost_DIR} CACHE STRING "Boost include directory" FORCE)
set(Boost_LIBRARY_DIRS ${Boost_DIR}/stage/lib CACHE STRING "Boost library directory" FORCE)
