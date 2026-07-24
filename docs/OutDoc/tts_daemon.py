#!/usr/bin/env python3
"""
GPT-SoVITS 常驻后台服务脚本

专为Qt/C++项目设计，通过stdin/stdout进行JSON通信，支持：
1. 流式音频输出（实时返回音频片段）
2. 音频缓存到本地
3. 启动时加载模型，持续待命，避免重复加载

使用方式：
    "<GPT-SoVITS项目路径>\runtime\python.exe" tts_daemon.py --gpt_sovits_path <路径> [其他参数]

协议格式：
    输入：JSON字符串（每行一条，通过stdin）
    输出：JSON字符串（每行一条，通过stdout），模型内部日志输出到stderr

命令类型：
    - "list_models": 获取模型列表
    - "tts": 执行语音合成（流式，不缓存）
    - "tts_cached": 执行语音合成（流式，同时缓存到本地）
    - "load_model": 切换模型
    - "exit": 退出服务
"""

import os
import sys
import json
import argparse
import threading
import io


# ============================================================
# stdout 保护：模型内部有大量 print，会污染 JSON 通信协议
# 把真正的 stdout 保存起来，只用于输出 JSON；
# 模型加载期间的 print 重定向到 stderr。
# ============================================================
REAL_STDOUT = sys.stdout


class FilteredStdout:
    """
    包装的 stdout：只允许以 '{' 开头且能解析为 JSON 的行通过到真正的 stdout，
    其余内容一律转发到 stderr。
    """
    def __init__(self):
        self._real_stdout = REAL_STDOUT
        self._buffer = ""

    def write(self, text):
        if not text:
            return
        self._buffer += text
        # 按行处理
        while "\n" in self._buffer:
            line, self._buffer = self._buffer.split("\n", 1)
            line_with_newline = line + "\n"
            stripped = line.strip()
            if stripped and stripped[0] == "{":
                # 尝试解析为 JSON，成功则输出到真正的 stdout
                try:
                    json.loads(stripped)
                    self._real_stdout.write(line_with_newline)
                    self._real_stdout.flush()
                    continue  # 继续处理 buffer 中的剩余行
                except (json.JSONDecodeError, ValueError):
                    pass
            # 非 JSON 行，转发到 stderr
            sys.stderr.write(line_with_newline)

    def flush(self):
        if self._buffer:
            stripped = self._buffer.strip()
            if stripped and stripped[0] == "{":
                try:
                    json.loads(stripped)
                    self._real_stdout.write(self._buffer)
                    self._real_stdout.flush()
                    self._buffer = ""
                    return
                except (json.JSONDecodeError, ValueError):
                    pass
            sys.stderr.write(self._buffer)
            self._buffer = ""


def send_json(obj):
    """向真正的 stdout 输出一行 JSON（保证不被过滤）"""
    REAL_STDOUT.write(json.dumps(obj, ensure_ascii=False) + "\n")
    REAL_STDOUT.flush()


def setup_sys_path(gpt_sovits_path):
    """设置sys.path"""
    gpt_sovits_path = os.path.abspath(gpt_sovits_path)
    if gpt_sovits_path not in sys.path:
        sys.path.insert(0, gpt_sovits_path)
    gpt_sovits_subdir = os.path.join(gpt_sovits_path, "GPT_SoVITS")
    if gpt_sovits_subdir not in sys.path:
        sys.path.insert(0, gpt_sovits_subdir)


def scan_models(gpt_sovits_path):
    """扫描所有可用模型"""
    models = []
    scan_dirs = [
        os.path.join(gpt_sovits_path, "GPT_SoVITS", "pretrained_models"),
        os.path.join(gpt_sovits_path, "logs"),
        os.path.join(gpt_sovits_path, "GPT_weights_v2"),
        os.path.join(gpt_sovits_path, "SoVITS_weights_v2"),
        os.path.join(gpt_sovits_path, "GPT_weights_v3"),
        os.path.join(gpt_sovits_path, "SoVITS_weights_v3"),
    ]
    for scan_dir in scan_dirs:
        if not os.path.exists(scan_dir):
            continue
        for root, dirs, files in os.walk(scan_dir):
            for file in files:
                if file.endswith(".ckpt"):
                    abs_path = os.path.join(root, file)
                    rel_path = os.path.relpath(abs_path, gpt_sovits_path).replace("\\", "/")
                    rel_dir = os.path.relpath(root, scan_dir).replace("\\", "/")
                    name = file.replace(".ckpt", "") if rel_dir == "." else f"{rel_dir}/{file.replace('.ckpt', '')}"
                    models.append({
                        "name": name,
                        "type": "gpt",
                        "path": rel_path,
                        "absolute_path": abs_path
                    })
                elif file.endswith(".pth"):
                    abs_path = os.path.join(root, file)
                    rel_path = os.path.relpath(abs_path, gpt_sovits_path).replace("\\", "/")
                    rel_dir = os.path.relpath(root, scan_dir).replace("\\", "/")
                    name = file.replace(".pth", "") if rel_dir == "." else f"{rel_dir}/{file.replace('.pth', '')}"
                    models.append({
                        "name": name,
                        "type": "sovits",
                        "path": rel_path,
                        "absolute_path": abs_path
                    })
    return models


class TTSDemon:
    def __init__(self, args):
        self.gpt_sovits_path = os.path.abspath(args.gpt_sovits_path)
        self.device = args.device
        self.is_half = args.is_half
        self.version = args.version
        self.bert_path = os.path.join(self.gpt_sovits_path, args.bert_path)
        self.cnhubert_path = os.path.join(self.gpt_sovits_path, args.cnhubert_path)
        self.gpt_model_path = os.path.join(self.gpt_sovits_path, args.gpt_model)
        self.sovits_model_path = os.path.join(self.gpt_sovits_path, args.sovits_model)
        
        self.tts_pipeline = None
        self.loaded = False
        self.stop_event = threading.Event()
    
    def load_models(self):
        """加载模型"""
        try:
            from GPT_SoVITS.TTS_infer_pack.TTS import TTS, TTS_Config
            
            config_dict = {
                "device": self.device,
                "is_half": self.is_half,
                "version": self.version,
                "t2s_weights_path": self.gpt_model_path,
                "vits_weights_path": self.sovits_model_path,
                "bert_base_path": self.bert_path,
                "cnhuhbert_base_path": self.cnhubert_path,
            }
            
            tts_config = TTS_Config(config_dict)
            self.tts_pipeline = TTS(tts_config)
            self.loaded = True
            return {"success": True, "message": "模型加载成功"}
        except Exception as e:
            import traceback
            return {"success": False, "message": str(e), "traceback": traceback.format_exc()}
    
    def switch_model(self, gpt_model, sovits_model):
        """切换模型"""
        try:
            self.gpt_model_path = os.path.join(self.gpt_sovits_path, gpt_model)
            self.sovits_model_path = os.path.join(self.gpt_sovits_path, sovits_model)
            
            if self.tts_pipeline:
                self.tts_pipeline.init_t2s_weights(self.gpt_model_path)
                self.tts_pipeline.init_vits_weights(self.sovits_model_path)
            else:
                return self.load_models()
            
            return {"success": True, "message": "模型切换成功"}
        except Exception as e:
            import traceback
            return {"success": False, "message": str(e), "traceback": traceback.format_exc()}
    
    def synthesize_streaming(self, req, cache_path=None):
        """执行流式语音合成"""
        import numpy as np
        import soundfile as sf
        
        try:
            if not self.loaded or not self.tts_pipeline:
                send_json({"type": "tts_complete", "success": False, "message": "模型未加载"})
                return
            
            # 构建推理请求
            infer_req = {
                "text": req.get("text", ""),
                "text_lang": req.get("text_lang", "zh"),
                "ref_audio_path": os.path.join(self.gpt_sovits_path, req["ref_audio"]) if req.get("ref_audio") else "",
                "prompt_text": req.get("prompt_text", ""),
                "prompt_lang": req.get("prompt_lang", "zh"),
                "top_k": req.get("top_k", 5),
                "top_p": req.get("top_p", 1.0),
                "temperature": req.get("temperature", 1.0),
                "text_split_method": req.get("text_split_method", "cut5"),
                "batch_size": req.get("batch_size", 1),
                "speed_factor": req.get("speed_factor", 1.0),
                "seed": req.get("seed", -1),
                "return_fragment": True,
                "fragment_interval": req.get("fragment_interval", 0.3),
            }
            
            # 检查参考音频
            if not infer_req["ref_audio_path"]:
                send_json({"type": "tts_complete", "success": False, "message": "ref_audio参数不能为空"})
                return
            
            # 执行推理
            tts_generator = self.tts_pipeline.run(infer_req)
            
            all_audio_data = []
            sampling_rate = None
            
            for sr, chunk in tts_generator:
                sampling_rate = sr
                all_audio_data.append(chunk)
                
                # 输出音频片段（十六进制编码）
                chunk_hex = chunk.tobytes().hex()
                send_json({
                    "type": "audio_fragment",
                    "sampling_rate": sr,
                    "chunk_size": len(chunk),
                    "audio_data": chunk_hex
                })
            
            # 合并所有音频
            if all_audio_data:
                full_audio = np.concatenate(all_audio_data)
                
                # 缓存到本地
                if cache_path:
                    output_dir = os.path.dirname(os.path.abspath(cache_path))
                    if output_dir and not os.path.exists(output_dir):
                        os.makedirs(output_dir)
                    sf.write(cache_path, full_audio, sampling_rate, format='wav')
                
                send_json({
                    "type": "tts_complete",
                    "success": True,
                    "sampling_rate": sampling_rate,
                    "total_length": len(full_audio),
                    "cache_path": cache_path if cache_path else None,
                    "message": "语音合成完成"
                })
            else:
                send_json({
                    "type": "tts_complete",
                    "success": False,
                    "message": "未生成任何音频"
                })
                
        except Exception as e:
            import traceback
            send_json({
                "type": "tts_complete",
                "success": False,
                "message": str(e),
                "traceback": traceback.format_exc()
            })
    
    def run(self):
        """主循环"""
        # 输出初始化完成信号
        send_json({
            "type": "initialized",
            "success": True,
            "message": "TTS服务已就绪，等待命令..."
        })
        
        while not self.stop_event.is_set():
            try:
                # 读取输入（每行一条JSON）
                line = sys.stdin.readline()
                if not line:
                    break
                
                line = line.strip()
                if not line:
                    continue
                
                try:
                    cmd = json.loads(line)
                except json.JSONDecodeError as e:
                    send_json({
                        "type": "error",
                        "success": False,
                        "message": f"JSON解析错误: {str(e)}"
                    })
                    continue
                
                command = cmd.get("command", "")
                
                if command == "exit":
                    self.stop_event.set()
                    send_json({"type": "exit", "success": True, "message": "服务已退出"})
                    break
                
                elif command == "list_models":
                    models = scan_models(self.gpt_sovits_path)
                    send_json({"type": "model_list", "success": True, "models": models})
                
                elif command == "load_model":
                    gpt_model = cmd.get("gpt_model", "")
                    sovits_model = cmd.get("sovits_model", "")
                    if not gpt_model or not sovits_model:
                        send_json({
                            "type": "model_loaded",
                            "success": False,
                            "message": "gpt_model和sovits_model参数不能为空"
                        })
                    else:
                        result = self.switch_model(gpt_model, sovits_model)
                        result["type"] = "model_loaded"
                        send_json(result)
                
                elif command == "tts":
                    # 流式语音合成（不缓存）
                    self.synthesize_streaming(cmd)
                
                elif command == "tts_cached":
                    # 流式语音合成（同时缓存）
                    cache_path = cmd.get("cache_path", "")
                    self.synthesize_streaming(cmd, cache_path)
                
                else:
                    send_json({
                        "type": "error",
                        "success": False,
                        "message": f"未知命令: {command}"
                    })
            
            except EOFError:
                break
            except Exception as e:
                import traceback
                send_json({
                    "type": "error",
                    "success": False,
                    "message": str(e),
                    "traceback": traceback.format_exc()
                })


def main():
    parser = argparse.ArgumentParser(description="GPT-SoVITS 常驻后台服务")
    parser.add_argument("--gpt_sovits_path", type=str, required=True,
                        help="GPT-SoVITS项目的绝对路径")
    parser.add_argument("--device", type=str, default="cuda", help="推理设备")
    parser.add_argument("--is_half", action="store_true", help="使用半精度推理")
    parser.add_argument("--version", type=str, default="v2", choices=["v1", "v2"], help="模型版本")
    parser.add_argument("--bert_path", type=str, default="GPT_SoVITS/pretrained_models/chinese-roberta-wwm-ext-large",
                        help="BERT模型路径")
    parser.add_argument("--cnhubert_path", type=str, default="GPT_SoVITS/pretrained_models/chinese-hubert-base",
                        help="CNHuBERT模型路径")
    parser.add_argument("--gpt_model", type=str, default="GPT_SoVITS/pretrained_models/s1bert25hz-2kh-longer-epoch=68e-step=50232.ckpt",
                        help="GPT模型路径（相对于GPT-SoVITS项目路径）")
    parser.add_argument("--sovits_model", type=str, default="GPT_SoVITS/pretrained_models/s2G488k.pth",
                        help="SoVITS模型路径（相对于GPT-SoVITS项目路径）")
    
    args = parser.parse_args()
    
    # 设置sys.path
    setup_sys_path(args.gpt_sovits_path)
    
    # 安装 stdout 过滤器：模型内部的 print 输出会被重定向到 stderr，
    # 只有通过 send_json() 输出的 JSON 才会出现在 stdout
    sys.stdout = FilteredStdout()
    
    # 创建并运行服务
    daemon = TTSDemon(args)
    
    # 加载模型（这期间会有大量 print 输出到 stderr）
    sys.stderr.write("开始加载模型，请稍候...\n")
    sys.stderr.flush()
    load_result = daemon.load_models()
    if load_result["success"]:
        sys.stderr.write("模型加载完成，启动服务...\n")
        sys.stderr.flush()
        daemon.run()
    else:
        send_json(load_result)


if __name__ == "__main__":
    main()
