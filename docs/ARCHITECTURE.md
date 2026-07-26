# 架构设计文档

> 项目：Windows_AI 桌面看板娘（桌宠）
> 版本：v0.4.1
> 更新日期：2026-07-26

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
| | TTSService | 语音合成队列、播放队列、策略模式（ITTSProvider） | ttsservice.h/cpp, ittsprovider.h, apittsprovider.h/cpp |
| | AppearanceManager | 四维状态管理、立绘路径生成、换图触发 | appearancemanager.h/cpp |
| | AnchorManager | 位置锚点管理、统一位置跟随 | anchormanager.h/cpp |
| **数据层** | ConfigManager | API Key配置、记忆长度配置、单例模式 | configmanager.h/cpp |
| | MemoryManager | AI对话历史管理、SQLite数据库（已实现） | memorymanager.h/cpp |
| **规划中** | TimeManager | 时间监控、服装自动切换 | timemanager.h/cpp（规划中） |
| | SettingsWidget | 设置界面、配置管理、记忆管理（已实现API配置页+记忆管理页） | settingswidget.h/cpp/ui |
| | ShortcutManager | 全局快捷键管理 | shortcutmanager.h/cpp（规划中） |
| **数据结构** | SentenceText | 句子数据模型（中文/日文/标签） | sentencedata.h |
| | HistoryTurn | 对话历史数据结构 | historyturn.h（已实现） |
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
│  │  TimeManager    │  ← 规划中（v0.4.0）                            │
│  │ 时间监控/服装切换 │                                               │
│  └─────────────────┘                                               │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                        LLMService                            │  │
│  │           AI对话/多句解析 → ConfigManager(单例)               │  │
│  │           ← AI状态同步机制（方案C）                            │  │
│  │           ← MemoryManager(历史上下文)                        │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                        TTSService                            │  │
│  │  合成队列(Producer) → ITTSProvider(ApiTTSProvider/Mock)      │  │
│  │  ← GPT-SoVITS API 接入完成（v0.5.0）                         │  │
│  │  播放队列(Consumer) → QMediaPlayer + playAudioAction信号     │  │
│  │  临时文件即用即删，不做缓存                                  │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐    │
│  │ SettingsWidget  │  │ ShortcutManager │  │  MemoryManager  │    │
│  │   设置界面       │  │   全局快捷键管理  │  │   AI记忆系统     │    │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘    │
│                                                                     │
│  核心数据结构：SentenceText (zhText + jaText + rawTags)             │
│              HistoryTurn (userInput + rawReply)                     │
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
| `startApp()` | 启动应用，显示角色（不再自动触发首次对话） | - | - |
| `handleMakoReply()` | 接收LLM解析结果，保存记忆+入队TTS+触发记忆提取 | QList\<SentenceText\> sentences, QString rawReply | - |
| `handleSystemError()` | 统一错误处理，显示错误气泡+切悲伤立绘 | QString errorMsg | - |
| `onPlayAudioAction()` | TTS播放同步：更新气泡+立绘 | QString zhText, QMap tags | - |

**信号槽连接清单**（全部在构造函数中建立）：
```cpp
// 用户输入 → LLM请求（通过ChatWidget，含记忆查询）
connect(m_chatWidget, &ChatWidget::textSubmitted, this, [this](const QString &text){
    m_lastUserInput = text;
    int memoryLength = 15;
    QList<HistoryTurn> shortTermMemory = m_memoryManager->getHistoryTurn(memoryLength);
    m_llmService->askDeepSeek(text, shortTermMemory);
});

// 右键菜单 → 设置界面
connect(m_character, &CharacterWidget::settingsRequested, m_settingsWidget, &SettingsWidget::show);

// 设置保存 → 更新LLMService API Key
connect(m_settingsWidget, &SettingsWidget::settingsSaved, this, [this](){
    m_llmService->setApiKey(ConfigManager::instance().getApiKey());});

// LLM回复 → AppController → 保存记忆 + 入队TTS
connect(m_llmService, &LLMService::sentenceReady, this, &AppController::handleMakoReply);

// TTS播放 → 同步UI（气泡+立绘）
connect(m_ttsService, &TTSService::playAudioAction, this, &AppController::onPlayAudioAction);

// 外观变化 → 角色立绘更新
connect(m_appearance, &AppearanceManager::characterPathChanged, m_character, &CharacterWidget::updatePath);

// 外观变化 → 位置更新
connect(m_appearance, &AppearanceManager::characterPathChanged, m_anchorManager, &AnchorManager::updateAllAnchors);

// 气泡显示 → 位置更新
connect(m_bubble, &BubbleWidget::bubbleShown, m_anchorManager, &AnchorManager::updateAllAnchors);

// 网络错误 → 统一处理
connect(m_llmService, &LLMService::internetErrorSignal, this, &AppController::handleSystemError);

// 状态提供者注册（AI状态同步）
m_llmService->registerStateProvider([this](){return m_appearance->getCurrentStateDescription();});
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
| `askDeepSeek()` | 发起DeepSeek API请求，动态追加状态上下文，支持传入对话历史 | QString userInput, QList\<HistoryTurn\> historyQA | - |
| `onReplyFinished()` | 解析JSON→拆句→提取标签→发信号（携带原始回复） | QNetworkReply* | - |
| `extractMemoryAsync()` | 后台异步提取记忆（用户画像+长期摘要） | QList\<HistoryTurn\> turns, qlonglong lastEndId, QList\<qlonglong\> sourceIds | - |
| `registerStateProvider()` | 注册状态提供者回调 | std::function\<QString()\> | - |
| `setMemoryManager()` | 设置MemoryManager实例（用于记忆提取） | MemoryManager* manager | - |
| `initializePromptFile()` | 初始化提示词文件，首次启动从资源释放默认prompt.txt | - | - |
| `loadSystemPrompt()` | 从文件加载系统提示词，失败时回退到资源文件 | - | QString |
| `setApiKey()` | 设置API Key（运行时更新） | QString apiKey | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `sentencesReady(sentences, rawReply)` | 回复解析完成，携带原始回复用于保存记忆 | QList\<SentenceText\>, QString |
| `internetErrorSignal(msg)` | 网络错误 | QString |
| `memoryExtractionReady(profiles, summary, lastEndId, sourceIdsJson)` | 记忆提取完成 | QJsonArray, QString, qlonglong, QString |

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
| `sentenceReady(sentences, rawReply)` | 回复解析完成，携带原始回复用于保存记忆 | QList\<SentenceText\>, QString |
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

**职责**：TTS语音合成与播放管理

**设计要点**：
- **策略模式**：通过 `ITTSProvider` 抽象接口支持多种 TTS 实现
- **双队列模型**：合成队列 `m_ttsQueue` → 播放队列 `m_playQueue`
- **双状态锁**：`m_isSynthesizing` / `m_isPlaying` 控制并发
- **实时合成**：使用 `ApiTTSProvider` 调用 GPT-SoVITS HTTP API
- **模拟回退**：`MockTTSProvider` 用于开发测试或 API 不可用时
- **临时文件管理**：音频即用即删，不做缓存；播放完成后自动清理临时文件
- **错误隔离**：合成失败时跳过当前句，不阻塞后续队列

**架构图**：
```
                    ┌──────────────────────────────────────────┐
                    │              TTSService                  │
                    │                                          │
  SentenceText ───► │  m_ttsQueue (待合成)                     │
  (enqueueSentences)│      │                                   │
                    │      ▼                                   │
                    │  processTtsQueue()                       │
                    │      │                                   │
                    │      ▼                                   │
                    │  ITTSProvider.synthesize()               │
                    │      │                                   │
                    │      ▼                                   │
                    │  synthesisFinished                       │
                    │      │                                   │
                    │      ▼                                   │
                    │  m_playQueue (待播放)                     │
                    │      │                                   │
                    │      ▼                                   │
                    │  processPlayQueue()                      │
                    │      │                                   │
                    │      ├──► playAudioAction(zhText, tags)  │ ──► UI更新
                    │      │                                   │
                    │      └──► QMediaPlayer::play()           │
                    │                                          │
                    │  模式: "api" / "mock"                     │
                    └──────────────────────────────────────────┘
```

**ITTSProvider 接口**：
```cpp
class ITTSProvider : public QObject {
    Q_OBJECT
public:
    virtual void synthesize(const SentenceText &sentence) = 0;
    virtual void warmUp() = 0;
signals:
    void synthesisFinished(const QString &audioPath, const SentenceText &sentence);
    void synthesisFailed(const QString &audioPath, const SentenceText &sentence);
};
```

**Provider 实现**：
| 实现类 | 说明 |
|--------|------|
| `ApiTTSProvider` | 通过 `QNetworkAccessManager` 调用 GPT-SoVITS HTTP API |
| `MockTTSProvider` | 模拟实现，直接返回空路径 |

**队列模型**：
```
LLM回复 → m_ttsQueue(待合成) → ApiTTSProvider.synthesize(HTTP POST)
                                        │
                                        ▼
                                  synthesisFinished
                                        │
                                        ▼
                                  m_playQueue(待播放) → processPlayQueue()
                                        │
                                        ▼
                                  playAudioAction信号(UI更新) + QMediaPlayer播放
```

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `enqueueSentences()` | 接收多句话，推入合成队列 | QList\<SentenceText\> | - |
| `reloadProvider()` | 根据配置重新加载 TTS Provider | - | - |
| `switchModel()` | 切换 GPT-SoVITS 模型 | gptPath, sovitsPath | - |
| `processTtsQueue()` | 取出一句调用 Provider 合成 | - | - |
| `onTtsFinished()` | 合成完成：入播放队列+继续合成 | audioPath, sentence | - |
| `onTtsFailed()` | 合成失败：跳过该句，继续合成 | errorMsg, sentence | - |
| `processPlayQueue()` | 取出一句：发信号+播放音频 | - | - |
| `onPlaybackStateChanged()` | 播放完成：清理+继续播放下一句 | state | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `playAudioAction(zhText, tags)` | 播放开始时，用于同步UI（气泡文字+立绘标签） | QString, QMap\<QString,QString\> |

**配置**：
- `tts.mode` = `"api"` → 加载 `ApiTTSProvider`
- `tts.mode` = `"mock"` → 加载 `MockTTSProvider`（无 Provider 实例）

**设计决策**：
- **不实现音频缓存**：简化设计，每次合成为独立请求
- **临时文件即用即删**：播放完成后立即删除，避免磁盘空间泄漏
- **单合成线程**：同一时间只处理一个合成请求，避免并发问题

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
| api.deepseek_api_key | `sk-placeholder-key` | DeepSeek API密钥 |
| api.gpt_sovits_url | `http://127.0.0.1:9880` | GPT-SoVITS服务地址 |
| memory.short_term_length | `15` | 短期记忆轮数 |

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `loadSetting()` | 从JSON文件加载配置，配置缺失时使用默认值 | - | bool |
| `saveSetting()` | 保存配置到JSON文件 | - | bool |
| `getApiKey()` | 获取API Key | - | QString |
| `setApiKey()` | 设置API Key | QString | - |
| `getTTSUrl()` | 获取TTS服务地址 | - | QString |
| `setTTSUrl()` | 设置TTS服务地址 | QString | - |
| `getShortMemoryLength()` | 获取短期记忆长度 | - | int |
| `setShortMemoryLength()` | 设置短期记忆长度 | int length | - |
| `getConfigDirPath()` | 获取配置目录路径 | - | QString |
| `getConfigFilePath()` | 获取配置文件路径 | - | QString |
| `getMemoryPath()` | 获取数据库文件路径 | - | QString |
| `getAppDataPath()` | 获取应用数据根目录路径 | - | QString |



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

### 2.12 SettingsWidget（已实现 · API配置页 + 记忆管理页）

**职责**：设置界面，管理API Key、记忆长度配置和对话历史管理

**设计要点**：
- 独立窗口（Qt::Window），支持关闭按钮
- 显示时自动加载当前配置（重写showEvent）
- 配置修改后立即保存到配置文件并通知AppController更新
- 需要通过 `setMemoryManager()` 注入 MemoryManager 实例
- 需要通过 `setMemoryLength()` 初始化记忆长度设置

**当前实现**：
- API Key配置页（输入框 + 保存按钮）
- 记忆长度配置（数字输入框 + 保存按钮）
- 对话历史管理页（表格展示、分页浏览、单条删除、清空全部）
- 设置保存后发出 `settingsSaved()` 信号
- AppController接收到信号后更新LLMService的API Key
- 历史记录删除需要API Key验证（防止误删）

**规划扩展**：
```
├── API配置页（已实现）
│   ├── DeepSeek API Key（输入框）
│   └── GPT-SoVITS服务地址（输入框）
├── 记忆配置页（已实现）
│   ├── 短期记忆长度（数字输入框，默认15）
│   ├── 历史记录列表（表格，分页浏览）
│   ├── 删除选中记录（需API Key验证）
│   └── 清空全部记录（需API Key验证）
├── TTS配置页（规划中）
│   ├── 启用TTS（开关）
│   ├── 语速（滑块）
│   ├── 温度（滑块）
│   └── 参考音频配置（路径选择）
├── 外观配置页（规划中）
│   ├── 气泡颜色（颜色选择器）
│   ├── 气泡透明度（滑块）
│   └── 窗口透明度（滑块）
├── 时间配置页（规划中）
│   ├── 启用自动服装切换（开关）
│   ├── 白天开始时间（时间选择器）
│   ├── 白天结束时间（时间选择器）
│   ├── 白天服装（下拉框）
│   └── 夜晚服装（下拉框）
└── 快捷键配置页（规划中）
    ├── 打开聊天（按键绑定）
    ├── 打开设置（按键绑定）
    └── 打开历史记录（按键绑定）
```

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `show()` | 显示设置窗口（重写showEvent自动加载配置） | - | - |
| `loadSettings()` | 从配置文件加载当前配置 | - | - |
| `setMemoryManager()` | 注入MemoryManager实例 | MemoryManager* | - |
| `setMemoryLength()` | 初始化记忆长度设置 | int length | - |
| `refreshHistoryTurnList()` | 刷新历史记录列表（重新获取总数和当前页） | - | - |
| `loadHistoryPage()` | 加载指定页的历史记录 | int page | - |
| `on_btn_saveApiKey_clicked()` | 保存API Key到配置文件 | - | - |
| `on_btn_saveMemoryLength_clicked()` | 保存记忆长度到配置文件 | - | - |
| `on_btn_deleteSelectedMemory_clicked()` | 删除选中的历史记录（需API Key验证） | - | - |
| `on_btn_claenAllMemory_clicked()` | 清空所有历史记录（需API Key验证） | - | - |

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

### 2.14 MemoryManager（已实现 · AI记忆系统）

**职责**：管理AI对话历史记录、用户画像和长期记忆摘要，支持短期记忆查询与持久化存储

**设计要点**：
- 使用SQLite数据库存储三类数据，支持跨会话记忆
- 每次对话后保存用户输入、AI原始回复和解析后的句子数据
- 对话前读取最近N轮历史作为短期记忆（默认15轮）
- 支持用户画像的置信度衰减机制（三级半衰期）
- 支持长期记忆摘要的自动生成和管理
- 数据库路径：`app_data/memory/QianDaoMoZi_memory.db`
- 支持数据库版本升级（PRAGMA user_version）

**数据库结构**：

**表1：chat_history（对话历史）**
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER PRIMARY KEY AUTOINCREMENT | 记录ID |
| timestamp | DATETIME | 创建时间（本地时间） |
| user_input | TEXT NOT NULL | 用户输入文本 |
| raw_reply | TEXT NOT NULL | AI原始回复 |
| parsed_json | TEXT NOT NULL | 解析后的句子JSON |

**表2：user_profile（用户画像）**
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER PRIMARY KEY AUTOINCREMENT | 记录ID |
| key | TEXT NOT NULL | 画像属性名 |
| value | TEXT NOT NULL | 画像属性值 |
| tier | INTEGER DEFAULT 2 | 半衰期等级（1=长期，2=中期，3=短期） |
| confidence | INTEGER DEFAULT 50 | 置信度（0-100） |
| first_seen | DATETIME | 首次记录时间 |
| last_triggered | DATETIME | 上次触发时间 |
| session_count | INTEGER DEFAULT 1 | 触发次数 |

**表3：long_term_summary（长期记忆摘要）**
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER PRIMARY KEY AUTOINCREMENT | 记录ID |
| summary_text | TEXT NOT NULL | 摘要内容 |
| covered_turn_end_id | INTEGER NOT NULL | 覆盖的对话结束ID |
| source_ids | TEXT NOT NULL | 来源对话ID列表（JSON） |
| is_dirty | INTEGER DEFAULT 0 | 是否待重建 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

**关键方法**：

**对话历史 CRUD**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `saveQATurn()` | 保存单轮对话记录 | QString userInput, QString rawReply, QList\<SentenceText\> sentences | bool |
| `getHistoryTurn(N)` | 获取最近N轮对话历史（按时间正序） | int N | QList\<HistoryTurn\> |
| `getHistoryTurn(offset, limit)` | 分页获取对话历史（按时间倒序） | int offset, int limit | QList\<HistoryTurn\> |
| `getTotalHistoryCount()` | 获取历史记录总数 | - | qlonglong |
| `deleteTurnByID(id)` | 根据ID删除单条记录 | int id | bool |
| `clearAllHistory()` | 清空所有历史记录 | - | bool |
| `getUnsummarizedTurns()` | 获取未摘要的对话记录 | qlonglong& outLastEndId, QList\<qlonglong\>& outSourceIds | QList\<HistoryTurn\> |

**用户画像 CRUD**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `upsertUserProfile()` | 插入或更新用户画像 | QString key, QString value, int tier, int confidenceGain | bool |
| `getActiveUserProfiles()` | 获取活跃用户画像（置信度>=阈值） | int minConfidence | QList\<UserProfile\> |
| `deleteUserProfile()` | 删除用户画像 | qlonglong id | bool |
| `scanAndApplyProfileDecay()` | 扫描并应用置信度衰减 | - | int（删除的记录数） |

**长期记忆摘要 CRUD**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `addLongTermSummary()` | 添加长期记忆摘要 | QString summaryText, qlonglong coveredEndId, QString sourceIdsJson | bool |
| `getLatestSummaries()` | 获取最新的长期摘要 | int limit | QList\<LongTermSummary\> |
| `deleteLongTermSummary()` | 删除长期记忆摘要 | qlonglong id | bool |

**数据结构**：

```cpp
// 对话历史
struct HistoryTurn {
    qlonglong id = -1;   // 记录ID（自增主键）
    QDateTime timestamp; // 创建时间（本地时间）
    QString userInput;   // 用户输入
    QString rawReply;    // AI原始回复
};

// 用户画像
struct UserProfile {
    qlonglong id = -1;         // 记录ID
    QString key;               // 画像属性名（如"职业"、"爱好"）
    QString value;             // 画像属性值（如"程序员"、"游戏"）
    int tier = 2;              // 半衰期等级
    int confidence = 50;       // 置信度（0-100）
    QDateTime firstSeen;       // 首次记录时间
    QDateTime lastTriggered;   // 上次触发时间
    int sessionCount = 1;      // 触发次数
};

// 长期记忆摘要
struct LongTermSummary {
    qlonglong id = -1;              // 记录ID
    QString summaryText;            // 摘要内容
    qlonglong coveredTurnEndId = -1;// 覆盖的对话结束ID
    QString sourceIds;              // 来源对话ID列表（JSON）
    bool isDirty = false;           // 是否待重建
    QDateTime createdAt;            // 创建时间
    QDateTime updatedAt;            // 更新时间
};
```

**记忆流程**：
```
用户提交输入 → MemoryManager::getHistoryTurn(15) 获取短期记忆
    → LLMService::askDeepSeek(userInput, historyQA) 发送请求（含历史上下文）
    → AI回复 → MemoryManager::saveQATurn(userInput, rawReply, sentences) 保存记忆
    → 判断未摘要对话数 >= 阈值 → LLMService::extractMemoryAsync() 后台提取
    → 提取结果 → upsertUserProfile() 更新画像 + addLongTermSummary() 添加摘要
```

**用户画像衰减机制**：
- 应用启动时调用 `scanAndApplyProfileDecay()`
- Tier 1（长期）：-0.8/天（职业、基本性格、长期爱好）
- Tier 2（中期）：-5.0/天（近期工作安排、本周习惯）
- Tier 3（短期）：-25.0/天（今日心情、即时打算）
- 置信度 <= 0 的画像自动删除

### 2.15 AI状态同步机制（方案C · 已实现）

**设计要点**：
- 本地切换服装/表情后，不立即通知AI
- 下一次对话时，将当前状态追加到系统提示词
- AI自动感知到状态变化，无需额外token消耗

**实现方式**：

```cpp
// LLMService中构建系统提示词时动态追加当前状态
void LLMService::askDeepSeek(const QString& userInput, const QList<HistoryTurn>& historyQA)
{
    QString finalSystemPrompt = m_systemPromptCache;
    if (m_stateProvider) {
        QString currentUiState = m_stateProvider();
        finalSystemPrompt += QString("\n\n【注意：千岛茉子当前的最新实时状态上下文】\n%1\n当前现实世界系统时间: %2\n...")
            .arg(currentUiState, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
    // ... 追加历史对话上下文 ...
    for (const HistoryTurn &turn : historyQA) {
        // 追加历史用户输入和AI回复到messages数组
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
| RC-009 | 未实现的信号槽 | characterwidget.h | `settingsRequested()` 信号已连接到SettingsWidget::show() | ✅ 已实现 |
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
| MemoryManager | memorymanager.h/cpp | AI记忆系统，SQLite数据库存储对话历史 | v0.2.4 |
| HistoryTurn | historyturn.h | 对话历史数据结构 | v0.2.4 |
| SettingsWidget | settingswidget.h/cpp/ui | 设置界面，API配置+记忆管理 | v0.2.4 |

### 7.5 当前模块关系图（更新）

```
AppController
    ├── CharacterWidget（立绘/右键菜单）
    ├── BubbleWidget（气泡/打字机）
    ├── ChatWidget（聊天输入框）
    ├── SettingsWidget（设置界面）
    ├── AnchorManager（位置锚点管理）
    ├── LLMService（AI对话/解析）
    ├── TTSService（语音合成/播放）
    ├── AppearanceManager（四维状态）
    ├── MemoryManager（AI记忆系统）
    └── ConfigManager（单例配置）

AnchorManager 管理：
    ├── BubbleWidget → HeadRight 位置
    └── ChatWidget → WaistCenter 位置

数据流向：
    ConfigManager → LLMService (API Key)
    MemoryManager → LLMService (历史上下文)
    MemoryManager → SettingsWidget (历史记录管理)
    ConfigManager → SettingsWidget (配置读取/保存)
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

---

## 9. 架构问题分析

### 9.1 职责越界问题

| 问题编号 | 模块 | 问题描述 | 严重程度 | 影响分析 |
|----------|------|----------|----------|----------|
| A-001 | AppController | 承担了过多职责：消息分发、记忆管理协调、UI更新、网络错误处理、记忆提取调度 | **高** | 违反单一职责原则，任何改动都可能影响其他功能 |
| A-002 | SettingsWidget | 同时处理配置管理（API Key、记忆长度）和历史记录管理（分页、删除、清空） | **中** | 两类功能耦合，增加了测试复杂度 |
| A-003 | LLMService | 负责网络请求、提示词管理、状态提供者注册、记忆提取 | **中** | 提示词管理和记忆提取可独立为服务 |
| A-004 | MemoryManager | 同时处理三种不同数据类型：聊天历史、用户画像、长期摘要 | **中** | 三类数据操作耦合，可拆分为独立仓储 |

### 9.2 过分耦合问题

| 问题编号 | 耦合类型 | 涉及模块 | 问题描述 | 严重程度 |
|----------|----------|----------|----------|----------|
| C-001 | 直接依赖 | AppController → MemoryManager/LLMService/SettingsWidget | AppController直接持有所有子模块指针 | **高** |
| C-002 | 双向依赖 | LLMService ↔ AppController | LLMService依赖MemoryManager头文件，AppController依赖LLMService | **中** |
| C-003 | 隐式耦合 | AppController内部lambda | 大量内联lambda表达式处理业务逻辑，难以测试和复用 | **高** |
| C-004 | 头文件依赖 | LLMService.h → memorymanager.h | LLMService头文件直接依赖MemoryManager，增加编译依赖链 | **中** |
| C-005 | 初始化顺序 | AppController构造函数 | 模块初始化顺序敏感，需严格按特定顺序创建 | **中** |
| C-006 | 循环包含 | 多个模块互相包含头文件 | 编译时可能出现前向声明问题 | **中** |

### 9.3 代码质量问题

| 问题编号 | 问题描述 | 涉及文件 | 严重程度 |
|----------|----------|----------|----------|
| Q-001 | 内存泄漏风险：AnchorManager未清理动态分配的对象 | anchormanager.cpp | **高** |
| Q-002 | 魔法数字：SUMMARY_THRESHOLD（设计意图：摘要轮数=短期记忆轮数） | appcontroller.cpp:113 | **低（非问题）** |
| Q-003 | 硬编码路径：MemoryManager中数据库路径拼接方式 | memorymanager.cpp:392 | **中** |
| Q-004 | 重复日志输出：MemoryManager中多处重复的错误日志格式 | memorymanager.cpp | **低** |
| Q-005 | 未使用变量：SettingsWidget中on_btn_claenTempMemory_clicked未使用功能 | settingswidget.cpp:122-126 | **低** |
| Q-006 | 注释不完整：部分函数缺少参数和返回值说明 | 多个文件 | **低** |

---

## 10. 改进建议

### 10.1 短期改进（快速修复）

| 优先级 | 改进项 | 说明 |
|--------|--------|------|
| P0 | 修复AnchorManager内存泄漏 | 在析构函数中清理动态分配的widget指针 |
| P0 | 将SUMMARY_THRESHOLD移至ConfigManager | 配置化记忆提取阈值 |
| P1 | 移除LLMService.h对MemoryManager的直接依赖 | 使用前向声明或接口抽象 |
| P1 | 将AppController中的lambda提取为独立槽函数 | 提高可测试性和代码可读性 |

### 10.2 中期改进（架构优化）

| 优先级 | 改进项 | 说明 |
|--------|--------|------|
| P1 | 拆分AppController职责 | 提取消息分发器、记忆协调器等独立组件 |
| P2 | 拆分SettingsWidget | 分离配置管理和历史记录管理为独立页面 |
| P2 | 拆分MemoryManager | 将聊天历史、用户画像、长期摘要拆分为独立仓储 |
| P2 | 引入依赖注入容器 | 管理模块依赖关系，降低初始化顺序敏感性 |

### 10.3 长期改进（架构重构）

| 优先级 | 改进项 | 说明 |
|--------|--------|------|
| P3 | 引入领域驱动设计 | 划分领域边界，定义清晰的领域服务和实体 |
| P3 | 实现插件化架构 | 支持动态加载模块，降低模块间耦合 |
| P3 | 引入状态机模式 | 管理角色状态转换，替代当前的标签驱动方式 |
| P3 | 实现命令模式 | 将用户操作封装为命令对象，支持撤销/重做 |

### 10.4 代码规范改进

| 改进项 | 说明 |
|--------|------|
| 统一日志格式 | 定义日志宏，统一日志前缀和格式 |
| 添加函数注释 | 为所有公共函数添加Doxygen风格注释 |
| 移除未使用代码 | 清理on_btn_claenTempMemory_clicked等未实现功能 |
| 统一错误处理 | 定义统一的错误码和错误处理策略 |
