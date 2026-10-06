cmake_minimum_required(VERSION 3.16)
project(LinuxSaviorToolbox VERSION 5.2.0 LANGUAGES CXX)

# 导出 compile_commands.json，消除 IDE/clangd 的 "QWidget file not found" 级假报错
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 默认构建类型为 Release（发布版必须使用优化构建）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Release 编译加固：优化 + 警告 + 剥离符号
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -Wall -Wextra")
# 安装时自动剥离二进制符号
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
    DBus
)

find_library(QTERMWIDGET_LIB qtermwidget6 QUIET)
if(QTERMWIDGET_LIB)
    set(QTERMWIDGET_VERSION 6)
else()
    find_library(QTERMWIDGET_LIB qtermwidget5 QUIET)
    if(QTERMWIDGET_LIB)
        set(QTERMWIDGET_VERSION 5)
    endif()
endif()

qt_add_executable(LinuxSaviorToolbox
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/core/CommandMetadata.h
    src/core/CommandMetadata.cpp
    src/core/PermissionManager.h
    src/core/PermissionManager.cpp
    src/core/SystemDetector.h
    src/core/SystemDetector.cpp
    src/core/TranslationManager.h
    src/core/TranslationManager.cpp
    src/core/CommandExecutor.h
    src/core/CommandExecutor.cpp
    src/core/PackageManager.h
    src/core/PackageManager.cpp
    src/widgets/SidebarWidget.h
    src/widgets/SidebarWidget.cpp
    src/widgets/TopBarWidget.h
    src/widgets/TopBarWidget.cpp
    src/widgets/DashboardWidget.h
    src/widgets/DashboardWidget.cpp
    src/widgets/EmergencyRepairWidget.h
    src/widgets/EmergencyRepairWidget.cpp
    src/widgets/TerminalWidget.h
    src/widgets/TerminalWidget.cpp
    src/widgets/SystemCleanupWidget.h
    src/widgets/SystemCleanupWidget.cpp
    src/widgets/GnomeExtensionWidget.h
    src/widgets/GnomeExtensionWidget.cpp
    src/widgets/DiagnosticReportWidget.h
    src/widgets/DiagnosticReportWidget.cpp
    src/widgets/BackupRestoreWidget.h
    src/widgets/BackupRestoreWidget.cpp
    src/widgets/CommandAliasWidget.h
    src/widgets/CommandAliasWidget.cpp
    src/widgets/CommandReferenceWidget.h
    src/widgets/CommandReferenceWidget.cpp
    src/widgets/DualBootWidget.h
    src/widgets/DualBootWidget.cpp
    src/widgets/SettingsWidget.h
    src/widgets/SettingsWidget.cpp
    src/widgets/CommandDetailDialog.h
    src/widgets/CommandDetailDialog.cpp
    src/widgets/ProgressDialog.h
    src/widgets/ProgressDialog.cpp
    src/widgets/BatchTaskWidget.h
    src/widgets/BatchTaskWidget.cpp
    src/widgets/HelpDialog.h
    src/widgets/HelpDialog.cpp
    src/widgets/VolumeDetectPage.h
    src/widgets/VolumeDetectPage.cpp
    src/widgets/MountChoicePage.h
    src/widgets/MountChoicePage.cpp
    src/widgets/LuksPasswordDialog.h
    src/widgets/LuksPasswordDialog.cpp
    src/widgets/ServiceManagerWidget.h
    src/widgets/ServiceManagerWidget.cpp
    src/widgets/ProcessManagerWidget.h
    src/widgets/ProcessManagerWidget.cpp
    src/widgets/NetworkDiagnosticWidget.h
    src/widgets/NetworkDiagnosticWidget.cpp
    src/widgets/LogViewerWidget.h
    src/widgets/LogViewerWidget.cpp
    src/widgets/SystemInfoWidget.h
    src/widgets/SystemInfoWidget.cpp
    src/widgets/StartupManagerWidget.h
    src/widgets/StartupManagerWidget.cpp
    src/resources/resources.qrc
)

target_link_libraries(LinuxSaviorToolbox PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::DBus
)

if(QTERMWIDGET_VERSION EQUAL 6)
    target_link_libraries(LinuxSaviorToolbox PRIVATE ${QTERMWIDGET_LIB})
    target_compile_definitions(LinuxSaviorToolbox PRIVATE HAVE_QTERMWIDGET QTERMWIDGET6)
    message(STATUS "Found QTermWidget 6")
elseif(QTERMWIDGET_VERSION EQUAL 5)
    target_link_libraries(LinuxSaviorToolbox PRIVATE ${QTERMWIDGET_LIB})
    target_compile_definitions(LinuxSaviorToolbox PRIVATE HAVE_QTERMWIDGET QTERMWIDGET5)
    message(STATUS "Found QTermWidget 5")
else()
    message(STATUS "QTermWidget not found, using fallback terminal")
endif()

target_include_directories(LinuxSaviorToolbox PRIVATE
    src
    src/core
    src/widgets
    src/utils
)

install(TARGETS LinuxSaviorToolbox
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/linux-savior-toolbox.desktop
    DESTINATION share/applications
    COMPONENT Runtime
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/linux-savior-toolbox.svg
    DESTINATION share/icons/hicolor/scalable/apps
    COMPONENT Runtime
)

foreach(ICON_SIZE 16 24 32 48 64 96 128 256 512)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor/${ICON_SIZE}x${ICON_SIZE}/apps/linux-savior-toolbox.png
        DESTINATION share/icons/hicolor/${ICON_SIZE}x${ICON_SIZE}/apps
        COMPONENT Runtime
    )
endforeach()

set(CPACK_PACKAGE_NAME "linux-savior-toolbox")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Linux Savior Toolbox - 全能系统维护工具箱")
set(CPACK_PACKAGE_DESCRIPTION "一款专为 Linux 桌面用户设计的全能系统维护工具箱，集成了终端模拟器、系统清理、备份恢复、GNOME 扩展管理、双系统工具箱等多种功能。")
set(CPACK_PACKAGE_VENDOR "LinuxSavior")
set(CPACK_PACKAGE_CONTACT "support@linuxsavior.org")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://linuxsavior.org")
set(CPACK_PACKAGE_LICENSE "GPL-3.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "LinuxSaviorToolbox")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_DEBIAN_PACKAGE_NAME "linux-savior-toolbox")
set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6widgets6, libqt6core6, libqt6gui6, libqt6network6, libc6 (>= 2.34)")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qtermwidget6, timeshift, ntfs-3g")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "LinuxSavior Team <support@linuxsavior.org>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://linuxsavior.org")

set(CPACK_RPM_PACKAGE_NAME "linux-savior-toolbox")
set(CPACK_RPM_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
set(CPACK_RPM_PACKAGE_VENDOR "LinuxSavior")
set(CPACK_RPM_PACKAGE_URL "https://linuxsavior.org")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.5, glibc >= 2.34")
set(CPACK_RPM_PACKAGE_SUGGESTS "qtermwidget6, timeshift, ntfs-3g")
set(CPACK_RPM_PACKAGE_DESCRIPTION "一款专为 Linux 桌面用户设计的全能系统维护工具箱。")
set(CPACK_RPM_PACKAGE_SUMMARY "Linux系统维护工具箱")

set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_COMPONENTS_ALL Runtime)

include(CPack)
