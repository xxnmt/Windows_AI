#!/usr/bin/env python3
"""
GPT-SoVITS 命令行接口脚本

用于Qt/C++项目通过QProcess直接调用，支持：
1. 模型列表扫描
2. 语音合成

使用方式：
    python tts_cli.py --action list_models --gpt_sovits_path /path/to/GPT-SoVITS
    python tts_cli.py --action tts --gpt_sovits_path /path/to/GPT-SoVITS --text "你好" --output output.wav ...
"""

import os
import sys
import json
import argparse


def setup_sys_path(gpt_sovits_path):
    """
    设置sys.path，确保能够导入GPT_SoVITS模块
    
    Args:
        gpt_sovits_path: GPT-SoVITS项目的绝对路径
    """
    gpt_sovits_path = os.path.abspath(gpt_sovits_path)
    
    # 添加项目根目录
    if gpt_sovits_path not in sys.path:
        sys.path.insert(0, gpt_sovits_path)
    
    # 添加GPT_SoVITS子目录
    gpt_sovits_subdir = os.path.join(gpt_sovits_path, "GPT_SoVITS")
    if gpt_sovits_subdir not in sys.path:
        sys.path.insert(0, gpt_sovits_subdir)


def scan_models(gpt_sovits_path):
    """
    扫描所有可用的模型文件
    
    Args:
        gpt_sovits_path: GPT-SoVITS项目的绝对路径
    
    Returns:
        dict: 包含模型列表的字典
    """
    models = []
    
    # 定义要扫描的目录
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
                    # GPT/T2S模型
                    abs_path = os.path.join(root, file)
                    rel_path = os.path.relpath(abs_path, gpt_sovits_path).replace("\\", "/")
                    # 获取相对目录用于名称显示
                    rel_dir = os.path.relpath(root, scan_dir).replace("\\", "/")
                    if rel_dir == ".":
                        name = file.replace(".ckpt", "")
                    else:
                        name = f"{rel_dir}/{file.replace('.ckpt', '')}"
                    models.append({
                        "name": name,
                        "type": "gpt",
                        "path": rel_path,
                        "absolute_path": abs_path
                    })
                elif file.endswith(".pth"):
                    # SoVITS/VITS模型
                    abs_path = os.path.join(root, file)
                    rel_path = os.path.relpath(abs_path, gpt_sovits_path).replace("\\", "/")
                    # 获取相对目录用于名称显示
                    rel_dir = os.path.relpath(root, scan_dir).replace("\\", "/")
                    if rel_dir == ".":
                        name = file.replace(".pth", "")
                    else:
                        name = f"{rel_dir}/{file.replace('.pth', '')}"
                    models.append({
                        "name": name,
                        "type": "sovits",
                        "path": rel_path,
                        "absolute_path": abs_path
                    })
    
    return {"models": models}


def synthesize_speech(gpt_sovits_path, args):
    """
    执行语音合成
    
    Args:
        gpt_sovits_path: GPT-SoVITS项目的绝对路径
        args: 命令行参数
    
    Returns:
        dict: 结果字典
    """
    try:
        # 延迟导入，避免启动时加载模型
        from GPT_SoVITS.TTS_infer_pack.TTS import TTS, TTS_Config
        import soundfile as sf
        
        # 构建配置字典
        config_dict = {
            "device": args.device,
            "is_half": args.is_half,
            "version": args.version,
            "t2s_weights_path": os.path.join(gpt_sovits_path, args.gpt_model),
            "vits_weights_path": os.path.join(gpt_sovits_path, args.sovits_model),
            "bert_base_path": os.path.join(gpt_sovits_path, args.bert_path),
            "cnhuhbert_base_path": os.path.join(gpt_sovits_path, args.cnhubert_path),
        }
        
        # 创建TTS配置和实例
        tts_config = TTS_Config(config_dict)
        tts_pipeline = TTS(tts_config)
        
        # 构建推理请求
        req = {
            "text": args.text,
            "text_lang": args.text_lang,
            "ref_audio_path": os.path.join(gpt_sovits_path, args.ref_audio) if args.ref_audio else "",
            "aux_ref_audio_paths": [],
            "prompt_text": args.prompt_text,
            "prompt_lang": args.prompt_lang,
            "top_k": args.top_k,
            "top_p": args.top_p,
            "temperature": args.temperature,
            "text_split_method": args.text_split_method,
            "batch_size": args.batch_size,
            "batch_threshold": args.batch_threshold,
            "split_bucket": args.split_bucket,
            "speed_factor": args.speed_factor,
            "fragment_interval": args.fragment_interval,
            "seed": args.seed,
            "parallel_infer": args.parallel_infer,
            "repetition_penalty": args.repetition_penalty,
        }
        
        # 执行推理
        tts_generator = tts_pipeline.run(req)
        sr, audio_data = next(tts_generator)
        
        # 保存音频文件
        output_path = os.path.abspath(args.output)
        output_dir = os.path.dirname(output_path)
        if output_dir and not os.path.exists(output_dir):
            os.makedirs(output_dir)
        
        sf.write(output_path, audio_data, sr, format='wav')
        
        return {
            "success": True,
            "output_path": output_path,
            "sampling_rate": sr,
            "message": "语音合成成功"
        }
    
    except Exception as e:
        import traceback
        return {
            "success": False,
            "message": str(e),
            "traceback": traceback.format_exc()
        }


def main():
    parser = argparse.ArgumentParser(description="GPT-SoVITS 命令行接口")
    parser.add_argument("--action", type=str, required=True, choices=["list_models", "tts"],
                        help="操作类型：list_models（获取模型列表）或 tts（语音合成）")
    parser.add_argument("--gpt_sovits_path", type=str, required=True,
                        help="GPT-SoVITS项目的绝对路径")
    
    # 语音合成参数
    parser.add_argument("--text", type=str, default="", help="待合成的文本")
    parser.add_argument("--text_lang", type=str, default="zh", help="文本语种")
    parser.add_argument("--ref_audio", type=str, default="", help="参考音频路径（相对于GPT-SoVITS项目路径）")
    parser.add_argument("--prompt_text", type=str, default="", help="参考音频对应的文本")
    parser.add_argument("--prompt_lang", type=str, default="zh", help="参考音频文本的语种")
    parser.add_argument("--gpt_model", type=str, default="GPT_SoVITS/pretrained_models/s1bert25hz-2kh-longer-epoch=68e-step=50232.ckpt",
                        help="GPT模型路径（相对于GPT-SoVITS项目路径）")
    parser.add_argument("--sovits_model", type=str, default="GPT_SoVITS/pretrained_models/s2G488k.pth",
                        help="SoVITS模型路径（相对于GPT-SoVITS项目路径）")
    parser.add_argument("--output", type=str, default="output.wav", help="输出音频文件路径")
    
    # 可选参数
    parser.add_argument("--device", type=str, default="cuda", help="推理设备：cuda或cpu")
    parser.add_argument("--is_half", action="store_true", help="使用半精度推理")
    parser.add_argument("--version", type=str, default="v2", choices=["v1", "v2"], help="模型版本")
    parser.add_argument("--bert_path", type=str, default="GPT_SoVITS/pretrained_models/chinese-roberta-wwm-ext-large",
                        help="BERT模型路径（相对于GPT-SoVITS项目路径）")
    parser.add_argument("--cnhubert_path", type=str, default="GPT_SoVITS/pretrained_models/chinese-hubert-base",
                        help="CNHuBERT模型路径（相对于GPT-SoVITS项目路径）")
    parser.add_argument("--top_k", type=int, default=5, help="Top-K采样参数")
    parser.add_argument("--top_p", type=float, default=1.0, help="Top-P采样参数")
    parser.add_argument("--temperature", type=float, default=1.0, help="温度参数")
    parser.add_argument("--text_split_method", type=str, default="cut5", help="文本切分方法")
    parser.add_argument("--batch_size", type=int, default=1, help="批量推理大小")
    parser.add_argument("--batch_threshold", type=float, default=0.75, help="批量切分阈值")
    parser.add_argument("--split_bucket", action="store_true", default=True, help="是否分桶")
    parser.add_argument("--speed_factor", type=float, default=1.0, help="语速控制因子")
    parser.add_argument("--fragment_interval", type=float, default=0.3, help="音频片段间隔")
    parser.add_argument("--seed", type=int, default=-1, help="随机种子")
    parser.add_argument("--parallel_infer", action="store_true", default=True, help="并行推理")
    parser.add_argument("--repetition_penalty", type=float, default=1.35, help="重复惩罚参数")
    
    args = parser.parse_args()
    
    # 设置sys.path
    setup_sys_path(args.gpt_sovits_path)
    
    try:
        if args.action == "list_models":
            result = scan_models(args.gpt_sovits_path)
        elif args.action == "tts":
            if not args.text:
                result = {"success": False, "message": "text参数不能为空"}
            else:
                result = synthesize_speech(args.gpt_sovits_path, args)
        
        # 输出JSON结果
        print(json.dumps(result, ensure_ascii=False))
        
    except Exception as e:
        import traceback
        error_result = {
            "success": False,
            "message": str(e),
            "traceback": traceback.format_exc()
        }
        print(json.dumps(error_result, ensure_ascii=False))


if __name__ == "__main__":
    main()
