cmake_minimum_required(VERSION 3.16)
project(KayteIDEUpdater VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/UpdateChecker.cpp
    src/BuildManager.cpp
    src/LogWidget.cpp
)

set(HEADERS
    src/MainWindow.h
    src/UpdateChecker.h
    src/BuildManager.h
    src/LogWidget.h
)

add_executable(KayteIDEUpdater ${SOURCES} ${HEADERS})

target_link_libraries(KayteIDEUpdater PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

target_compile_definitions(KayteIDEUpdater PRIVATE
    KAYTEIDE_REPO_URL="https://github.com/ringsce/kayteide.git"
    KAYTEIDE_API_URL="https://api.github.com/repos/ringsce/kayteide"
)

install(TARGETS KayteIDEUpdater
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
