#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import pathlib
import subprocess
import re
import sys
import glob
from collections import defaultdict
from datetime import datetime

class CppcheckAnalyzer:
    def __init__(self):
        self.project_root = self._get_project_root()
        self.log_dir = self.project_root / "logs"
        self.reports_dir = self.project_root / "reports"
        self._setup_directories()

    def _get_project_root(self):
        """动态获取项目根目录（脚本所在目录的上一级）"""
        script_path = pathlib.Path(__file__).resolve()
        return script_path.parent.parent

    def _setup_directories(self):
        """创建必要的目录结构"""
        self.log_dir.mkdir(exist_ok=True)
        self.reports_dir.mkdir(exist_ok=True)

    def _run_cppcheck(self):
        """执行Cppcheck检查"""
        cmd = [
            "cppcheck",
            "--enable=all",
            "--std=c++17",
            "--suppress=missingInclude",
            "--inconclusive",
            "-i", str(self.project_root / "third_party"),
            "-i", str(self.project_root / "build"),
            "-i", str(self.project_root / "cmake"),
            "-i", str(self.project_root / "docs"),
            "-i", str(self.project_root / "examples"),
            "-i", str(self.project_root / "scripts"),
            "-j", "4",
            str(self.project_root / "src"),
            str(self.project_root / "core"),
            str(self.project_root / "main"),
            str(self.project_root / "plugins"),
            str(self.project_root / "resources"),
            str(self.project_root / "tests"),
            str(self.project_root / "lib")
        ]

        report_path = self.log_dir / f"cppcheck_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"

        try:
            with open(report_path, "w", encoding="utf-8") as f:
                subprocess.run(
                    cmd,
                    stdout=f,
                    stderr=subprocess.STDOUT,
                    text=True,
                )
            return report_path
        except FileNotFoundError:
            print("错误: 未找到 cppcheck 命令，请确保已安装 cppcheck", file=sys.stderr)
            sys.exit(1)
        except Exception as e:
            print(f"Cppcheck 执行异常: {str(e)}", file=sys.stderr)
            sys.exit(1)

    def _filter_report(self, input_path, output_path):
        """过滤指定警告信息"""
        # 匹配标准的 cppcheck 警告格式: filename:line:col: severity: message [id]
        warning_pattern = re.compile(r'^.+:\d+:\d+:\s+(\w+):')

        with open(input_path, "r", encoding="utf-8") as infile, \
                open(output_path, "w", encoding="utf-8") as outfile:

            for line in infile:
                line = line.strip()
                if not line:
                    continue

                # 过滤 missingIncludeSystem 类型的警告
                if '[missingIncludeSystem]' in line:
                    continue

                # 保留其他有效警告
                match = warning_pattern.match(line)
                if match:
                    outfile.write(line + '\n')

    def _parse_issues(self, filtered_path):
        """解析过滤后的报告，返回按严重程度分类的问题"""
        issues = {
            'error': [],
            'warning': [],
            'style': [],
            'performance': [],
            'portability': [],
            'information': []
        }

        # 匹配格式: filepath:line:col: severity: message [id]
        # 使用正则表达式，正确处理 Windows 路径
        pattern = re.compile(r'^(.+):(\d+):(\d+):\s+(\w+):\s+(.+)\s+\[([^\]]+)\]$')

        with open(filtered_path, "r", encoding="utf-8") as f:
            for line in f:
                line_stripped = line.strip()
                if not line_stripped:
                    continue

                match = pattern.match(line_stripped)
                if match:
                    filepath, line_no, col, severity, message, error_id = match.groups()
                    severity = severity.lower()

                    # 计算相对路径
                    try:
                        rel_path = pathlib.Path(filepath).relative_to(self.project_root)
                    except:
                        rel_path = pathlib.Path(filepath)

                    issue = {
                        'file': str(rel_path),
                        'line': line_no,
                        'col': col,
                        'message': message,
                        'id': error_id
                    }

                    if severity in issues:
                        issues[severity].append(issue)

        return issues

    def _generate_statistics(self, issues):
        """生成统计信息"""
        stats = {}
        total = 0
        critical = 0
        warnings = 0

        for severity, issue_list in issues.items():
            count = len(issue_list)
            stats[severity] = count
            total += count
            if severity == 'error':
                critical += count
            elif severity == 'warning':
                warnings += count

        stats['total'] = total
        stats['critical'] = critical
        stats['warnings'] = warnings

        return stats

    def _generate_markdown_report(self, issues, stats):
        """生成详细的 Markdown 报告"""

        def format_issue(issue, index):
            return f"""### {index}. `{issue['file']}:{issue['line']}`

```cpp
{issue['message']} [{issue['id']}']
```

"""

        report = f"""# Cppcheck 代码分析报告

生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

## 分析摘要

- **Total Issues**: {stats['total']}
- **ERROR (严重错误)**: {stats.get('error', 0)} {'✅ 全部修复' if stats.get('error', 0) == 0 else ''}
- **WARNING (警告)**: {stats.get('warning', 0)}
- **STYLE (风格问题)**: {stats.get('style', 0)}
- **PERFORMANCE (性能问题)**: {stats.get('performance', 0)}
- **PORTABILITY (可移植性)**: {stats.get('portability', 0)}
- **INFORMATION (信息)**: {stats.get('information', 0)}

---

## 严重错误 (ERROR) - {len(issues['error'])} 个

"""

        if issues['error']:
            for i, issue in enumerate(issues['error'], 1):
                report += format_issue(issue, i)
        else:
            report += "\n✅ **无严重错误**\n\n"

        report += f"""---

## 警告 (WARNING) - {len(issues['warning'])} 个

"""

        if issues['warning']:
            for i, issue in enumerate(issues['warning'], 1):
                report += format_issue(issue, i)
        else:
            report += "\n✅ **无警告**\n\n"

        report += f"""---

## 性能问题 (PERFORMANCE) - {len(issues['performance'])} 个

"""

        if issues['performance']:
            # 按文件分组显示前 20 个
            perf_by_file = defaultdict(list)
            for issue in issues['performance']:
                perf_by_file[issue['file']].append(issue)

            for filepath, file_issues in sorted(perf_by_file.items()):
                report += f"### `{filepath}` ({len(file_issues)} 个问题)\n\n"
                for issue in file_issues[:3]:  # 每个文件最多显示 3 个
                    report += f"- **行 {issue['line']}**: {issue['message']} [`{issue['id']}`]\n"
                if len(file_issues) > 3:
                    report += f"  - *还有 {len(file_issues) - 3} 个类似问题...*\n"
                report += "\n"
        else:
            report += "\n✅ **无性能问题**\n\n"

        report += f"""---

## 风格问题 (STYLE) - {len(issues['style'])} 个

"""

        if issues['style']:
            # 统计常见问题类型
            issue_types = defaultdict(int)
            for issue in issues['style']:
                issue_types[issue['id']] += 1

            report += "### 常见问题统计\n\n"
            for error_id, count in sorted(issue_types.items(), key=lambda x: -x[1])[:10]:
                report += f"- **{error_id}**: {count} 处\n"
            report += "\n"

            # 按文件分组显示
            style_by_file = defaultdict(list)
            for issue in issues['style']:
                style_by_file[issue['file']].append(issue)

            report += "### 问题列表（按文件分组）\n\n"
            for filepath, file_issues in sorted(style_by_file.items())[:20]:  # 最多显示 20 个文件
                report += f"#### `{filepath}` ({len(file_issues)} 个问题)\n\n"
                for issue in file_issues[:5]:  # 每个文件最多显示 5 个
                    report += f"- **行 {issue['line']}**: {issue['message']}\n"
                if len(file_issues) > 5:
                    report += f"  - *还有 {len(file_issues) - 5} 个问题...*\n"
                report += "\n"
        else:
            report += "\n✅ **无风格问题**\n\n"

        report += f"""---

## 信息提示 (INFORMATION) - {len(issues['information'])} 个

主要是 `normalCheckLevelMaxBranches` 提示，表示 cppcheck 为提高分析速度限制了分支分析深度，不影响代码正确性。

"""

        if issues['information']:
            # 只显示非 normalCheckLevelMaxBranches 的信息
            other_info = [i for i in issues['information'] if i['id'] != 'normalCheckLevelMaxBranches']
            if other_info:
                for issue in other_info[:10]:
                    report += format_issue(issue, other_info.index(issue) + 1)

        report += """---

## 修复优先级建议

### 高优先级 (建议修复)

1. **严重错误 (ERROR)** - 会导致编译或运行时问题
2. **警告 (WARNING)** - 可能导致潜在问题

### 中优先级 (可选修复)

- **性能问题 (PERFORMANCE)** - 影响程序性能
- **未使用的变量** - 清理代码

### 低优先级 (代码风格)

- **风格问题 (STYLE)** - 代码规范建议
- 使用 STL 算法替代原始循环
- 函数可以声明为 static 或 const

---

## 总结

"""

        if stats.get('error', 0) == 0:
            report += """✅ **代码质量良好！**

- 所有严重错误已修复
- 剩余问题主要是风格建议和性能优化提示
- 代码可以正常编译和运行
"""
        else:
            report += f"""⚠️ **发现 {stats.get('error', 0)} 个严重错误需要修复**

请优先处理 ERROR 级别的问题，确保代码正确性。
"""

        return report

    def analyze(self):
        """主分析流程"""
        raw_report = self._run_cppcheck()
        filtered_report = self.log_dir / "filtered_cppcheck.txt"

        print(f"正在过滤报告: {raw_report} -> {filtered_report}")
        self._filter_report(raw_report, filtered_report)

        print("正在分析报告...")
        issues = self._parse_issues(filtered_report)
        stats = self._generate_statistics(issues)

        print("生成 Markdown 报告...")
        markdown = self._generate_markdown_report(issues, stats)

        # 保存到项目根目录
        report_path = self.project_root / "report.md"
        with open(report_path, "w", encoding="utf-8") as f:
            f.write(markdown)

        # 同时保存简单统计到脚本目录
        script_dir = pathlib.Path(__file__).parent
        with open(script_dir / "analysis_report.txt", "w", encoding="utf-8") as f:
            f.write(f"""Cppcheck Analysis Report ({datetime.now().strftime('%Y-%m-%d %H:%M:%S')})
=====================================================================
Total Issues: {stats['total']}
Critical Errors: {stats['critical']}
Major Warnings: {stats['warnings']}

Category Breakdown:
""")
            for severity in ['error', 'warning', 'style', 'performance', 'portability', 'information']:
                f.write(f"  {severity.upper()}: {stats.get(severity, 0)}\n")

        print(f"\n分析完成！")
        print(f"- 详细报告: {report_path}")
        print(f"- 统计信息: {script_dir / 'analysis_report.txt'}")
        print(f"\n摘要:")
        print(f"  ERROR: {stats.get('error', 0)}")
        print(f"  WARNING: {stats.get('warning', 0)}")
        print(f"  STYLE: {stats.get('style', 0)}")
        print(f"  PERFORMANCE: {stats.get('performance', 0)}")

if __name__ == "__main__":
    try:
        analyzer = CppcheckAnalyzer()
        analyzer.analyze()
    except KeyboardInterrupt:
        print("\n检测到中断信号，正在清理临时文件...")
        pattern = str(analyzer.log_dir / "cppcheck_*.txt")
        for file in glob.glob(pattern):
            try:
                os.unlink(file)
            except:
                pass
        sys.exit(1)
