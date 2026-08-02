# 架构设计文档

> 项目：Windows_AI 桌面看板娘（桌宠）
> 版本：v0.6.0
> 更新日期：2026-08-02

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
| **业务层** | LLMService | DeepSeek请求、JSON协议解析、句子拆分、记忆提取 | llmservice.h/cpp |
| | TTSService | 语音合成队列、播放队列、策略模式（ITTSProvider） | ttsservice.h/cpp, ittsprovider.h, apittsprovider.h/cpp |
| | ApiTTSProvider | GPT-SoVITS API 调用、流式 PCM 解析 | apittsprovider.h/cpp |
| | IPcmPlayer | 播放器抽象接口（PcmPlayerFinished/Error 信号） | ipcmplayer.h |
| | StreamPlayer | 流式 PCM 播放（QAudioSink + QBuffer，预填充+兜底检测） | streamplayer.h/cpp |
| | FilePlayer | 文件型 WAV 播放（QAudioSink + QFile，WAV头解析） | fileplayer.h/cpp |
| | AppearanceManager | 四维状态管理、立绘路径生成、换图触发 | appearancemanager.h/cpp |
| | AnchorManager | 位置锚点管理、统一位置跟随 | anchormanager.h/cpp |
| | TagValidator | 标签合法性校验、编辑距离修正 | llmservice.h/cpp（内部类） |
| **数据层** | ConfigManager | API Key配置、记忆长度配置、单例模式 | configmanager.h/cpp |
| | MemoryManager | AI对话历史管理、SQLite数据库 | memorymanager.h/cpp |
| **设置层** | SettingsWidget | 设置界面、API配置、记忆管理 | settingswidget.h/cpp/ui |
| **数据结构** | SentenceText | 句子数据模型（中文/日文/标签） | sentencedata.h |
| | HistoryTurn | 对话历史数据结构 | historyturn.h |
| | UserProfile | 用户画像数据结构 | historyturn.h |
| | LongTermSummary | 长期摘要数据结构 | historyturn.h |
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
│  └─────────────────┘           └─────────────────┘                │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                        LLMService                            │  │
│  │           AI对话/JSON解析 → ConfigManager(单例)               │  │
│  │           ← AI状态同步机制（方案C）                            │  │
│  │           ← MemoryManager(历史上下文+用户画像+摘要)            │  │
│  │           ← TagValidator(标签校验与修正)                      │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                        TTSService                            │  │
│  │  合成队列(Producer) → ITTSProvider(ApiTTSProvider)           │  │
│  │  ← 流式合成：pcmDataReady 信号 → StreamPlayer.writePcm()     │  │
│  │  ← 非流式合成：synthesisFinished → 临时WAV文件路径            │  │
│  │  播放队列(Consumer) → IPcmPlayer(StreamPlayer/FilePlayer)    │  │
│  │  ← playAudioAction 信号同步 UI（气泡+立绘）                  │  │
│  │  ← setSynthesisDone() 标记流式合成完成，触发播放结束检测      │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐    │
│  │ SettingsWidget  │  │ ConfigManager   │  │  MemoryManager  │    │
│  │   设置界面       │  │   单例配置管理   │  │   AI记忆系统     │    │
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
- 负责记忆提取触发与结果写入

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `startApp()` | 启动应用，显示角色 | - | - |
| `handleMakoReply()` | 接收LLM解析结果，保存记忆+入队TTS+触发记忆提取 | QList\<SentenceText\> sentences, QString rawReply | - |
| `handleSystemError()` | 统一错误处理，显示错误气泡+切悲伤立绘 | QString errorMsg | - |
| `onPlayAudioAction()` | TTS播放同步：更新气泡+立绘 | QString zhText, QMap tags | - |

**信号槽连接清单**（全部在构造函数中建立）：
```cpp
// 用户输入 → LLM请求（通过ChatWidget，含记忆查询）
connect(m_chatWidget, &ChatWidget::textSubmitted, this, [this](const QString &text){
    m_lastUserInput = text;
    int memoryLength = ConfigManager::instance().getShortMemoryLength();
    QList<HistoryTurn> shortTermMemory = m_memoryManager->getHistoryTurn(memoryLength);
    m_llmService->askDeepSeek(text, shortTermMemory);
});

// 右键菜单 → 设置界面
connect(m_character, &CharacterWidget::settingsRequested, m_settingsWidget, &SettingsWidget::show);

// 设置保存 → 更新LLMService API Key
connect(m_settingsWidget, &SettingsWidget::settingsSaved, this, [this](){
    m_llmService->setApiKey(ConfigManager::instance().getApiKey());
});

// LLM回复 → AppController → 保存记忆 + 入队TTS + 触发记忆提取
connect(m_llmService, &LLMService::sentencesReady, this, &AppController::handleMakoReply);

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

// 记忆提取结果 → 写入用户画像和长期摘要
connect(m_llmService, &LLMService::memoryExtractionReady, this, [this](const QJsonArray &profiles, const QString &summary, qlonglong lastEndId, const QString &sourceIdsJson) {
    // 遍历写入用户画像
    for (const QJsonValue &val : profiles) {
        QJsonObject obj = val.toObject();
        QString key = obj["key"].toString();
        QString value = obj["value"].toString();
        int tier = obj["tier"].toInt();
        m_memoryManager->upsertUserProfile(key, value, tier, 10);
    }
    // 写入长期记忆摘要
    if (!summary.isEmpty()) {
        m_memoryManager->addLongTermSummary(summary, lastEndId, sourceIdsJson);
    }
});

// 状态提供者注册（AI状态同步）
m_llmService->registerStateProvider([this](){return m_appearance->getCurrentStateDescription();});
```

---

### 2.2 CharacterWidget

**职责**：角色立绘显示与鼠标交互

**设计要点**：
- 仅负责UI渲染和用户交互
- 不再直接持有LLMService和BubbleWidget（解耦）
- 通过 `updatePath()` 接收外观变化

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `updatePath()` | 切换立绘图片 | QString imagePath | - |
| `calculateVisibleRect()` | 扫描像素透明度，计算有效区域 | QPixmap pixmap | QRect |
| `mousePressEvent()` | 记录拖拽起始偏移 | QMouseEvent* | - |
| `mouseMoveEvent()` | 移动窗口并发出角色移动 | QMouseEvent* | - |

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
- 位置跟随由 AnchorManager 统一管理
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

**职责**：AI对话服务，负责DeepSeek请求、JSON协议解析、标签校验

**设计要点**：
- 与UI完全解耦，纯业务逻辑
- 使用 JSON Mode（`response_format: {type: "json_object"}`）强制 LLM 输出合法 JSON
- 系统指令要求输出 `{"sentences": [...]}` 格式
- 将回复拆分为多句 `SentenceText`
- 每句独立解析 emotion/blush/distance/clothing 标签
- **状态提供者模式**：通过 `registerStateProvider()` 注册回调，实现AI状态同步
- **提示词外部化**：`initializePromptFile()` 和 `loadSystemPrompt()` 已实现
- **标签校验**：`TagValidator` 类实现标签合法性校验和编辑距离修正

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `askDeepSeek()` | 发起DeepSeek API请求 | QString userInput, QList\<HistoryTurn\> historyQA | - |
| `onReplyFinished()` | 解析JSON→拆句→标签校验→发信号 | QNetworkReply* | - |
| `parseJsonReply()` | 解析JSON回复，提取句子和标签 | QString replyText | QPair\<QList\<SentenceText\>, QString\> |
| `extractMemoryAsync()` | 后台异步提取记忆（用户画像+长期摘要） | QList\<HistoryTurn\> turns, qlonglong lastEndId, QList\<qlonglong\> sourceIds | - |
| `registerStateProvider()` | 注册状态提供者回调 | std::function\<QString()\> | - |
| `setMemoryManager()` | 设置MemoryManager实例 | MemoryManager* manager | - |
| `initializePromptFile()` | 初始化提示词文件 | - | - |
| `loadSystemPrompt()` | 从文件加载系统提示词 | - | QString |
| `setApiKey()` | 设置API Key | QString apiKey | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `sentencesReady(sentences, rawReply)` | 回复解析完成 | QList\<SentenceText\>, QString |
| `internetErrorSignal(msg)` | 网络错误 | QString |
| `memoryExtractionReady(profiles, summary, lastEndId, sourceIdsJson)` | 记忆提取完成 | QJsonArray, QString, qlonglong, QString |

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

**标签参数**：
| 标签 | 必填 | 可选值 | 说明 |
|------|------|--------|------|
| emotion | 是 | happyIdle, happyMore, amazing, loving, caring, sad, conscientious | 表情 |
| blush | 否 | unblushing, blushing | 脸红状态 |
| distance | 否 | far, closer | 距离 |
| clothing | 否 | pajama, schoolUniform, schoolUniformWithoutCap, schoolUniformWithoutCoat | 服装 |

**API请求参数**：
```cpp
rootObj["model"] = "deepseek-v4-flash";
rootObj["messages"] = messagesArray;
rootObj["temperature"] = 0.7;
rootObj["max_tokens"] = 4096;
rootObj["response_format"] = QJsonObject{{"type", "json_object"}};
```

---

### 2.5 TagValidator

**职责**：标签合法性校验与修正

**设计要点**：
- 支持精确匹配、大小写忽略匹配、编辑距离匹配、继承上一状态、使用默认值
- 编辑距离阈值根据标签长度动态调整

**校验流程**：
```
1. 精确匹配 → 直接通过
2. 大小写忽略匹配 → 修正大小写
3. 编辑距离匹配 → 修正拼写错误
4. 继承上一状态 → 使用上一句的标签值
5. 使用默认值 → 使用该标签的默认值
```

**合法标签值**：
| 标签 | 合法值列表 |
|------|------------|
| emotion | happyIdle, happyMore, amazing, loving, caring, sad, conscientious |
| blush | unblushing, blushing |
| distance | far, closer |
| clothing | pajama, schoolUniform, schoolUniformWithoutCap, schoolUniformWithoutCoat |

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `levenshteinDistance()` | 计算两个字符串的编辑距离 | QString s1, QString s2 | int |
| `validateTag()` | 校验并修正标签值 | tagName, rawValue, validValues, contextText, prevValue | QString |
| `getEditDistanceThreshold()` | 获取编辑距离阈值 | QString value | int |

**默认值**：
| 标签 | 默认值 |
|------|--------|
| emotion | happyIdle |
| blush | unblushing |
| distance | far |
| clothing | schoolUniform |

---

### 2.6 TTSService

**职责**：TTS语音合成与播放管理

**设计要点**：
- **策略模式**：通过 `ITTSProvider` 抽象接口支持多种 TTS 实现
- **双队列模型**：合成队列 `m_ttsQueue` → 播放队列 `m_playQueue`
- **双状态锁**：`m_isSynthesizing` / `m_isPlaying` 控制并发
- **流式合成**：`ApiTTSProvider` 调用 GPT-SoVITS HTTP API，`streaming_mode=true`，通过 `pcmDataReady` 信号实时推送 PCM 数据
- **播放器抽象**：`IPcmPlayer` 接口统一 `StreamPlayer`（流式）和 `FilePlayer`（文件型）两种播放器
- **预填充防吞开头**：`startPlayer` 时先把队列中已到达的 PCM 写入 QBuffer，再 `QAudioSink::start`，避免设备冷启动期间数据丢失
- **播放完成检测**：`setSynthesisDone()` 标记合成结束，`onStateChanged(IdleState)` + `checkPlayEnd()` 兜底定时器双重检测
- **模拟回退**：合成失败时走模拟播放（按字数估算时长）

**架构图**：
```
                    ┌──────────────────────────────────────────────────┐
                    │                  TTSService                      │
                    │                                                  │
  SentenceText ───► │  m_ttsQueue (待合成)                             │
  (enqueueSentences)│      │                                           │
                    │      ▼                                           │
                    │  processTtsQueue()                               │
                    │      │ 流式：创建 StreamPlayer，连接 pcmDataReady │
                    │      ▼                                           │
                    │  ApiTTSProvider.synthesize() (HTTP POST /tts)    │
                    │      │                                           │
                    │      ├─ pcmDataReady(QByteArray) ─► StreamPlayer │
                    │      │   (writePcm → m_queue)                    │
                    │      ▼                                           │
                    │  onTtsFinished()                                 │
                    │      │ 流式：setSynthesisDone() 标记合成完成      │
                    │      ▼                                           │
                    │  m_playQueue (待播放)                             │
                    │      │                                           │
                    │      ▼                                           │
                    │  processPlayQueue()                              │
                    │      │                                           │
                    │      ├──► playAudioAction(zhText, tags) ──► UI   │
                    │      │                                           │
                    │      └──► IPcmPlayer::startPlayer()              │
                    │           ├─ StreamPlayer: 预填充+QAudioSink     │
                    │           └─ FilePlayer: WAV头解析+预填充        │
                    │                                                  │
                    │  onPcmPlayFinished() → 清理+继续下一句            │
                    └──────────────────────────────────────────────────┘
```

**ITTSProvider 接口**：
```cpp
class ITTSProvider : public QObject {
    Q_OBJECT
public:
    virtual void synthesize(const SentenceText &sentence) = 0;
    virtual void warmUp() = 0;
    virtual bool isStreamingMode() const = 0;
signals:
    void synthesisFinished(const QString &audioPath, const SentenceText &sentence);
    void synthesisFailed(const QString &errorMsg, const SentenceText &sentence);
    void pcmDataReady(const QByteArray &pcmData);  // 流式专用
};
```

**IPcmPlayer 接口**：
```cpp
class IPcmPlayer : public QObject {
    Q_OBJECT
public:
    virtual void startPlayer(int sampleRate, int channels, int sampleBits) = 0;
    virtual void stopPlayer() = 0;
    virtual void writePcm(const QByteArray &pcmData) = 0;
    virtual bool getisPlaying() const = 0;
    virtual void setSynthesisDone() {}  // 流式专用：标记合成完成
signals:
    void PcmPlayerFinished();
    void PcmPlayerError(const QString &error);
};
```

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `enqueueSentences()` | 接收多句话，推入合成队列 | QList\<SentenceText\> | - |
| `reloadProvider()` | 根据配置重新加载 TTS Provider | - | - |
| `switchModel()` | 切换 GPT-SoVITS 模型 | gptPath, sovitsPath | - |
| `processTtsQueue()` | 取出一句调用 Provider 合成 | - | - |
| `onTtsFinished()` | 合成完成：入播放队列+setSynthesisDone+继续合成 | audioPath, sentence | - |
| `onTtsFailed()` | 合成失败：跳过该句，继续合成 | errorMsg, sentence | - |
| `processPlayQueue()` | 取出一句：发信号+创建播放器+startPlayer | - | - |
| `onPcmPlayFinished()` | 播放完成：清理播放器+继续播放下一句 | - | - |
| `onPcmPlayError()` | 播放错误：直接调用 onPcmPlayFinished | msg | - |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `playAudioAction(zhText, tags)` | 播放开始时，用于同步UI | QString, QMap\<QString,QString\> |

**StreamPlayer 播放完成检测机制**：
- **setSynthesisDone()**：TTSService 在 onTtsFinished 中调用，标记所有 PCM 已到达
- **onStateChanged(IdleState)**：QAudioSink 状态变 Idle 时检查 `queueEmpty && bufferDone && synthesisDone`
- **checkPlayEnd() 兜底定时器**：500ms 间隔，检测 `queueEmpty && bufferDone && deviceEmpty`，连续3次（1.5秒）满足则判定播放完
- **finishAndEmit()**：统一停止定时器+emit PcmPlayerFinished

**预填充防吞开头机制**：
- StreamPlayer：startPlayer 时从 m_queue 取出所有已到达 PCM，写入 m_buffer，seek(0) 后再 `QAudioSink::start`
- FilePlayer：playFile 中 seek(44) 跳过 WAV 头后，预读取 CHUNK_SIZE*4 数据写入 m_buffer，再 resume()

---

### 2.7 AppearanceManager

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
| `getCurrentStateDescription()` | 获取当前状态描述字符串 | - | QString |
| `getDistance()` | 获取当前距离 | - | QString |
| `getClothing()` | 获取当前服装 | - | QString |
| `getBlush()` | 获取当前脸红状态 | - | QString |
| `getEmotion()` | 获取当前表情 | - | QString |

**信号**：
| 信号 | 触发时机 | 参数 |
|------|----------|------|
| `characterPathChanged(newPath)` | 立绘路径变化时 | QString |

---

### 2.8 SentenceText（数据结构）

**职责**：贯穿LLM→TTS→UI的核心数据模型

```cpp
struct SentenceText {
    QString zhText;                  // 中文文本 → BubbleWidget显示
    QString jaText;                  // 日文文本 → TTS合成输入
    QMap<QString, QString> rawTags;  // 四维状态标签 → AppearanceManager换图
    bool isValidated;                // 标签是否已校验
};
```

**流转路径**：
```
LLMService::parseJsonReply() → TagValidator校验
    → AppController::handleMakoReply()
        → MemoryManager::saveQATurn()（持久化）
        → TTSService::enqueueSentences()（入队）
            → playAudioAction信号 → UI更新
```

---

### 2.9 ConfigManager

**职责**：全局配置管理（单例），支持配置文件持久化

**设计要点**：
- Meyers单例，线程安全
- 配置文件路径：`app_data/config/setting.json`
- 首次启动自动创建配置文件，使用默认值
- 支持 API Key、GPT-SoVITS 服务地址、短期记忆长度配置

**配置项**：
| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| api.deepseek_api_key | `sk-placeholder-key` | DeepSeek API密钥 |
| api.gpt_sovits_url | `http://127.0.0.1:9880` | GPT-SoVITS服务地址 |
| memory.short_term_length | `15` | 短期记忆轮数 |

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `loadSetting()` | 从JSON文件加载配置 | - | bool |
| `saveSetting()` | 保存配置到JSON文件 | - | bool |
| `getApiKey()` | 获取API Key | - | QString |
| `setApiKey()` | 设置API Key | QString | - |
| `getTTSUrl()` | 获取TTS服务地址 | - | QString |
| `getShortMemoryLength()` | 获取短期记忆长度 | - | int |
| `getConfigDirPath()` | 获取配置目录路径 | - | QString |
| `getMemoryPath()` | 获取数据库文件路径 | - | QString |

---

### 2.10 AnchorManager

**职责**：位置锚点管理器，统一协调所有跟随角色的Widget位置

**设计要点**：
- 通过 `eventFilter` 监听 CharacterWidget 的 Move/Resize 事件
- 支持预定义锚点策略和自定义位置计算函数
- 立绘切换时通过信号连接自动更新位置

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `registerWidget()` | 注册Widget到预定义锚点位置 | QWidget*, AnchorConfig | - |
| `unregisterWidget()` | 注销Widget | QWidget* | - |
| `updateAllAnchors()` | 计算并更新所有锚点位置 | - | - |
| `calculatePosition()` | 根据锚点策略计算目标位置 | QRect, QPoint, QSize, AnchorConfig | QPoint |

**位置更新触发机制**：
| 触发条件 | 机制 |
|----------|------|
| 角色移动 | eventFilter 捕获 QEvent::Move |
| 角色大小变化 | eventFilter 捕获 QEvent::Resize |
| 立绘切换 | characterPathChanged 信号 |
| 气泡显示 | bubbleShown 信号 |

---

### 2.11 ChatWidget

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

### 2.12 SettingsWidget

**职责**：设置界面，管理API Key、记忆长度配置和对话历史管理

**设计要点**：
- 独立窗口（Qt::Window），支持关闭按钮
- 显示时自动加载当前配置
- 配置修改后立即保存并通知AppController更新

**当前实现**：
- API Key配置页（输入框 + 保存按钮）
- 记忆长度配置（数字输入框 + 保存按钮）
- 对话历史管理页（表格展示、分页浏览、单条删除、清空全部）

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `show()` | 显示设置窗口 | - | - |
| `setMemoryManager()` | 注入MemoryManager实例 | MemoryManager* | - |
| `setMemoryLength()` | 初始化记忆长度设置 | int length | - |
| `refreshHistoryTurnList()` | 刷新历史记录列表 | - | - |
| `loadHistoryPage()` | 加载指定页的历史记录 | int page | - |
| `on_btn_saveApiKey_clicked()` | 保存API Key | - | - |
| `on_btn_deleteSelectedMemory_clicked()` | 删除选中记录 | - | - |
| `on_btn_claenAllMemory_clicked()` | 清空全部记录 | - | - |

---

### 2.13 MemoryManager（已实现 · AI记忆系统）

**职责**：管理AI对话历史记录、用户画像和长期记忆摘要

**设计要点**：
- 使用SQLite数据库存储三类数据
- 每次对话后保存用户输入、AI回复（JSON对象）
- 对话前读取最近N轮历史作为短期记忆
- 支持用户画像的置信度衰减机制
- 支持长期记忆摘要的自动生成和管理
- 数据库路径：`app_data/memory/QianDaoMoZi_memory.db`

**数据库结构**：

**表1：chat_history（对话历史）**
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER PRIMARY KEY AUTOINCREMENT | 记录ID |
| timestamp | DATETIME | 创建时间（本地时间） |
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

**关键方法**：

**对话历史 CRUD**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `saveQATurn()` | 保存单轮对话 | userInput, rawReply(JSON), sentences | bool |
| `getHistoryTurn(N)` | 获取最近N轮对话 | int N | QList\<HistoryTurn\> |
| `getHistoryTurn(offset, limit)` | 分页获取对话 | int offset, int limit | QList\<HistoryTurn\> |
| `getTotalHistoryCount()` | 获取记录总数 | - | qlonglong |
| `deleteTurnByID(id)` | 删除单条记录 | int id | bool |
| `clearAllHistory()` | 清空所有记录 | - | bool |
| `getUnsummarizedTurns()` | 获取未摘要对话 | outLastEndId, outSourceIds | QList\<HistoryTurn\> |

**用户画像 CRUD**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `upsertUserProfile()` | 插入或更新画像 | key, value, tier, confidenceGain | bool |
| `getActiveUserProfiles()` | 获取活跃画像 | minConfidence | QList\<UserProfile\> |
| `deleteUserProfile()` | 删除画像 | id | bool |
| `scanAndApplyProfileDecay()` | 应用置信度衰减 | - | int |

**长期记忆摘要 CRUD**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `addLongTermSummary()` | 添加摘要 | summaryText, coveredEndId, sourceIdsJson | bool |
| `getLatestSummaries()` | 获取最新摘要 | limit | QList\<LongTermSummary\> |
| `deleteLongTermSummary()` | 删除摘要 | id | bool |

**数据结构**：

```cpp
// 对话历史
struct HistoryTurn {
    qlonglong id = -1;
    QDateTime timestamp;
    QString userInput;
    QString rawReply;  // JSON对象: {"sentences": [...]}
};

// 用户画像
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

// 长期记忆摘要
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

**记忆流程**：
```
用户提交输入 → getHistoryTurn(N) 获取短期记忆
    → askDeepSeek(userInput, historyQA) 发送请求
    → AI回复 → saveQATurn(userInput, rawReply, sentences) 保存记忆
    → 判断未摘要对话数 >= 阈值 → extractMemoryAsync() 后台提取
    → 提取结果 → upsertUserProfile() + addLongTermSummary()
```

**用户画像衰减机制**：
- 应用启动时调用 `scanAndApplyProfileDecay()`
- Tier 1（长期）：-0.8/天
- Tier 2（中期）：-5.0/天
- Tier 3（短期）：-25.0/天
- 置信度 <= 0 的画像自动删除

---

## 3. 核心数据流

### 3.1 完整对话流程

```
1. 用户输入
   │
   ▼
2. CharacterWidget::chatRequested() → ChatWidget::popup() → 用户输入 → textSubmitted(input)
   │
   ▼
3. AppController lambda
   │
   ├── 保存 m_lastUserInput
   ├── 读取短期记忆: m_memoryManager->getHistoryTurn(memoryLength)
   └── 调用 askDeepSeek(userInput, shortTermMemory)
   │
   ▼
4. LLMService::askDeepSeek()
   │
   ├── 构建系统提示词（含用户画像+长期摘要+当前状态）
   ├── 构建短期记忆（纯文本注入，不含标签）
   ├── 构建JSON请求体（含 response_format: json_object）
   └── 发送POST到DeepSeek API
   │
   ▼
5. API响应 → onReplyFinished()
   │
   ├── 提取content文本
   ├── 解析JSON对象: {"sentences": [...]}
   ├── TagValidator校验标签合法性
   ├── 生成修正后的JSON
   └── 发射 sentencesReady(sentences, correctedReply)
   │
   ▼
6. AppController::handleMakoReply()
   │
   ├── saveQATurn(userInput, correctedReply, sentences) 保存对话
   ├── enqueueSentences(sentences) 入队TTS
   └── 检查是否触发记忆提取
   │
   ▼
7. TTSService::enqueueSentences()
   │
   ├── 入合成队列
   └── 启动合成流水线
   │
   ▼
8. 合成队列（Producer）
   │
   ├── 取出一句 → ApiTTSProvider.synthesize(HTTP POST)
   └── 继续取下一句
   │
   ▼
9. 播放队列（Consumer）
   │
   ├── 取出一句 → playAudioAction信号
   └── QMediaPlayer播放音频
   │
   ▼
10. AppController::onPlayAudioAction(zhText, tags)
    │
    ├── BubbleWidget::showMessage(zhText) → 打字机显示
    └── AppearanceManager::applyTags(tags) → 立绘切换
```

### 3.2 历史记忆注入

```
读取历史记录 → 遍历每个HistoryTurn
    │
    ├── 用户输入: 直接注入
    │
    └── AI回复: 
        ├── 尝试解析JSON对象
        │   └── 成功 → 提取zh_text拼接为纯文本
        └── 解析失败
            └── 剥离[xxx:yyy]格式标签 → 纯文本
        
        注入为: {"role": "assistant", "content": "纯文本内容"}
```

**设计决策**：只注入纯文本，不注入标签格式，避免影响LLM的输出格式。

---

## 4. 资源管理

### 4.1 立绘资源结构

```
image/
├── closer/                    # 近景（大尺寸）
│   ├── pajama/
│   │   ├── blushing/ (7张)
│   │   └── unblushing/ (7张)
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
| **策略模式** | TTSService（ITTSProvider接口） | 多种TTS实现可切换 |
| **多态接口** | IPcmPlayer（StreamPlayer/FilePlayer） | 统一播放器生命周期管理 |
| **状态管理** | AppearanceManager | 角色外观状态机 |
| **编辑距离算法** | TagValidator | 标签拼写修正 |

---

## 6. 代码规范

### 6.1 命名规范

| 类型 | 规则 | 示例 |
|------|------|------|
| 类名 | PascalCase | CharacterWidget, AppController |
| 成员变量 | m_前缀 + camelCase | m_llmService, m_visibleRect |
| 函数名 | camelCase | askDeepSeek, calculateVisibleRect |
| 信号 | camelCase | sentencesReady, characterPathChanged |
| 结构体 | PascalCase | SentenceText |

### 6.2 信号命名约定

- 已发生事件：`xxxReady` / `xxxChanged` / `xxxFinished`
- 错误事件：`xxxErrorSignal`
- 用户动作：`userXxx`

---

## 7. 技术债务与优化项

### 7.1 已修复问题

| 编号 | 问题 | 修复方案 | 状态 |
|------|------|----------|------|
| IC-001 | 服务定位器反模式 | 构造函数注入apiKey | ✅ 已修复 |
| RC-001~RC-011 | 代码冗余与死代码清理 | 逐个清理 | ✅ 已修复 |
| TD-005 | TTS为模拟实现 | 接入GPT-SoVITS API | ✅ 已修复 |
| TD-010 | AI记忆系统未实现 | 实现MemoryManager | ✅ 已修复 |
| TD-009 | 系统提示词硬编码 | 外部化prompt.txt | ✅ 已修复 |
| TD-018 | 流式PCM解析丢失数据 | readyRead跳过首个WAV头后透传裸PCM | ✅ 已修复 |
| TD-019 | StreamPlayer野指针崩溃 | m_buffer初始化nullptr + signals去重 | ✅ 已修复 |
| TD-020 | 播放完成信号不触发 | setSynthesisDone+IdleState+兜底定时器 | ✅ 已修复 |
| TD-021 | 播放吞开头 | startPlayer预填充PCM再启动QAudioSink | ✅ 已修复 |
| TD-022 | FilePlayer写入位置错误 | pushData先seek到末尾再write | ✅ 已修复 |

### 7.2 待优化项

| 编号 | 问题 | 位置 | 说明 | 状态 |
|------|------|------|------|------|
| A-001 | AppController职责过重 | appcontroller.cpp | 持有全部模块实例 | ⚠️ 可接受 |
| C-003 | 内联lambda过多 | appcontroller.cpp | 难以测试和复用 | ⚠️ 待优化 |
| C-004 | LLMService头文件依赖MemoryManager | llmservice.h | 增加编译依赖链 | ❌ 待修复 |
| RC-012 | AnchorManager析构未清理 | anchormanager.cpp | 内存泄漏风险 | ❌ 待修复 |
| TD-023 | TTSService直接new具体播放器 | ttsservice.cpp | 未通过工厂/IPcmPlayer静态方法创建，抽象不彻底 | ⚠️ 待优化 |

### 7.3 架构演进规划

| 阶段 | 版本 | 内容 |
|------|------|------|
| 当前 | v0.6.0 | 流式TTS播放、IPcmPlayer抽象、播放完成检测、预填充防吞开头 |
| 中期 | v0.7.0 | 播放器工厂模式重构、设置界面完善、时间驱动服装切换 |
| 远期 | v0.8.0+ | 架构改进、独立存档、插件化 |

---

## 8. 扩展点

| 扩展方向 | 接入方式 |
|----------|----------|
| 新LLM模型 | 继承/替换 LLMService，保持 sentencesReady 信号接口 |
| 新TTS引擎 | 实现 ITTSProvider 接口，流式需发 pcmDataReady 信号 |
| 新播放器类型 | 实现 IPcmPlayer 接口，TTSService 通过工厂创建（待重构） |
| 新服装/表情 | 添加资源文件，AppearanceManager自动支持 |
| 设置界面 | 连接 ConfigManager 的setter方法 |
| 新交互方式 | 在CharacterWidget中添加新信号，连接到AppController |
| 插件系统 | AppController中注册新模块，通过信号槽通信 |
