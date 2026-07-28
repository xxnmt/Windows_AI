# 【项目技术快照】

> 项目：Windows_AI 桌面看板娘（桌宠）
> 日期：2026-07-29
> 版本：v0.5.0
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
| | `m_memoryManager` | MemoryManager*，AI记忆系统 |
| | `m_lastUserInput` | QString，最后一次用户输入 |
| **函数签名** | `void startApp()` | 启动应用，显示角色 |
| | `void handleMakoReply(const QList<SentenceText>& sentences, const QString& rawReply)` | 处理AI回复，保存记忆+入队TTS+触发记忆提取 |
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
| | `QPoint getChatAnchorPos() const` | 获取聊天窗口锚点坐标 |
| | `QRect getVisibleRect() const` | 获取立绘有效区域 |
| | `QRect calculateVisibleRect(const QPixmap& pixmap)` | 计算立绘有效区域 |
| **信号** | `void chatRequested()` | 用户请求聊天（右键菜单触发） |
| | `void settingsRequested()` | 用户请求设置（右键菜单触发） |

### AnchorManager（位置锚点管理）

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
| | `QString getCurrentStateDescription() const` | 获取当前状态描述字符串 |
| | `QString getDistance() const` | 获取当前距离 |
| | `QString getClothing() const` | 获取当前服装 |
| | `QString getBlush() const` | 获取当前脸红状态 |
| | `QString getEmotion() const` | 获取当前表情 |
| **信号** | `void characterPathChanged(const QString& newPath)` | 路径变化信号 |

### LLMService

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_networkManager` | QNetworkAccessManager*，网络请求管理器 |
| | `m_apiKey` | QString，API Key（构造函数注入） |
| | `m_localPromptPath` | QString，提示词文件路径 |
| | `m_systemPromptCache` | QString，系统提示词缓存 |
| | `m_stateProvider` | std::function\<QString()\>，状态提供者回调 |
| | `m_memoryManager` | MemoryManager*，AI记忆系统 |
| **函数签名** | `LLMService(const QString& apiKey, QObject* parent)` | 构造函数，接收API Key参数 |
| | `void askDeepSeek(const QString& userInput, const QList<HistoryTurn>& historyQA = QList<HistoryTurn>())` | 发起DeepSeek请求（JSON协议，含response_format） |
| | `void onReplyFinished(QNetworkReply* reply)` | 解析JSON回复→拆句→标签校验→发信号 |
| | `QPair<QList<SentenceText>, QString> parseJsonReply(const QString& replyText)` | 解析JSON回复，提取句子和标签 |
| | `void extractMemoryAsync(const QList<HistoryTurn>& turns, qlonglong lastEndId, const QList<qlonglong>& sourceIds)` | 异步提取记忆（用户画像+长期摘要） |
| | `void registerStateProvider(std::function<QString()> provider)` | 注册状态提供者 |
| | `void setMemoryManager(MemoryManager* manager)` | 设置MemoryManager实例 |
| | `void initializePromptFile()` | 初始化提示词文件 |
| | `QString loadSystemPrompt()` | 从文件加载系统提示词 |
| | `void setApiKey(const QString& apiKey)` | 设置API Key |
| **信号** | `void sentencesReady(const QList<SentenceText>& sentences, const QString& rawReply)` | 句子解析完成（复数形式，语义正确） |
| | `void internetErrorSignal(const QString& errorMessage)` | 网络错误 |
| | `void memoryExtractionReady(const QJsonArray& profiles, const QString& summary, qlonglong lastEndId, const QString& sourceIdsJson)` | 记忆提取完成 |

**JSON输出协议**：
```json
{
  "sentences": [
    {
      "zh_text": "中文文本",
      "ja_text": "日文翻译",
      "tags": {
        "emotion": "happyIdle",
        "blush": "unblushing",
        "distance": "far",
        "clothing": "schoolUniform"
      }
    }
  ]
}
```

**API请求参数**：
```cpp
rootObj["model"] = "deepseek-v4-flash";
rootObj["messages"] = messagesArray;
rootObj["temperature"] = 0.7;
rootObj["max_tokens"] = 4096;
rootObj["response_format"] = QJsonObject{{"type", "json_object"}};
```

### TagValidator（标签校验类）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 标签合法性校验与修正 | - |
| **校验流程** | 精确匹配 → 大小写忽略 → 编辑距离 → 继承上一状态 → 使用默认值 | 5级回退 |
| **关键方法** | `int levenshteinDistance(const QString& s1, const QString& s2)` | 计算编辑距离 |
| | `QString validateTag(const QString& tagName, const QString& rawValue, const QStringList& validValues, const QString& contextText = "", const QString& prevValue = "")` | 校验并修正标签值 |
| | `int getEditDistanceThreshold(const QString& value)` | 获取编辑距离阈值 |

**合法标签值**：
| 标签 | 合法值列表 |
|------|------------|
| emotion | happyIdle, happyMore, amazing, loving, caring, sad, conscientious |
| blush | unblushing, blushing |
| distance | far, closer |
| clothing | pajama, schoolUniform, schoolUniformWithoutCap, schoolUniformWithoutCoat |

### TTSService

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_ttsQueue` | QQueue\<SentenceText\>，合成队列 |
| | `m_playQueue` | QQueue\<SentenceText\>，播放队列 |
| | `m_isSynthesizing` | bool，合成状态锁 |
| | `m_isPlaying` | bool，播放状态锁 |
| | `m_provider` | ITTSProvider*，TTS Provider接口 |
| **函数签名** | `void enqueueSentences(const QList<SentenceText>& sentences)` | 入队多句话 |
| | `void processTtsQueue()` | 合成生产者 |
| | `void processPlayQueue()` | 播放消费者 |
| | `void reloadProvider()` | 根据配置重新加载TTS Provider |
| | `void switchModel(const QString& gptPath, const QString& sovitsPath)` | 切换GPT-SoVITS模型 |
| **信号** | `void playAudioAction(const QString& zhText, const QMap<QString,QString>& tags)` | 播放同步信号 |

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

**实现类**：
- `ApiTTSProvider`：调用 GPT-SoVITS HTTP API
- `MockTTSProvider`：模拟实现，用于开发测试

### MemoryManager（AI记忆系统）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 管理AI对话历史、用户画像和长期记忆摘要 | - |
| **私有变量** | `m_db` | QSqlDatabase，SQLite数据库连接 |
| **函数签名** | `bool saveQATurn(const QString& userInput, const QString& rawReply)` | 保存单轮对话（raw_reply为JSON对象） |
| | `QList<HistoryTurn> getHistoryTurn(int N)` | 获取最近N轮对话历史 |
| | `QList<HistoryTurn> getHistoryTurn(int offset, int limit)` | 分页获取对话历史 |
| | `qlonglong getTotalHistoryCount()` | 获取历史记录总数 |
| | `bool deleteTurnByID(int id)` | 根据ID删除单条记录 |
| | `bool clearAllHistory()` | 清空所有历史记录 |
| | `bool upsertUserProfile(const QString& key, const QString& value, int tier, int confidenceGain)` | 插入或更新用户画像 |
| | `QList<UserProfile> getActiveUserProfiles(int minConfidence = 30)` | 获取活跃用户画像 |
| | `int scanAndApplyProfileDecay()` | 应用置信度衰减 |
| | `bool addLongTermSummary(const QString& summaryText, qlonglong coveredEndId, const QString& sourceIdsJson)` | 添加长期摘要 |
| | `QList<LongTermSummary> getLatestSummaries(int limit = 5)` | 获取最新摘要 |
| | `QList<HistoryTurn> getUnsummarizedTurns(qlonglong& outLastEndId, QString& outSourceIds)` | 获取未摘要对话 |
| | `void initDatabase()` | 初始化数据库，创建三张表 |
| **数据库路径** | `app_data/memory/QianDaoMoZi_memory.db` | - |

**数据库结构**：

**表1：chat_history（对话历史）**
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER PRIMARY KEY AUTOINCREMENT | 记录ID |
| timestamp | DATETIME | 创建时间 |
| user_input | TEXT NOT NULL | 用户输入文本 |
| raw_reply | TEXT NOT NULL | AI回复的JSON对象 `{"sentences": [...]}` |

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

### 数据结构

**HistoryTurn（对话历史）**：
```cpp
struct HistoryTurn {
    qlonglong id = -1;
    QDateTime timestamp;
    QString userInput;
    QString rawReply;  // JSON对象: {"sentences": [...]}
};
```

**UserProfile（用户画像）**：
```cpp
struct UserProfile {
    qlonglong id = -1;
    QString key;
    QString value;
    int tier = 2;
    int confidence = 50;
    QDateTime firstSeen;
    QDateTime lastTriggered;
    int sessionCount = 1;
};
```

**LongTermSummary（长期记忆摘要）**：
```cpp
struct LongTermSummary {
    qlonglong id = -1;
    QString summaryText;
    qlonglong coveredTurnEndId = -1;
    QString sourceIds;
    bool isDirty = false;
    QDateTime createdAt;
    QDateTime updatedAt;
};
```

**SentenceText（句子数据）**：
```cpp
struct SentenceText {
    QString zhText;                  // 中文文本 → BubbleWidget显示
    QString jaText;                  // 日文文本 → TTS合成输入
    QMap<QString, QString> rawTags;  // 四维状态标签 → AppearanceManager换图
    bool isValidated;                // 标签是否已校验
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
| **信号** | `void bubbleShown()` | 气泡显示时发出，通知位置管理器 |

### ConfigManager

| 类型 | 名称 | 说明 |
|------|------|------|
| **模式** | Meyers单例（线程安全） | - |
| **私有变量** | `m_apiKey` | QString，DeepSeek API Key |
| | `m_ttsUrl` | QString，GPT-SoVITS服务地址 |
| | `m_configDirPath` | QString，配置目录路径 |
| | `m_configFilePath` | QString，配置文件路径 |
| **函数签名** | `static ConfigManager& instance()` | 获取单例 |
| | `QString getApiKey() const` | 获取API Key |
| | `void setApiKey(const QString& apiKey)` | 设置API Key |
| | `QString getTTSUrl() const` | 获取TTS服务地址 |
| | `void setTTSUrl(const QString& ttsUrl)` | 设置TTS服务地址 |
| | `int getShortMemoryLength() const` | 获取短期记忆长度 |
| | `QString getConfigDirPath() const` | 获取配置目录路径 |
| | `QString getMemoryPath() const` | 获取数据库文件路径 |
| | `bool loadSetting()` | 从JSON文件加载配置 |
| | `bool saveSetting()` | 保存配置到JSON文件 |

> ✅ 配置文件持久化已实现：`app_data/config/setting.json`

### SettingsWidget（设置界面）

| 类型 | 名称 | 说明 |
|------|------|------|
| **职责** | 设置界面，管理API Key、记忆长度配置和对话历史管理 | - |
| **配置项** | API Key配置、记忆长度配置、历史记录管理 | - |
| **函数签名** | `void show()` | 显示设置窗口 |
| | `void setMemoryManager(MemoryManager* manager)` | 注入MemoryManager实例 |
| | `void setMemoryLength(int length)` | 初始化记忆长度设置 |
| | `void refreshHistoryTurnList()` | 刷新历史记录列表 |
| | `void loadHistoryPage(int page)` | 加载指定页的历史记录 |
| | `void on_btn_saveApiKey_clicked()` | 保存API Key |
| | `void on_btn_deleteSelectedMemory_clicked()` | 删除选中的历史记录 |
| | `void on_btn_claenAllMemory_clicked()` | 清空所有历史记录 |
| **信号** | `void settingsSaved()` | 设置保存后发出 |

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
                                                             DeepSeek API (JSON Mode)
                                                                    │
                                                                    ▼
                                                          onReplyFinished 解析JSON
                                                                    │
                                                         TagValidator校验标签
                                                                    │
                                                                    ▼
                                                      sentencesReady(QList<SentenceText>, rawReply)
                                                                    │
                                                                    ▼
                                             AppController::handleMakoReply
                                               ┌────────────────┴────────────────┐
                                               ▼                                 ▼
                                        MemoryManager::saveQATurn        TTSService::enqueueSentences
                                        (保存用户输入+JSON回复)            │
                                                          ┌───────────┴───────────┐
                                                          ▼                       ▼
                                                   合成队列(Producer)       播放队列(Consumer)
                                                          │                       │
                                                          ▼                       ▼
                                                   ApiTTSProvider        playAudioAction信号
                                                   (GPT-SoVITS HTTP)              │
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
| LLMService | `sentencesReady(sentences, rawReply)` | AppController | `handleMakoReply()` |
| LLMService | `internetErrorSignal(msg)` | AppController | `handleSystemError()` |
| LLMService | `memoryExtractionReady(profiles, summary, ...)` | AppController lambda | 写入用户画像+长期摘要 |
| TTSService | `playAudioAction(zhText, tags)` | AppController | `onPlayAudioAction()` |
| AppearanceManager | `characterPathChanged(path)` | CharacterWidget | `updatePath()` |
| AppearanceManager | `characterPathChanged(path)` | AnchorManager | `updateAllAnchors()` |
| BubbleWidget | `bubbleShown()` | AnchorManager | `updateAllAnchors()` |
| SettingsWidget | `settingsSaved()` | AppController lambda | 更新LLMService API Key |

### AI状态同步机制（方案C · 已实现）

| 特性 | 说明 |
|------|------|
| **策略** | 本地切换后，下轮对话时将状态追加到系统提示词 |
| **状态内容** | 服装、心情、距离、脸红、时间 |
| **同步时机** | 每次调用askDeepSeek()前，动态获取并追加 |
| **优势** | 不额外消耗token，AI自动感知状态变化 |
| **实现方式** | 通过 `registerStateProvider()` 注册回调 |

### AI记忆系统（已实现·三层记忆）

| 层级 | 说明 | 存储位置 |
|------|------|----------|
| **短期记忆** | 最近N轮对话历史（默认15轮） | chat_history表 |
| **用户画像** | 持久化用户特征（置信度衰减） | user_profile表 |
| **长期摘要** | 重要对话的摘要总结 | long_term_summary表 |

**历史记忆注入**：只注入纯文本，剥离标签格式，避免影响LLM输出格式。

---

## 3. 现有痛点/未解逻辑

### 模拟/占位功能

| 位置 | 说明 |
|------|------|
| TTSService | MockTTSProvider为模拟实现，ApiTTSProvider需GPT-SoVITS服务运行 |

### 待开发功能

- [x] 配置文件持久化 ✅ 已实现（JSON文件）
- [x] AI记忆系统（短期记忆）✅ 已实现（SQLite数据库）
- [x] AI记忆系统（中期/长期记忆）✅ 已实现（用户画像+长期摘要）
- [x] 系统提示词外部化 ✅ 已实现（prompt.txt文件）
- [x] AI状态同步机制 ✅ 已实现（状态提供者模式）
- [x] JSON输出协议 ✅ 已实现（response_format+JSON解析）
- [x] 标签校验 ✅ 已实现（TagValidator+编辑距离）
- [ ] 设置界面完善（TTS配置、外观配置、时间配置等）
- [ ] 立绘切换过渡动画
- [ ] 错误重试机制（LLM请求失败自动重试）
- [ ] AnchorManager内存泄漏修复（析构函数清理）

### 已知代码问题

| 位置 | 问题描述 | 严重程度 |
|------|----------|----------|
| anchormanager.cpp | AnchorManager析构未清理m_anchors（内存泄漏风险） | **高** |
| appcontroller.cpp | SUMMARY_THRESHOLD魔法数字硬编码 | 低 |
| llmservice.h | 头文件依赖memorymanager.h，增加编译依赖链 | 中 |

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
├── historyturn.h            # 历史数据结构（HistoryTurn/UserProfile/LongTermSummary）
├── appearancemanager.h/cpp  # 外观管理器
├── ttsservice.h/cpp         # TTS语音服务
├── ittsprovider.h           # TTS Provider接口
├── apittsprovider.h/cpp     # GPT-SoVITS API实现
├── mockttsprovider.h/cpp     # Mock实现
├── llmservice.h/cpp         # AI服务模块（含TagValidator内部类）
├── configmanager.h/cpp      # 配置管理器
├── memorymanager.h/cpp      # AI记忆系统
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
│  │   Manager   │         │  AI对话/JSON解析/标签校验                │
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
│  │  合成队列(Producer) → ITTSProvider → 播放队列(Consumer)       │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌─────────────┐  ┌─────────────┐                                  │
│  │ Appearance  │  │  Memory     │                                  │
│  │   Manager   │  │   Manager   │                                  │
│  │ 四维状态管理 │  │  AI记忆系统  │                                  │
│  └─────────────┘  └─────────────┘                                  │
│                                                                     │
│  核心数据结构：                                                     │
│  SentenceText (zhText + jaText + rawTags)                          │
│  HistoryTurn / UserProfile / LongTermSummary                       │
│  TagValidator (编辑距离标签校验)                                    │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 6. 近期变更

| 日期 | 变更内容 |
|------|----------|
| 2026-07-29 | 实现TagValidator类，支持5级标签校验流程（精确匹配→编辑距离→继承→默认值） |
| 2026-07-29 | 更新文档：ARCHITECTURE.md、development_log.md、project_tech_snapshot.md、ROADMAP.md |
| 2026-07-28 | 数据库结构简化：删除parsed_json列，raw_reply直接存储JSON对象 |
| 2026-07-28 | LLM输出格式从标签协议迁移到JSON协议，添加response_format参数 |
| 2026-07-28 | 重构parseJsonReply()，从正则拆分改为JSON对象解析 |
| 2026-07-28 | 历史记忆注入优化：只注入纯文本内容，剥离标签格式 |
| 2026-07-26 | TTSService重构为策略模式，新增ITTSProvider接口和ApiTTSProvider实现 |
| 2026-07-26 | 接入GPT-SoVITS HTTP API，实现实时语音合成 |
| 2026-07-25 | TTSService双队列模型优化，集成Qt Multimedia实现音频播放 |
| 2026-07-23 | 新增用户画像管理（置信度衰减）和长期记忆摘要功能 |
| 2026-07-23 | MemoryManager新增getUnsummarizedTurns()，LLMService新增extractMemoryAsync() |
| 2026-07-23 | AppController集成记忆提取流程：检查未摘要对话数，触发后台提取 |
| 2026-07-22 | SettingsWidget新增记忆管理页，支持历史记录分页浏览、单条删除、清空全部 |
| 2026-07-22 | MemoryManager新增分页查询、计数、删除、清空方法 |
| 2026-07-22 | 修复ConfigManager配置文件损坏问题，修复SettingsWidget空指针访问问题 |
| 2026-07-19 | 新增 MemoryManager（AI记忆系统），使用SQLite数据库存储对话历史 |
| 2026-07-19 | LLMService::askDeepSeek 新增 historyQA 参数，支持传入对话历史上下文 |
| 2026-07-19 | AppController 集成 MemoryManager，实现对话历史的保存与加载（默认15轮短期记忆） |
| 2026-07-17 | ConfigManager实现配置文件持久化，LLMService实现提示词外部化和AI状态同步机制 |
| 2026-07-16 | 新增 AnchorManager、AnchorStrategy、ChatWidget，修复多个位置跟随问题 |
| 2026-07-15 | 新增句子解析、TTS服务、外观管理、AppController中枢 |
