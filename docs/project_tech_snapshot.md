# 【项目技术快照】

> 项目：Windows_AI 桌面看板娘（桌宠）
> 日期：2026-07-16
> 版本：v0.2.1
> 状态：开发中

---

## 1. 核心类与成员

### AppController（应用中枢）

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_character` | CharacterWidget*，角色组件 |
| | `m_bubble` | BubbleWidget*，气泡组件 |
| | `m_llmService` | LLMService*，AI服务 |
| | `m_appearance` | AppearanceManager*，外观管理 |
| | `m_ttsService` | TTSService*，TTS服务 |
| | `m_chatWidget` | ChatWidget*，聊天输入窗口（新增） |
| | `m_anchorManager` | AnchorManager*，位置锚点管理（新增） |
| **函数签名** | `void startApp()` | 启动应用，显示角色并触发首次对话 |
| | `void handleMakoReply(const QList<SentenceText>& sentences)` | 处理AI回复，交给TTS队列 |
| | `void handleSystemError(const QString& errorMsg)` | 统一错误处理 |
| | `void onPlayAudioAction(const QString& zhText, const QMap<QString,QString>& tags)` | 播放同步：更新气泡+立绘 |
| | `void initConnections()` | 集中初始化信号槽连接 |

### CharacterWidget

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `dragPosition` | QPoint，鼠标拖拽偏移 |
| | `imageLabel` | QLabel*，立绘图片标签 |
| | `m_visibleRect` | QRect，立绘有效像素区域 |
| **函数签名** | `void updatePath(const QString& imagePath)` | 切换立绘图片 |
| | `QPoint getBubbleAnchorPos() const` | 获取气泡锚点坐标（已不再直接使用） |
| | `QPoint getChatAnchorPos() const` | 获取聊天窗口锚点坐标（新增） |
| | `QRect getVisibleRect() const` | 获取立绘有效区域（新增，供AnchorManager使用） |
| | `QRect calculateVisibleRect(const QPixmap& pixmap)` | 计算立绘有效区域 |
| **信号** | `void userChat(const QString& input)` | 用户发起聊天 |
| | `void chatRequested()` | 用户请求聊天（右键菜单触发，新增） |
| | `void settingsRequested()` | 用户请求设置（右键菜单触发，新增） |

### AnchorManager（新增·位置锚点管理）

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_character` | CharacterWidget*，角色组件 |
| | `m_anchors` | std::unordered_map\<QWidget*, AnchorInfo\>，锚点映射 |
| **函数签名** | `void registerWidget(QWidget* widget, AnchorConfig config)` | 注册Widget到预定义锚点位置 |
| | `void registerWidget(QWidget* widget, std::function<QPoint()> customCalculator)` | 注册Widget到自定义位置计算 |
| | `void unregisterWidget(QWidget* widget)` | 注销Widget |
| | `void updateAllAnchors()` | 更新所有锚点位置 |
| | `QPoint calculatePosition(const QRect& visibleRect, const QPoint& characterPos, const QSize& widgetSize, const AnchorConfig& config)` | 根据配置计算目标位置 |

### AnchorStrategy（锚点策略）

| 枚举 | 值 | 说明 |
|------|------|------|
| AnchorPosition | Bottom | 底部 |
| | WaistCenter | 腰部居中 |
| | WaistLeft | 腰部左侧 |
| | WaistRight | 腰部右侧 |
| | TopCenter | 顶部居中 |
| | HeadRight | 头部右侧 |
| | Custom | 自定义 |

### ChatWidget（聊天输入窗口）

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_lineEdit` | QLineEdit*，输入框 |
| **函数签名** | `void popup()` | 弹出聊天窗口 |
| **信号** | `void textSubmitted(const QString& text)` | 用户提交文本 |

### AppearanceManager

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_distance` | QString，距离：far/closer |
| | `m_clothing` | QString，服装：pajama/schoolUniform/schoolUniformWithoutCap/schoolUniformWithoutCoat |
| | `m_blush` | QString，脸红：unblushing/blushing |
| | `m_emotion` | QString，表情：happyIdle/happyMore/amazing/loving/caring/sad/conscientious |
| | `m_lastPath` | QString，上次路径缓存 |
| **函数签名** | `void applyTags(const QMap<QString,QString>& tags)` | 应用标签，触发换图 |
| | `QString getPath() const` | 生成资源路径 |
| | `void setDefault()` | 重置为默认状态 |
| **信号** | `void characterPathChanged(const QString& newPath)` | 路径变化信号 |

### LLMService

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_networkManager` | QNetworkAccessManager*，网络请求管理器 |
| | `m_apiKey` | QString，API Key（构造函数注入） |
| **函数签名** | `LLMService(const QString& apiKey, QObject* parent)` | 构造函数，接收API Key参数 |
| | `void askDeepSeek(const QString& userInput)` | 发起DeepSeek请求（系统指令含多标签协议） |
| | `void onReplyFinished(QNetworkReply* reply)` | 解析回复，拆分为多句SentenceText |
| **信号** | `void sentenceReady(const QList<SentenceText>& sentence)` | 句子解析完成 |
| | `void internetErrorSignal(const QString& errorMessage)` | 网络错误 |

### TTSService

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_ttsQueue` | QQueue\<SentenceText\>，合成队列 |
| | `m_playQueue` | QQueue\<SentenceText\>，播放队列 |
| | `m_isSynthesizing` | bool，合成状态锁 |
| | `m_isPlaying` | bool，播放状态锁 |
| **函数签名** | `void enqueueSentences(const QList<SentenceText>& sentences)` | 入队多句话 |
| | `void processTtsQueue()` | 合成生产者 |
| | `void processPlayQueue()` | 播放消费者 |
| **信号** | `void playAudioAction(const QString& zhText, const QMap<QString,QString>& tags)` | 播放同步信号 |

> ⚠️ 当前为模拟实现（QTimer模拟耗时），尚未接入真实TTS引擎。

### BubbleWidget

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_timer` | QTimer*，打字机定时器 |
| | `m_text` | QString，完整文本 |
| | `m_idex` | int，当前显示位置 |
| **函数签名** | `void showMessage(const QString& text)` | 显示气泡并启动打字机 |
| | `void typeWriteEffect()` | 打字机特效 |
| **信号** | `void bubbleShown()` | 气泡显示时发出，通知位置管理器更新位置 |

### ConfigManager

| 类型 | 名称 | 说明 |
|------|------|------|
| **模式** | 单例模式 | - |
| **私有变量** | `m_apiKey` | QString，DeepSeek API Key |
| **函数签名** | `static ConfigManager& instance()` | 获取单例 |
| | `QString getApiKey() const` | 获取API Key |
| | `void setApiKey(const QString& apiKey)` | 设置API Key |

### TimeManager（规划中 · v0.3.0）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 时间监控与服装自动切换 | - |
| **时间段** | 白天(6:00-18:00)校服 / 夜晚(18:00-6:00)睡衣 | - |
| **函数签名** | `void start()` | 启动时间监控 |
| | `QString getCurrentTimePeriod()` | 获取当前时间段 |
| | `void checkTime()` | 检查时间并触发切换 |
| **信号** | `void timePeriodChanged(const QString& period)` | 时间段变化 |
| | `void clothingAutoChanged(const QString& clothing)` | 服装自动切换 |

### SettingsWidget（规划中 · v0.3.0）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 设置界面，管理所有配置项 | - |
| **权限** | 高于右键菜单，包含所有功能 | - |
| **配置项** | API配置、TTS配置、外观配置、时间配置、快捷键配置、记忆配置 | - |
| **函数签名** | `void loadSettings()` | 从配置文件加载 |
| | `void saveSettings()` | 保存到配置文件 |
| | `void applySettings()` | 应用到各模块 |

### ShortcutManager（规划中 · v0.3.0）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 全局快捷键管理 | - |
| **实现** | QShortcut绑定全局快捷键 | - |
| **默认快捷键** | Ctrl+Shift+C(聊天)、Ctrl+Shift+S(设置)、Ctrl+Shift+H(历史) | - |
| **函数签名** | `void registerShortcut(const QString& key, QObject* receiver, const char* method)` | 注册快捷键 |
| | `void updateShortcuts()` | 根据配置更新 |

### AI状态同步机制（方案C · 规划中 · v0.3.0）

| 特性 | 说明 |
|------|------|
| **策略** | 本地切换后，下轮对话时将状态追加到系统提示词 |
| **状态内容** | 服装、心情、距离、脸红、时间 |
| **同步时机** | 每次调用askDeepSeek()前 |
| **优势** | 不额外消耗token，AI自动感知状态变化 |

### SentenceText

```cpp
struct SentenceText {
    QString zhText;                  // 中文文本（气泡显示）
    QString jaText;                  // 日文文本（TTS合成）
    QMap<QString, QString> rawTags;  // 多维状态标签
};
```

---

## 2. 关键通信逻辑

### AI对话完整流程

```
用户右键 → chatRequested信号 → ChatWidget::popup() → 用户输入 → textSubmitted信号 → LLMService::askDeepSeek
                                                                                                              │
                                                                                                              ▼
                                                                                                    DeepSeek API
                                                                                                              │
                                                                                                              ▼
                                                                                                   onReplyFinished 解析
                                                                                                              │
                                                                                               正则拆分多句 + 提取标签
                                                                                                              │
                                                                                                              ▼
                                                                                                   sentenceReady(QList<SentenceText>)
                                                                                                              │
                                                                                                              ▼
                                                                                            AppController::handleMakoReply
                                                                                                              │
                                                                                                              ▼
                                                                                            TTSService::enqueueSentences
                                                                                                              │
                                                                                           ┌───────────┴───────────┐
                                                                                           ▼                       ▼
                                                                                    合成队列(Producer)       播放队列(Consumer)
                                                                                           │                       │
                                                                                           ▼                       ▼
                                                                                    onMockTtsFinished      playAudioAction信号
                                                                                                                   │
                                                                                                                   ▼
                                                                                                   AppController::onPlayAudioAction
                                                                                                        ┌──────┴──────┐
                                                                                                        ▼             ▼
                                                                                                气泡显示中文    立绘切换状态
```

### 信号槽连接（AppController中统一管理）

| 发送方 | 信号 | 接收方 | 槽 |
|--------|------|--------|-----|
| CharacterWidget | `userChat(input)` | LLMService | `askDeepSeek()` |
| CharacterWidget | `chatRequested()` | ChatWidget | `popup()` |
| ChatWidget | `textSubmitted(text)` | LLMService | `askDeepSeek()` |
| LLMService | `sentenceReady(sentences)` | AppController | `handleMakoReply()` |
| LLMService | `internetErrorSignal(msg)` | AppController | `handleSystemError()` |
| TTSService | `playAudioAction(zhText, tags)` | AppController | `onPlayAudioAction()` |
| AppearanceManager | `characterPathChanged(path)` | CharacterWidget | `updatePath()` |
| AppearanceManager | `characterPathChanged(path)` | AnchorManager | `updateAllAnchors()` |
| BubbleWidget | `bubbleShown()` | AnchorManager | `updateAllAnchors()` |

### 位置跟随机制（AnchorManager）

| Widget | 锚点位置 | 偏移 |
|--------|----------|------|
| BubbleWidget | HeadRight | 默认 |
| ChatWidget | WaistCenter | (0, 20) |

---

## 3. 现有痛点/未解逻辑

### 模拟/占位功能

| 位置 | 说明 |
|------|------|
| TTSService | TTS为模拟实现（QTimer），未接入真实语音合成引擎 |
| ConfigManager::ConfigManager() | API Key硬编码，未从文件/环境加载 |
| settingsRequested信号 | 右键菜单"设置"入口已预留但无处理槽函数 |

### 待开发功能

- [ ] 真实TTS引擎接入（GPT-SoVITS）
- [ ] 设置界面（API Key配置、音量、速度等）
- [ ] 配置文件持久化
- [ ] 对话历史（短期记忆+长期记忆）
- [ ] 立绘切换过渡动画

### 已知代码问题

| 位置 | 问题描述 |
|------|----------|
| chatwidget.cpp | `#include <QVBoxLayout>` 出现两次 |
| anchormanager.cpp | `onCharacterChanged()` 方法从未被调用 |
| appcontroller.cpp | 未清理动态分配的AnchorManager（内存泄漏风险） |

---

## 4. 项目文件结构

```
Windows_AI/
├── docs/                    # 文档目录
│   ├── project_tech_snapshot.md
│   ├── development_log.md
│   ├── ARCHITECTURE.md
│   └── ROADMAP.md
├── main.cpp                 # 入口函数（创建AppController）
├── CMakeLists.txt           # 构建配置
├── image.qrc                # 资源文件
├── appcontroller.h/cpp      # 应用中枢
├── anchormanager.h/cpp      # 位置锚点管理（新增）
├── anchorstrategy.h         # 锚点策略（新增）
├── chatwidget.h/cpp         # 聊天输入窗口（新增）
├── sentencedata.h           # 句子数据结构
├── appearancemanager.h/cpp  # 外观管理器
├── ttsservice.h/cpp         # TTS语音服务
├── llmservice.h/cpp         # AI服务模块
├── configmanager.h/cpp      # 配置管理器
├── characterwidget.h/cpp    # 角色主组件（已瘦身）
├── bubblewidget.h/cpp/ui    # 气泡组件
└── image/                   # 立绘资源目录
    ├── closer/              # 近景
    └── far/                 # 远景
```

---

## 5. 模块关系图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         AppController                                │
│                    （应用中枢 / 信号调度中心）                          │
│                                                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                 │
│  │  Character  │  │   Bubble    │  │   Chat      │                 │
│  │   Widget    │  │   Widget    │  │   Widget    │                 │
│  │ 立绘/右键菜单 │  │ 气泡/打字机  │  │ 聊天输入框  │                 │
│  └─────────────┘  └──────┬──────┘  └──────┬──────┘                 │
│         │                │                │                         │
│         │                └────────┬───────┘                         │
│         │                         │                                 │
│         ▼                         ▼                                 │
│  ┌─────────────┐         ┌─────────────┐                           │
│  │  Anchor     │         │  LLMService │                           │
│  │   Manager   │         │  AI对话/解析  │                           │
│  │ 位置锚点管理 │         └──────┬──────┘                           │
│  └─────────────┘                ▼                                   │
│                          ┌─────────────┐                           │
│                          │ ConfigM-    │                           │
│                          │   anager    │                           │
│                          │  单例配置    │                           │
│                          └─────────────┘                           │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                        TTSService                            │  │
│  │  合成队列(Producer)  →  播放队列(Consumer)                    │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌─────────────┐                                                   │
│  │ Appearance  │                                                   │
│  │   Manager   │                                                   │
│  │ 四维状态管理 │                                                   │
│  └─────────────┘                                                   │
│                                                                     │
│  SentenceText 数据结构（贯穿 LLM → TTS → UI）                        │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 6. 近期变更

| 日期 | 变更内容 |
|------|----------|
| 2026-07-16 | 修复 LLMService 构造函数中 API Key 未赋值问题（m_apiKey = apiKey） |
| 2026-07-16 | 修复气泡首次显示位置未初始化问题（新增 bubbleShown 信号） |
| 2026-07-16 | 修复立绘切换时位置未更新问题（连接 characterPathChanged 到 updateAllAnchors） |
| 2026-07-16 | 移除 BubbleWidget/ChatWidget 中重复的位置跟随逻辑（attachTo/eventFilter/updatePosition） |
| 2026-07-16 | 实现 HeadRight 锚点策略 |
| 2026-07-16 | 新增 AnchorManager（位置锚点管理）、AnchorStrategy（锚点策略枚举）、ChatWidget（聊天输入窗口） |
| 2026-07-16 | CharacterWidget 新增右键菜单（和茉子聊天/设置/退出）、getChatAnchorPos()、getVisibleRect() |
| 2026-07-16 | AppController 集成 AnchorManager，气泡和聊天窗口位置统一管理 |
| 2026-07-16 | 清理 CharacterWidget 构造函数中的硬编码图片加载 |
| 2026-07-15 | 新增句子解析、TTS服务、外观管理、AppController中枢 |
| 2026-07-15 | 统一图片命名为7种emotion规范格式 |
