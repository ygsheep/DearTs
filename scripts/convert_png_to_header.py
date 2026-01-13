#!/usr/bin/env python3
"""
PNG 转 C 头文件工具
将 PNG 图片文件转换为 C/C++ 头文件，可以直接嵌入到程序中使用
"""

import os
import sys
import argparse
from pathlib import Path


def png_to_header(input_file, output_file, array_name=None, namespace=None):
    """
    将 PNG 文件转换为 C 头文件

    Args:
        input_file: 输入的 PNG 文件路径
        output_file: 输出的头文件路径
        array_name: 数组名称（默认为文件名）
        namespace: 命名空间（可选）
    """
    # 读取 PNG 文件
    with open(input_file, 'rb') as f:
        data = f.read()

    file_size = len(data)
    print(f"读取文件: {input_file}")
    print(f"文件大小: {file_size} 字节")

    # 生成数组名称
    if array_name is None:
        array_name = Path(input_file).stem.replace('-', '_').replace('.', '_')
    array_name_const = array_name.upper()
    array_name_size = f"{array_name_const}_SIZE"

    # 生成头文件内容
    content = []
    content.append(f"/**")
    content.append(f" * @file {Path(output_file).name}")
    content.append(f" * @brief 自动生成的 PNG 资源数据")
    content.append(f" *")
    content.append(f" * 源文件: {Path(input_file).name}")
    content.append(f" * 大小: {file_size} 字节")
    content.append(f" */")
    content.append("")
    content.append(f"#pragma once")
    content.append("")
    content.append(f"#include <cstddef>")
    content.append(f"#include <cstdint>")
    content.append("")

    # 添加命名空间（如果指定）
    if namespace:
        content.append(f"namespace {namespace} {{")
        content.append("")

    # 数据数组
    content.append(f"// PNG 数据数组")
    content.append(f"constexpr std::uint8_t {array_name}[] = {{")

    # 每行 16 字节
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_values = ", ".join(f"0x{b:02X}" for b in chunk)
        content.append(f"    {hex_values},")

    content.append(f"}};")
    content.append("")

    # 大小常量
    content.append(f"// 数据大小")
    content.append(f"constexpr std::size_t {array_name_size} = {file_size};")
    content.append("")

    # 结束命名空间
    if namespace:
        content.append(f"}} // namespace {namespace}")
        content.append("")

    # 写入文件
    output_path = Path(output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("\n".join(content))

    print(f"生成文件: {output_path}")
    print(f"数组名称: {array_name}")
    print(f"数据大小: {array_name_size} = {file_size}")

    return array_name, array_name_size


def main():
    parser = argparse.ArgumentParser(
        description='将 PNG 文件转换为 C/C++ 头文件',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例用法:
  python convert_png_to_header.py icon.png icon_data.h
  python convert_png_to_header.py icon.png icon_data.h --array-name g_icon --namespace Resources
  python convert_png_to_header.py *.png --output-dir resources/embedded
        """
    )

    parser.add_argument('input', nargs='+', help='输入的 PNG 文件路径（支持多个文件）')
    parser.add_argument('-o', '--output', help='输出的头文件路径（单文件模式）')
    parser.add_argument('-d', '--output-dir', help='输出目录（多文件模式）')
    parser.add_argument('-a', '--array-name', help='数组名称（默认使用文件名）')
    parser.add_argument('-n', '--namespace', help='C++ 命名空间（可选）')

    args = parser.parse_args()

    # 检查参数
    if len(args.input) == 1:
        # 单文件模式
        input_file = Path(args.input[0])
        if not input_file.exists():
            print(f"错误: 文件不存在: {input_file}")
            return 1

        if args.output:
            output_file = args.output
        else:
            output_file = input_file.with_suffix('.h')

        png_to_header(
            str(input_file),
            output_file,
            args.array_name,
            args.namespace
        )
    else:
        # 多文件模式
        if not args.output_dir:
            print("错误: 多文件模式必须指定 --output-dir")
            return 1

        output_dir = Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        for input_file in args.input:
            input_path = Path(input_file)
            if not input_path.exists():
                print(f"警告: 文件不存在，跳过: {input_file}")
                continue

            output_file = output_dir / (input_path.stem + ".h")
            png_to_header(
                str(input_path),
                str(output_file),
                args.array_name,
                args.namespace
            )

    print("\n转换完成！")
    return 0


if __name__ == "__main__":
    sys.exit(main())
