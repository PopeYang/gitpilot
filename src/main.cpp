#include <QApplication>
#include <QMessageBox>
#include "config/ConfigManager.h"
#include "config/FontConfig.h"
#include "ui/FirstRunWizard.h"
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
    
    // 检查配置是否完整
    ConfigManager& config = ConfigManager::instance();
    
    // 如果是首次运行或配置不完整，显示向导
    while (config.isFirstRun() || 
           config.getGitLabToken().isEmpty() || 
           config.getRepoPath().isEmpty()) {
        
        FirstRunWizard wizard;
        wizard.setWindowTitle("配置向导 - GitPilot客户端");
        
        if (wizard.exec() == QDialog::Rejected) {
            // 用户取消配置，询问是否退出
            int ret = QMessageBox::question(nullptr, "退出确认", 
                "配置尚未完成，是否退出程序？\n\n"
                "点击'Yes'退出，点击'No'重新配置。",
                QMessageBox::Yes | QMessageBox::No);
            
            if (ret == QMessageBox::Yes) {
                return 0;
            }
            // 否则继续循环，重新显示向导
        } else {
            // 配置完成，标记首次运行已完成
            config.setFirstRunCompleted();
            break;
        }
    }
    
    // 加载主窗口
    MainWindow mainWindow;
    mainWindow.setWindowTitle("Git Pilot 客户端 - " + config.getCurrentProjectName());
    mainWindow.show();
    
    return app.exec();
}
