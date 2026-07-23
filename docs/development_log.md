# 开发记录

> 项目：Windows_AI 桌面看板娘（桌宠）
> 最后更新：2026-07-20

---

## 提交历史

| 哈希 | 日期 | 类型 | 描述 | 关键变更 |
|------|------|------|------|----------|
| `7dd8ce4` | 2026-07-16 | feat | 添加句子解析、TTS服务和外观管理功能 | 新增 AppController/TTSService/AppearanceManager/SentenceText，LLM多标签解析升级 |
| `88df6b2` | 2026-07-15 | refactor | 重构项目资源与应用架构 | 统一图片命名，BubbleWidget解耦attachTo，CharacterWidget瘦身 |
| `08a4808` | 2026-07-15 | docs | 添加GPT-SoVITS API接口文档 | 新增TTS接口文档 |
| `e296241` | 2026-07-14 | docs | 添加开发日志、路线图、架构设计文档与技术快照 | docs目录建立 |
| `c9f1f5f` | 2026-07-14 | refactor | 重构AI交互逻辑，抽离LLM服务模块 | 新增 LLMService 类，CharacterWidget通过信号槽通信 |
| `4e3126e` | 2026-07-14 | refactor | 添加立绘有效区域计算与气泡位置修正 | 新增 calculateVisibleRect() |
| `57b3251` | 2026-07-14 | build | 新增.gitignore文件 | 添加.gitignore |
| `cd2ae4e` | 2026-07-14 | chore | 清理构建目录临时文件 | 清理git索引 |
| `d654a9c` | 2026-07-13 | refactor | 重构主窗口部件并添加对话气泡组件 | 新增 BubbleWidget |
| `8381be2` | 2026-07-13 | feat | 添加浮动角色部件与DeepSeek AI聊天支持 | 可拖拽窗口+AI对话 |
| `0a3c5b7` | 2026-07-13 | feat | 集成DeepSeek AI网络请求功能 | QNetworkAccessManager |
| `d93db33` | 2026-07-13 | init | 简单demo窗口 | 项目初始化 |

---

## 里程碑

### M1: 项目初始化
- 日期：2026-07-13
- 提交：`d93db33`
- 内容：创建基础窗口框架，配置CMake构建环境

### M2: AI集成
- 日期：2026-07-13
- 提交：`0a3c5b7` → `8381be2`
- 内容：集成DeepSeek API，实现AI聊天功能，浮动角色窗口

### M3: UI组件化
- 日期：2026-07-13
- 提交：`d654a9c`
- 内容：添加BubbleWidget气泡组件，实现打字机特效

### M4: 交互优化
- 日期：2026-07-14
- 提交：`4e3126e` → `c9f1f5f`
- 内容：立绘有效区域计算，抽离LLMService模块

### M5: 架构重构
- 日期：2026-07-15
- 提交：`88df6b2`
- 内容：统一图片命名规范，BubbleWidget解耦（attachTo+eventFilter），CharacterWidget瘦身

### M6: 多标签协议 & TTS框架
- 日期：2026-07-16
- 提交：`7dd8ce4`
- 内容：
  - 新增 **AppController**（应用中枢，信号调度）
  - 新增 **AppearanceManager**（四维状态管理，立绘切换）
  - 新增 **TTSService**（生产者-消费者队列，模拟TTS）
  - 新增 **SentenceText** 数据结构
  - LLMService升级：多标签协议解析，多句拆分
  - 系统指令定义完整协议规范

### M7: 位置锚点系统 & Bug修复
- 日期：2026-07-16
- 内容：
  - 新增 **AnchorManager**（位置锚点管理，统一位置跟随）
  - 新增 **AnchorStrategy**（锚点位置枚举与配置结构）
  - 新增 **ChatWidget**（聊天输入窗口）
  - 修复气泡首次显示位置未初始化问题（bubbleShown信号）
  - 修复立绘切换时位置未更新问题（characterPathChanged信号连接）
  - 移除 BubbleWidget/ChatWidget 中重复的位置跟随逻辑
  - 实现 HeadRight 锚点策略
  - 修复 LLMService 构造函数中 API Key 未赋值问题（m_apiKey = apiKey）
  - 修复 IC-001 服务定位器反模式（LLMService改为构造函数注入apiKey）

### M8: 配置文件持久化
- 日期：2026-07-17
- 内容：
  - ConfigManager实现配置文件读写（JSON格式）
  - 配置文件路径：`app_data/config/setting.json`
  - 支持API Key和GPT-SoVITS服务地址的持久化
  - 首次启动自动创建配置文件，使用默认值

### M9: AI状态同步机制 & 提示词外部化（已实现）
- 日期：2026-07-17
- 内容：
  - LLMService实现状态提供者模式：`registerStateProvider()` 回调接口，`askDeepSeek()` 中动态追加状态上下文
  - LLMService实现提示词外部化：`initializePromptFile()` 首次启动自动释放默认prompt.txt，`loadSystemPrompt()` 从文件加载提示词
  - AppearanceManager新增状态描述方法：`getCurrentStateDescription()`、`getDistance()`、`getClothing()`、`getBlush()`、`getEmotion()`
  - AppController连接状态提供者：`m_llmService->registerStateProvider([this](){return m_appearance->getCurrentStateDescription();})`
  - 资源文件新增默认提示词路径：`image/default_config/prompt.txt`
  - 修复DeepSeek API请求格式错误：系统提示词包装为对象而非字符串

### M10: Git分支管理配置（已实现）
- 日期：2026-07-18
- 内容：
  - 创建 `docs` 分支（代码+文档，本地开发）
  - `main` 分支移除docs跟踪（仅代码，用于推送GitHub）
  - 更新 `.gitignore` 和 `.gitattributes` 配置
  - 清理已修复的技术债务文档标记

### M11: 设置界面实现 & 代码清理（已实现）
- 日期：2026-07-19
- 内容：
  - 新增 SettingsWidget 设置界面（API Key配置页）
  - 右键菜单"设置"入口已连接到SettingsWidget
  - 移除 CharacterWidget 中无用的 userChat 信号
  - 移除 AppController 中注释的自动问候代码
  - 简化Git分支管理（docs目录重新加入追踪）

### M12: AI记忆系统（已实现）
- 日期：2026-07-19
- 内容：
  - 新增 **MemoryManager**（AI记忆系统），使用SQLite数据库存储对话历史
  - 新增 **HistoryTurn** 数据结构，定义对话历史模型
  - LLMService::askDeepSeek 新增 historyQA 参数，支持传入对话历史上下文
  - sentenceReady 信号新增 rawReply 参数，用于保存原始回复
  - AppController 集成 MemoryManager，实现对话历史的保存与加载（默认15轮短期记忆）
  - 数据库路径：`app_data/memory/QianDaoMoZi_memory.db`
  - 数据库结构：chat_history表（id, timestamp, user_input, raw_reply, parsed_json）

### M13: 设置界面完善（记忆管理页）
- 日期：2026-07-22
- 内容：
  - SettingsWidget新增记忆管理页，支持历史记录查看、分页浏览、单条删除、清空全部
  - 历史记录删除需要API Key验证，防止误删
  - HistoryTurn数据结构新增 id 和 timestamp 字段
  - MemoryManager新增分页查询方法 getHistoryTurn(offset, limit)、计数方法 getTotalHistoryCount()、删除方法 deleteTurnByID() 和 clearAllHistory()
  - AppController中添加 setMemoryManager() 和 setMemoryLength() 调用，确保SettingsWidget正确获取MemoryManager实例

### M14: 代码修复与文档更新
- 日期：2026-07-22~2026-07-23
- 内容：
  - 修复ConfigManager配置文件损坏问题：配置缺失时使用默认值而非返回false
  - 修复SettingsWidget空指针访问问题：四个方法添加空指针检查后的return语句
  - 更新README.md：添加安装说明、使用指南、API参考、已知限制、故障排除章节
  - 更新ARCHITECTURE.md：补充MemoryManager、SettingsWidget完整描述，更新模块关系图

### M15: 架构审查与改进建议
- 日期：2026-07-23
- 内容：
  - **职责越界问题**：识别出4个职责越界问题（AppController、SettingsWidget、LLMService、MemoryManager）
  - **过分耦合问题**：识别出6个耦合问题（直接依赖、双向依赖、隐式耦合、头文件依赖、初始化顺序、循环包含）
  - **代码质量问题**：识别出6个代码质量问题（内存泄漏、魔法数字、硬编码路径、重复日志、未使用代码、注释不完整）
  - 更新ARCHITECTURE.md：新增第9章「架构问题分析」和第10章「改进建议」，包含短期、中期、长期改进计划

### M16: 用户画像与长期记忆系统（已实现）
- 日期：2026-07-23
- 内容：
  - MemoryManager新增用户画像管理：`upsertUserProfile()`、`getActiveUserProfiles()`、`deleteUserProfile()`
  - MemoryManager新增长期记忆摘要管理：`addLongTermSummary()`、`getLatestSummaries()`、`deleteLongTermSummary()`
  - MemoryManager新增置信度衰减机制：`scanAndApplyProfileDecay()`（三级半衰期：Tier1=-0.8/天，Tier2=-5.0/天，Tier3=-25.0/天）
  - MemoryManager新增未摘要对话查询：`getUnsummarizedTurns()`
  - LLMService新增记忆提取功能：`extractMemoryAsync()`（异步调用LLM分析对话，提取用户画像和长期摘要）
  - LLMService新增MemoryManager依赖：`setMemoryManager()`
  - AppController集成记忆提取流程：`handleMakoReply()`中判断未摘要对话数，触发后台记忆提取；`memoryExtractionReady`信号处理中写入用户画像和长期摘要
  - LLMService::askDeepSeek()新增用户画像和长期摘要注入：将活跃画像和最新摘要追加到系统提示词
  - 数据库v2升级：新增user_profile表（含索引）和long_term_summary表（含索引）
  - AppController启动时调用`scanAndApplyProfileDecay()`进行画像衰减扫描

---

## 技术债务

| 编号 | 描述 | 状态 | 优先级 | 引入版本 |
|------|------|------|--------|----------|
| TD-001 | API Key硬编码在ConfigManager构造函数中 | ✅ 已修复（配置文件持久化） | 高 | v0.1.0 |
| TD-002 | 立绘路径硬编码（已部分解决，AppearanceManager统一生成） | ✅ 已解决 | 高 | v0.1.0 |
| TD-003 | BubbleWidget位置计算硬编码（已解决，attachTo解耦） | ✅ 已解决 | 中 | v0.1.0 |
| TD-004 | 立绘切换功能未实现（已实现，AppearanceManager） | ✅ 已解决 | 高 | v0.1.0 |
| TD-005 | TTS为模拟实现，未接入真实引擎 | 待修复 | 高 | v0.2.0 |
| TD-006 | 用户输入入口缺失（硬编码测试文本） | ✅ 已修复（ChatWidget实现，移除userChat死信号） | 高 | v0.2.0 |
| TD-007 | characterwidget.cpp大量注释代码残留 | ✅ 已清理 | 中 | v0.2.0 |
| TD-008 | appcontroller.cpp重复include头文件 | ✅ 已修复 | 低 | v0.2.0 |
| TD-009 | 系统提示词硬编码在LLMService源码中 | ✅ 已修复（提示词外部化，prompt.txt文件） | 中 | v0.2.0 |
| TD-010 | AI记忆系统未实现 | ✅ 已实现（MemoryManager，SQLite短期记忆） | 高 | v0.2.4 |
| TD-011 | 对话历史查看界面缺失 | ✅ 已实现（SettingsWidget记忆管理页） | 中 | v0.4.0 |
| TD-012 | 长期记忆（重要事件摘要）未实现 | 待实现 | 中 | v0.4.0 |
| TD-013 | AppController职责过重（上帝对象） | 待重构 | 高 | v0.2.0 |
| TD-014 | AnchorManager内存泄漏风险 | 待修复 | 高 | v0.2.1 |
| TD-015 | LLMService头文件依赖MemoryManager | 待优化 | 中 | v0.2.4 |
| TD-016 | AppController中魔法数字SUMMARY_THRESHOLD | ✅ 非问题（设计意图：摘要轮数=短期记忆轮数） | 低 | v0.2.4 |

---

## 架构演进记录

| 阶段 | 架构模式 | 核心模块 | 特点 |
|------|----------|----------|------|
| v0.1.0 | 单体式 | CharacterWidget（大而全） | LLM/Bubble/Image全部内嵌 |
| v0.1.5 | 初步解耦 | CharacterWidget + LLMService | 网络层抽离，信号槽通信 |
| v0.2.0 | 中央控制器 | AppController + 6个独立模块 | 中枢调度，模块独立，多标签协议 |
| v0.2.1 | 位置锚点系统 | AnchorManager + ChatWidget | 统一位置跟随，聊天输入窗口 |
| v0.2.2 | 配置持久化 | ConfigManager（JSON文件） | API Key/TTS地址持久化，首次启动自动创建配置 |
| v0.2.3 | 状态同步 & 提示词外部化 | LLMService + AppearanceManager | 状态提供者模式（动态追加状态），提示词外部化（prompt.txt），DeepSeek API格式修复 |
| v0.2.4 | AI记忆系统 | MemoryManager + HistoryTurn | SQLite数据库存储对话历史，短期记忆查询（默认15轮），LLM对话上下文注入，跨会话记忆支持 |
| v0.2.5 | 设置界面完善 | SettingsWidget + MemoryManager | 记忆管理页实现（历史记录查看/删除/清空），分页浏览，API Key验证保护，配置文件损坏降级处理，代码健壮性提升 |
| v0.2.6 | 用户画像与长期记忆系统 | MemoryManager + LLMService | 用户画像管理（置信度衰减三级半衰期）、长期记忆摘要（LLM自动提取）、记忆提取异步流程、数据库v2升级（user_profile/long_term_summary表）、画像和摘要注入到LLM提示词 |

---

## 修复记录

| 日期 | 问题描述 | 修复方式 | 提交 |
|------|----------|----------|------|
| 2026-07-14 | 气泡位置偏移，跟随立绘不准确 | 实现 calculateVisibleRect() 计算立绘有效区域 | `4e3126e` |
| 2026-07-14 | AI交互逻辑与UI强耦合 | 抽离 LLMService 模块，使用信号槽通信 | `c9f1f5f` |
| 2026-07-15 | 图片命名混乱，不利于程序控制 | 统一为7种emotion规范命名 | `88df6b2` |
| 2026-07-15 | BubbleWidget与CharacterWidget紧耦合 | 实现attachTo()+eventFilter解耦 | `88df6b2` |
| 2026-07-16 | 立绘切换逻辑缺失 | 新增AppearanceManager四维管理状态 | `7dd8ce4` |
| 2026-07-16 | 单句回复限制，无法多句对话 | LLMService多句拆分+SentenceText结构 | `7dd8ce4` |
| 2026-07-16 | 气泡首次显示位置未初始化 | 新增bubbleShown信号，连接到updateAllAnchors | v0.2.1 |
| 2026-07-16 | 立绘切换时位置未更新 | 连接characterPathChanged到updateAllAnchors | v0.2.1 |
| 2026-07-16 | 位置跟随逻辑重复 | 移除BubbleWidget/ChatWidget的attachTo/eventFilter/updatePosition | v0.2.1 |
| 2026-07-16 | HeadRight锚点未实现 | 在calculatePosition中补全HeadRight分支 | v0.2.1 |
| 2026-07-17 | API Key硬编码 | ConfigManager实现JSON配置文件读写，支持API Key和TTS服务地址持久化 | v0.2.2 |
| 2026-07-17 | AI状态同步机制缺失 | LLMService实现状态提供者模式，AppearanceManager新增状态描述方法，动态追加状态到提示词 | v0.2.3 |
| 2026-07-17 | 提示词外部化框架缺失 | LLMService实现initializePromptFile()和loadSystemPrompt()，首次启动自动释放默认提示词 | v0.2.3 |
| 2026-07-22 | SettingsWidget数据库获取失败 | AppController中添加setMemoryManager()调用，确保SettingsWidget正确获取MemoryManager实例 | v0.2.5 |
| 2026-07-22 | SettingsWidget空指针解引用 | 在refreshHistoryTurnList、loadHistoryPage、on_btn_deleteSelectedMemory_clicked、on_btn_claenAllMemory_clicked四个方法中添加空指针检查后的return语句 | v0.2.5 |
| 2026-07-22 | ConfigManager配置文件损坏 | 修改loadSetting()方法，配置缺失时使用默认值并继续执行，而非返回false | v0.2.5 |

---

## 代码审查记录

| 日期 | 审查内容 | 发现问题 | 处理方式 |
|------|----------|----------|----------|
| 2026-07-14 | 仓库清理 | 大量构建产物被追踪 | 添加.gitignore，清理git索引 |
| 2026-07-15 | 资源命名 | 图片命名不统一 | 批量重命名为规范格式 | - |
| 2026-07-16 | 架构评审 | 模块职责清晰，信号槽连接合理 | 通过 |
