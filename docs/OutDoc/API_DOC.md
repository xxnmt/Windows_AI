# GPT-SoVITS API 接口文档

## 概述

GPT-SoVITS 提供两个版本的Web API接口：

| 版本 | 文件 | 特点 |
|------|------|------|
| v1 | `api.py` | 基础版，功能简洁 |
| v2 | `api_v2.py` | 增强版，支持流式、批量推理、多参数控制 |

---

## v1 与 v2 API 差异对比

| 特性 | v1 (api.py) | v2 (api_v2.py) |
|------|-------------|----------------|
| 核心接口 | `/` | `/tts` |
| 流式返回 | 支持（基础） | 支持（完善） |
| 批量推理 | 不支持 | 支持 |
| 文本切分方法 | 自定义标点 | 6种预设方法 |
| 辅助参考音频 | 支持 | 支持 |
| 语速控制 | 支持 | 支持 |
| 随机种子 | 不支持 | 支持 |
| 重复惩罚 | 不支持 | 支持 |
| 并行推理 | 不支持 | 支持 |
| 参数校验 | 手动检查 | Pydantic模型校验 |
| 单独切换模型 | `/set_model` | `/set_gpt_weights` + `/set_sovits_weights` |
| 默认参考音频 | `/change_refer` | `/set_refer_audio` |

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
| `split_bucket` | bool | 否 | `true` | 是否将批次拆分为多个桶 |
| `speed_factor` | float | 否 | `1.0` | 语速控制因子 |
| `fragment_interval` | float | 否 | `0.3` | 音频片段间隔 |
| `seed` | int | 否 | `-1` | 随机种子，-1表示随机 |
| `media_type` | string | 否 | `"wav"` | 输出音频格式：`wav`、`ogg`、`aac` |
| `streaming_mode` | bool | 否 | `false` | 是否启用流式返回 |
| `parallel_infer` | bool | 否 | `true` | 是否使用并行推理 |
| `repetition_penalty` | float | 否 | `1.35` | T2S模型重复惩罚参数 |

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

---

## 二、api.py (基础版)

### 2.1 启动方式

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

### 2.2 接口列表

| 接口 | 方法 | 功能 |
|------|------|------|
| `/` | GET/POST | 语音合成 |
| `/set_model` | GET/POST | 设置模型 |
| `/change_refer` | GET/POST | 更换默认参考音频 |
| `/control` | GET/POST | 命令控制 |

---

### 2.3 TTS语音合成接口

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

### 2.4 更换默认参考音频接口

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

### 2.5 设置模型接口

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

### 2.6 命令控制接口

**Endpoint:** `POST /control`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `command` | string | 是 | `restart` 或 `exit` |

---

## 三、文本切分方法说明

v2 API 提供6种文本切分方法（定义在 `GPT_SoVITS/TTS_infer_pack/text_segmentation_method.py`）：

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

## 四、支持的语种

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

## 五、音频格式说明

| 格式 | 支持流式 | 说明 |
|------|----------|------|
| `wav` | 否 | 默认格式，无损 |
| `ogg` | 是 | 流式默认格式，有损压缩 |
| `aac` | 是 | 有损压缩，需要FFmpeg |

---

## 六、配置文件说明

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
