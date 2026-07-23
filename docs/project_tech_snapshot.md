# 【项目技术快照】

> 项目：Windows_AI 桌面看板娘（桌宠）
> 日期：2026-07-23
> 版本：v0.2.6
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
| | `m_chatWidget` | ChatWidget*，聊天输入窗口 |
| | `m_anchorManager` | AnchorManager*，位置锚点管理 |
| | `m_settingsWidget` | SettingsWidget*，设置界面 |
| | `m_memoryManager` | MemoryManager*，AI记忆系统（新增） |
| | `m_lastUserInput` | QString，最后一次用户输入（新增） |
| **函数签名** | `void startApp()` | 启动应用，显示角色（不再自动触发首次对话） |
| | `void handleMakoReply(const QList<SentenceText>& sentences, const QString& rawReply)` | 处理AI回复，交给TTS队列并保存记忆 |
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
| **信号** | `void chatRequested()` | 用户请求聊天（右键菜单触发） |
| | `void settingsRequested()` | 用户请求设置（右键菜单触发） |

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
| | `QString getCurrentStateDescription() const` | 获取当前状态描述字符串（新增） |
| | `QString getDistance() const` | 获取当前距离（新增） |
| | `QString getClothing() const` | 获取当前服装（新增） |
| | `QString getBlush() const` | 获取当前脸红状态（新增） |
| | `QString getEmotion() const` | 获取当前表情（新增） |
| **信号** | `void characterPathChanged(const QString& newPath)` | 路径变化信号 |

### LLMService

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_networkManager` | QNetworkAccessManager*，网络请求管理器 |
| | `m_apiKey` | QString，API Key（构造函数注入） |
| | `m_localPromptPath` | QString，提示词文件路径 |
| | `m_systemPromptCache` | QString，系统提示词缓存 |
| | `m_stateProvider` | std::function\<QString()\>，状态提供者回调 |
| **函数签名** | `LLMService(const QString& apiKey, QObject* parent)` | 构造函数，接收API Key参数，初始化提示词文件并加载系统提示词 |
| | `void askDeepSeek(const QString& userInput, const QList<HistoryTurn>& historyQA = QList<HistoryTurn>())` | 发起DeepSeek请求（支持传入对话历史，系统指令含多标签协议，动态追加状态上下文） |
| | `void onReplyFinished(QNetworkReply* reply)` | 解析回复，拆分为多句SentenceText |
| | `void registerStateProvider(std::function<QString()> provider)` | 注册状态提供者（已实现） |
| | `void initializePromptFile()` | 初始化提示词文件，首次启动从资源释放默认prompt.txt（已实现） |
| | `QString loadSystemPrompt()` | 从文件加载系统提示词，失败时回退到资源文件（已实现） |
| | `void setApiKey(const QString& apiKey)` | 设置API Key（新增） |
| **信号** | `void sentenceReady(const QList<SentenceText>& sentences, const QString& rawReply)` | 句子解析完成，携带原始回复用于保存记忆 |
| | `void internetErrorSignal(const QString& errorMessage)` | 网络错误 |

> ⚠️ **已知问题**：`sentenceReady` 信号参数命名为单数 `sentence`，但实际传递的是 `QList<SentenceText>` 复数列表，语义不一致。

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

### MemoryManager（新增·AI记忆系统）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 管理AI对话历史记录，支持短期记忆查询 | - |
| **私有变量** | `m_db` | QSqlDatabase，SQLite数据库连接 |
| **函数签名** | `bool saveQATurn(const QString& userInput, const QString& rawReply, const QList<SentenceText>& sentences)` | 保存单轮对话（用户输入+原始回复+解析后句子） |
| | `QList<HistoryTurn> getHistoryTurn(int N)` | 获取最近N轮对话历史（按时间正序） |
| | `QList<HistoryTurn> getHistoryTurn(int offset, int limit)` | 分页获取对话历史（按时间倒序） |
| | `qlonglong getTotalHistoryCount()` | 获取历史记录总数 |
| | `bool deleteTurnByID(int id)` | 根据ID删除单条记录 |
| | `bool clearAllHistory()` | 清空所有历史记录 |
| | `void initDatabase()` | 初始化数据库，创建聊天记录表 |
| **数据库结构** | chat_history表（id, timestamp, user_input, raw_reply, parsed_json） | - |
| **数据库路径** | app_data/memory/QianDaoMoZi_memory.db | - |

### HistoryTurn（新增·对话历史数据结构）

```cpp
struct HistoryTurn {
    qlonglong id = -1;   // 记录ID（自增主键）
    QDateTime timestamp; // 创建时间（本地时间）
    QString userInput;   // 用户输入
    QString rawReply;    // AI原始回复
};
```

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
| **模式** | Meyers单例（线程安全） | - |
| **私有变量** | `m_apiKey` | QString，DeepSeek API Key |
| | `m_ttsUrl` | QString，GPT-SoVITS服务地址 |
| | `m_configDirPath` | QString，配置目录路径（新增） |
| | `m_configFilePath` | QString，配置文件路径 |
| **函数签名** | `static ConfigManager& instance()` | 获取单例 |
| | `QString getApiKey() const` | 获取API Key |
| | `void setApiKey(const QString& apiKey)` | 设置API Key |
| | `QString getTTSUrl() const` | 获取TTS服务地址 |
| | `void setTTSUrl(const QString& ttsUrl)` | 设置TTS服务地址 |
| | `QString getConfigDirPath() const` | 获取配置目录路径（新增） |
| | `QString getConfigFilePath() const` | 获取配置文件路径（新增） |
| | `bool loadSetting()` | 从JSON文件加载配置 |
| | `bool saveSetting()` | 保存配置到JSON文件 |

> ✅ 配置文件持久化已实现：首次启动创建 `app_data/config/setting.json`，包含API Key和TTS服务地址，默认值分别为 `sk-placeholder-key` 和 `http://127.0.0.1:9880`。

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

### SettingsWidget（已实现 · API配置页 + 记忆管理页）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 设置界面，管理API Key、记忆长度配置和对话历史管理 | - |
| **配置项** | API Key配置、记忆长度配置、历史记录管理 | - |
| **函数签名** | `void show()` | 显示设置窗口（重写showEvent加载配置） |
| | `void loadSettings()` | 从配置文件加载当前配置 |
| | `void setMemoryManager(MemoryManager* manager)` | 注入MemoryManager实例 |
| | `void setMemoryLength(int length)` | 初始化记忆长度设置 |
| | `void refreshHistoryTurnList()` | 刷新历史记录列表 |
| | `void loadHistoryPage(int page)` | 加载指定页的历史记录 |
| | `void on_btn_saveApiKey_clicked()` | 保存API Key到配置文件 |
| | `void on_btn_saveMemoryLength_clicked()` | 保存记忆长度到配置文件 |
| | `void on_btn_deleteSelectedMemory_clicked()` | 删除选中的历史记录（需API Key验证） |
| | `void on_btn_claenAllMemory_clicked()` | 清空所有历史记录（需API Key验证） |
| **信号** | `void settingsSaved()` | 设置保存后发出，通知AppController更新LLMService的API Key |

### ShortcutManager（规划中 · v0.3.0）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 全局快捷键管理 | - |
| **实现** | QShortcut绑定全局快捷键 | - |
| **默认快捷键** | Ctrl+Shift+C(聊天)、Ctrl+Shift+S(设置)、Ctrl+Shift+H(历史) | - |
| **函数签名** | `void registerShortcut(const QString& key, QObject* receiver, const char* method)` | 注册快捷键 |
| | `void updateShortcuts()` | 根据配置更新 |

### AI状态同步机制（方案C · 已实现）

| 特性 | 说明 |
|------|------|
| **策略** | 本地切换后，下轮对话时将状态追加到系统提示词 |
| **状态内容** | 服装、心情、距离、脸红、时间 |
| **同步时机** | 每次调用askDeepSeek()前，动态获取并追加 |
| **优势** | 不额外消耗token，AI自动感知状态变化 |
| **实现方式** | 通过 `registerStateProvider()` 注册回调，`askDeepSeek()` 中调用获取当前状态 |
| **状态描述格式** | 距离/服装/脸红状态/当前表情（四维状态） |

### AI记忆系统（已实现·短期记忆）

| 特性 | 说明 |
|------|------|
| **策略** | 使用SQLite数据库存储对话历史，每次对话前读取最近N轮作为短期记忆 |
| **记忆长度** | 可配置（当前默认15轮） |
| **存储内容** | 用户输入、AI原始回复、解析后的句子JSON |
| **数据库路径** | app_data/memory/QianDaoMoZi_memory.db |
| **同步时机** | 用户提交输入后，先读取记忆再发送请求；AI回复后保存记忆 |
| **优势** | 支持跨会话记忆，AI能记住之前的对话内容 |

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

### AI对话完整流程（含记忆系统）

```
用户右键 → chatRequested信号 → ChatWidget::popup() → 用户输入 → textSubmitted信号
                                                                    │
                                                                    ▼
                                                      AppController lambda (保存m_lastUserInput)
                                                                    │
                                                                    ▼
                                                      MemoryManager::getHistoryTurn(15) 获取短期记忆
                                                                    │
                                                                    ▼
                                                      LLMService::askDeepSeek(userInput, historyQA)
                                                                    │
                                                                    ▼
                                                             DeepSeek API (含历史上下文)
                                                                    │
                                                                    ▼
                                                          onReplyFinished 解析
                                                                    │
                                                           正则拆分多句 + 提取标签
                                                                    │
                                                                    ▼
                                                      sentenceReady(QList<SentenceText>, rawReply)
                                                                    │
                                                                    ▼
                                             AppController::handleMakoReply
                                               ┌────────────────┴────────────────┐
                                               ▼                                 ▼
                                        MemoryManager::saveQATurn        TTSService::enqueueSentences
                                        (保存用户输入+原始回复+解析句子)        │
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

| 发送方 | 信号 | 接收方 | 槽/处理 |
|--------|------|--------|---------|
| CharacterWidget | `chatRequested()` | ChatWidget | `popup()` |
| CharacterWidget | `settingsRequested()` | SettingsWidget | `show()` |
| ChatWidget | `textSubmitted(text)` | AppController lambda | 保存输入→读取记忆→调用askDeepSeek |
| LLMService | `sentenceReady(sentences, rawReply)` | AppController | `handleMakoReply()` |
| LLMService | `internetErrorSignal(msg)` | AppController | `handleSystemError()` |
| TTSService | `playAudioAction(zhText, tags)` | AppController | `onPlayAudioAction()` |
| AppearanceManager | `characterPathChanged(path)` | CharacterWidget | `updatePath()` |
| AppearanceManager | `characterPathChanged(path)` | AnchorManager | `updateAllAnchors()` |
| BubbleWidget | `bubbleShown()` | AnchorManager | `updateAllAnchors()` |
| SettingsWidget | `settingsSaved()` | AppController lambda | 更新LLMService API Key |

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

### 待开发功能

- [ ] 真实TTS引擎接入（GPT-SoVITS）
- [ ] 设置界面完善（TTS配置、外观配置、时间配置等）
- [x] 配置文件持久化 ✅ 已实现（JSON文件）
- [x] AI记忆系统（短期记忆）✅ 已实现（SQLite数据库，MemoryManager）
- [ ] AI记忆系统（中期/长期记忆）
- [x] 系统提示词外部化 ✅ 已实现（prompt.txt文件，首次启动自动释放）
- [x] AI状态同步机制（方案C）✅ 已实现（状态提供者模式，动态追加到提示词）
- [ ] 立绘切换过渡动画
- [ ] 错误重试机制（LLM请求失败自动重试）
- [ ] AnchorManager内存泄漏修复（析构函数清理）



### 已知代码问题

| 位置 | 问题描述 | 严重程度 |
|------|----------|----------|
| appcontroller.cpp | 未清理动态分配的AnchorManager（内存泄漏风险） | **高** |
| llmservice.h | `sentenceReady` 信号参数命名为单数 `sentence`，实际传递复数列表 | 中 |
| appcontroller.cpp | SUMMARY_THRESHOLD魔法数字硬编码 | 中 |
| llmservice.h | 头文件直接依赖memorymanager.h，增加编译依赖链 | 中 |
| settingswidget.cpp | on_btn_claenTempMemory_clicked未实现功能 | 低 |
| memorymanager.cpp | 多处重复的错误日志格式 | 低 |

### 架构问题（详细见ARCHITECTURE.md第9章）

| 问题类型 | 数量 | 说明 |
|----------|------|------|
| 职责越界 | 4 | AppController、SettingsWidget、LLMService、MemoryManager职责过重 |
| 过分耦合 | 6 | 直接依赖、双向依赖、隐式耦合、头文件依赖、初始化顺序、循环包含 |
| 代码质量 | 6 | 内存泄漏、魔法数字、硬编码路径、重复日志、未使用代码、注释不完整 |

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
├── anchormanager.h/cpp      # 位置锚点管理
├── anchorstrategy.h         # 锚点策略
├── chatwidget.h/cpp         # 聊天输入窗口
├── sentencedata.h           # 句子数据结构
├── appearancemanager.h/cpp  # 外观管理器
├── ttsservice.h/cpp         # TTS语音服务
├── llmservice.h/cpp         # AI服务模块
├── configmanager.h/cpp      # 配置管理器
├── memorymanager.h/cpp      # AI记忆系统（新增）
├── historyturn.h            # 对话历史数据结构（新增）
├── characterwidget.h/cpp    # 角色主组件
├── bubblewidget.h/cpp/ui    # 气泡组件
├── settingswidget.h/cpp/ui  # 设置界面
└── image/                   # 立绘资源目录
    ├── closer/              # 近景
    ├── far/                 # 远景
    └── default_config/      # 默认配置（prompt.txt）
```

---

## 5. 模块关系图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         AppController                                │
│                    （应用中枢 / 信号调度中心）                          │
│                                                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐ │
│  │  Character  │  │   Bubble    │  │   Chat      │  │  Settings   │ │
│  │   Widget    │  │   Widget    │  │   Widget    │  │   Widget    │ │
│  │ 立绘/右键菜单 │  │ 气泡/打字机  │  │ 聊天输入框  │  │   设置界面   │ │
│  └─────────────┘  └──────┬──────┘  └──────┬──────┘  └─────────────┘ │
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
│  ┌─────────────┐  ┌─────────────┐                                  │
│  │ Appearance  │  │  Memory     │                                  │
│  │   Manager   │  │   Manager   │                                  │
│  │ 四维状态管理 │  │  AI记忆系统  │                                  │
│  └─────────────┘  └─────────────┘                                  │
│                                                                     │
│  SentenceText / HistoryTurn 数据结构（贯穿 LLM → Memory → UI）        │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 6. 近期变更

| 日期 | 变更内容 |
|------|----------|
| 2026-07-23 | 架构审查完成，识别16个技术债务项，更新ARCHITECTURE.md新增架构问题分析和改进建议章节 |
| 2026-07-22 | SettingsWidget新增记忆管理页，支持历史记录分页浏览、单条删除、清空全部 |
| 2026-07-22 | MemoryManager新增分页查询、计数、删除、清空方法，支持用户画像和长期摘要存储 |
| 2026-07-22 | 修复ConfigManager配置文件损坏问题，配置缺失时使用默认值 |
| 2026-07-22 | 修复SettingsWidget空指针访问问题，四个方法添加空指针检查 |
| 2026-07-19 | 新增 MemoryManager（AI记忆系统），使用SQLite数据库存储对话历史 |
| 2026-07-19 | 新增 HistoryTurn 数据结构，定义对话历史模型 |
| 2026-07-19 | LLMService::askDeepSeek 新增 historyQA 参数，支持传入对话历史上下文 |
| 2026-07-19 | sentenceReady 信号新增 rawReply 参数，用于保存原始回复 |
| 2026-07-19 | AppController 集成 MemoryManager，实现对话历史的保存与加载（默认15轮短期记忆） |
| 2026-07-19 | 移除 CharacterWidget 中无用的 userChat 信号和自动问候代码 |
| 2026-07-19 | 新增 SettingsWidget 设置界面（API配置页），右键菜单"设置"入口已连接 |
| 2026-07-17 | ConfigManager实现配置文件持久化（app_data/config/setting.json），支持API Key和TTS服务地址的读写 |
| 2026-07-17 | LLMService实现提示词外部化（prompt.txt），首次启动自动释放默认提示词 |
| 2026-07-17 | LLMService实现AI状态同步机制（方案C），动态追加当前状态到系统提示词 |
| 2026-07-17 | AppearanceManager新增状态描述方法（getCurrentStateDescription等），供AI状态同步使用 |
| 2026-07-17 | 修复DeepSeek API请求格式错误（系统提示词包装为对象而非字符串） |
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
