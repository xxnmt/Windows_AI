include("D:/QtProject/Windows_AI/build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Windows_AI-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/QtProject/Windows_AI/build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/Windows_AI.exe"
    GENERATE_QT_CONF
)
