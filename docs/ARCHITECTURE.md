# 架构设计文档

> 项目：Windows_AI 桌面看板娘（桌宠）
> 版本：v0.1.0

---

## 1. 架构概览

### 1.1 架构风格

采用 **分层架构 + 模块化设计**，将UI层、业务层、数据层分离，通过信号槽机制实现模块间解耦。

### 1.2 模块划分

| 层级 | 模块 | 职责 | 文件 |
|------|------|------|------|
| **UI层** | CharacterWidget | 角色显示、拖拽交互、气泡管理 | characterwidget.h/cpp |
| | BubbleWidget | 气泡显示、打字机特效 | bubblewidget.h/cpp/ui |
| **业务层** | LLMService | AI网络请求、响应解析、[emotion]标签提取 | llmservice.h/cpp |
| **数据层** | ConfigManager | 配置管理（API Key）、单例模式 | configmanager.h/cpp |

### 1.3 模块关系图

```
┌─────────────────────────────────────────────────────────┐
│                      UI Layer                           │
│  ┌──────────────────┐         ┌──────────────────────┐  │
│  │ CharacterWidget  │────────▶│   BubbleWidget       │  │
│  │  - 立绘显示       │ 信号槽   │  - 打字机特效        │  │
│  │  - 拖拽交互       │         │  - 气泡动画          │  │
│  │  - 气泡位置计算   │         └──────────────────────┘  │
│  └────────┬─────────┘                                   │
│           │ 信号槽                                       │
└───────────┼─────────────────────────────────────────────┘
            │
┌───────────▼─────────────────────────────────────────────┐
│                    Business Layer                       │
│  ┌──────────────────┐                                   │
│  │    LLMService    │                                   │
│  │  - DeepSeek API  │                                   │
│  │  - JSON解析      │                                   │
│  │  - [emotion]提取 │                                   │
│  └────────┬─────────┘                                   │
│           │ 调用                                         │
└───────────┼─────────────────────────────────────────────┘
            │
┌───────────▼─────────────────────────────────────────────┐
│                     Data Layer                          │
│  ┌──────────────────┐                                   │
│  │  ConfigManager   │                                   │
│  │  - API Key管理   │                                   │
│  │  - 单例模式      │                                   │
│  └──────────────────┘                                   │
└─────────────────────────────────────────────────────────┘
```

---

## 2. 核心组件设计

### 2.1 CharacterWidget

**职责**：角色主组件，负责立绘显示、用户交互、气泡管理

**设计要点**：
- 继承 QWidget，无边框透明窗口
- 通过信号槽与 LLMService 通信
- 计算立绘有效区域用于气泡定位

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `mousePressEvent()` | 处理鼠标按下，记录拖拽偏移 | QMouseEvent* | - |
| `mouseMoveEvent()` | 处理鼠标移动，更新窗口位置和气泡位置 | QMouseEvent* | - |
| `onMakoReplyReady()` | 接收LLMService信号，显示气泡 | QString cleanText, QString emotion | - |
| `onMakoError()` | 接收网络错误信号 | QString errorMsg | - |
| `calculateVisibleRect()` | 计算立绘有效像素区域 | QPixmap pixmap | QRect |

### 2.2 LLMService

**职责**：AI服务模块，负责网络请求、响应解析、标签提取

**设计要点**：
- 继承 QObject，独立于UI
- 通过信号通知调用方处理结果
- 从 ConfigManager 获取 API Key

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `askDeepSeek()` | 发起DeepSeek API请求 | QString userInput | - |
| `onReplyFinished()` | 解析响应，提取文本和表情标签 | QNetworkReply* | - |

**信号定义**：
| 信号 | 作用 | 参数 |
|------|------|------|
| `replyReady()` | 回复就绪 | QString cleanText, QString emotion |
| `internetErrorSignal()` | 网络错误 | QString errorMessage |

### 2.3 ConfigManager

**职责**：配置管理器，管理全局配置项

**设计要点**：
- 单例模式，全局唯一实例
- 禁止拷贝构造和赋值操作
- 当前仅管理 API Key

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `instance()` | 获取单例实例 | - | ConfigManager& |
| `getApiKey()` | 获取API Key | - | QString |
| `setApiKey()` | 设置API Key | QString apiKey | - |

### 2.4 BubbleWidget

**职责**：气泡组件，负责消息显示和打字机特效

**设计要点**：
- 无边框透明窗口，自定义绘制气泡背景
- 使用 QTimer 实现打字机效果
- 独立于主窗口的浮动组件

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `showMessage()` | 显示气泡并启动打字机 | QString text | - |
| `typeWriteEffect()` | 逐字显示文本 | - | - |
| `paintEvent()` | 绘制气泡背景（圆角矩形） | QPaintEvent* | - |

---

## 3. 数据流

### 3.1 AI对话流程

```
用户交互 → CharacterWidget → LLMService → DeepSeek API
                                              │
                                              ▼
                                         API响应 → LLMService → [解析JSON] → [提取emotion]
                                              │
                                              ▼
                                         replyReady信号 → CharacterWidget → BubbleWidget
                                              │
                                              ▼
                                         打字机特效显示
```

### 3.2 信号槽连接

| 发送方 | 信号 | 接收方 | 槽函数 | 作用 |
|--------|------|--------|--------|------|
| LLMService | `replyReady(cleanText, emotion)` | CharacterWidget | `onMakoReplyReady()` | 显示AI回复 |
| LLMService | `internetErrorSignal(error)` | CharacterWidget | `onMakoError()` | 处理网络错误 |
| QNetworkAccessManager | `finished(reply)` | LLMService | `onReplyFinished()` | 处理API响应 |
| QTimer | `timeout()` | BubbleWidget | `typeWriteEffect()` | 打字机特效 |

---

## 4. 资源管理

### 4.1 立绘资源结构

```
image/
├── closer/              # 近景立绘（大尺寸）
│   ├── pajama/          # 睡衣皮肤
│   ├── schoolUniform/   # 校服皮肤
│   ├── schoolUniformWithoutCat/   # 校服无猫
│   └── schoolUniformWithoutCoat/  # 校服无外套
└── far/                 # 远景立绘（小尺寸）
    ├── pajama/
    ├── schoolUniform/
    ├── schoolUniformWithoutCat/
    └── schoolUniformWithoutCoat/
```

### 4.2 资源加载

- 通过 Qt 资源系统（image.qrc）打包
- 使用 `:/image/far/schoolUniform/1.png` 路径加载
- 当前仅支持固定路径加载，缺乏动态切换机制

---

## 5. 构建配置

### 5.1 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.19)
project(Windows_AI LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup()

qt6_add_resources(RESOURCE_FILES image.qrc)

qt_add_executable(Windows_AI
    WIN32 MACOSX_BUNDLE
    main.cpp
    characterwidget.cpp characterwidget.h
    bubblewidget.cpp bubblewidget.h bubblewidget.ui
    llmservice.cpp llmservice.h
    configmanager.cpp configmanager.h
    ${RESOURCE_FILES}
)

target_link_libraries(Windows_AI PRIVATE Qt::Core Qt::Widgets Qt::Network)
```

### 5.2 依赖组件

| 组件 | 用途 |
|------|------|
| Qt::Core | 核心功能、信号槽、容器 |
| Qt::Widgets | UI组件、窗口管理 |
| Qt::Network | 网络请求（HTTP） |

---

## 6. 设计模式

| 模式 | 应用位置 | 作用 |
|------|----------|------|
| **单例模式** | ConfigManager | 全局配置管理 |
| **观察者模式** | LLMService → CharacterWidget | 信号槽解耦 |
| **组合模式** | CharacterWidget + BubbleWidget | 父子组件管理 |

---

## 7. 代码规范

### 7.1 命名规范

| 类型 | 规则 | 示例 |
|------|------|------|
| 类名 | PascalCase | CharacterWidget |
| 成员变量 | m_前缀 + camelCase | m_llmService, m_visibleRect |
| 函数名 | camelCase | askDeepSeek, calculateVisibleRect |
| 信号 | camelCase | replyReady |

### 7.2 文件组织

- 头文件（.h）与源文件（.cpp）同名
- UI文件（.ui）与对应组件同名
- 资源文件（.qrc）单独管理

---

## 8. 待优化项

| 优先级 | 优化项 | 描述 |
|--------|--------|------|
| 高 | API Key安全 | 从配置文件加载，而非硬编码 |
| 高 | 立绘切换 | 实现基于emotion标签的立绘动态切换 |
| 高 | 设置界面 | 添加配置UI，支持API Key修改 |
| 中 | 气泡位置解耦 | 提取独立的位置计算模块 |
| 中 | 资源管理 | 实现动态资源加载和缓存 |
| 低 | 错误处理 | 完善网络错误和资源加载失败的处理 |
