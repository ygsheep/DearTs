#!/usr/bin/env python3
import re
import subprocess
import os

# 读取 markdown 文件
md_file = "D:/develop/CPlusPlus/Dear_SDL/DearTsd/docs/architecture.md"
output_dir = "D:/develop/CPlusPlus/Dear_SDL/DearTsd/docs/diagrams"

# 创建输出目录
os.makedirs(output_dir, exist_ok=True)

# 读取文件内容
with open(md_file, 'r', encoding='utf-8') as f:
    content = f.read()

# 提取所有 mermaid 代码块
pattern = r'```mermaid\n(.*?)\n```'
matches = re.findall(pattern, content, re.DOTALL)

print(f"Found {len(matches)} Mermaid diagrams")

# 生成每个图表
for i, mermaid_code in enumerate(matches, 1):
    # 保存 mermaid 代码
    mmd_file = os.path.join(output_dir, f"{i:02d}-diagram.mmd")
    with open(mmd_file, 'w', encoding='utf-8') as f:
        f.write(mermaid_code)

    # 生成 PNG
    png_file = os.path.join(output_dir, f"{i:02d}-diagram.png")

    try:
        subprocess.run([
            'C:\\Users\\ygshe\\AppData\\Roaming\\npm\\mmdc.cmd',
            '-i', mmd_file,
            '-o', png_file,
            '-b', 'transparent',
            '-s', '2'
        ], check=True, capture_output=True, shell=True)
        print(f"Generated: {png_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error generating {png_file}: {e}")
        print(f"stderr: {e.stderr.decode() if e.stderr else 'No stderr'}")

print(f"\nComplete! Generated {len(matches)} diagrams in {output_dir}")
