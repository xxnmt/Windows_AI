cmake_minimum_required(VERSION 3.19)
project(Windows_AI LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Sql REQUIRED Multimedia)

qt_standard_project_setup()

# 添加资源文件
qt6_add_resources(RESOURCE_FILES image.qrc)

qt_add_executable(Windows_AI
    WIN32 MACOSX_BUNDLE
    main.cpp
    characterwidget.cpp
    characterwidget.h
    ${RESOURCE_FILES}
    bubblewidget.h bubblewidget.cpp bubblewidget.ui
    configmanager.h configmanager.cpp
    llmservice.h llmservice.cpp
    appcontroller.h appcontroller.cpp
    sentencedata.h
    appearancemanager.h appearancemanager.cpp
    ttsservice.h ttsservice.cpp
    chatwidget.h chatwidget.cpp
    anchorstrategy.h
    anchormanager.h anchormanager.cpp
    settingswidget.h settingswidget.cpp settingswidget.ui
    memorymanager.h memorymanager.cpp
    historyturn.h
    ttsreference.h
    ittsprovider.h
    apittsprovider.h apittsprovider.cpp
    mockttsprovider.h mockttsprovider.cpp
    ttsreferencemanager.h ttsreferencemanager.cpp
)

target_link_libraries(Windows_AI
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::Sql
        Qt::Multimedia
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