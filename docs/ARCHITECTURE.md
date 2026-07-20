# 架构设计文档

> 项目：Windows_AI 桌面看板娘（桌宠）
> 版本：v0.2.3
> 更新日期：2026-07-19

---

## 1. 架构概览

### 1.1 架构风格

采用 **中央控制器 + 模块化设计**，以 `AppController` 为信号调度中枢，各功能模块独立实现，通过信号槽解耦。

### 1.2 模块划分

| 层级 | 模块 | 职责 | 文件 |
|------|------|------|------|
| **中枢层** | AppController | 应用生命周期管理、信号调度、模块组装 | appcontroller.h/cpp |
| **UI层** | CharacterWidget | 立绘显示、鼠标拖拽、右键菜单 | characterwidget.h/cpp |
| | BubbleWidget | 气泡显示、打字机特效 | bubblewidget.h/cpp/ui |
| | ChatWidget | 聊天输入窗口、弹出跟随 | chatwidget.h/cpp |
| **业务层** | LLMService | DeepSeek请求、多标签协议解析、句子拆分 | llmservice.h/cpp |
| | TTSService | 语音合成队列、播放队列、模拟双线程 | ttsservice.h/cpp |
| | AppearanceManager | 四维状态管理、立绘路径生成、换图触发 | appearancemanager.h/cpp |
| | AnchorManager | 位置锚点管理、统一位置跟随 | anchormanager.h/cpp |
| **数据层** | ConfigManager | API Key配置、单例模式 | configmanager.h/cpp |
| **规划中** | TimeManager | 时间监控、服装自动切换 | timemanager.h/cpp（规划中） |
| | SettingsWidget | 设置界面、配置管理 | settingswidget.h/cpp/ui（规划中） |
| | ShortcutManager | 全局快捷键管理 | shortcutmanager.h/cpp（规划中） |
| **数据结构** | SentenceText | 句子数据模型（中文/日文/标签） | sentencedata.h |
| | AnchorStrategy | 锚点位置枚举与配置结构 | anchorstrategy.h |

### 1.3 架构总览图

```
┌─────────────────────────────────────────────────────────────────────┐
│                        AppController                                │
│                    （应用中枢 / 信号调度中心）                          │
│                                                                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐     │
│  │ CharacterWidget │  │   BubbleWidget  │  │    ChatWidget   │     │
│  │ 立绘/拖拽/右键菜单 │  │   气泡/打字机    │  │   聊天输入框    │     │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘     │
│           │                    │                    │              │
│           │                    └────────┬───────────┘              │
│           │                             │                          │
│           ▼                             ▼                          │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │ AppearanceMgr   │           │  AnchorManager  │                │
│  │   四维状态/换图   │           │   位置锚点管理    │                │
│  └────────┬────────┘           └─────────────────┘                │
│           │                                                        │
│           ▼                                                        │
│  ┌─────────────────┐                                               │
│  │  TimeManager    │  ← 规划中（v0.3.0）                            │
│  │ 时间监控/服装切换 │                                               │
│  └─────────────────┘                                               │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                        LLMService                            │  │
│  │           AI对话/多句解析 → ConfigManager(单例)               │  │
│  │           ← AI状态同步机制（方案C）                            │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                        TTSService                            │  │
│  │  合成队列(Producer)  →  播放队列(Consumer)                    │  │
│  │  ← GPT-SoVITS真实接入（v0.3.0）                               │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌─────────────────┐  ┌─────────────────┐                         │
│  │ SettingsWidget  │  │ ShortcutManager │  ← 规划中（v0.3.0）       │
│  │   设置界面       │  │   全局快捷键管理  │                         │
│  └─────────────────┘  └─────────────────┘                         │
│                                                                     │
│  核心数据结构：SentenceText (zhText + jaText + rawTags)             │
│  位置策略：AnchorStrategy (AnchorPosition枚举 + AnchorConfig)       │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 2. 核心组件设计

### 2.1 AppController

**职责**：应用中央控制器，负责模块创建、信号连接、生命周期管理

**设计要点**：
- 唯一持有所有模块实例
- 统一管理信号槽连接
- 作为LLM回复→TTS→UI的协调中枢
- 集中错误处理

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `startApp()` | 启动应用，显示角色并触发首次对话 | - | - |
| `handleMakoReply()` | 接收LLM解析结果，入队TTS | QList\<SentenceText\> sentences | - |
| `handleSystemError()` | 统一错误处理，显示错误气泡+切悲伤立绘 | QString errorMsg | - |
| `onPlayAudioAction()` | TTS播放同步：更新气泡+立绘 | QString zhText, QMap tags | - |

**信号槽连接清单**（全部在构造函数中建立）：
```cpp
// 用户输入 → LLM请求（通过ChatWidget）
connect(m_chatWidget, &ChatWidget::textSubmitted, m_llmService, &LLMService::askDeepSeek);

// LLM回复 → AppController → TTS队列
connect(m_llmService, &LLMService::sentenceReady, this, &AppController::handleMakoReply);

// TTS播放 → 同步UI（气泡+立绘）
connect(m_ttsService, &TTSService::playAudioAction, this, &AppController::onPlayAudioAction);

// 外观变化 → 角色立绘更新
connect(m_appearance, &AppearanceManager::characterPathChanged, m_character, &CharacterWidget::updatePath);

// 网络错误 → 统一处理
connect(m_llmService, &LLMService::internetErrorSignal, this, &AppController::handleSystemError);
```

---

### 2.2 CharacterWidget

**职责**：角色立绘显示与鼠标交互

**设计要点**：
- 仅负责UI渲染和用户交互
- 不再直接持有LLMService和BubbleWidget（解耦）
- 通过 `getBubbleAnchorPos()` 暴露气泡锚点
- 通过 `updatePath()` 接收外观变化

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `updatePath()` | 切换立绘图片 | QString imagePath | - |
| `getBubbleAnchorPos()` | 获取气泡锚点坐标（立绘右上角+偏移） | - | QPoint |
| `calculateVisibleRect()` | 扫描像素透明度，计算有效区域 | QPixmap pixmap | QRect |
| `mousePressEvent()` | 记录拖拽起始偏移 | QMouseEvent* | - |
| `mouseMoveEvent()` | 移动窗口并发出characterMoved信号 | QMouseEvent* | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `chatRequested()` | 用户请求聊天（右键菜单） | - |
| `settingsRequested()` | 用户请求设置（右键菜单） | - |

---

### 2.3 BubbleWidget

**职责**：对话气泡组件，支持打字机特效

**设计要点**：
- 独立的无边框顶层窗口（Qt::Tool）
- 位置跟随由 AnchorManager 统一管理，本组件不再负责位置计算
- 通过 `bubbleShown()` 信号通知位置管理器更新位置

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `showMessage()` | 显示气泡并启动打字机，通知位置管理器 | QString text | - |
| `typeWriteEffect()` | 逐字显示文本（定时器触发） | - | - |
| `paintEvent()` | 绘制圆角气泡背景 | QPaintEvent* | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `bubbleShown()` | 气泡显示时，通知位置管理器更新位置 | - |

---

### 2.4 LLMService

**职责**：AI对话服务，负责DeepSeek请求和多标签协议解析

**设计要点**：
- 与UI完全解耦，纯业务逻辑
- 系统指令包含完整的多标签协议
- 将回复拆分为多句 SentenceText
- 每句独立解析 emotion/blush/distance/clothing/ja 标签
- **状态提供者模式**：通过 `registerStateProvider()` 注册回调，实现AI状态同步（方案C · 已实现）
- **提示词外部化**：`initializePromptFile()` 和 `loadSystemPrompt()` 已实现，首次启动自动释放默认提示词

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `askDeepSeek()` | 发起DeepSeek API请求，动态追加状态上下文 | QString userInput | - |
| `onReplyFinished()` | 解析JSON→拆句→提取标签→发信号 | QNetworkReply* | - |
| `registerStateProvider()` | 注册状态提供者回调 | std::function\<QString()\> | - |
| `initializePromptFile()` | 初始化提示词文件，首次启动从资源释放默认prompt.txt | - | - |
| `loadSystemPrompt()` | 从文件加载系统提示词，失败时回退到资源文件 | - | QString |

**多标签协议系统指令（核心）**：
```
[emotion:值][blush:值][distance:值][clothing:值][ja:日文] 中文内容
```

**标签参数**：
| 标签 | 必填 | 可选值 | 说明 |
|------|------|--------|------|
| emotion | 是 | happyIdle, happyMore, amazing, loving, caring, sad, conscientious | 表情 |
| blush | 否 | unblushing, blushing | 脸红状态 |
| distance | 否 | far, closer | 距离 |
| clothing | 否 | pajama, schoolUniform, schoolUniformWithoutCap, schoolUniformWithoutCoat | 服装 |
| ja | 是 | 日文翻译 | 用于TTS |

**解析正则**：
```cpp
// 拆句：匹配连续标签 + 后面的中文
QRegularExpression sentenceRegex("((?:\\[[^\\]]+\\])+)([^\\[]+)");

// 提取单个标签键值对
QRegularExpression tagRegex("\\[([a-zA-Z0-9_]+):([^\\]]+)\\]");
```

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `sentenceReady(sentence)` | 回复解析完成（参数名单数，但实际传递复数列表） | QList\<SentenceText\> |
| `internetErrorSignal(msg)` | 网络错误 | QString |

**AI状态同步机制（方案C · 已实现）**：
- 通过 `registerStateProvider()` 注册 AppearanceManager 的状态描述回调
- 每次调用 `askDeepSeek()` 前，动态获取当前状态并追加到系统提示词
- 状态包含：距离、服装、脸红状态、当前表情 + 系统时间
- 无需额外token消耗，AI自动感知状态变化
- AppController中注册：`m_llmService->registerStateProvider([this](){return m_appearance->getCurrentStateDescription();})`

> ⚠️ **已知问题**：`sentenceReady` 信号参数命名为单数 `sentence`，语义与实际类型 `QList<SentenceText>` 不一致。

---

### 2.5 TTSService

**职责**：TTS语音合成与播放管理（当前为模拟实现）

**设计要点**：
- 生产者-消费者模式：合成队列 → 播放队列
- 双状态锁：`m_isSynthesizing` / `m_isPlaying`
- 合成与播放可并发（合成下一句时播放上一句）
- 当前用 QTimer::singleShot 模拟耗时

**队列模型**：
```
LLM回复 → [TTS合成队列] → (合成中) → [播放队列] → (播放中) → UI同步
           Producer                        Consumer
```

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `enqueueSentences()` | 接收多句话，推入合成队列 | QList\<SentenceText\> | - |
| `processTtsQueue()` | 合成消费者：取出一句开始合成 | - | - |
| `onMockTtsFinished()` | 合成完成：入播放队列+继续合成 | - | - |
| `processPlayQueue()` | 播放消费者：取出一句并通知UI | - | - |
| `onMockPlayFinished()` | 播放完成：继续播放下一句 | - | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `playAudioAction(zhText, tags)` | 播放开始时，用于同步UI | QString, QMap\<QString,QString\> |

---

### 2.6 AppearanceManager

**职责**：角色外观四维状态管理，负责立绘切换

**设计要点**：
- 维护四维状态：distance / clothing / blush / emotion
- blush 有退热机制（未显式指定时自动 unblushing）
- 路径变化时才发出 `characterPathChanged` 信号（防抖）
- 路径格式与资源目录结构严格对应
- **状态描述输出**：提供 `getCurrentStateDescription()` 用于AI状态同步

**状态机**：
```
距离: far ↔ closer
服装: pajama ↔ schoolUniform ↔ schoolUniformWithoutCap ↔ schoolUniformWithoutCoat
脸红: unblushing ↔ blushing（自动退热）
表情: happyIdle / happyMore / amazing / loving / caring / sad / conscientious
```

**路径生成规则**：
```
:/image/{distance}/{clothing}/{blush}/{emotion}.png
```

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `applyTags()` | 应用一组标签，触发换图检查 | QMap\<QString,QString\> | - |
| `getPath()` | 生成当前资源路径 | - | QString |
| `setDefault()` | 重置为默认状态 | - | - |
| `checkPathAndUpdate()` | 路径变化检查+发信号（防抖） | - | - |
| `getCurrentStateDescription()` | 获取当前状态描述字符串（用于AI状态同步） | - | QString |
| `getDistance()` | 获取当前距离 | - | QString |
| `getClothing()` | 获取当前服装 | - | QString |
| `getBlush()` | 获取当前脸红状态 | - | QString |
| `getEmotion()` | 获取当前表情 | - | QString |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `characterPathChanged(newPath)` | 立绘路径变化时 | QString |



---

### 2.7 SentenceText（数据结构）

**职责**：贯穿LLM→TTS→UI的核心数据模型

```cpp
struct SentenceText {
    QString zhText;                  // 中文文本 → BubbleWidget显示
    QString jaText;                  // 日文文本 → TTS合成输入
    QMap<QString, QString> rawTags;  // 原始标签 → AppearanceManager换图
};
```

**流转路径**：
```
LLMService解析 → AppController中转 → TTSService队列 → playAudioAction信号 → UI更新
```

---

### 2.8 ConfigManager

**职责**：全局配置管理（单例），支持配置文件持久化

**设计要点**：
- Meyers单例，线程安全
- 删除拷贝构造和赋值操作
- 配置文件路径：`app_data/config/setting.json`（应用程序目录下）
- 首次启动自动创建配置文件，使用默认值
- JSON结构：`{ "api": { "deepseek_api_key": "...", "gpt_sovits_url": "..." } }`
- **配置目录管理**：自动创建 `app_data/config/` 目录

**配置项**：
| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| deepseek_api_key | `sk-placeholder-key` | DeepSeek API密钥 |
| gpt_sovits_url | `http://127.0.0.1:9880` | GPT-SoVITS服务地址 |

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `loadSetting()` | 从JSON文件加载配置 | - | bool |
| `saveSetting()` | 保存配置到JSON文件 | - | bool |
| `getApiKey()` | 获取API Key | - | QString |
| `setApiKey()` | 设置API Key | QString | - |
| `getTTSUrl()` | 获取TTS服务地址 | - | QString |
| `setTTSUrl()` | 设置TTS服务地址 | QString | - |
| `getConfigDirPath()` | 获取配置目录路径 | - | QString |
| `getConfigFilePath()` | 获取配置文件路径 | - | QString |



---

### 2.9 AnchorManager

**职责**：位置锚点管理器，统一协调所有跟随角色的Widget位置

**设计要点**：
- 通过 `eventFilter` 监听 CharacterWidget 的 Move/Resize 事件
- 支持预定义锚点策略和自定义位置计算函数
- 立绘切换时通过信号连接自动更新位置

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `registerWidget()` | 注册Widget到预定义锚点位置 | QWidget*, AnchorConfig | - |
| `registerWidget()` | 注册Widget到自定义位置计算 | QWidget*, function\<QPoint()\> | - |
| `unregisterWidget()` | 注销Widget | QWidget* | - |
| `updateAllAnchors()` | 计算并更新所有锚点位置 | - | - |
| `calculatePosition()` | 根据锚点策略计算目标位置 | QRect, QPoint, QSize, AnchorConfig | QPoint |

**位置更新触发机制**：
| 触发条件 | 机制 |
|----------|------|
| 角色移动 | eventFilter 捕获 QEvent::Move |
| 角色大小变化 | eventFilter 捕获 QEvent::Resize |
| 立绘切换 | appearanceChanged / characterPathChanged 信号 |
| 气泡显示 | bubbleShown 信号 |

---

### 2.10 ChatWidget

**职责**：聊天输入窗口，支持弹出和输入

**设计要点**：
- 独立的顶层窗口（Qt::Tool）
- 位置跟随由 AnchorManager 统一管理
- 回车键提交，自动隐藏

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `popup()` | 弹出聊天窗口并激活 | - | - |
| `showEvent()` | 显示时自动聚焦输入框 | QShowEvent* | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `textSubmitted(text)` | 用户提交文本时 | QString |

---

### 2.11 TimeManager（规划中 · v0.3.0）

**职责**：时间监控与服装自动切换

**设计要点**：
- 每分钟检查当前时间，判断时间段
- 时间段变化时自动切换服装
- 服装切换后触发AI状态同步
- 支持配置文件自定义时间段和服装映射

**时间段定义**：
| 时间段 | 时间范围 | 默认服装 |
|--------|----------|----------|
| 白天 | 6:00 - 18:00 | schoolUniform |
| 夜晚 | 18:00 - 6:00 | pajama |

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `start()` | 启动时间监控 | - | - |
| `getCurrentTimePeriod()` | 获取当前时间段 | - | QString (day/night) |
| `checkTime()` | 检查时间并触发切换 | - | - |
| `setTimeConfig()` | 设置时间配置 | TimeConfig | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `timePeriodChanged(period)` | 时间段变化时 | QString |
| `clothingAutoChanged(clothing)` | 服装自动切换时 | QString |

**配置结构**：
```cpp
struct TimeConfig {
    bool enabled;           // 是否启用自动切换
    int dayStartHour;       // 白天开始时间（小时）
    int dayEndHour;         // 白天结束时间（小时）
    QString dayClothing;    // 白天服装
    QString nightClothing;  // 夜晚服装
};
```

---

### 2.12 SettingsWidget（规划中 · v0.3.0）

**职责**：设置界面，管理所有配置项

**设计要点**：
- 权限级别高于右键菜单（设置界面包含所有功能）
- 配置修改后立即生效并持久化
- 支持修改快捷键（QShortcut全局绑定）
- 使用 QTabWidget 分栏管理

**界面结构**：
```
├── API配置页
│   ├── DeepSeek API Key（输入框）
│   └── GPT-SoVITS服务地址（输入框）
├── TTS配置页
│   ├── 启用TTS（开关）
│   ├── 语速（滑块）
│   ├── 温度（滑块）
│   └── 参考音频配置（路径选择）
├── 外观配置页
│   ├── 气泡颜色（颜色选择器）
│   ├── 气泡透明度（滑块）
│   └── 窗口透明度（滑块）
├── 时间配置页
│   ├── 启用自动服装切换（开关）
│   ├── 白天开始时间（时间选择器）
│   ├── 白天结束时间（时间选择器）
│   ├── 白天服装（下拉框）
│   └── 夜晚服装（下拉框）
├── 快捷键配置页
│   ├── 打开聊天（按键绑定）
│   ├── 打开设置（按键绑定）
│   └── 打开历史记录（按键绑定）
└── 记忆配置页
    ├── 启用记忆（开关）
    ├── 短期记忆长度（数字输入）
    └── 自动摘要（开关）
```

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `loadSettings()` | 从配置文件加载设置 | - | - |
| `saveSettings()` | 保存设置到配置文件 | - | - |
| `applySettings()` | 应用设置到各模块 | - | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `settingsSaved()` | 设置保存时 | - |

---

### 2.13 ShortcutManager（规划中 · v0.3.0）

**职责**：全局快捷键管理

**设计要点**：
- 使用 QShortcut 绑定全局快捷键（即使窗口不在前台也能触发）
- 支持配置文件自定义快捷键
- 快捷键冲突检测
- 动态更新绑定

**快捷键配置**：
| 功能 | 默认快捷键 | 说明 |
|------|------------|------|
| 打开聊天窗口 | Ctrl+Shift+C | 弹出 ChatWidget |
| 打开设置界面 | Ctrl+Shift+S | 弹出 SettingsWidget |
| 打开历史记录 | Ctrl+Shift+H | 弹出历史记录界面 |

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `registerShortcut()` | 注册快捷键 | QString key, QObject* receiver, const char* method | - |
| `unregisterShortcut()` | 注销快捷键 | QString key | - |
| `updateShortcuts()` | 根据配置更新所有快捷键 | - | - |

---

### 2.14 AI状态同步机制（方案C · 已实现）

**设计要点**：
- 本地切换服装/表情后，不立即通知AI
- 下一次对话时，将当前状态追加到系统提示词
- AI自动感知到状态变化，无需额外token消耗

**实现方式**：

```cpp
// LLMService中构建系统提示词时动态追加当前状态
QString LLMService::askDeepSeek(const QString& userInput)
{
    QString finalSystemPrompt = m_systemPromptCache;
    if (m_stateProvider) {
        QString currentUiState = m_stateProvider();
        finalSystemPrompt += QString("\n\n【注意：千岛茉子当前的最新实时状态上下文】\n%1\n当前现实世界系统时间: %2\n...")
            .arg(currentUiState, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
    // ... 构建请求并发送
}
```

**状态字符串格式**：
```
距离：far
服装：schoolUniform
脸红状态：unblushing
当前表情：happyIdle
```

**同步时机**：
1. 每次调用 `askDeepSeek()` 前，从 AppearanceManager 获取当前状态
2. 将状态追加到系统提示词
3. AI回复时根据当前状态生成符合情境的内容

**注册方式**（AppController构造函数）：
```cpp
m_llmService->registerStateProvider([this](){return m_appearance->getCurrentStateDescription();});
```

---

## 3. 核心数据流

### 3.1 完整对话流程

```
1. 用户输入
   │
   ▼
2. CharacterWidget::chatRequested()  [信号] → ChatWidget::popup() → 用户输入 → textSubmitted(input)
   │
   ▼
3. LLMService::askDeepSeek(input)
   │
   ├── 构建系统指令（多标签协议）
   ├── 构建JSON请求体
   └── 发送POST到DeepSeek API
   │
   ▼
4. API响应 → LLMService::onReplyFinished()
   │
   ├── 提取content文本
   ├── 正则拆分多句
   ├── 每句提取所有标签
   ├── 生成 QList<SentenceText>
   │
   ▼
5. LLMService::sentenceReady(sentences)  [信号]
   │
   ▼
6. AppController::handleMakoReply(sentences)
   │
   ▼
7. TTSService::enqueueSentences(sentences)
   │
   ├── 入合成队列
   ├── 启动合成流水线
   │
   ▼
8. 合成队列（Producer）
   │
   ├── 取出一句 → 模拟合成 → 入播放队列
   └── 继续取下一句（并发合成）
   │
   ▼
9. 播放队列（Consumer）
   │
   ├── 取出一句 → 发出playAudioAction信号
   └── 模拟播放耗时 → 取下一句
   │
   ▼
10. AppController::onPlayAudioAction(zhText, tags)
    │
    ├── BubbleWidget::showMessage(zhText)  → 打字机显示
    └── AppearanceManager::applyTags(tags)  → 立绘切换
```

### 3.2 位置跟随机制（AnchorManager）

```
AnchorManager::registerWidget(bubble, AnchorConfig{HeadRight})
    │
    └── 保存Widget + 锚点配置
    │
    ▼
触发条件（任一）：
    ├── 角色移动 → eventFilter → updateAllAnchors()
    ├── 角色大小变化 → eventFilter → updateAllAnchors()
    ├── 立绘切换 → characterPathChanged信号 → updateAllAnchors()
    └── 气泡显示 → bubbleShown信号 → updateAllAnchors()
    │
    ▼
calculatePosition(visibleRect, characterPos, widgetSize, config)
    │
    └── 根据锚点策略计算目标位置
    │
    ▼
widget->move(targetPos)
```

**锚点策略配置**：
| Widget | 锚点位置 | 偏移 |
|--------|----------|------|
| BubbleWidget | HeadRight | 默认 |
| ChatWidget | WaistCenter | (0, 20) |

---

## 4. 资源管理

### 4.1 立绘资源结构

```
image/
├── closer/                    # 近景（大尺寸）
│   ├── pajama/                # 睡衣
│   │   ├── blushing/          # 脸红
│   │   │   ├── happyIdle.png
│   │   │   ├── happyMore.png
│   │   │   ├── amazing.png
│   │   │   ├── loving.png
│   │   │   ├── caring.png
│   │   │   ├── sad.png
│   │   │   └── conscientious.png
│   │   └── unblushing/        # 常态（7张同名）
│   ├── schoolUniform/
│   ├── schoolUniformWithoutCap/
│   └── schoolUniformWithoutCoat/
└── far/                       # 远景（小尺寸）
    └── (同上结构)
```

### 4.2 资源数量

- 距离：2种（far / closer）
- 服装：4种（pajama / schoolUniform / schoolUniformWithoutCap / schoolUniformWithoutCoat）
- 脸红：2种（unblushing / blushing）
- 表情：7种（happyIdle ~ conscientious）
- **总计**：2 × 4 × 2 × 7 = **112张立绘**

---

## 5. 设计模式

| 模式 | 应用位置 | 作用 |
|------|----------|------|
| **单例模式** | ConfigManager | 全局配置唯一实例 |
| **观察者模式** | 信号槽系统（AppController为中枢） | 模块间松耦合通信 |
| **生产者-消费者** | TTSService（合成队列→播放队列） | TTS流水线解耦 |
| **策略模式** | AppearanceManager（四维组合） | 立绘路径动态生成 |
| **状态管理** | AppearanceManager | 角色外观状态机 |
| **事件过滤器** | AnchorManager::eventFilter | 统一位置跟随 |

---

## 6. 代码规范

### 6.1 命名规范

| 类型 | 规则 | 示例 |
|------|------|------|
| 类名 | PascalCase | CharacterWidget, AppController |
| 成员变量 | m_前缀 + camelCase | m_llmService, m_visibleRect |
| 函数名 | camelCase | askDeepSeek, calculateVisibleRect |
| 信号 | camelCase | sentenceReady, characterPathChanged |
| 结构体 | PascalCase | SentenceText |

### 6.2 信号命名约定

- 已发生事件：`xxxReady` / `xxxChanged` / `xxxFinished`
- 错误事件：`xxxErrorSignal`
- 用户动作：`userXxx`

---

## 7. 技术债务与优化项

### 7.1 隐式耦合问题

| 编号 | 问题 | 位置 | 说明 | 建议 | 状态 |
|------|------|------|------|------|------|
| IC-001 | 服务定位器反模式 | llmservice.cpp | LLMService 直接调用 `ConfigManager::instance()` 获取API Key，隐式依赖单例 | 改为构造函数注入apiKey参数 | ✅ 已修复（LLMService构造函数接收apiKey参数） |
| IC-002 | 上帝控制器 | appcontroller.cpp | AppController 持有全部8个模块实例，建立全部信号连接 | 短期可接受；长期考虑按功能域拆分子控制器 | ⚠️ 可接受 |
| IC-003 | TTS服务感知UI关注点 | ttsservice.h | `playAudioAction(zhText, tags)` 同时传递了气泡文本和立绘标签，TTS不应关心appearance | TTS信号只通知"音频就绪"，UI更新由AppController基于SentenceText自行决策 | ❌ 待修复 |
| IC-004 | 控制器直接操作UI | appcontroller.cpp `handleSystemError()` | 控制器直接调 `m_bubble->showMessage()` 和 `m_appearance->applyTags()`，混入UI逻辑 | 错误应通过信号通知，由专门的UI协调层处理 | ❌ 待修复 |
| IC-005 | 位置计算硬编码 | anchormanager.cpp `calculatePosition()` | switch-case硬编码锚点位置，新增位置需修改源码 | 采用策略模式或注册机制，外部扩展锚点策略 | ❌ 待修复 |
| IC-006 | 位置跟随逻辑重复 | bubblewidget.cpp / chatwidget.cpp | 两者都有独立的 `attachTo()` + `eventFilter()` + `updatePosition()` 逻辑，代码重复约20行 | 统一由AnchorManager管理，移除各Widget的独立跟随逻辑 | ✅ 已修复 |

### 7.2 代码冗余与死代码

| 编号 | 问题 | 位置 | 说明 | 状态 |
|------|------|------|------|------|
| RC-001 | 死信号 | characterwidget.h | `characterMoved()` 信号已删除 | ✅ 已修复 |
| RC-002 | 立绘双重加载 | characterwidget.cpp | 构造函数已不再加载图片 | ✅ 已修复 |
| RC-003 | 重复include | appcontroller.cpp | 已修复 | ✅ 已修复 |
| RC-004 | 注释代码残留 | characterwidget.cpp | 已清理 | ✅ 已修复 |
| RC-005 | 重复include | chatwidget.cpp | `#include <QVBoxLayout>` 出现两次 | ✅ 已修复 |
| RC-006 | 死方法 | anchormanager.cpp | `onCharacterChanged()` 方法从未被调用 | ✅ 已删除 |
| RC-007 | 未使用的接口 | chatwidget.h/cpp | `attachTo()` 方法已存在但未被使用（已改用AnchorManager） | ✅ 已删除 |
| RC-008 | 未实现的枚举 | anchorstrategy.h | `HeadRight` 枚举值已定义但未在 `calculatePosition()` 中实现 | ✅ 已实现 |
| RC-009 | 未实现的信号槽 | characterwidget.h | `settingsRequested()` 信号已连接但无处理槽函数 | ❌ 待实现 |
| RC-010 | 气泡首次显示位置未初始化 | bubblewidget.cpp | 气泡显示时位置未计算，首次移动时突然跳转 | ✅ 已修复（bubbleShown信号） |
| RC-011 | 立绘切换时位置未更新 | anchormanager.cpp | 立绘切换导致visibleRect变化时未触发位置更新 | ✅ 已修复（characterPathChanged信号连接）

### 7.3 功能性技术债务

| 优先级 | 项目 | 描述 | 当前状态 |
|--------|------|------|----------|
| 高 | TTS真实接入 | 接入GPT-SoVITS推理引擎 | 模拟实现 |
| 高 | 设置界面 | 右键菜单"设置"入口已预留但无实现 | 预留入口 |
| 高 | 对话历史 | 当前每轮无状态，AI无记忆 | 未实现 |
| 中 | 错误重试 | LLM请求失败后自动重试 | 无 |
| 中 | AnchorManager析构 | 未清理动态分配的AnchorManager（内存泄漏风险） | 待修复 |
| 低 | 立绘过渡 | 换图时添加淡入淡出动画 | 无 |
| 低 | 资源缓存 | 立绘pixmap缓存机制 | 每次重新加载 |

### 7.4 架构演进新模块

| 模块 | 文件 | 职责 | 新增版本 |
|------|------|------|----------|
| AnchorManager | anchormanager.h/cpp | 位置锚点管理，统一协调气泡/聊天窗口的位置跟随 | v0.2.1 |
| AnchorStrategy | anchorstrategy.h | 锚点位置枚举与配置结构 | v0.2.1 |
| ChatWidget | chatwidget.h/cpp | 聊天输入窗口，支持弹出/跟随 | v0.2.1 |

### 7.5 当前模块关系图（更新）

```
AppController
    ├── CharacterWidget（立绘/右键菜单）
    ├── BubbleWidget（气泡/打字机）
    ├── ChatWidget（聊天输入框）
    ├── AnchorManager（位置锚点管理）← 新增
    ├── LLMService（AI对话/解析）
    ├── TTSService（语音合成/播放）
    ├── AppearanceManager（四维状态）
    └── ConfigManager（单例配置）

AnchorManager 管理：
    ├── BubbleWidget → HeadRight 位置
    └── ChatWidget → WaistCenter 位置
```

---

## 8. 扩展点

| 扩展方向 | 接入方式 |
|----------|----------|
| 新LLM模型 | 继承/替换 LLMService，保持 sentenceReady 信号接口 |
| 新TTS引擎 | 替换 TTSService 内部实现，保持 playAudioAction 信号接口 |
| 新服装/表情 | 添加资源文件，AppearanceManager自动支持（路径匹配） |
| 设置界面 | 连接 ConfigManager 的setter方法 |
| 新交互方式 | 在CharacterWidget中添加新信号，连接到AppController |
| 插件系统 | AppController中注册新模块，通过信号槽通信 |
