# 【项目技术快照】

> 项目：Windows_AI 桌面看板娘（桌宠）
> 日期：2026-07-15
> 状态：开发中

---

## 1. 核心类与成员

### CharacterWidget

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `dragPosition` | QPoint，鼠标拖拽偏移 |
| | `imageLabel` | QLabel*，立绘图片标签 |
| | `m_llmService` | LLMService*，AI服务模块 |
| | `speakBubble` | BubbleWidget*，说话气泡组件 |
| | `m_visibleRect` | QRect，立绘有效像素区域 |
| **函数签名** | `void onMakoReplyReady(const QString& cleanText, const QString& emotion)` | 处理AI回复 |
| | `void onMakoError(const QString& errorMsg)` | 处理网络错误 |
| | `QRect calculateVisibleRect(const QPixmap& pixmap)` | 计算立绘有效区域 |

### LLMService

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_networkManager` | QNetworkAccessManager*，网络请求管理器 |
| **函数签名** | `void askDeepSeek(const QString& userInput)` | 发起DeepSeek请求 |
| | `void onReplyFinished(QNetworkReply* reply)` | 处理响应并解析[emotion]标签 |
| **信号** | `replyReady(const QString& cleanText, const QString& emotion)` | 回复就绪信号 |
| | `internetErrorSignal(const QString& errorMessage)` | 网络错误信号 |

### ConfigManager

| 类型 | 名称 | 说明 |
|------|------|------|
| **模式** | 单例模式 | - |
| **私有变量** | `m_apiKey` | QString，DeepSeek API Key |
| **函数签名** | `static ConfigManager& instance()` | 获取单例实例 |
| | `QString getApiKey() const` | 获取API Key |
| | `void setApiKey(const QString& apiKey)` | 设置API Key |

### BubbleWidget

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `m_timer` | QTimer*，打字机定时器 |
| | `m_text` | QString，完整文本 |
| | `m_idex` | int，当前显示位置 |
| **函数签名** | `void showMessage(const QString& text)` | 显示气泡并启动打字机 |
| | `void typeWriteEffect()` | 打字机特效槽函数 |

---

## 2. 关键通信逻辑

### 请求发起

**文件**：llmservice.cpp#L16-L50

```cpp
void LLMService::askDeepSeek(const QString &userInput) {
    QUrl url("https://api.deepseek.com/chat/completions");
    QString authHeader = "Bearer " + ConfigManager::instance().getApiKey();
    request.setRawHeader("Authorization", authHeader.toUtf8());
    // 构建JSON请求体，发送POST
    networkManager->post(request, postData);
}
```

### 回复处理与信号传递

**文件**：llmservice.cpp#L53-L109

```cpp
void LLMService::onReplyFinished(QNetworkReply *reply) {
    // 解析JSON -> 提取content -> 正则过滤[emotion]标签
    QRegularExpression regex("\\[(.*?)\\]");
    // ... 提取 emotion 和 cleanText
    replyReady(cleanText.trimmed(), emotion);  // 发送信号
}
```

### CharacterWidget接收信号

**文件**：characterwidget.cpp#L92-L107

```cpp
void CharacterWidget::onMakoReplyReady(const QString &cleanText, const QString &emotion) {
    if(speakBubble){
        QPoint bubblePos = this->pos() + m_visibleRect.topRight() + QPoint(5, 10);
        speakBubble->move(bubblePos);
        speakBubble->showMessage(cleanText);
    }
}
```

### 打字机特效

**文件**：bubblewidget.cpp#L49-L57

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

### 屏蔽功能

| 位置 | 说明 |
|------|------|
| llmservice.cpp#L83-L91 | 立绘切换逻辑被注释，原因：文件路径管理未做好 |
| characterwidget.h | `updateBubblePosition()` 函数声明被注释 |

### 强耦合逻辑

| 位置 | 问题描述 |
|------|----------|
| characterwidget.cpp#L83-L86 | BubbleWidget位置计算硬编码在mouseMoveEvent()中 |
| configmanager.cpp#L16 | API Key硬编码在构造函数中 |
| characterwidget.cpp#L24 | CharacterWidget直接创建并管理BubbleWidget生命周期 |
| characterwidget.cpp#L28 | 立绘路径硬编码 |

### 待开发功能

- [ ] TTS语音功能
- [ ] 设置界面
- [ ] 立绘切换系统（emotion标签→图片映射）

---

## 4. 项目文件结构

```
Windows_AI/
├── docs/                    # 文档目录
│   ├── project_tech_snapshot.md
│   ├── development_log.md
│   ├── ARCHITECTURE.md
│   └── ROADMAP.md
├── main.cpp                 # 入口函数
├── CMakeLists.txt           # 构建配置
├── image.qrc                # 资源文件（已更新）
├── llmservice.h/cpp         # AI服务模块（网络请求+解析）
├── configmanager.h/cpp      # 配置管理器（单例）
├── characterwidget.h/cpp    # 角色主组件（UI+交互）
├── bubblewidget.h/cpp/ui    # 气泡组件（UI文件）
└── image/                   # 立绘资源目录（已统一命名）
    ├── closer/              # 近景立绘
    │   ├── pajama/
    │   │   ├── blushing/    # 脸红状态
    │   │   │   ├── happyIdle.png
    │   │   │   ├── happyMore.png
    │   │   │   ├── amazing.png
    │   │   │   ├── loving.png
    │   │   │   ├── caring.png
    │   │   │   ├── sad.png
    │   │   │   └── conscientious.png
    │   │   └── unblushing/  # 常态
    │   │       └── (同上7个文件)
    │   ├── schoolUniform/
    │   │   ├── blushing/
    │   │   └── unblushing/
    │   ├── schoolUniformWithoutCap/
    │   │   ├── blushing/
    │   │   └── unblushing/
    │   └── schoolUniformWithoutCoat/
    │       ├── blushing/
    │       └── unblushing/
    └── far/                 # 远景立绘
        ├── pajama/
        ├── schoolUniform/
        ├── schoolUniformWithoutCap/
        └── schoolUniformWithoutCoat/
```

### 图片命名规范

| 序号 | 文件名 | 含义 |
|------|--------|------|
| 1 | happyIdle.png | 开心空闲 |
| 2 | happyMore.png | 更开心 |
| 3 | amazing.png | 惊讶 |
| 4 | loving.png | 爱慕 |
| 5 | caring.png | 关心 |
| 6 | sad.png | 伤心 |
| 7 | conscientious.png | 认真 |

---

## 5. CMake依赖

| 组件 | 版本 |
|------|------|
| Qt6 | 6.5+ |
| Qt::Core | - |
| Qt::Widgets | - |
| Qt::Network | - |

---

## 6. 模块关系图

```
CharacterWidget (UI层)
    │
    ├── speakBubble (BubbleWidget) - 气泡显示
    └── m_llmService (LLMService) - AI交互
            │
            └── ConfigManager (单例) - API Key配置
```

---

## 7. 近期变更

| 日期 | 变更内容 | 文件 |
|------|----------|------|
| 2026-07-15 | 统一图片命名为规范格式（happyIdle/happyMore/amazing/loving/caring/sad/conscientious） | image/ 目录下所有图片 |
| 2026-07-15 | 更新资源文件，匹配新图片路径 | image.qrc |
| 2026-07-15 | 更新默认立绘加载路径 | characterwidget.cpp#L28 |
| 2026-07-14 | 创建docs目录，整理项目文档 | docs/ |
| 2026-07-14 | 抽离LLMService模块，引入ConfigManager单例 | llmservice.h/cpp, configmanager.h/cpp |
