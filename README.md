# GitPilot - 智能 GitLab 工作流助手 ✈️

[![Build Status](https://github.com/PopeYang/gitpilot/actions/workflows/build-release.yml/badge.svg)](https://github.com/PopeYang/gitpilot/actions/workflows/build-release.yml)
[![Latest Release](https://img.shields.io/github/v/release/PopeYang/gitpilot?color=blue&label=%E6%9C%80%E6%96%B0%E7%89%88%E6%9C%AC)](https://github.com/PopeYang/gitpilot/releases/latest)
[![License](https://img.shields.io/github/license/PopeYang/gitpilot)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-0078d7.svg)](https://github.com/PopeYang/gitpilot)

> **GitLab Workflow Automation Tool：简化 GitLab CI/CD 流程的智能 Git 客户端。**
> 让 GitLab 工作流更专业、更简单。GitPilot 是一款专为开发者和团队量身打造的 GitLab 辅助工具，旨在将复杂的分支策略与 CI/CD 流程封装为直观、高效的操作体验。

---

## 🎯 项目目标

开发一款基于 Qt 的定制化 Git 客户端，将复杂的 GitLab CI/CD 流程与严格的团队分支管理策略进行"封装"，实现从本地代码提交到自动构建的全链路自动化，降低操作门槛，提高研发效率。

## ✨ 核心特性

### 🚀 现代化分支管理 (New in v0.2.0)
*   **智能切换面板**：内置“核心分支”快捷区（`main`, `develop`, `internal`, `database` 等），支持一键触达。
*   **全量搜索切换**：强大的下拉搜索框，支持模糊匹配，让上百个开发分支的切换变得轻而易举。
*   **上下文感知模式**：
    *   🔴 **主分支只读模式**：仅允许触发正式版发布。
    *   🔒 **安全同步模式**：`develop`/`internal` 强制新建分支，保障主干代码安全。
    *   🟢 **活跃开发模式**：完整 MR 提交功能及流水线实时监控。

### 📝 深度集成 Merge Request (MR)
*   **一键提审**：自动识别最新提交，推送代码并同步创建 MR，彻底告别网页端繁琐流程。
*   **智能指派**：全新设计的审核人多选列表，支持快速筛选与状态实时更新。
*   **自动预填**：基于 Commit 自动预填标题与说明，符合团队命名规范。

### 🔄 CI/CD 全链路自动化
*   **流水线监控**：实时追踪流水线状态（Running, Success, Failed），支持手动触发构建任务。
*   **产物提取**：自动提取构建产物下载链接，支持多种匹配规则。

---

## 🛠️ 技术栈

*   **语言**: C++ 17
*   **框架**: Qt 6.2 (MSVC 2022)
*   **GitLab API**: v4
*   **环境**: Windows 10/11
*   **构建工具**: CMake 3.16+ & GitHub Actions

---

## 🚀 快速开始

### 本地编译
```powershell
# 环境要求：需要安装 Qt 6.2+ 和 CMake
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="你的Qt安装路径"
cmake --build . --config Release
```

### 环境配置
首次运行会引导您配置：
1.  **GitLab 实例地址** 与 **Access Token**。
2.  **目标 Project ID**。
3.  **产物正则匹配规则**。

---

## 📦 发布与下载

访问 [Releases 页面](https://github.com/PopeYang/gitpilot/releases) 下载最新安装包。

*   🟢 **绿色版** (Portable Zip) - 解压即用。
*   📦 **安装包** (Setup EXE) - 完整安装程序。

---

## 📄 许可证 (License)

本项目采用 **GNU General Public License v3.0** 开源协议。
详情请参阅 [LICENSE](LICENSE) 文件。

> **注意**：本项目使用 Qt 6 框架开发。Qt 开源版遵循 LGPLv3/GPLv3 协议，本项目与之完全兼容。
