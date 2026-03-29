cmake_minimum_required(VERSION 3.16)

project(BSTE VERSION 0.2 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    bste.h
    bste.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(BSTE
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(BSTE ${PROJECT_SOURCES})
endif()

target_link_libraries(BSTE PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(BSTE PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS BSTE
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(BSTE)
    install(FILES bste.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
endif()