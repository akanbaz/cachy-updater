cmake_minimum_required(VERSION 3.28)

project(cachyos-updater
    VERSION 1.0.0
    DESCRIPTION "Fast, native CachyOS system updater"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Kirigami is consumed purely as a QML runtime import, so no KF6 C++ linkage
# (and therefore no extra-cmake-modules) is required to build the binary.
find_package(Qt6 6.6 REQUIRED COMPONENTS
    Core Gui Widgets Qml Quick QuickControls2 Network)

qt_standard_project_setup(REQUIRES 6.6)

qt_add_executable(cachyos-updater
    src/main.cpp
    src/Pkg.h
    src/Classifier.h
    src/Classifier.cpp
    src/ProcessRunner.h
    src/ProcessRunner.cpp
    src/UpdatesModel.h
    src/UpdatesModel.cpp
    src/UpdateController.h
    src/UpdateController.cpp
    src/NewsController.h
    src/NewsController.cpp
    src/MaintainController.h
    src/MaintainController.cpp
    src/TrayController.h
    src/TrayController.cpp
)

# Theme is a QML singleton; the property must be set before the module is added.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(cachyos-updater
    URI org.cachyos.updater
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/pages/UpdatesPage.qml
        qml/pages/NewsPage.qml
        qml/pages/CleanupPage.qml
        qml/components/CachyCard.qml
        qml/components/Banner.qml
        qml/components/SectionHeader.qml
        qml/components/PackageDelegate.qml
        qml/components/KernelCard.qml
        qml/components/TerminalPanel.qml
)

qt_add_resources(cachyos-updater tray_icons
    PREFIX /tray
    FILES
        assets/tray-uptodate.svg
        assets/tray-updates.svg
)

target_link_libraries(cachyos-updater PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

include(GNUInstallDirs)

install(TARGETS cachyos-updater
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES org.cachyos.updater.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

install(FILES org.cachyos.updater-tray.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

install(FILES org.cachyos.updater-tray.service
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/systemd/user)

# App launcher icon (installed to the system icon theme).
install(FILES assets/logo.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    RENAME org.cachyos.updater.svg)

install(FILES assets/tray-uptodate.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/status
    RENAME org.cachyos.updater-tray.svg)

install(FILES assets/tray-updates.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/status
    RENAME org.cachyos.updater-tray-updates.svg)

# Optional polkit action (see the file for details).
install(FILES org.cachyos.updater.policy
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/polkit-1/actions)
