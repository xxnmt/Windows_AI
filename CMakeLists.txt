cmake_minimum_required(VERSION 3.19)
project(Windows_AI LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

# 添加资源文件
qt6_add_resources(RESOURCE_FILES image.qrc)

qt_add_executable(Windows_AI
    WIN32 MACOSX_BUNDLE
    main.cpp
    widget.cpp
    widget.h
    ${RESOURCE_FILES}
)

target_link_libraries(Windows_AI
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS Windows_AI
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Windows_AI
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})