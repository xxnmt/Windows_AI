# 【项目技术快照】

> 项目：Windows_AI 桌面看板娘（桌宠）
> 日期：2026-07-16
> 版本：v0.2.0
> 状态：开发中

---

## 1. 核心类与成员

### AppController（新增·应用中枢）

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_character` | CharacterWidget*，角色组件 |
| | `m_bubble` | BubbleWidget*，气泡组件 |
| | `m_llmService` | LLMService*，AI服务 |
| | `m_appearance` | AppearanceManager*，外观管理 |
| | `m_ttsService` | TTSService*，TTS服务 |
| **函数签名** | `void startApp()` | 启动应用，显示角色并触发首次对话 |
| | `void handleMakoReply(const QList<SentenceText>& sentences)` | 处理AI回复，交给TTS队列 |
| | `void handleSystemError(const QString& errorMsg)` | 统一错误处理 |
| | `void onPlayAudioAction(const QString& zhText, const QMap<QString,QString>& tags)` | 播放同步：更新气泡+立绘 |

### CharacterWidget

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `dragPosition` | QPoint，鼠标拖拽偏移 |
| | `imageLabel` | QLabel*，立绘图片标签 |
| | `m_visibleRect` | QRect，立绘有效像素区域 |
| **函数签名** | `void updatePath(const QString& imagePath)` | 切换立绘图片 |
| | `QPoint getBubbleAnchorPos() const` | 获取气泡锚点坐标 |
| | `QRect calculateVisibleRect(const QPixmap& pixmap)` | 计算立绘有效区域 |
| **信号** | `void userChat(const QString& input)` | 用户发起聊天 |
| | `void characterMoved()` | 角色位置移动 |

> ⚠️ 原LLMService/BubbleWidget成员已从CharacterWidget中移除，改由AppController统一管理。

### AppearanceManager（新增·外观管理）

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
| **函数签名** | `void askDeepSeek(const QString& userInput)` | 发起DeepSeek请求（系统指令含多标签协议） |
| | `void onReplyFinished(QNetworkReply* reply)` | 解析回复，拆分为多句SentenceText |
| **信号** | `void sentenceReady(const QList<SentenceText>& sentence)` | 句子解析完成 |
| | `void internetErrorSignal(const QString& errorMessage)` | 网络错误 |

### TTSService（新增·TTS语音服务）

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_ttsQueue` | QQueue\<SentenceText\>，合成队列 |
| | `m_playQueue` | QQueue\<SentenceText\>，播放队列 |
| | `m_isSynthesizing` | bool，合成状态锁 |
| | `m_isPlaying` | bool，播放状态锁 |
| | `m_currentSynthesisSentence` | SentenceText，当前合成句 |
| | `m_currentPlaySentence` | SentenceText，当前播放句 |
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
| | `m_master` | QWidget*，跟踪的宿主组件 |
| | `m_positionProvider` | std::function\<QPoint()\>，位置回调 |
| **函数签名** | `void showMessage(const QString& text)` | 显示气泡并启动打字机 |
| | `void attachTo(QWidget* master, std::function<QPoint()> positionProvider)` | 绑定宿主+位置计算 |
| | `void typeWriteEffect()` | 打字机特效 |
| | `bool eventFilter(QObject* watched, QEvent* event)` | 监听宿主移动/大小变化 |

### ConfigManager

| 类型 | 名称 | 说明 |
|------|------|------|
| **模式** | 单例模式 | - |
| **私有变量** | `m_apiKey` | QString，DeepSeek API Key |
| **函数签名** | `static ConfigManager& instance()` | 获取单例 |
| | `QString getApiKey() const` | 获取API Key |
| | `void setApiKey(const QString& apiKey)` | 设置API Key |

### SentenceText（新增·数据结构）

```cpp
struct SentenceText {
    QString zhText;           // 中文文本（气泡显示）
    QString jaText;           // 日文文本（TTS合成）
    QMap<QString,QString> rawTags;  // 多维状态标签
};
```

---

## 2. 关键通信逻辑

### AI对话完整流程

```
用户输入 → userChat信号 → LLMService::askDeepSeek
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
| LLMService | `sentenceReady(sentences)` | AppController | `handleMakoReply()` |
| LLMService | `internetErrorSignal(msg)` | AppController | `handleSystemError()` |
| TTSService | `playAudioAction(zhText, tags)` | AppController | `onPlayAudioAction()` |
| AppearanceManager | `characterPathChanged(path)` | CharacterWidget | `updatePath()` |
| CharacterWidget | `characterMoved()` | （BubbleWidget通过eventFilter跟随） | - |

### 多标签协议解析（LLMService）

```cpp
// 正则拆分多句
QRegularExpression sentenceRegex("((?:\\[[^\\]]+\\])+)([^\\[]+)");
// 每句中匹配标签
QRegularExpression tagRegex("\\[([a-zA-Z0-9_]+):([^\\]]+)\\]");
```

### 立绘路径生成（AppearanceManager）

```cpp
QString(":/image/%1/%2/%3/%4.png")
    .arg(m_distance)   // far / closer
    .arg(m_clothing)   // pajama / schoolUniform / ...
    .arg(m_blush)      // unblushing / blushing
    .arg(m_emotion);   // happyIdle / happyMore / ...
```

### 打字机特效（BubbleWidget）

```cpp
void BubbleWidget::typeWriteEffect() {
    if(m_idex < m_text.length()){
        m_idex++;
        ui->label_text->setText(m_text.left(m_idex));
    } else {
        m_timer->stop();
    }
}
```

---

## 3. 现有痛点/未解逻辑

### 模拟/占位功能

| 位置 | 说明 |
|------|------|
| TTSService | TTS为模拟实现（QTimer），未接入真实语音合成引擎 |
| AppController::startApp() | 首次对话为硬编码测试文本，缺少用户输入入口 |
| ConfigManager::ConfigManager() | API Key硬编码，未从文件/环境加载 |

### 待开发功能

- [ ] 真实TTS引擎接入（GPT-SoVITS）
- [ ] 用户输入界面（输入框/托盘菜单）
- [ ] 设置界面（API Key配置、音量、速度等）
- [ ] 配置文件持久化
- [ ] 立绘切换过渡动画
- [ ] 角色点击交互

### 已知代码问题

| 位置 | 问题描述 |
|------|----------|
| appcontroller.cpp | 头文件重复include（`#include "appcontroller.h"` 出现两次） |
| characterwidget.cpp | 大量注释代码残留（旧的LLM/Bubble逻辑） |
| ttsservice.cpp | 仅模拟实现，无真实音频输出 |

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
├── appcontroller.h/cpp      # 应用中枢（新增）
├── sentencedata.h           # 句子数据结构（新增）
├── appearancemanager.h/cpp  # 外观管理器（新增）
├── ttsservice.h/cpp         # TTS语音服务（新增）
├── llmservice.h/cpp         # AI服务模块（升级：多标签+多句拆分）
├── configmanager.h/cpp      # 配置管理器（单例）
├── characterwidget.h/cpp    # 角色主组件（瘦身：仅UI+拖拽）
├── bubblewidget.h/cpp/ui    # 气泡组件（升级：attachTo解耦）
└── image/                   # 立绘资源目录
    ├── closer/              # 近景
    │   ├── pajama/
    │   │   ├── blushing/
    │   │   │   ├── happyIdle.png ~ conscientious.png (7张)
    │   │   └── unblushing/  (7张)
    │   ├── schoolUniform/
    │   ├── schoolUniformWithoutCap/
    │   └── schoolUniformWithoutCoat/
    └── far/                 # 远景
        ├── pajama/
        ├── schoolUniform/
        ├── schoolUniformWithoutCap/
        └── schoolUniformWithoutCoat/
```

### 图片命名规范

| 序号 | 文件名 | emotion值 | 含义 |
|------|--------|-----------|------|
| 1 | happyIdle.png | happyIdle | 开心空闲 |
| 2 | happyMore.png | happyMore | 更开心 |
| 3 | amazing.png | amazing | 惊讶 |
| 4 | loving.png | loving | 爱慕 |
| 5 | caring.png | caring | 关心 |
| 6 | sad.png | sad | 伤心 |
| 7 | conscientious.png | conscientious | 认真 |

---

## 5. CMake依赖

| 组件 | 版本 |
|------|------|
| Qt6 | 6.5+ |
| Qt::Core | - |
| Qt::Widgets | - |
| Qt::Network | - |
| CMake | 3.19+ |

---

## 6. 模块关系图

```
┌──────────────────────────────────────────────────────────────────┐
│                         AppController                             │
│                    （应用中枢 / 信号调度中心）                       │
│                                                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────...    │
│  │  Character  │  │   Bubble    │  │  LLMService │  │  TTS...   │
│  │   Widget    │  │   Widget    │  │             │  │            │
│  │  立绘/拖拽   │  │  气泡/打字机 │  │  AI对话/解析  │  │  语音服务  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬... │
│         │ eventFilter      │                  │                  │
│         │ (attachTo)       │                  │                  │
│         ▼                  │                  ▼                  │
│  ┌─────────────┐           │          ┌─────────────┐         │
│  │  Appearance │           │          │  ConfigM-   │         │
│  │   Manager   │           │          │   anager    │         │
│  │ 四维状态管理 │           │          │  单例配置    │         │
│  └─────────────┘           │          └─────────────┘         │
│                              │                                  │
│         SentenceText 数据结构（贯穿 LLM → TTS → UI）            │
└──────────────────────────────────────────────────────────────────┘
```

---

## 7. 近期变更

| 日期 | 变更内容 | 提交 |
|------|----------|------|
| 2026-07-16 | 新增句子解析、TTS服务、外观管理、AppController中枢 | `7dd8ce4` |
| 2026-07-15 | 重构资源架构，统一图片命名，BubbleWidget解耦 | `88df6b2` |
| 2026-07-15 | 统一图片命名为7种emotion规范格式 | - |
| 2026-07-15 | 更新image.qrc匹配新资源路径 | - |
| 2026-07-14 | 创建docs目录，整理项目文档 | `e296241` |
| 2026-07-14 | 抽离LLMService模块，引入ConfigManager单例 | `c9f1f5f` |
