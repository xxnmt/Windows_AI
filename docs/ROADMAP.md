# 开发路线图

> 项目：Windows_AI 桌面看板娘（桌宠）
> 版本：v0.2.6
> 更新日期：2026-07-23

---

## 项目愿景

打造一款功能完善、交互丰富的桌面看板娘应用，支持AI对话、立绘切换、语音交互、智能助手等核心功能。

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

### v0.3.0 - AI资源管理（优先）
**状态**：🚧 进行中

**功能清单**：
- [x] 配置文件持久化（app_data/config/settings.json） ✅ 已实现
  - [x] API Key存储与读取 ✅
  - [x] GPT-SoVITS服务地址配置 ✅
  - [ ] TTS参数配置（语速、温度、参考音频）
- [x] 系统提示词外部化（app_data/config/prompt.txt） ✅ 已实现
  - [x] 框架预留：LLMService预留 `loadSystemPrompt()` 方法 ✅
  - [x] 提示词从文件加载 ✅
  - [x] 支持动态追加当前状态（方案C） ✅
- [x] AI状态同步机制（方案C：本地切换后在下轮对话同步给AI） ✅ 已实现
  - [x] 框架预留：LLMService状态提供者模式 ✅
  - [x] 框架预留：AppearanceManager状态描述方法 ✅
  - [x] 框架预留：AppController连接状态提供者 ✅
  - [x] 实际实现：动态构建包含状态的系统提示词 ✅
- [x] 设置界面（SettingsWidget） ✅ 已实现API配置页+记忆管理页
  - [x] API配置页 ✅
  - [x] 记忆配置页（记忆长度、历史记录管理）✅
- [x] 配置目录结构建立（app_data/config/） ✅ 已实现

**技术任务**：
- [x] ConfigManager支持从文件加载/保存（JSON格式） ✅ 已实现
- [x] LLMService状态提供者模式（框架预留） ✅ 已实现
- [x] AppearanceManager状态描述方法（框架预留） ✅ 已实现
- [x] LLMService支持动态构建系统提示词 ✅ 已实现
- [x] 创建SettingsWidget设置界面（API配置页已实现）
- [ ] 更新.gitignore规则（配置目录追踪但内容不上传）

**预计时间**：2-3周

### v0.4.0 - AI记忆系统（次优先）
**状态**：✅ 已完成（短期记忆+用户画像+长期摘要）

**功能清单**：
- [x] 分层记忆系统
  - [x] 短期记忆（最近N轮对话上下文，SQLite）✅ 已实现（默认15轮）
  - [x] 用户画像（置信度衰减机制，3级半衰期）✅ 已实现
  - [x] 长期记忆摘要（LLM自动提取重要事件）✅ 已实现
- [x] 对话历史查看界面 ✅ 已实现
  - [x] 历史记录列表（表格展示，分页浏览）✅
  - [x] 单条记录详情（悬停显示完整内容）✅
  - [x] 删除功能（单条删除、清空全部）✅
  - [ ] 导出功能
- [x] AI记忆写入与读取 ✅ 已实现
  - [x] 每次对话后写入历史 ✅
  - [x] 对话前读取短期记忆 ✅
  - [x] 用户画像和长期摘要注入LLM提示词 ✅
  - [x] 自动记忆提取（未摘要对话达到阈值时触发）✅

**技术任务**：
- [x] 创建MemoryManager记忆管理器 ✅
- [x] SQLite数据库设计与实现（v2升级：chat_history、user_profile、long_term_summary三张表）✅
- [x] 历史记录CRUD界面（SettingsWidget记忆管理页）✅
- [x] LLM摘要生成器（extractMemoryAsync异步提取）✅
- [x] 用户画像置信度衰减机制（三级半衰期）✅

**预计时间**：已完成

### v0.5.0 - TTS模块（第三优先）
**状态**：📋 待开始

**功能清单**：
- [ ] 真实TTS引擎接入（GPT-SoVITS v2 API）
- [ ] TTS配置完善
  - [ ] 启用开关
  - [ ] 语速/温度调节
  - [ ] 参考音频配置
- [ ] 音频播放（Qt Multimedia）
- [ ] 设置界面TTS配置页

**技术任务**：
- [ ] TTSService替换模拟实现为真实TTS调用（POST /tts）
- [ ] 集成Qt Multimedia音频播放
- [ ] 参考音频路径配置与管理

**预计时间**：2-3周

### v0.6.0 - 设置界面 & 交互增强
**状态**：📋 待开始

**功能清单**：
- [ ] 设置界面完善
  - [ ] 外观配置（气泡样式、窗口透明度）
  - [ ] 时间配置（自动服装切换）
  - [ ] 快捷键配置（全局快捷键）
- [ ] 时间驱动服装切换（白天校服 / 夜晚睡衣）
- [ ] 右键菜单扩展（服装/表情快捷切换）
- [ ] 用户输入界面（托盘菜单）

**技术任务**：
- [ ] 创建TimeManager时间管理器
- [ ] 创建ShortcutManager全局快捷键管理
- [ ] 右键菜单扩展
- [ ] 系统托盘图标

**预计时间**：2-3周

### v0.7.0 - 智能助手 & 性能优化
**状态**：📋 待开始

**功能清单**：
- [ ] 点击交互（不同部位触发不同回应）
- [ ] 立绘切换过渡动画（淡入淡出）
- [ ] 立绘资源缓存
- [ ] TTS音频缓存
- [ ] 天气查询
- [ ] 多角色支持

**预计时间**：3-4周

---

## 技术债务清理计划

| 编号 | 债务 | 优先级 | 计划版本 | 状态 |
|------|------|--------|----------|------|
| TD-001 | API Key硬编码 | 高 | v0.3.0 | ✅ 已修复（配置文件持久化） |
| TD-010 | AI状态同步机制未实现（方案C） | 高 | v0.3.0 | ✅ 已实现（状态提供者模式） |
| TD-011 | 记忆系统未实现 | 高 | v0.4.0 | ✅ 已实现（短期记忆，MemoryManager） |
| TD-013 | AppController职责过重（上帝对象） | 高 | v0.6.0 | 待重构 |
| TD-014 | AnchorManager内存泄漏风险 | 高 | v0.5.0 | 待修复 |
| TD-005 | TTS为模拟实现 | 高 | v0.5.0 | 待修复 |
| TD-015 | LLMService头文件依赖MemoryManager | 中 | v0.5.0 | 待优化 |
| TD-016 | AppController中魔法数字SUMMARY_THRESHOLD | 低 | v0.5.0 | ✅ 非问题（设计意图：摘要轮数=短期记忆轮数） |
| TD-012 | 系统提示词硬编码在LLMService源码中 | 中 | v0.3.0 | ✅ 已实现（prompt.txt文件） |
| TD-006 | 用户输入入口缺失 | 中 | v0.6.0 | ✅ 已修复（ChatWidget实现） |
| TD-002 | 立绘路径硬编码（AppearanceManager已部分解决） | 中 | v0.2.0 | ✅ 已解决 |
| TD-007 | characterwidget大量注释代码残留 | 低 | v0.3.0 | ✅ 已清理 |
| TD-008 | appcontroller重复include | 低 | v0.3.0 | ✅ 已修复 |
| TD-003 | BubbleWidget位置计算硬编码 | 中 | v0.2.0 | ✅ 已解决 |
| TD-004 | 立绘切换功能未实现 | 高 | v0.2.0 | ✅ 已解决 |

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

### 阶段4：位置锚点系统（v0.2.1）

```
AppController（信号调度中枢）
    ├── CharacterWidget（立绘/右键菜单）
    ├── BubbleWidget（气泡/打字机）
    ├── ChatWidget（聊天输入框）
    ├── AnchorManager（位置锚点管理）
    ├── LLMService（AI对话/解析）
    ├── TTSService（语音合成/播放）
    ├── AppearanceManager（四维状态）
    └── ConfigManager（单例配置）

AnchorManager 管理：
    ├── BubbleWidget → HeadRight 位置
    └── ChatWidget → WaistCenter 位置
```

### 阶段5：配置持久化（v0.2.2）

```
AppController（信号调度中枢）
    ├── CharacterWidget（立绘/右键菜单）
    ├── BubbleWidget（气泡/打字机）
    ├── ChatWidget（聊天输入框）
    ├── AnchorManager（位置锚点管理）
    ├── LLMService（AI对话/解析）
    ├── TTSService（语音合成/播放）
    ├── AppearanceManager（四维状态）
    └── ConfigManager（单例配置 → JSON文件持久化）

配置文件：app_data/config/setting.json
    ├── api.deepseek_api_key
    └── api.gpt_sovits_url
```

### 阶段6：状态同步框架（v0.2.3）

```
AppController（信号调度中枢）
    ├── CharacterWidget（立绘/右键菜单）
    ├── BubbleWidget（气泡/打字机）
    ├── ChatWidget（聊天输入框）
    ├── AnchorManager（位置锚点管理）
    ├── LLMService（AI对话/解析 → 状态提供者模式）
    │   └── m_stateProvider → AppearanceManager.getCurrentStateDescription()
    ├── TTSService（语音合成/播放）
    ├── AppearanceManager（四维状态 → 状态描述输出）
    └── ConfigManager（单例配置 → JSON文件持久化）

状态同步机制（方案C）：
    本地状态变化 → askDeepSeek前动态获取状态 → 追加到系统提示词 → AI感知状态
```

### 阶段7：AI记忆系统（v0.2.4，当前）

```
AppController（信号调度中枢）
    ├── CharacterWidget（立绘/右键菜单）
    ├── BubbleWidget（气泡/打字机）
    ├── ChatWidget（聊天输入框）
    ├── AnchorManager（位置锚点管理）
    ├── LLMService（AI对话/解析 → 状态提供者模式 + 历史上下文）
    │   ├── m_stateProvider → AppearanceManager.getCurrentStateDescription()
    │   └── historyQA → MemoryManager.getHistoryTurn(15)
    ├── TTSService（语音合成/播放）
    ├── AppearanceManager（四维状态 → 状态描述输出）
    ├── MemoryManager（AI记忆系统 → SQLite数据库）
    │   ├── saveQATurn() 保存对话
    │   └── getHistoryTurn(N) 获取短期记忆
    └── ConfigManager（单例配置 → JSON文件持久化）

记忆系统：
    用户输入 → 读取最近15轮历史 → 注入LLM请求上下文 → AI回复 → 保存对话历史
```

### 阶段8：插件化（v0.5.0+）

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
| Qt | 6.5+ | 6.6+ | 可能用到Qt Multimedia做音频 |
| CMake | 3.19+ | 3.20+ | - |
| GPT-SoVITS | - | - | v0.3.0接入TTS引擎 |

---

## 风险评估

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|----------|
| DeepSeek API变更 | AI对话功能失效 | 中 | LLMService封装层，便于适配 |
| TTS服务不可用 | 语音功能失效 | 中 | 支持多TTS引擎切换 |
| 立绘资源过大 | 打包体积增加 | 低 | 支持资源按需加载/外部资源 |
| 网络延迟 | 用户体验下降 | 中 | 添加加载状态提示，TTS预合成 |
| GPT-SoVITS部署复杂 | TTS接入周期长 | 高 | 保留模拟实现，逐步替换 |

---

## 当前进度总览

```
v0.1.0  ████████████████████ 100%  基础版本
v0.2.0  ████████████████████ 100%  架构重构 & 多标签协议
v0.2.1  ████████████████████ 100%  位置锚点系统
v0.2.2  ████████████████████ 100%  配置持久化
v0.2.3  ████████████████████ 100%  状态同步框架 & 提示词外部化
v0.2.4  ████████████████████ 100%  AI记忆系统（短期记忆）
v0.2.5  ████████████████████ 100%  设置界面完善
v0.2.6  ████████████████████ 100%  架构审查
v0.3.0  ████████████████████ 100%  AI资源管理（优先）
v0.4.0  ████████████████████ 100%  AI记忆系统（短期记忆+用户画像+长期摘要）
v0.5.0  ░░░░░░░░░░░░░░░░░░░░   0%  TTS模块（第三优先）
v0.6.0  ░░░░░░░░░░░░░░░░░░░░   0%  设置界面 & 交互增强
v0.7.0  ░░░░░░░░░░░░░░░░░░░░   0%  智能助手 & 性能优化
```

---

## 近期任务（Next Up）

1. **架构改进** - 修复AnchorManager内存泄漏、优化LLMService依赖
2. **TTS真实接入** - 集成GPT-SoVITS推理引擎
3. **设置界面扩展** - 添加TTS配置页、外观配置、时间配置、快捷键配置
4. **代码质量提升** - 统一日志格式、添加函数注释、清理未使用代码
5. **长期记忆增强** - 历史记录导出功能、摘要重建机制
