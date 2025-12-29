#!/usr/bin/env python3
"""
DearTs Framework Build Script
编译脚本，支持错误解析、去重和快速定位关键错误
"""

import os
import sys
import subprocess
import re
import argparse
from pathlib import Path
from typing import List, Dict, Set, Tuple
from collections import defaultdict
from datetime import datetime


class Colors:
    """终端颜色代码"""
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

    @classmethod
    def disable(cls):
        """禁用颜色输出（用于非终端环境）"""
        cls.RED = ''
        cls.GREEN = ''
        cls.YELLOW = ''
        cls.BLUE = ''
        cls.MAGENTA = ''
        cls.CYAN = ''
        cls.WHITE = ''
        cls.BOLD = ''
        cls.RESET = ''


class BuildError:
    """编译错误信息"""

    def __init__(self, file: str, line: int, error_type: str, message: str, code: str = ""):
        self.file = file
        self.line = line
        self.error_type = error_type  # 'error', 'warning', 'note'
        self.message = message.strip()
        self.code = code.strip()  # 相关代码片段

    def __hash__(self):
        return hash((self.file, self.line, self.message))

    def __eq__(self, other):
        if not isinstance(other, BuildError):
            return False
        return (self.file, self.line, self.message) == (other.file, other.line, other.message)

    def __str__(self):
        location = f"{self.file}:{self.line}" if self.line > 0 else self.file
        return f"{location}: {self.error_type}: {self.message}"


class BuildParser:
    """编译输出解析器"""

    # MSVC 错误格式
    MSVC_PATTERN = re.compile(
        r'^(?P<file>[^<(]+?)[(<](?P<line>\d+)[>):]*\s*:\s*'
        r'(?P<type>error|warning|fatal error)\s*'
        r'(?P<code>\w+\d+)\s*:\s*'
        r'(?P<message>.+)$',
        re.IGNORECASE
    )

    # GCC/Clang 错误格式
    GCC_PATTERN = re.compile(
        r'^(?P<file>[^:]+):(?P<line>\d+):(?P<col>\d+):\s*'
        r'(?P<type>\w+):\s*'
        r'(?P<message>.+)$'
    )

    # CMake 错误格式
    CMAKE_PATTERN = re.compile(
        r'CMake (Error|Warning) at (?P<file>[^:]+):(?P<line>\d+) '
    )

    def __init__(self):
        self.errors: List[BuildError] = []
        self.warnings: List[BuildError] = []
        self.error_lines: Set[str] = set()  # 用于快速去重

    def parse_line(self, line: str) -> BuildError:
        """解析单行输出"""
        # 尝试 MSVC 格式
        match = self.MSVC_PATTERN.match(line)
        if match:
            error = BuildError(
                file=match.group('file').strip(),
                line=int(match.group('line')),
                error_type=match.group('type').lower(),
                message=match.group('message'),
                code=match.group('code')
            )
            return error

        # 尝试 GCC/Clang 格式
        match = self.GCC_PATTERN.match(line)
        if match:
            error_type = match.group('type').lower()
            if 'error' in error_type or 'warning' in error_type:
                return BuildError(
                    file=match.group('file').strip(),
                    line=int(match.group('line')),
                    error_type=error_type,
                    message=match.group('message')
                )

        # 尝试 CMake 格式
        match = self.CMAKE_PATTERN.search(line)
        if match:
            return BuildError(
                file=match.group('file'),
                line=int(match.group('line')),
                error_type='error',
                message=line
            )

        return None

    def parse_output(self, output: str) -> Tuple[List[BuildError], List[BuildError]]:
        """解析编译输出"""
        self.errors.clear()
        self.warnings.clear()
        self.error_lines.clear()

        lines = output.split('\n')
        i = 0

        while i < len(lines):
            line = lines[i].strip()
            if not line:
                i += 1
                continue

            error = self.parse_line(line)
            if error:
                # 去重：检查是否已存在相同的错误
                error_key = str(error)
                if error_key not in self.error_lines:
                    self.error_lines.add(error_key)

                    # 尝试获取代码片段（下一行可能是代码）
                    if i + 1 < len(lines):
                        next_line = lines[i + 1].strip()
                        if next_line and not self.parse_line(next_line):
                            error.code = next_line

                    if 'error' in error.error_type or 'fatal' in error.error_type:
                        self.errors.append(error)
                    elif 'warning' in error.error_type:
                        self.warnings.append(error)

            i += 1

        return self.errors, self.warnings


class BuildRunner:
    """构建执行器"""

    def __init__(self, build_dir: Path, target: str = None, config: str = "Debug"):
        self.build_dir = Path(build_dir)
        self.target = target
        self.config = config
        self.parser = BuildParser()

    def build(self) -> Tuple[int, str, str]:
        """执行构建"""
        if not self.build_dir.exists():
            print(f"{Colors.YELLOW}构建目录不存在: {self.build_dir}{Colors.RESET}")
            return 1, "", "Build directory not found"

        cmd = ["cmake", "--build", str(self.build_dir)]

        if self.target:
            cmd.extend(["--target", self.target])

        cmd.extend(["--config", self.config])

        print(f"{Colors.CYAN}执行构建命令:{Colors.RESET}")
        print(f"{Colors.BOLD}{' '.join(cmd)}{Colors.RESET}\n")

        start_time = datetime.now()

        try:
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,  # 行缓冲
                universal_newlines=True
            )

            output_lines = []
            for line in process.stdout:
                output_lines.append(line)
                print(line, end='')  # 实时输出

            process.wait()
            output = ''.join(output_lines)

            elapsed = (datetime.now() - start_time).total_seconds()

            if process.returncode == 0:
                print(f"\n{Colors.GREEN}{'='*60}{Colors.RESET}")
                print(f"{Colors.GREEN}构建成功! 耗时: {elapsed:.2f}秒{Colors.RESET}")
                print(f"{Colors.GREEN}{'='*60}{Colors.RESET}")
            else:
                print(f"\n{Colors.RED}{'='*60}{Colors.RESET}")
                print(f"{Colors.RED}构建失败，返回码: {process.returncode}{Colors.RESET}")
                print(f"{Colors.RED}{'='*60}{Colors.RESET}")

            return process.returncode, output, ""

        except FileNotFoundError:
            error = "错误: 找不到 cmake 命令，请确保已安装 CMake 并添加到 PATH"
            print(f"{Colors.RED}{error}{Colors.RESET}")
            return 1, "", error
        except Exception as e:
            error = f"构建异常: {e}"
            print(f"{Colors.RED}{error}{Colors.RESET}")
            return 1, "", str(e)


class ErrorReporter:
    """错误报告器"""

    @staticmethod
    def print_summary(errors: List[BuildError], warnings: List[BuildError]):
        """打印错误摘要"""
        print(f"\n{Colors.BOLD}{'='*60}{Colors.RESET}")
        print(f"{Colors.BOLD}构建摘要{Colors.RESET}")
        print(f"{Colors.BOLD}{'='*60}{Colors.RESET}")

        if errors:
            print(f"{Colors.RED}  错误: {len(errors)}{Colors.RESET}")
        else:
            print(f"{Colors.GREEN}  错误: 0{Colors.RESET}")

        if warnings:
            print(f"{Colors.YELLOW}  警告: {len(warnings)}{Colors.RESET}")
        else:
            print(f"  警告: 0")

        print(f"{Colors.BOLD}{'='*60}{Colors.RESET}\n")

    @staticmethod
    def print_errors(errors: List[BuildError], show_code: bool = True):
        """打印错误详情"""
        if not errors:
            print(f"{Colors.GREEN}没有发现编译错误！{Colors.RESET}\n")
            return

        print(f"{Colors.RED}{Colors.BOLD}发现 {len(errors)} 个错误:{Colors.RESET}\n")

        # 按文件分组
        errors_by_file: Dict[str, List[BuildError]] = defaultdict(list)
        for error in errors:
            errors_by_file[error.file].append(error)

        for file, file_errors in sorted(errors_by_file.items()):
            print(f"{Colors.MAGENTA}{Colors.BOLD}文件: {file}{Colors.RESET}")
            print(f"{Colors.MAGENTA}{'-' * 60}{Colors.RESET}")

            for error in file_errors:
                location = f"{Colors.CYAN}{error.file}:{error.line}{Colors.RESET}" if error.line > 0 else error.file
                print(f"  {location}")
                print(f"  {Colors.RED}{error.error_type.upper()}: {error.message}{Colors.RESET}")

                if show_code and error.code:
                    print(f"  {Colors.YELLOW}代码: {error.code}{Colors.RESET}")
                print()

    @staticmethod
    def print_key_errors(errors: List[BuildError], limit: int = 20):
        """打印关键错误（快速定位）"""
        if not errors:
            return

        print(f"{Colors.RED}{Colors.BOLD}关键错误 (前 {min(limit, len(errors))} 个):{Colors.RESET}\n")

        # 提取唯一错误消息
        unique_messages: Dict[str, List[BuildError]] = defaultdict(list)
        for error in errors:
            # 简化错误消息（去除行号等细节）
            simplified = re.sub(r'\d+', 'N', error.message)
            simplified = re.sub(r'[/\\][^/\\]+', '/...', simplified)
            unique_messages[simplified].append(error)

        print(f"{Colors.BOLD}去重后的关键错误 ({len(unique_messages)} 种):{Colors.RESET}\n")

        for i, (msg, error_list) in enumerate(sorted(unique_messages.items())[:limit], 1):
            error = error_list[0]
            location = f"{error.file}:{error.line}" if error.line > 0 else error.file

            print(f"{Colors.RED}{i}. {location}{Colors.RESET}")
            print(f"   {msg}")
            if error_list:
                print(f"   {Colors.YELLOW}出现 {len(error_list)} 次{Colors.RESET}")
            print()

    @staticmethod
    def print_warnings(warnings: List[BuildError], limit: int = 10):
        """打印警告（限制数量）"""
        if not warnings:
            return

        print(f"{Colors.YELLOW}{Colors.BOLD}警告 (前 {min(limit, len(warnings))} 个):{Colors.RESET}\n")

        for warning in sorted(warnings, key=lambda w: w.file)[:limit]:
            location = f"{Colors.CYAN}{warning.file}:{warning.line}{Colors.RESET}" if warning.line > 0 else warning.file
            print(f"  {location}")
            print(f"  {Colors.YELLOW}warning: {warning.message}{Colors.RESET}\n")

        if len(warnings) > limit:
            print(f"{Colors.YELLOW}... 还有 {len(warnings) - limit} 个警告{Colors.RESET}\n")

    @staticmethod
    def export_errors(errors: List[BuildError], warnings: List[BuildError], output_file: Path):
        """导出错误到文件"""
        try:
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(f"构建报告 - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write(f"{'='*60}\n\n")
                f.write(f"错误数: {len(errors)}\n")
                f.write(f"警告数: {len(warnings)}\n\n")

                if errors:
                    f.write(f"\n{'='*60}\n")
                    f.write(f"错误详情\n")
                    f.write(f"{'='*60}\n\n")
                    for error in errors:
                        f.write(f"{error.file}:{error.line}: {error.error_type}: {error.message}\n")
                        if error.code:
                            f.write(f"  代码: {error.code}\n")
                        f.write("\n")

                if warnings:
                    f.write(f"\n{'='*60}\n")
                    f.write(f"警告详情\n")
                    f.write(f"{'='*60}\n\n")
                    for warning in warnings:
                        f.write(f"{warning.file}:{warning.line}: {warning.message}\n")
                        f.write("\n")

            print(f"{Colors.GREEN}错误报告已导出到: {output_file}{Colors.RESET}")
        except Exception as e:
            print(f"{Colors.YELLOW}导出失败: {e}{Colors.RESET}")


def main():
    parser = argparse.ArgumentParser(
        description='DearTs Framework 构建脚本',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s                           # 使用默认配置构建（deartsdl_gui）
  %(prog)s -t demo_imhex_style       # 构建指定目标
  %(prog)s -c Release                # Release 模式构建
  %(prog)s --no-color                # 禁用彩色输出
  %(prog)s --export-errors           # 导出错误到文件
  %(prog)s --all-errors              # 显示所有详细错误（默认仅显示去重后的关键错误）
        """
    )

    parser.add_argument('-t', '--target', default='deartsdl_gui',
                       help='构建目标 (默认: deartsdl_gui)')
    parser.add_argument('-c', '--config', choices=['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'],
                       default='Debug', help='构建配置 (默认: Debug)')
    parser.add_argument('-b', '--build-dir', type=Path, default=Path('build'),
                       help='构建目录 (默认: build)')
    parser.add_argument('--no-color', action='store_true', help='禁用彩色输出')
    parser.add_argument('--show-code', action='store_true', help='显示错误相关代码')
    parser.add_argument('--export-errors', action='store_true', help='导出错误到文件')
    parser.add_argument('--all-errors', action='store_true', help='显示所有详细错误（默认仅显示去重后的关键错误）')
    parser.add_argument('--max-warnings', type=int, default=10, help='显示的最大警告数 (默认: 10)')
    parser.add_argument('--max-key-errors', type=int, default=20, help='显示的最大关键错误数 (默认: 20)')

    args = parser.parse_args()

    # 默认启用去重显示，除非用户明确要求显示所有错误
    key_only = not args.all_errors

    # 禁用颜色（如果指定或非终端）
    if args.no_color or not sys.stdout.isatty():
        Colors.disable()

    print(f"{Colors.CYAN}{Colors.BOLD}{'='*60}{Colors.RESET}")
    print(f"{Colors.CYAN}{Colors.BOLD}DearTs Framework 构建脚本{Colors.RESET}")
    print(f"{Colors.CYAN}{Colors.BOLD}{'='*60}{Colors.RESET}\n")

    # 执行构建
    runner = BuildRunner(args.build_dir, args.target, args.config)
    returncode, output, error_msg = runner.build()

    # 解析错误
    if returncode != 0:
        errors, warnings = runner.parser.parse_output(output)

        # 打印摘要
        ErrorReporter.print_summary(errors, warnings)

        if key_only:
            # 默认仅显示关键错误（去重后）
            ErrorReporter.print_key_errors(errors, args.max_key_errors)
        else:
            # 显示完整错误信息
            ErrorReporter.print_errors(errors, args.show_code)
            ErrorReporter.print_warnings(warnings, args.max_warnings)

        # 导出错误
        if args.export_errors:
            output_file = Path(f"build_errors_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt")
            ErrorReporter.export_errors(errors, warnings, output_file)

        sys.exit(returncode)
    else:
        # 构建成功，检查是否有警告
        errors, warnings = runner.parser.parse_output(output)
        if warnings:
            print(f"\n{Colors.YELLOW}构建成功，但有 {len(warnings)} 个警告:{Colors.RESET}\n")
            ErrorReporter.print_warnings(warnings, args.max_warnings)
        else:
            print(f"{Colors.GREEN}构建完全成功，没有警告！{Colors.RESET}")

        sys.exit(0)


if __name__ == '__main__':
    main()
