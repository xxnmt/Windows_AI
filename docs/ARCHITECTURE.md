# 架构设计文档

> 项目：Windows_AI 桌面看板娘（桌宠）
> 版本：v0.2.0
> 更新日期：2026-07-16

---

## 1. 架构概览

### 1.1 架构风格

采用 **中央控制器 + 模块化设计**，以 `AppController` 为信号调度中枢，各功能模块独立实现，通过信号槽解耦。

### 1.2 模块划分

| 层级 | 模块 | 职责 | 文件 |
|------|------|------|------|
| **中枢层** | AppController | 应用生命周期管理、信号调度、模块组装 | appcontroller.h/cpp |
| **UI层** | CharacterWidget | 立绘显示、鼠标拖拽、气泡锚点计算 | characterwidget.h/cpp |
| | BubbleWidget | 气泡显示、打字机特效、位置跟随 | bubblewidget.h/cpp/ui |
| **业务层** | LLMService | DeepSeek请求、多标签协议解析、句子拆分 | llmservice.h/cpp |
| | TTSService | 语音合成队列、播放队列、模拟双线程 | ttsservice.h/cpp |
| | AppearanceManager | 四维状态管理、立绘路径生成、换图触发 | appearancemanager.h/cpp |
| **数据层** | ConfigManager | API Key配置、单例模式 | configmanager.h/cpp |
| **数据结构** | SentenceText | 句子数据模型（中文/日文/标签） | sentencedata.h |

### 1.3 架构总览图

```
┌─────────────────────────────────────────────────────────────────────┐
│                        AppController                                │
│                    （应用中枢 / 信号调度中心）                          │
│                                                                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐     │
│  │ CharacterWidget │  │   BubbleWidget  │  │   LLMService    │     │
│  │  立绘/拖拽       │  │   气泡/打字机    │  │  AI对话/多句解析  │     │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘     │
│           │                    │                    │              │
│           │ eventFilter        │                    │              │
│           │ (attachTo)         │                    ▼              │
│  ┌────────▼────────┐           │            ┌─────────────┐       │
│  │AppearanceManager│           │            │ConfigManager│       │
│  │ 四维状态/换图    │           │            │  单例配置    │       │
│  └─────────────────┘           │            └─────────────┘       │
│                                 │                                  │
│  ┌──────────────────────────────▼───────────────────────────────┐  │
│  │                        TTSService                            │  │
│  │  合成队列(Producer)  →  播放队列(Consumer)                    │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  核心数据结构：SentenceText (zhText + jaText + rawTags)             │
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
// 用户输入 → LLM请求
connect(m_character, &CharacterWidget::userChat, m_llmService, &LLMService::askDeepSeek);

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
| `userChat(input)` | 用户发起聊天时 | QString input |
| `characterMoved()` | 窗口位置变化时 | - |

---

### 2.3 BubbleWidget

**职责**：对话气泡组件，支持打字机特效和位置跟随

**设计要点**：
- 独立的无边框顶层窗口（Qt::Tool）
- 通过 `attachTo()` 与宿主组件解耦绑定
- 使用 `std::function<QPoint()>` 回调获取位置
- 用 `eventFilter` 监听宿主移动/大小变化自动跟随

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `showMessage()` | 显示气泡并启动打字机 | QString text | - |
| `attachTo()` | 绑定宿主组件和位置回调 | QWidget* master, function\<QPoint()\> provider | - |
| `typeWriteEffect()` | 逐字显示文本（定时器触发） | - | - |
| `eventFilter()` | 监听宿主移动/大小变化 | QObject*, QEvent* | bool |
| `paintEvent()` | 绘制圆角气泡背景 | QPaintEvent* | - |

---

### 2.4 LLMService

**职责**：AI对话服务，负责DeepSeek请求和多标签协议解析

**设计要点**：
- 与UI完全解耦，纯业务逻辑
- 系统指令包含完整的多标签协议
- 将回复拆分为多句 SentenceText
- 每句独立解析 emotion/blush/distance/clothing/ja 标签

**关键方法**：
| 方法 | 作用 | 参数 | 返回值 |
|------|------|------|--------|
| `askDeepSeek()` | 发起DeepSeek API请求 | QString userInput | - |
| `onReplyFinished()` | 解析JSON→拆句→提取标签→发信号 | QNetworkReply* | - |

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
| `sentenceReady(sentences)` | 回复解析完成 | QList\<SentenceText\> |
| `internetErrorSignal(msg)` | 网络错误 | QString |

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

**职责**：全局配置管理（单例）

**设计要点**：
- Meyers单例，线程安全
- 删除拷贝构造和赋值操作
- 当前仅管理API Key（硬编码，待持久化）

---

## 3. 核心数据流

### 3.1 完整对话流程

```
1. 用户输入
   │
   ▼
2. CharacterWidget::userChat(input)  [信号]
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

### 3.2 气泡位置跟随机制

```
BubbleWidget::attachTo(character, positionProvider)
    │
    ├── 安装eventFilter到宿主
    └── 初始位置更新
    │
    ▼
宿主移动/大小变化 → eventFilter → updatePosition() → move(positionProvider())
```

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
| **事件过滤器** | BubbleWidget::eventFilter | 气泡位置跟随 |

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

| 优先级 | 项目 | 描述 | 当前状态 |
|--------|------|------|----------|
| 高 | TTS真实接入 | 接入GPT-SoVITS等真实TTS引擎 | 模拟实现 |
| 高 | 配置持久化 | API Key等配置保存到文件 | 硬编码 |
| 高 | 用户输入入口 | 添加输入框/托盘菜单 | 硬编码测试 |
| 中 | 代码清理 | 移除characterwidget中大量注释 | 待清理 |
| 中 | 重复include | appcontroller.cpp重复include | 待修复 |
| 中 | 错误重试 | LLM请求失败后自动重试 | 无 |
| 低 | 立绘过渡 | 换图时添加淡入淡出动画 | 无 |
| 低 | 资源缓存 | 立绘pixmap缓存机制 | 每次重新加载 |

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
