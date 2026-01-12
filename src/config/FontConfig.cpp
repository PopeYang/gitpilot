#include "FontConfig.h"
#include <QApplication>

QFont FontConfig::titleFont() {
    QFont font;
    font.setPointSize(TITLE_SIZE);
    font.setBold(true);
    return font;
}

QFont FontConfig::normalFont() {
    QFont font;
    font.setPointSize(NORMAL_SIZE);
    return font;
}

QFont FontConfig::smallFont() {
    QFont font;
    font.setPointSize(SMALL_SIZE);
    return font;
}

QString FontConfig::titleFontCSS() {
    return QString("font-size: %1px; font-weight: bold;").arg(TITLE_SIZE);
}

QString FontConfig::normalFontCSS() {
    return QString("font-size: %1px;").arg(NORMAL_SIZE);
}

QString FontConfig::smallFontCSS() {
    return QString("font-size: %1px;").arg(SMALL_SIZE);
}

QString FontConfig::hintFontCSS() {
    return QString("font-size: %1px; color: #666;").arg(SMALL_SIZE);
}

void FontConfig::applyGlobalStyle(QApplication* app) {
    QString globalStyle = R"(
        /* 全局基础字体 */
        * {
            font-family: "Microsoft YaHei UI", "微软雅黑", sans-serif;
            font-size: 13px;
        }
        
        /* GroupBox 标题 */
        QGroupBox {
            font-size: 14px;
            font-weight: bold;
        }
        
        /* 按钮 */
        QPushButton {
            font-size: 13px;
        }
        
        /* 列表和树形控件 */
        QTreeWidget, QListWidget {
            font-size: 12px;
        }
        
        /* 标签 */
        QLabel {
            font-size: 13px;
        }
    )";
    
    app->setStyleSheet(globalStyle);
}
