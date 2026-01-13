# GitPilot - 智能 GitLab 工作流助手 ✈️

[![Latest Release](https://img.shields.io/github/v/release/PopeYang/gitpilot?color=blue&label=%E6%9C%80%E6%96%B0%E7%89%88%E6%9C%AC)](https://github.com/PopeYang/gitpilot/releases/latest)
[![License](https://img.shields.io/github/license/PopeYang/gitpilot)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-0078d7.svg)](https://github.com/PopeYang/gitpilot)

> **让 GitLab 工作流更专业、更简单。**
> GitPilot 是一款专为开发者和团队量身打造的 GitLab 客户端，旨在将复杂的分支策略与 CI/CD 流程封装为直观、防呆的操作体验。

---

## ✨ 核心特性

### 🚀 现代化分支管理 (New in v0.2.0)
*   **智能切换面板**：内置“核心分支”快捷区（`main`, `develop`, `internal` 等），一键触达。
*   **全量搜索切换**：强大的下拉搜索框，让上百个开发分支的切换不再困难。
*   **上下文感知**：根据当前分支类型自动切换工作界面（主干/集成/开发/数据库模式）。

### 📝 深度集成 Merge Request (MR)
*   **一键提审**：自动识别修改内容，推送代码并同步创建 MR，彻底告别命令行复杂操作。
*   **实时人员指派**：全新设计的审核人多选列表，支持快速筛选与 Emoji 视觉反馈。
*   **规则防呆**：自动根据当前分支锁定目标分支，确保代码合并路径符合团队规范。

### 🔄 CI/CD 全链路监控
*   **流水线触发**：支持在客户端手动触发 GitLab Pipeline，灵活管理构建任务。
*   **构建状态追踪**：实时展示 CI 状态（Running, Success, Failed），直接提取产物下载链接。

---

## 🛠️ 技术栈

*   **核心**: C++ 17
*   **框架**: Qt 6.2 (MSVC 2022)
*   **通信**: GitLab API v4 (JSON)
*   **构建**: CMake 3.16+ & GitHub Actions (自动化发布)

---

## 🚀 快速上手

### 环境准备
- Windows 10/11
- 已配置好的 GitLab Access Token ([获取方式](https://github.com/PopeYang/gitpilot#gitlab-access-token配置))

### 编译运行
```powershell
# 手动编译
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="你的Qt安装路径"
cmake --build . --config Release
