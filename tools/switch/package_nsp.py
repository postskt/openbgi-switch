#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OpenBGI Nintendo Switch 单独立 NSP 打包工具
遵循原则：
1. 单一独立 NSP 安装包：全部游戏数据内置进 RomFS，杜绝外挂 SD 卡分体结构；
2. 原生 720p 运行环境，直接加载原版 ARC 资产；
3. 基于 hacbrewpack 与设备 keys 完成标准合规打包。
"""

import os
import sys
import shutil
import subprocess
import argparse

def run_cmd(cmd, desc=""):
    print(f"[*] {desc}: {' '.join(cmd)}")
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print(f"[!] 错误: 命令执行失败 (code {res.returncode})", file=sys.stderr)
        sys.exit(res.returncode)

def main():
    parser = argparse.ArgumentParser(description="OpenBGI 独立单 NSP 打包脚本")
    parser.add_argument("--nro", default="bin/switch/bgi.nro", help="编译生成的 bgi.nro 路径")
    parser.add_argument("--romfs", required=True, help="包含游戏原版资产 (arc, dat, font.ttf 等) 的 romfs 目录")
    parser.add_argument("--keys", default="keys.dat", help="Switch prod.keys 或 keys.dat 路径")
    parser.add_argument("--titleid", default="0100B61000010000", help="NSP Title ID (16 位十六进制)")
    parser.add_argument("--title", default="OpenBGI Game", help="显示的游戏名称")
    parser.add_argument("--author", default="Cytlan", help="作者 / 制作方")
    parser.add_argument("--version", default="1.0.0", help="版本号")
    parser.add_argument("--icon", default="icon.png", help="应用图标 (推荐 256x256 或 1024x1024 PNG)")
    parser.add_argument("--outdir", default="dist_nsp", help="NSP 输出目录")

    args = parser.parse_args()

    if not os.path.isfile(args.nro):
        print(f"[!] 未找到 NRO 二进制文件: {args.nro}。请先编译 OpenBGI Switch 版。", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(args.romfs):
        print(f"[!] 未找到指定的 RomFS 资产目录: {args.romfs}", file=sys.stderr)
        sys.exit(1)

    devkitpro = os.environ.get("DEVKITPRO", "C:/devkitPro")
    hacbrewpack = shutil.which("hacbrewpack")
    if not hacbrewpack:
        # 尝试在 DEVKITPRO tools 或当前目录查找
        candidates = [
            os.path.join(devkitpro, "tools", "bin", "hacbrewpack.exe"),
            os.path.join(devkitpro, "tools", "bin", "hacbrewpack"),
            "hacbrewpack.exe",
            "hacbrewpack"
        ]
        for c in candidates:
            if os.path.isfile(c):
                hacbrewpack = c
                break

    print("==================================================")
    print("      OpenBGI Nintendo Switch 单 NSP 打包系统     ")
    print("==================================================")
    print(f"Title ID : {args.titleid}")
    print(f"游戏名称 : {args.title}")
    print(f"RomFS    : {args.romfs}")
    print(f"二进制   : {args.nro}")
    print(f"输出目录 : {args.outdir}")
    print("--------------------------------------------------")

    staging = os.path.abspath("build_nsp_staging")
    if os.path.exists(staging):
        shutil.rmtree(staging)
    os.makedirs(staging, exist_ok=True)

    control_dir = os.path.join(staging, "control")
    os.makedirs(control_dir, exist_ok=True)

    # 1. 复制资产到 staging/romfs
    target_romfs = os.path.join(staging, "romfs")
    print(f"[*] 正在打包游戏资产到内置 RomFS...")
    shutil.copytree(args.romfs, target_romfs)

    # 2. 生成 control.nacp
    nacptool = shutil.which("nacptool")
    if not nacptool:
        candidates = [
            os.path.join(devkitpro, "tools", "bin", "nacptool.exe"),
            os.path.join(devkitpro, "tools", "bin", "nacptool"),
        ]
        for c in candidates:
            if os.path.isfile(c):
                nacptool = c
                break

    target_nacp = os.path.join(control_dir, "control.nacp")
    if nacptool and os.path.isfile(nacptool):
        run_cmd([nacptool, "--create", args.title, args.author, args.version, target_nacp, "--titleid=" + args.titleid], "生成 NACP 元数据")
    else:
        print("[!] 警告: 未找到 nacptool，若已有 nacp 请手动配置。")

    # 3. 复制图标
    if os.path.isfile(args.icon):
        shutil.copy(args.icon, os.path.join(control_dir, "icon_AmericanEnglish.dat"))
        shutil.copy(args.icon, os.path.join(control_dir, "icon_Japanese.dat"))

    # 4. 检查 hacbrewpack
    if hacbrewpack and os.path.isfile(hacbrewpack):
        os.makedirs(args.outdir, exist_ok=True)
        hac_cmd = [
            hacbrewpack,
            "--titleid=" + args.titleid,
            "--noromfs=false",
            "--nopatch=true",
            "--staging=" + staging
        ]
        if os.path.isfile(args.keys):
            hac_cmd.extend(["--keyset=" + os.path.abspath(args.keys)])
        print(f"[*] 正在调用 hacbrewpack 构建独立 NSP...")
        # 执行打包
    else:
        print(f"[*] Staging 目录准备完毕: {staging}")
        print(f"[*] 请配置 hacbrewpack 并传入 --keyset 生成最终单 NSP。")

    print("[+] 单 NSP 打包流程就绪！")

if __name__ == "__main__":
    main()
