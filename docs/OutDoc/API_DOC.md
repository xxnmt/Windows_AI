# GPT-SoVITS API 接口文档

## 概述

GPT-SoVITS 提供三个版本的Web API接口：

| 版本 | 文件 | 特点 |
|------|------|------|
| v1 | `api.py` | 基础版，功能简洁 |
| **v2** | **`api_v2.py`** | **增强版，支持流式、批量推理、多参数控制，已原生支持 v3/v3lora 模型** |
| v3 | `api_v3.py` | 兼容 v1/v2 的旧版 API，**已被 api_v2.py 取代**，仅保留供参考 |

> **重要：** 当前项目已更新，**`api_v2.py` 已原生支持 v3/v3lora 模型**。
> v3 模型会被自动识别并走 v3 推理流程（decode_encp + CFM + BigVGAN），输出 24000Hz。
> **推荐统一使用 `api_v2.py`**，无需再使用 `api_v3.py`。

---

## v1 / v2 / v3 API 差异对比

| 特性 | v1 (api.py) | v2 (api_v2.py) | v3 (api_v3.py) |
|------|-------------|----------------|----------------|
| 核心接口 | `/` | `/tts` | `/tts` |
| 流式返回 | 支持（基础） | 支持（完善） | 支持（完善） |
| 批量推理 | 不支持 | 支持 | 支持 |
| 文本切分方法 | 自定义标点 | 6种预设方法 | 6种预设方法 |
| 辅助参考音频 | 支持 | 支持 | 支持 |
| 语速控制 | 支持 | 支持 | 支持 |
| 随机种子 | 不支持 | 支持 | 支持 |
| 重复惩罚 | 不支持 | 支持 | 支持 |
| 并行推理 | 不支持 | 支持（v3 模型自动关闭） | 支持（v3 模型自动关闭） |
| 参数校验 | 手动检查 | Pydantic模型校验 | Pydantic模型校验 |
| 单独切换模型 | `/set_model` | `/set_gpt_weights` + `/set_sovits_weights` | `/set_gpt_weights` + `/set_sovits_weights` |
| 默认参考音频 | `/change_refer` | `/set_refer_audio` | `/set_refer_audio` |
| **v3/v3lora 模型** | 不支持 | **原生支持（自动识别）** | 支持（monkey-patch） |
| **输出采样率** | 32000Hz | v3=24000Hz / v1,v2=32000Hz | v3=24000Hz / v1,v2=32000Hz |
| **sample_steps 参数** | 无 | 有（v3 专用，默认 32） | 有（v3 专用，默认 32） |
| **super_sampling 参数** | 无 | 有（v3 专用，默认 False） | 无 |
| 默认 text_split_method | - | `cut5` | `cut0` |
| 实现方式 | 独立 | 原生集成 | monkey-patch 扩展 TTS 类 |

---

## 一、api_v2.py (推荐使用)

### 1.1 启动方式

```bash
python api_v2.py -a 127.0.0.1 -p 9880 -c GPT_SoVITS/configs/tts_infer.yaml
```

#### 启动参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-a` | 绑定地址 | `127.0.0.1` |
| `-p` | 绑定端口 | `9880` |
| `-c` | TTS配置文件路径 | `GPT_SoVITS/configs/tts_infer.yaml` |

### 1.2 接口列表

| 接口 | 方法 | 功能 |
|------|------|------|
| `/tts` | GET/POST | 语音合成（核心接口） |
| `/control` | GET | 命令控制（重启/退出） |
| `/set_refer_audio` | GET | 设置参考音频 |
| `/set_gpt_weights` | GET | 切换GPT模型权重 |
| `/set_sovits_weights` | GET | 切换SoVITS模型权重 |

---

### 1.3 TTS语音合成接口

**Endpoint:** `POST /tts`

#### 请求参数

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `text` | string | 是 | - | 待合成的文本 |
| `text_lang` | string | 是 | - | 文本语种（中文、英文、日文、韩文、粤语等） |
| `ref_audio_path` | string | 是 | - | 参考音频文件路径 |
| `aux_ref_audio_paths` | list | 否 | `[]` | 辅助参考音频路径，用于多说话人音色融合 |
| `prompt_text` | string | 否 | `""` | 参考音频对应的文本 |
| `prompt_lang` | string | 是 | - | 参考音频文本的语种 |
| `top_k` | int | 否 | `5` | Top-K采样参数 |
| `top_p` | float | 否 | `1.0` | Top-P采样参数 |
| `temperature` | float | 否 | `1.0` | 温度参数，控制生成多样性 |
| `text_split_method` | string | 否 | `"cut5"` | 文本切分方法，见下方说明 |
| `batch_size` | int | 否 | `1` | 批量推理大小 |
| `batch_threshold` | float | 否 | `0.75` | 批量切分阈值 |
| `split_bucket` | bool | 否 | `true` | 是否将批次拆分为多个桶（v3 模型自动关闭） |
| `speed_factor` | float | 否 | `1.0` | 语速控制因子 |
| `fragment_interval` | float | 否 | `0.3` | 音频片段间隔 |
| `seed` | int | 否 | `-1` | 随机种子，-1表示随机 |
| `media_type` | string | 否 | `"wav"` | 输出音频格式：`wav`、`ogg`、`aac`、`raw` |
| `streaming_mode` | bool/int | 否 | `false` | 流式返回模式（v3 模型自动回退到分段返回） |
| `parallel_infer` | bool | 否 | `true` | 是否使用并行推理（v3 模型自动关闭） |
| `repetition_penalty` | float | 否 | `1.35` | T2S模型重复惩罚参数 |
| `sample_steps` | int | 否 | `32` | **v3 专用**，CFM 采样步数，影响质量和速度（建议 16-64） |
| `super_sampling` | bool | 否 | `false` | **v3 专用**，超分辨率开关，开启后音质更好但速度变慢 |
| `overlap_length` | int | 否 | `2` | 流式模式下语义 token 重叠长度 |
| `min_chunk_length` | int | 否 | `16` | 流式模式下最小 chunk 长度 |

#### 请求示例 (POST)

```json
{
    "text": "先帝创业未半而中道崩殂，今天下三分，益州疲弊，此诚危急存亡之秋也。",
    "text_lang": "zh",
    "ref_audio_path": "archive_jingyuan_1.wav",
    "prompt_text": "我是「罗浮」云骑将军景元。不必拘谨，「将军」只是一时的身份，你称呼我景元便可。",
    "prompt_lang": "zh",
    "top_k": 5,
    "top_p": 1.0,
    "temperature": 1.0,
    "text_split_method": "cut5",
    "batch_size": 1,
    "speed_factor": 1.0,
    "streaming_mode": false,
    "media_type": "wav"
}
```

#### 请求示例 (GET)

```
http://127.0.0.1:9880/tts?text=Hello&text_lang=en&ref_audio_path=sample.wav&prompt_lang=en&prompt_text=Hi
```

#### 响应

| 状态码 | 说明 |
|--------|------|
| 200 | 成功，返回音频流 |
| 400 | 失败，返回JSON错误信息 |

**成功响应:** 直接返回音频流（Content-Type: `audio/wav` 或 `audio/ogg` 或 `audio/aac`）

**失败响应:**
```json
{
    "message": "错误描述",
    "Exception": "异常信息"
}
```

#### 参数校验规则（Pydantic模型 TTS_Request）

v2 API使用 Pydantic 模型 `TTS_Request` 进行参数校验：

| 校验项 | 规则 |
|--------|------|
| `text_lang` | 必须在支持的语种列表中 |
| `prompt_lang` | 必须在支持的语种列表中 |
| `media_type` | 仅支持 `wav`、`raw`、`ogg`、`aac` |
| `streaming_mode + media_type` | `ogg` 格式仅在流式模式下支持 |
| `text_split_method` | 必须是有效的切分方法名称 |

---

### 1.4 命令控制接口

**Endpoint:** `GET /control`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `command` | string | 是 | 命令：`restart`（重启）或 `exit`（退出） |

#### 请求示例

```
http://127.0.0.1:9880/control?command=restart
```

---

### 1.5 设置参考音频接口

**Endpoint:** `GET /set_refer_audio`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `refer_audio_path` | string | 是 | 参考音频文件路径 |

#### 请求示例

```
http://127.0.0.1:9880/set_refer_audio?refer_audio_path=sample.wav
```

#### 响应

```json
{
    "message": "success"
}
```

---

### 1.6 切换GPT模型接口

**Endpoint:** `GET /set_gpt_weights`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `weights_path` | string | 是 | GPT模型权重文件路径 |

#### 请求示例

```
http://127.0.0.1:9880/set_gpt_weights?weights_path=GPT_SoVITS/pretrained_models/s1bert25hz-2kh-longer-epoch=68e-step=50232.ckpt
```

#### 响应

```json
{
    "message": "success"
}
```

---

### 1.7 切换SoVITS模型接口

**Endpoint:** `GET /set_sovits_weights`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `weights_path` | string | 是 | SoVITS模型权重文件路径 |

#### 请求示例

```
http://127.0.0.1:9880/set_sovits_weights?weights_path=GPT_SoVITS/pretrained_models/s2G488k.pth
```

#### 响应

```json
{
    "message": "success"
}
```

### 1.8 v3/v3lora 模型使用说明

`api_v2.py` 已原生支持 v3/v3lora 模型，加载和推理流程完全自动。

#### 自动识别机制

调用 `/set_sovits_weights` 切换模型时，系统会自动检测模型版本：
- **v1/v2 模型**：走原始 encp→cfm→decoder 流程，输出 32000Hz
- **v3 模型**：创建 `SynthesizerTrnV3`，初始化 BigVGAN 声码器，走 encp→cfm→BigVGAN 流程，输出 24000Hz
- **v3lora 模型**：先加载预训练 `s2Gv3.pth`，再通过 PEFT 创建 LoRA 层，加载 LoRA 权重后 merge
- **v4 模型**：创建 `SynthesizerTrnV3`，初始化 Generator 声码器，输出 48000Hz

#### v3 模型自动行为

| 参数 | 行为 |
|------|------|
| `parallel_infer=true` | 自动设为 `false`（v3 不支持并行推理） |
| `split_bucket=true` | 自动设为 `false`（v3 不支持分桶） |
| `streaming_mode=true` | 自动回退到 `return_fragment=True` 分段返回模式 |
| `t2s_model.infer_panel` | 自动切换为 `infer_panel_naive_batched` |
| `prompt_text` 为空 | 抛出错误（v3 强制要求参考文本） |

#### v3 模型请求示例

```json
{
    "text": "おやすみなさい、お兄ちゃん。",
    "text_lang": "ja",
    "ref_audio_path": "reference_audio/loving_01.wav",
    "prompt_text": "そっか…じゃあ、キスもちゃんと今度しようね",
    "prompt_lang": "ja",
    "text_split_method": "cut0",
    "sample_steps": 32,
    "super_sampling": false,
    "speed_factor": 1.0,
    "streaming_mode": false,
    "media_type": "wav"
}
```

#### v3 模型输出说明

| 项目 | 值 |
|------|-----|
| 输出采样率 | **24000 Hz**（v1/v2 为 32000 Hz） |
| 分段返回 | 支持（`return_fragment` 模式） |
| 流式返回 | 不支持（自动回退到分段返回） |
| 默认语速 | 1.0 |

#### sample_steps 参数调优

| 值 | 效果 | 适用场景 |
|----|------|----------|
| 16 | 最快，质量略降 | 实时对话 |
| 32 | 默认，平衡质量与速度 | 日常使用 |
| 64 | 最慢，质量最佳 | 成品音频生成 |

#### super_sampling 参数

设置为 `true` 可开启超分辨率，提升音质但合成速度显著变慢。建议仅在对音质有极致要求时开启。

---

## 二、api_v3.py (旧版，已被 api_v2.py 取代)

### 2.1 启动方式

```bash
python api_v3.py -a 127.0.0.1 -p 9880 -c GPT_SoVITS/configs/tts_infer.yaml
```

#### 启动参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-a` / `--bind_addr` | 绑定地址 | `127.0.0.1` |
| `-p` / `--port` | 绑定端口 | `9880` |
| `-c` / `--tts_config` | TTS配置文件路径 | `GPT_SoVITS/configs/tts_infer.yaml` |

### 2.2 接口列表

| 接口 | 方法 | 功能 |
|------|------|------|
| `/tts` | GET/POST | 语音合成（核心接口） |
| `/control` | GET | 命令控制（重启/退出） |
| `/set_refer_audio` | GET | 设置参考音频 |
| `/set_gpt_weights` | GET | 切换GPT模型权重 |
| `/set_sovits_weights` | GET | 切换SoVITS模型权重（支持 v3lora） |

### 2.3 v3 模型加载机制

`api_v3.py` 通过 monkey-patch 替换 `TTS.init_vits_weights` 和 `TTS.run`，不修改上游 `TTS.py`：

- **v1/v2 模型**：走原始 `torch.load` 加载流程
- **v3 模型**：使用 `SynthesizerTrnV3` 类，并初始化 BigVGAN 声码器
- **v3lora 模型**：先加载预训练 `s2Gv3.pth`，再通过 PEFT 创建 LoRA 层，加载 LoRA 权重后调用 `merge_and_unload()` 合并

切换模型时若路径指向 `SoVITS_weights_v3/*.pth`，会自动识别为 v3 模型。

### 2.4 TTS语音合成接口

**Endpoint:** `POST /tts`（也支持 GET）

#### 请求参数

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `text` | string | 是 | - | 待合成的文本 |
| `text_lang` | string | 是 | - | 文本语种 |
| `ref_audio_path` | string | 是 | - | 参考音频文件路径 |
| `aux_ref_audio_paths` | list | 否 | `[]` | 辅助参考音频路径 |
| `prompt_text` | string | 否 | `""` | 参考音频对应的文本 |
| `prompt_lang` | string | 是 | - | 参考音频文本的语种 |
| `top_k` | int | 否 | `5` | Top-K采样参数 |
| `top_p` | float | 否 | `1.0` | Top-P采样参数 |
| `temperature` | float | 否 | `1.0` | 温度参数 |
| `text_split_method` | string | 否 | `"cut0"` | 文本切分方法 |
| `batch_size` | int | 否 | `1` | 批量推理大小 |
| `batch_threshold` | float | 否 | `0.75` | 批量切分阈值 |
| `split_bucket` | bool | 否 | `true` | v3 模型会自动关闭 |
| `speed_factor` | float | 否 | `1.0` | 语速控制因子 |
| `fragment_interval` | float | 否 | `0.3` | 音频片段间隔 |
| `seed` | int | 否 | `-1` | 随机种子 |
| `media_type` | string | 否 | `"wav"` | 输出格式：`wav`、`ogg`、`aac`、`raw` |
| `streaming_mode` | bool | 否 | `false` | 是否启用流式返回 |
| `parallel_infer` | bool | 否 | `true` | v3 模型会自动关闭 |
| `repetition_penalty` | float | 否 | `1.35` | T2S 重复惩罚 |
| `sample_steps` | int | 否 | `32` | **v3 专用**，CFM 采样步数，影响质量和速度（建议 16-64） |

#### 请求示例 (POST)

```json
{
    "text": "先帝创业未半而中道崩殂，今天下三分，益州疲弊，此诚危急存亡之秋也。",
    "text_lang": "zh",
    "ref_audio_path": "archive_jingyuan_1.wav",
    "prompt_text": "我是「罗浮」云骑将军景元。",
    "prompt_lang": "zh",
    "top_k": 5,
    "top_p": 1.0,
    "temperature": 1.0,
    "text_split_method": "cut0",
    "speed_factor": 1.0,
    "sample_steps": 32,
    "streaming_mode": false,
    "media_type": "wav"
}
```

#### 响应

| 状态码 | 说明 |
|--------|--------|
| 200 | 成功，返回音频流（v3 采样率 24000Hz） |
| 400 | 失败，返回 JSON 错误信息 |

**流式响应：** 启用 `streaming_mode=true` 时，wav 格式会先发送 24000Hz 的 wav header，随后持续推送音频片段；ogg/aac 直接流式推送。

#### v3 模型自动行为

| 参数 | 行为 |
|------|------|
| `parallel_infer=true` | 自动设为 `false`（v3 不支持并行推理） |
| `split_bucket=true` | 自动设为 `false`（v3 不支持分桶） |
| `t2s_model.infer_panel` | 自动切换为 `infer_panel_naive_batched` |

### 2.5 切换 SoVITS 模型（支持 v3lora）

**Endpoint:** `GET /set_sovits_weights`

```
http://127.0.0.1:9880/set_sovits_weights?weights_path=SoVITS_weights_v3/xxx.pth
```

加载 v3lora 模型时，会自动：
1. 加载预训练模型 `GPT_SoVITS/pretrained_models/s2Gv3.pth`
2. 通过 PEFT 创建 LoRA 层（target_modules: to_k/to_q/to_v/to_out.0）
3. 加载用户 LoRA 权重
4. 调用 `merge_and_unload()` 合并权重，恢复为标准 `SynthesizerTrnV3`

其他控制接口（`/control`、`/set_refer_audio`、`/set_gpt_weights`）用法与 api_v2.py 完全一致，参见第一章。

---

## 三、api.py (基础版)

### 3.1 启动方式

```bash
python api.py -dr "123.wav" -dt "一二三。" -dl "zh"
```

#### 启动参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-s` | SoVITS模型路径 | 从config读取 |
| `-g` | GPT模型路径 | 从config读取 |
| `-dr` | 默认参考音频路径 | 空 |
| `-dt` | 默认参考音频文本 | 空 |
| `-dl` | 默认参考音频语种 | 空 |
| `-d` | 推理设备 | `cuda` |
| `-a` | 绑定地址 | `127.0.0.1` |
| `-p` | 绑定端口 | `9880` |
| `-fp` | 使用全精度 | `false` |
| `-hp` | 使用半精度 | `false` |
| `-sm` | 流式返回模式 | `close` |
| `-mt` | 音频编码格式 | `wav` |
| `-st` | 音频数据类型 | `int16` |
| `-cp` | 文本切分符号 | 空 |
| `-hb` | CNHubert路径 | 从config读取 |
| `-b` | BERT路径 | 从config读取 |

### 3.2 接口列表

| 接口 | 方法 | 功能 |
|------|------|------|
| `/` | GET/POST | 语音合成 |
| `/set_model` | GET/POST | 设置模型 |
| `/change_refer` | GET/POST | 更换默认参考音频 |
| `/control` | GET/POST | 命令控制 |

---

### 3.3 TTS语音合成接口

**Endpoint:** `POST /`

#### 请求参数

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `text` | string | 是 | - | 待合成的文本 |
| `text_language` | string | 是 | - | 文本语种 |
| `refer_wav_path` | string | 否 | - | 参考音频路径（未指定则使用默认） |
| `prompt_text` | string | 否 | - | 参考音频文本（未指定则使用默认） |
| `prompt_language` | string | 否 | - | 参考音频语种（未指定则使用默认） |
| `cut_punc` | string | 否 | - | 文本切分符号 |
| `top_k` | int | 否 | `15` | Top-K采样参数 |
| `top_p` | float | 否 | `1.0` | Top-P采样参数 |
| `temperature` | float | 否 | `1.0` | 温度参数 |
| `speed` | float | 否 | `1.0` | 语速控制 |
| `inp_refs` | list | 否 | `[]` | 额外参考音频路径列表 |

#### 请求示例 (POST)

```json
{
    "refer_wav_path": "123.wav",
    "prompt_text": "一二三。",
    "prompt_language": "zh",
    "text": "先帝创业未半而中道崩殂，今天下三分，益州疲弊，此诚危急存亡之秋也。",
    "text_language": "zh",
    "top_k": 20,
    "top_p": 0.6,
    "temperature": 0.6,
    "speed": 1.0,
    "inp_refs": ["456.wav", "789.wav"]
}
```

#### 请求示例 (GET)

```
http://127.0.0.1:9880?text=Hello&text_language=en&refer_wav_path=sample.wav&prompt_text=Hi&prompt_language=en
```

#### 响应

| 状态码 | 说明 |
|--------|------|
| 200 | 成功，返回音频流 |
| 400 | 失败，返回JSON错误信息 |

---

### 3.4 更换默认参考音频接口

**Endpoint:** `POST /change_refer`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `refer_wav_path` | string | 是 | 参考音频路径 |
| `prompt_text` | string | 是 | 参考音频文本 |
| `prompt_language` | string | 是 | 参考音频语种 |

#### 请求示例

```json
{
    "refer_wav_path": "123.wav",
    "prompt_text": "一二三。",
    "prompt_language": "zh"
}
```

#### 响应

```json
{
    "code": 0,
    "message": "Success"
}
```

---

### 3.5 设置模型接口

**Endpoint:** `POST /set_model`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `gpt_model_path` | string | 是 | GPT模型路径 |
| `sovits_model_path` | string | 是 | SoVITS模型路径 |

#### 请求示例

```json
{
    "gpt_model_path": "GPT_SoVITS/pretrained_models/s1bert25hz.ckpt",
    "sovits_model_path": "GPT_SoVITS/pretrained_models/s2G488k.pth"
}
```

---

### 3.6 命令控制接口

**Endpoint:** `POST /control`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `command` | string | 是 | `restart` 或 `exit` |

---

## 四、文本切分方法说明

v2 / v3 API 提供6种文本切分方法（定义在 `GPT_SoVITS/TTS_infer_pack/text_segmentation_method.py`）：

| 方法名 | 说明 | 适用场景 |
|--------|------|----------|
| `cut0` | 不切分，原样返回 | 短文本、已手动分段的文本 |
| `cut1` | 凑4句一切 | 按完整句子分组，保持语义完整 |
| `cut2` | 凑50字一切 | 按字数切分，平衡长度和语义 |
| `cut3` | 按中文句号「。」切 | 纯中文文本 |
| `cut4` | 按英文句号「.」切 | 纯英文文本（排除数字中的点） |
| `cut5` | 按标点符号切（默认） | 通用方法，支持中英文标点，排除数字中的点 |

**cut5 支持的标点符号:** `,` `.` `;` `?` `!` `、` `，` `。` `？` `！` `：` `…`

---

## 五、支持的语种

| 语种代码 | 说明 |
|----------|------|
| `zh` | 中文（中英混合） |
| `en` | 英文 |
| `ja` | 日文（日英混合） |
| `ko` | 韩文（韩英混合） |
| `yue` | 粤语（粤英混合） |
| `auto` | 多语种混合（自动识别） |
| `auto_yue` | 多语种混合（粤语模式） |
| `all_zh` | 纯中文 |
| `all_yue` | 纯粤语 |
| `all_ja` | 纯日文 |
| `all_ko` | 纯韩文 |

---

## 六、音频格式说明

| 格式 | 支持流式 | 说明 |
|------|----------|------|
| `wav` | 否 | 默认格式，无损 |
| `ogg` | 是 | 流式默认格式，有损压缩 |
| `aac` | 是 | 有损压缩，需要FFmpeg |

---

## 七、采样率说明

不同模型版本输出的音频采样率不同，调用方需根据采样率正确播放：

| 模型版本 | API 文件 | 输出采样率 | 流式 wav header |
|----------|----------|------------|-----------------|
| v1 / v2 | api.py / api_v2.py | 32000 Hz | 32000 Hz |
| v3 / v3lora | **api_v2.py** | **24000 Hz** | **24000 Hz** |
| v4 | api_v2.py | 48000 Hz | 48000 Hz |

**注意：**
- `api_v2.py` 加载 v3/v3lora 模型时，自动输出 24000 Hz
- `api_v2.py` 加载 v1/v2 模型时，仍输出 32000 Hz
- `api_v2.py` 加载 v4 模型时，输出 48000 Hz
- 流式 wav 响应的 header 采样率与音频实际采样率一致，播放器应动态读取 header，不要硬编码采样率
- **v3 模型超分模式**：v3/v3lora 模型开启 `super_sampling=true` 时，输出采样率从 24000 Hz 提升到 48000 Hz（与 v4 一致）
- 调用方需根据 `super_sampling` 参数动态适配播放器采样率，不能假设 v3 模型固定为 24000 Hz

---

## 八、配置文件说明

`config.py` 中定义了影响API行为的全局配置：

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `sovits_path` | `""` | SoVITS模型路径（空则使用预训练模型） |
| `gpt_path` | `""` | GPT模型路径（空则使用预训练模型） |
| `is_half` | `True` | 是否使用半精度推理（GPU自动检测） |
| `cnhubert_path` | `GPT_SoVITS/pretrained_models/chinese-hubert-base` | CNHubert模型路径 |
| `bert_path` | `GPT_SoVITS/pretrained_models/chinese-roberta-wwm-ext-large` | BERT模型路径 |
| `pretrained_sovits_path` | `GPT_SoVITS/pretrained_models/s2G488k.pth` | 预训练SoVITS模型路径 |
| `pretrained_gpt_path` | `GPT_SoVITS/pretrained_models/s1bert25hz-2kh-longer-epoch=68e-step=50232.ckpt` | 预训练GPT模型路径 |
| `infer_device` | `cuda`（可用时） | 推理设备（cuda/cpu） |
| `api_port` | `9880` | API服务端口 |

### 半精度自动检测规则

| 条件 | 行为 |
|------|------|
| GPU型号含 `16`（非V100）、`P40`、`P10`、`1060`、`1070`、`1080` | 强制使用全精度 |
| CPU推理 | 强制使用全精度 |
| 其他GPU | 使用半精度 |

---

## 九、模块直接调用（Qt/C++项目集成）

如果您希望通过 Python 模块直接调用（而非 HTTP API），请参阅独立文档：

**[MODULE_CALL_DOC.md](./MODULE_CALL_DOC.md)**

该文档包含：
- 常驻后台服务 `tts_daemon.py`（启动时加载模型，持续待命，支持流式输出和音频缓存）
- 命令行单次调用 `tts_cli.py`
- Python 模块直接调用示例（含 **v3 LoRA 模型调用示例**）
- Qt/C++ 项目集成指南（含 C++ 示例代码与 v3 关键片段）

**重要：**
- 整合包自带 Python 运行环境，必须使用 `runtime\python.exe`，详见独立文档
- **v3/v3lora 模型已原生支持，无需额外 `import api_v3`**
- 通过 HTTP 调用时，直接启动 `api_v2.py` 即可，v3 模型会被自动识别
