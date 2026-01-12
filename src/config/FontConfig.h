#ifndef FONTCONFIG_H
#define FONTCONFIG_H

#include <QFont>
#include <QString>

class QApplication;

/**
 * @brief 统一字体配置类
 * 提供标准化的字体大小规范，确保界面一致性和老年友好性
 */
class FontConfig {
public:
    // 字体大小常量
    static constexpr int TITLE_SIZE = 14;      // 标题（GroupBox、对话框标题）
    static constexpr int NORMAL_SIZE = 13;     // 正文、按钮
    static constexpr int SMALL_SIZE = 12;      // 列表、辅助信息、提示
    
    // QFont 对象获取
    static QFont titleFont();
    static QFont normalFont();
    static QFont smallFont();
    
    // CSS 样式字符串
    static QString titleFontCSS();
    static QString normalFontCSS();
    static QString smallFontCSS();
    static QString hintFontCSS();
    
    // 应用全局样式到应用程序
    static void applyGlobalStyle(QApplication* app);
};

#endif // FONTCONFIG_H
