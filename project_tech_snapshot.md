# 【项目技术快照】

> 项目：Windows_AI 桌面看板娘（桌宠）
> 日期：2026-07-14
> 状态：开发中

---

## 1. 核心类与成员

### CharacterWidget

| 类型 | 名称 | 说明 |
|------|------|------|
| **私有变量** | `dragPosition` | QPoint，鼠标拖拽偏移 |
| | `imageLabel` | QLabel*，立绘图片标签 |
| | `networkManager` | QNetworkAccessManager*，网络请求管理器 |
| | `speakBubble` | BubbleWidget*，说话气泡组件 |
| | `m_visibleRect` | QRect，立绘有效像素区域 |
| **函数签名** | `void askDeepSeek(const QString& userInput)` | 发起DeepSeek请求 |
| | `void onReplyFinished(QNetworkReply* reply)` | 处理AI回复 |
| | `QRect calculateVisibleRect(const QPixmap& pixmap)` | 计算立绘有效区域 |

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

**文件**：characterwidget.cpp#L64-L97

```cpp
void CharacterWidget::askDeepSeek(const QString &userInput) {
    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer sk-8150d4f2c95644b39d35c9fae00baa81");
    
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "你叫千岛茉子，简称茉子，今年7岁,请暂时称呼我为欧尼酱";
    
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = userInput;
    
    QJsonArray messagesArray;
    messagesArray.append(systemMessage);
    messagesArray.append(userMessage);
    
    QJsonObject rootObj;
    rootObj["model"] = "deepseek-chat";
    rootObj["messages"] = messagesArray;
    rootObj["temperature"] = 0.7;
    
    QByteArray postData = QJsonDocument(rootObj).toJson();
    networkManager->post(request, postData);
}
```

### 回复处理

**文件**：characterwidget.cpp#L124-L178

```cpp
void CharacterWidget::onReplyFinished(QNetworkReply *reply) {
    if(reply->error() == QNetworkReply::NoError){
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject rootObj = jsonDoc.object();
        QJsonArray choices = rootObj["choices"].toArray();
        
        if(!choices.isEmpty()){
            QJsonObject messageObj = choices[0].toObject()["message"].toObject();
            QString replyText = messageObj["content"].toString();
            
            // 解析[emotion]标签 + 显示气泡
        }    
    }
    reply->deleteLater();
}
```

### [emotion]标签过滤

**文件**：characterwidget.cpp#L139-L150

```cpp
QRegularExpression regex("\\[(.*?)\\]");
QRegularExpressionMatch match = regex.match(replyText);

QString emotion = "idle";
QString cleanText = replyText;

if(match.hasMatch()){
    emotion = match.captured(1);
    cleanText.remove(match.captured(0));
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
| characterwidget.cpp#L154-L162 | 立绘切换逻辑被注释，原因：文件路径管理未做好 |
| characterwidget.h#L44 | `updateBubblePosition()` 函数声明存在但被注释 |

### 强耦合逻辑

| 位置 | 问题描述 |
|------|----------|
| characterwidget.cpp#L115-L118 | BubbleWidget位置计算硬编码在mouseMoveEvent()中 |
| characterwidget.cpp#L71 | DeepSeek API Key硬编码在代码中 |
| characterwidget.cpp#L25 | CharacterWidget直接创建并管理BubbleWidget生命周期，未解耦 |
| characterwidget.cpp#L29 | 立绘路径硬编码 |

### 待开发功能

- [ ] TTS语音功能
- [ ] 设置界面
- [ ] `configmanager.h/cpp` - 被CMake引用但代码缺失

---

## 4. 项目文件结构

```
Windows_AI/
├── main.cpp                    # 入口函数
├── CMakeLists.txt              # 构建配置
├── image.qrc                   # 资源文件
├── characterwidget.h/cpp       # 角色主组件
├── bubblewidget.h/cpp/ui       # 气泡组件（UI文件）
└── image/                      # 立绘资源目录
    ├── closer/                 # 近景立绘
    │   ├── pajama/             # 睡衣
    │   ├── schoolUniform/      # 校服
    │   ├── schoolUniformWithoutCat/   # 校服无猫
    │   └── schoolUniformWithoutCoat/  # 校服无外套
    └── far/                    # 远景立绘
        ├── pajama/
        ├── schoolUniform/
        ├── schoolUniformWithoutCat/
        └── schoolUniformWithoutCoat/
```

---

## 5. CMake依赖

| 组件 | 版本 |
|------|------|
| Qt6 | 6.5+ |
| Qt::Core | - |
| Qt::Widgets | - |
| Qt::Network | - |
