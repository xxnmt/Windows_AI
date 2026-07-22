# Windows_AI 桌面看板娘

一款基于 Qt 框架开发的桌面看板娘应用，支持 AI 对话、立绘切换、语音交互等功能。

## 📁 项目简介

这是一个可爱的桌面看板娘（桌宠）应用，名为「千岛茉子」。她可以与你对话、切换表情和服装、甚至未来还能与你语音交流。

## ✨ 功能特性

### 已实现功能
- 🎭 **AI 对话**：集成 DeepSeek API，支持多轮对话（含短期记忆）
- 🎨 **立绘切换**：支持表情、服装、距离、脸红四维状态切换
- 💬 **对话气泡**：打字机特效，位置跟随角色
- 🖱️ **拖拽移动**：支持自由拖动角色窗口
- 📍 **位置锚点**：气泡和聊天窗口自动跟随角色
- ⚙️ **AI 资源管理**：配置文件持久化（JSON格式）、系统提示词外部化（prompt.txt）
- 🧠 **AI 记忆系统**：短期记忆（SQLite数据库，默认15轮）、对话历史管理
- ⚙️ **设置界面**：API Key配置、记忆长度配置、历史记录管理

### 开发中功能
- 🔊 **语音合成**：接入 GPT-SoVITS 推理引擎
- ⏰ **时间驱动**：自动根据时间切换服装（白天校服 / 夜晚睡衣）
- ⌨️ **全局快捷键**：支持自定义快捷键

### 计划功能
- 🎯 **点击交互**：不同部位触发不同回应
- 🎬 **过渡动画**：立绘切换淡入淡出效果
- 🌤️ **智能助手**：天气查询、日程提醒等
- 👥 **多角色支持**：可切换不同角色

## 🛠️ 技术栈

- **框架**: Qt 6.5+
- **语言**: C++17
- **构建系统**: CMake 3.19+
- **AI 服务**: DeepSeek API
- **语音合成**: GPT-SoVITS（规划中）
- **数据库**: SQLite（已实现，用于对话历史存储）

## 📦 安装与构建

### 环境要求

- Windows 10/11
- Qt 6.5+（带 Qt Creator 或 qmake）
- CMake 3.19+
- Visual Studio 2019/2022（或其他 C++ 编译器）

### 构建步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/your-username/Windows_AI.git
   cd Windows_AI
   ```

2. **创建构建目录**
   ```bash
   mkdir build
   cd build
   ```

3. **配置 CMake**
   ```bash
   cmake .. -DCMAKE_PREFIX_PATH="你的Qt安装路径"
   ```

4. **构建项目**
   ```bash
   cmake --build . --config Release
   ```

5. **运行**
   ```bash
   ./Release/Windows_AI.exe
   ```

### Qt Creator 构建

1. 打开 Qt Creator
2. 选择「打开项目」，选择项目根目录的 `CMakeLists.txt`
3. 配置 Kit（确保 Qt 6.5+）
4. 点击「构建」按钮

## ⚙️ 配置

### API Key 配置

首次运行后，在 `app_data/config/settings.json` 中配置你的 API Key：

```json
{
    "api": {
        "deepseek_api_key": "your-api-key-here",
        "gpt_sovits_url": "http://127.0.0.1:9880"
    },
    "memory": {
        "short_term_length": 15
    }
}
```

### 配置项说明

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `api.deepseek_api_key` | `sk-placeholder-key` | DeepSeek API 密钥 |
| `api.gpt_sovits_url` | `http://127.0.0.1:9880` | GPT-SoVITS 服务地址 |
| `memory.short_term_length` | `15` | 短期记忆轮数（AI对话时携带的历史上下文数量） |

### 获取 DeepSeek API Key

1. 访问 [DeepSeek Platform](https://platform.deepseek.com/api_keys)
2. 注册/登录账号
3. 创建 API Key

### GPT-SoVITS 配置（规划中）

参考项目文档 `docs/OutDoc/API_DOC.md` 进行配置。

## 📁 项目结构

```
Windows_AI/
├── docs/                    # 项目文档
│   ├── ARCHITECTURE.md      # 架构设计文档
│   ├── ROADMAP.md           # 开发路线图
│   ├── project_tech_snapshot.md  # 技术快照
│   ├── development_log.md   # 开发记录
│   └── OutDoc/              # 外部文档（API文档等）
├── app_data/                # 应用数据（运行时生成）
│   ├── config/              # 配置文件
│   │   ├── setting.json     # 配置文件（JSON格式）
│   │   └── prompt.txt       # 系统提示词
│   └── memory/              # 记忆数据（SQLite数据库）
│       └── QianDaoMoZi_memory.db
├── image/                   # 立绘资源目录
│   ├── closer/              # 近景立绘
│   ├── far/                 # 远景立绘
│   └── default_config/      # 默认配置（prompt.txt）
├── image.qrc                # Qt 资源文件
├── CMakeLists.txt           # CMake 构建配置
├── main.cpp                 # 入口函数
├── appcontroller.h/cpp      # 应用中枢控制器
├── characterwidget.h/cpp    # 角色主组件
├── bubblewidget.h/cpp/ui    # 对话气泡组件
├── chatwidget.h/cpp         # 聊天输入窗口
├── anchormanager.h/cpp      # 位置锚点管理
├── anchorstrategy.h         # 锚点策略枚举
├── llmservice.h/cpp         # AI 服务模块
├── ttsservice.h/cpp         # TTS 语音服务
├── appearancemanager.h/cpp  # 外观管理器
├── configmanager.h/cpp      # 配置管理器（单例）
├── memorymanager.h/cpp      # AI 记忆系统（SQLite）
├── historyturn.h            # 对话历史数据结构
├── settingswidget.h/cpp/ui  # 设置界面
└── sentencedata.h           # 句子数据结构
```

## 📖 使用说明

1. **启动应用**：运行 `Windows_AI.exe`，茉子会出现在桌面上
2. **拖动移动**：鼠标左键拖动茉子可以移动位置
3. **右键菜单**：右键点击茉子可以打开菜单（聊天/设置/退出）
4. **聊天**：选择「和茉子聊天」或使用快捷键打开聊天窗口

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

MIT License（待完善）

## 📧 联系方式

如有问题或建议，欢迎提交 Issue。

---

> 千岛茉子正在期待与你相遇 ❤️