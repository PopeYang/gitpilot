#include <QApplication>
#include <QMessageBox>
#include "config/ConfigManager.h"
#include "config/FontConfig.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 应用全局字体规范
    FontConfig::applyGlobalStyle(&app);
    
    // 设置应用程序信息（用于QSettings）
    QCoreApplication::setOrganizationName("GitPilot");
    QCoreApplication::setApplicationName("GitPilot");
    QCoreApplication::setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon(":/app_icon.png"));
    
    // 设置应用程序样式（可选）
    QApplication::setStyle("Fusion");
    
    // 初始化配置管理器
    ConfigManager& config = ConfigManager::instance();
    
    // 直接加载主窗口，允许在未配置的情况下启动
    MainWindow mainWindow;
    
    // 如果有项目名称，在标题中显示，否则显示默认标题
    QString projectName = config.getCurrentProjectName();
    if (!projectName.isEmpty()) {
        mainWindow.setWindowTitle("GitPilot 客户端 - " + projectName);
    } else {
        mainWindow.setWindowTitle("GitPilot 客户端");
    }
    
    mainWindow.show();
    
    // 如果是首次运行，提示用户进行配置（非强制）
    if (config.isFirstRun()) {
        QMessageBox::information(&mainWindow, 
            QString::fromUtf8("欢迎使用 GitPilot"), 
            QString::fromUtf8("欢迎使用 GitPilot！\n\n"
                             "请通过菜单栏 文件 > 设置 来配置：\n"
                             "• GitLab 服务器地址和 Personal Access Token\n"
                             "• Git 仓库路径\n\n"
                             "配置完成后即可开始使用。"));
        
        // 标记首次运行已完成
        config.setFirstRunCompleted();
    }
    
    return app.exec();
}
