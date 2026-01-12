cmake_minimum_required(VERSION 3.16)
project(GitPilot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找Qt6组件（支持6.2到6.8+）
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network
    Concurrent
)

# MSVC编译器：设置源文件为UTF-8编码
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/FirstRunWizard.cpp
    src/ui/SettingsDialog.cpp
    src/service/GitService.cpp
    src/api/GitLabApi.cpp
    src/api/ApiModels.cpp
    src/views/MainBranchView.cpp
    src/views/ProtectedBranchView.cpp
    src/views/FeatureBranchView.cpp
    src/views/DatabaseBranchView.cpp
    src/widgets/MrZone.cpp
    src/widgets/DownloadLinkWidget.cpp
    src/widgets/BranchCreatorDialog.cpp
    src/widgets/ProgressDialog.cpp
    src/widgets/PipelineTriggerDialog.cpp
    src/automation/WorkflowEngine.cpp
    src/automation/BuildMonitor.cpp
    src/config/ConfigManager.cpp
    src/utils/Logger.cpp
    src/resources/gitpilot.rc
    src/resources/resources.qrc
)

# 头文件
set(HEADERS
    src/ui/MainWindow.h
    src/ui/FirstRunWizard.h
    src/ui/SettingsDialog.h
    src/service/GitService.h
    src/api/GitLabApi.h
    src/api/ApiModels.h
    src/views/MainBranchView.h
    src/views/ProtectedBranchView.h
    src/views/FeatureBranchView.h
    src/views/DatabaseBranchView.h
    src/widgets/MrZone.h
    src/widgets/DownloadLinkWidget.h
    src/widgets/BranchCreatorDialog.h
    src/widgets/ProgressDialog.h
    src/widgets/PipelineTriggerDialog.h
    src/automation/WorkflowEngine.h
    src/automation/BuildMonitor.h
    src/config/ConfigManager.h
    src/utils/Logger.h
)

# 创建可执行文件
add_executable(gitpilot ${SOURCES} ${HEADERS})

# 链接Qt库
target_link_libraries(gitpilot PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

# 设置包含目录
target_include_directories(gitpilot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Windows特定设置
if(WIN32)
    set_target_properties(gitpilot PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

# 安装规则
install(TARGETS gitpilot
    RUNTIME DESTINATION bin
)
