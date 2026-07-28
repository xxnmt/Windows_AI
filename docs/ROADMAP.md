# 开发路线图

> 项目：Windows_AI 桌面看板娘（桌宠）
> 版本：v0.5.0
> 更新日期：2026-07-29

---

## 项目愿景

打造一款功能完善、交互丰富的桌面看板娘应用，支持AI对话、立绘切换、语音交互、智能助手等核心功能。

## 项目定位

相比同类项目（如 LingChat），本项目专注于：
- **科学的记忆系统**：三层记忆架构（短期记忆+用户画像+长期摘要）+ 置信度衰减机制
- **系统化的状态管理**：四维立绘状态（emotion/blush/distance/clothing）设计 + 标签合法性校验
- **稳定的AI协议**：JSON输出格式 + response_format 强制JSON + TagValidator 编辑距离修正
- **轻量级高性能**：C++/Qt 方案，内存占用和性能优于 Electron/Tauri 方案

---

## 版本规划

### v0.1.0 - 基础版本
**状态**：✅ 已完成

- [x] 浮动角色窗口（无边框、透明背景、置顶）
- [x] 鼠标拖拽移动
- [x] DeepSeek AI对话集成
- [x] 对话气泡显示
- [x] 打字机特效
- [x] 立绘有效区域计算

### v0.2.0 - 架构重构 & 多标签协议
**状态**：✅ 已完成

- [x] AppController中央控制器
- [x] AppearanceManager四维状态管理
- [x] TTSService生产者-消费者框架（模拟实现）
- [x] SentenceText数据结构
- [x] LLMService多标签协议解析
- [x] 多句拆分支持
- [x] 统一图片命名规范（7种emotion）
- [x] BubbleWidget解耦（attachTo + eventFilter）
- [x] 立绘切换系统（emotion/blush/distance/clothing）
- [x] 脸红自动退热机制

### v0.3.0 - AI资源管理
**状态**：✅ 已完成

**功能清单**：
- [x] 配置文件持久化（app_data/config/setting.json）
  - [x] API Key存储与读取
  - [x] GPT-SoVITS服务地址配置
  - [ ] TTS参数配置（语速、温度、参考音频）
- [x] 系统提示词外部化（app_data/config/prompt.txt）
  - [x] 框架预留：LLMService预留 `loadSystemPrompt()` 方法
  - [x] 提示词从文件加载
  - [x] 支持动态追加当前状态（方案C）
- [x] AI状态同步机制（方案C：本地切换后在下轮对话同步给AI）
  - [x] 框架预留：LLMService状态提供者模式
  - [x] 框架预留：AppearanceManager状态描述方法
  - [x] 框架预留：AppController连接状态提供者
  - [x] 实际实现：动态构建包含状态的系统提示词
- [x] 设置界面（SettingsWidget）
  - [x] API配置页
  - [x] 记忆配置页（记忆长度、历史记录管理）

### v0.4.0 - AI记忆系统
**状态**：✅ 已完成

**功能清单**：
- [x] 分层记忆系统
  - [x] 短期记忆（最近N轮对话上下文，SQLite）✅ 已实现（默认15轮）
  - [x] 用户画像（置信度衰减机制，3级半衰期）✅ 已实现
  - [x] 长期记忆摘要（LLM自动提取重要事件）✅ 已实现
- [x] 对话历史查看界面 ✅ 已实现
  - [x] 历史记录列表（表格展示，分页浏览）
  - [x] 删除功能（单条删除、清空全部）
  - [ ] 导出功能
- [x] AI记忆写入与读取 ✅ 已实现
  - [x] 每次对话后写入历史
  - [x] 对话前读取短期记忆
  - [x] 用户画像和长期摘要注入LLM提示词
  - [x] 自动记忆提取（未摘要对话达到阈值时触发）

### v0.4.1 - TTS策略模式重构
**状态**：✅ 已完成

**功能清单**：
- [x] TTSService重构为策略模式（ITTSProvider抽象接口）
- [x] 新增ApiTTSProvider（GPT-SoVITS HTTP API接入）
- [x] 新增MockTTSProvider（模拟实现）
- [x] 双队列模型优化（合成队列→播放队列）
- [x] 集成Qt Multimedia音频播放
- [x] TTS模式配置（api/mock切换）
- [x] 错误处理（合成失败跳过，不阻塞队列）
- [x] 临时音频即用即删

### v0.5.0 - JSON协议 & 标签校验（当前版本）
**状态**：✅ 已完成

**功能清单**：
- [x] LLM输出格式从标签协议迁移到JSON协议
  - [x] 旧格式 `[emotion:xxx][blush:yyy]` → 新格式 `{"sentences": [...]}`
  - [x] 添加 `response_format: {type: "json_object"}` 强制JSON输出
  - [x] 更新系统提示词，要求JSON格式输出
- [x] 重构 `parseJsonReply()`：从正则拆分改为JSON对象解析
- [x] 新增TagValidator标签校验类
  - [x] 5级校验流程：精确匹配→大小写忽略→编辑距离→继承上一状态→使用默认值
  - [x] 实现Levenshtein编辑距离算法
  - [x] 编辑距离阈值根据标签长度动态调整
- [x] 数据库结构简化
  - [x] 删除 `chat_history` 表的 `parsed_json` 列
  - [x] `raw_reply` 直接存储完整JSON对象
- [x] 历史记忆注入优化
  - [x] 只注入纯文本内容，不注入标签格式
  - [x] 剥离 `[xxx:yyy]` 格式标签
- [x] 修复正则表达式语法错误

**技术债务清理**：
- [x] TD-017: LLM标签输出格式不稳定 → JSON协议+TagValidator校验
- [x] TD-017: 数据库冗余字段 → 删除parsed_json列

### v0.6.0 - 设置界面 & 交互增强
**状态**：📋 待开始

**功能清单**：
- [ ] 设置界面完善
  - [ ] 外观配置（气泡样式、窗口透明度）
  - [ ] TTS配置（语速、温度、模型切换）
  - [ ] 时间配置（自动服装切换）
  - [ ] 快捷键配置（全局快捷键）
- [ ] 时间驱动服装切换（白天校服 / 夜晚睡衣）
- [ ] 右键菜单扩展（服装/表情快捷切换）
- [ ] 系统托盘图标
- [ ] 历史记录导出功能

**技术任务**：
- [ ] 创建TimeManager时间管理器
- [ ] 创建ShortcutManager全局快捷键管理
- [ ] 修复AnchorManager内存泄漏（TD-014）
- [ ] 优化LLMService头文件依赖（TD-015）
- [ ] 添加错误重试机制（LLM请求失败自动重试）

**预计时间**：2-3周

### v0.7.0 - 架构改进 & 独立存档
**状态**：📋 待开始

**功能清单**：
- [ ] 架构改进
  - [ ] 拆分AppController职责（SignalRouter/MemoryCoordinator/ErrorHandler）
  - [ ] 消除LLMService头文件对MemoryManager的直接依赖
- [ ] 独立存档系统
  - [ ] 支持多个存档
  - [ ] 每个存档有独立的记忆和角色关系
  - [ ] 存档导入/导出
- [ ] 时间驱动主动对话
  - [ ] 到饭点提醒、睡前聊天
  - [ ] 日程提醒

**技术任务**：
- [ ] 拆分AppController职责
- [ ] 扩展MemoryManager支持多存档
- [ ] 创建SaveManager存档管理
- [ ] 创建ScheduleManager日程管理

**预计时间**：3-4周

### v0.8.0 - 角色自定义 & 剧本系统
**状态**：📋 待开始

**功能清单**：
- [ ] 角色自定义
  - [ ] 立绘导入
  - [ ] 角色设定配置
  - [ ] 语音模型配置
- [ ] 剧本系统
  - [ ] 剧本编辑器
  - [ ] 多角色对话支持
  - [ ] 羁绊系统
- [ ] 点击交互（不同部位触发不同回应）
- [ ] 立绘切换过渡动画（淡入淡出）

**技术任务**：
- [ ] 创建CharacterManager角色管理
- [ ] 创建ScriptEngine剧本引擎
- [ ] 立绘资源系统扩展

**预计时间**：4-5周

### v0.9.0 - 智能助手 & 性能优化
**状态**：📋 待开始

**功能清单**：
- [ ] 天气查询
- [ ] 立绘资源缓存
- [ ] TTS音频缓存
- [ ] 多角色支持
- [ ] 长期记忆增强（摘要重建机制）

**预计时间**：2-3周

---

## 技术债务清理计划

| 编号 | 债务 | 优先级 | 计划版本 | 状态 |
|------|------|--------|----------|------|
| TD-001 | API Key硬编码 | 高 | v0.3.0 | ✅ 已修复（配置文件持久化） |
| TD-005 | TTS为模拟实现 | 高 | v0.4.1 | ✅ 已修复（策略模式+GPT-SoVITS API接入完成） |
| TD-009 | 系统提示词硬编码 | 中 | v0.3.0 | ✅ 已修复（提示词外部化，prompt.txt文件） |
| TD-010 | AI状态同步机制未实现 | 高 | v0.3.0 | ✅ 已实现（状态提供者模式） |
| TD-011 | 记忆系统未实现 | 高 | v0.4.0 | ✅ 已实现（短期记忆+用户画像+长期摘要） |
| TD-012 | 长期记忆未实现 | 中 | v0.4.0 | ✅ 已实现（LLM自动提取摘要） |
| TD-016 | SUMMARY_THRESHOLD魔法数字 | 低 | v0.5.0 | ✅ 非问题（设计意图：摘要轮数=短期记忆轮数） |
| TD-017 | LLM标签输出格式不稳定 | 高 | v0.5.0 | ✅ 已修复（JSON协议+TagValidator校验） |
| TD-006 | 用户输入入口缺失 | 中 | v0.2.0 | ✅ 已修复（ChatWidget实现） |
| TD-002 | 立绘路径硬编码 | 中 | v0.2.0 | ✅ 已解决（AppearanceManager统一生成） |
| TD-003 | BubbleWidget位置计算硬编码 | 中 | v0.2.0 | ✅ 已解决（attachTo解耦） |
| TD-004 | 立绘切换功能未实现 | 高 | v0.2.0 | ✅ 已解决（AppearanceManager） |
| TD-007 | characterwidget大量注释代码残留 | 低 | v0.3.0 | ✅ 已清理 |
| TD-008 | appcontroller重复include | 低 | v0.3.0 | ✅ 已修复 |
| TD-013 | AppController职责过重（上帝对象） | 高 | v0.7.0 | ⚠️ 可接受（当前规模合适） |
| TD-014 | AnchorManager内存泄漏风险 | 高 | v0.6.0 | ❌ 待修复 |
| TD-015 | LLMService头文件依赖MemoryManager | 中 | v0.6.0 | ❌ 待优化 |

---

## 架构演进路线

### 阶段1：单体式（v0.1.0）
```
CharacterWidget（大而全）
    ├── LLM逻辑
    ├── BubbleWidget
    └── 立绘显示
```

### 阶段2：初步解耦（v0.1.5）
```
CharacterWidget
    ├── BubbleWidget
    └── LLMService（信号槽）
```

### 阶段3：中央控制器（v0.2.0）
```
AppController（信号调度中枢）
    ├── CharacterWidget
    ├── BubbleWidget
    ├── LLMService
    ├── TTSService
    ├── AppearanceManager
    └── ConfigManager
```

### 阶段4：AI资源管理（v0.3.0）
```
AppController（信号调度中枢）
    ├── CharacterWidget（立绘/右键菜单）
    ├── BubbleWidget（气泡/打字机）
    ├── ChatWidget（聊天输入框）
    ├── AnchorManager（位置锚点管理）
    ├── LLMService（AI对话/解析 → 状态提供者模式 + 提示词外部化）
    ├── TTSService（语音合成/播放）
    ├── AppearanceManager（四维状态 → 状态描述输出）
    └── ConfigManager（单例配置 → JSON文件持久化）
```

### 阶段5：AI记忆系统（v0.4.0）
```
AppController（信号调度中枢）
    ├── LLMService（AI对话 → JSON解析 + 记忆注入）
    │   ├── m_stateProvider → AppearanceManager.getCurrentStateDescription()
    │   ├── historyQA → MemoryManager.getHistoryTurn(15)
    │   ├── profiles → MemoryManager.getActiveUserProfiles()
    │   ├── summaries → MemoryManager.getLatestSummaries()
    │   └── extractMemoryAsync() 后台记忆提取
    └── MemoryManager（AI记忆系统 → SQLite数据库）
        ├── chat_history表 → 对话历史
        ├── user_profile表 → 用户画像
        └── long_term_summary表 → 长期摘要
```

### 阶段6：TTS策略模式（v0.4.1）
```
TTSService（策略模式）
    ├── m_ttsQueue（待合成）
    ├── ITTSProvider（抽象接口）
    │   ├── ApiTTSProvider（GPT-SoVITS HTTP API）
    │   └── MockTTSProvider（模拟实现）
    └── m_playQueue（待播放）
```

### 阶段7：JSON协议 & 标签校验（v0.5.0，当前）
```
LLMService（JSON协议）
    ├── response_format: {type: "json_object"} → 强制JSON输出
    ├── parseJsonReply() → JSON对象解析
    ├── TagValidator → 5级标签校验
    │   ├── 精确匹配
    │   ├── 大小写忽略
    │   ├── 编辑距离（Levenshtein算法）
    │   ├── 继承上一状态
    │   └── 使用默认值
    └── 历史记忆注入 → 纯文本（剥离标签格式）

数据库简化：
    chat_history表 → 仅保留 raw_reply（JSON对象），删除parsed_json列
```

### 阶段8：插件化（v0.7.0+）
```
Core
    ├── PluginManager
    ├── ConfigManager
    ├── EventBus
    └── AppController

Plugins
    ├── CharacterPlugin
    ├── LLMPlugin
    ├── TTSPlugin
    ├── WeatherPlugin
    └── SchedulePlugin
```

---

## 依赖规划

| 依赖 | 当前版本 | 计划升级 | 说明 |
|------|----------|----------|------|
| Qt | 6.5+ | 6.11+ | Qt Multimedia已用于TTS播放 |
| CMake | 3.19+ | 3.20+ | - |
| GPT-SoVITS | - | - | v0.4.1已接入TTS引擎 |
| deepseek-v4-flash | - | - | 当前LLM模型 |

---

## 风险评估

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|----------|
| DeepSeek API变更 | AI对话功能失效 | 中 | LLMService封装层，便于适配 |
| LLM输出格式不一致 | 标签解析失败 | 低 | response_format强制JSON + TagValidator校验 |
| TTS服务不可用 | 语音功能失效 | 中 | 支持Api/Mock双模式切换 |
| 立绘资源过大 | 打包体积增加 | 低 | 支持资源按需加载/外部资源 |
| 网络延迟 | 用户体验下降 | 中 | 加载状态提示，TTS预合成 |
| GPT-SoVITS部署复杂 | TTS接入周期长 | 中 | 保留Mock实现，逐步替换 |

---

## 当前进度总览

```
v0.1.0  ████████████████████ 100%  基础版本
v0.2.0  ████████████████████ 100%  架构重构 & 多标签协议
v0.3.0  ████████████████████ 100%  AI资源管理
v0.4.0  ████████████████████ 100%  AI记忆系统（三层记忆）
v0.4.1  ████████████████████ 100%  TTS策略模式重构
v0.5.0  ████████████████████ 100%  JSON协议 & 标签校验（当前）
v0.6.0  ░░░░░░░░░░░░░░░░░░░░   0%  设置界面 & 交互增强
v0.7.0  ░░░░░░░░░░░░░░░░░░░░   0%  架构改进 & 独立存档
v0.8.0  ░░░░░░░░░░░░░░░░░░░░   0%  角色自定义 & 剧本系统
v0.9.0  ░░░░░░░░░░░░░░░░░░░░   0%  智能助手 & 性能优化
```

---

## 近期任务（Next Up）

1. **代码质量提升**
   - 修复AnchorManager内存泄漏（TD-014）
   - 优化LLMService头文件依赖（TD-015）
   - 添加错误重试机制
   
2. **设置界面扩展**
   - TTS配置页（语速、温度、模型切换）
   - 外观配置（气泡样式、窗口透明度）
   - 时间配置（自动服装切换）
   - 快捷键配置

3. **功能增强**
   - 历史记录导出功能
   - 立绘切换过渡动画
   - 时间驱动服装切换

4. **长期规划**
   - AppController职责拆分
   - 独立存档系统
   - 插件化架构
