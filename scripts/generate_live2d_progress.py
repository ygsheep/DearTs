#!/usr/bin/env python3
"""
Live2D 集成项目进度生成脚本

从 featurelist_live2d.json 生成人类可读的 progress_live2d.md
"""

import json
from datetime import datetime
from pathlib import Path

def load_featurelist():
    """加载功能清单"""
    with open('featurelist_live2d.json', 'r', encoding='utf-8') as f:
        return json.load(f)

def format_status(status):
    """格式化状态图标"""
    icons = {
        'pending': '⏳',
        'in_progress': '🚧',
        'completed': '✅',
        'failed': '❌',
        'blocked': '🔒'
    }
    return icons.get(status, '❓')

def format_test_status(status):
    """格式化测试状态"""
    if status == 'passed':
        return '✅ Passed'
    elif status == 'failed':
        return '❌ Failed'
    else:
        return '⏳ Pending'

def calculate_progress(data):
    """计算进度统计"""
    features = data['features']
    total = len(features)
    completed = sum(1 for f in features if f['implementation_status'] == 'completed')
    passed = sum(1 for f in features if f['test_status'] == 'passed')
    in_progress = sum(1 for f in features if f['implementation_status'] == 'in_progress')

    # 计算预估和实际时间
    total_estimated = sum(f.get('estimated_hours', 0) for f in features)
    total_actual = sum(f.get('actual_hours', 0) or 0 for f in features)

    return {
        'total': total,
        'completed': completed,
        'passed': passed,
        'in_progress': in_progress,
        'total_estimated': total_estimated,
        'total_actual': total_actual,
        'progress_percent': (completed / total * 100) if total > 0 else 0
    }

def generate_progress_report(data):
    """生成进度报告"""
    stats = calculate_progress(data)
    features = data['features']

    # 按阶段分组
    phases = {}
    for f in features:
        phase = f['phase']
        if phase not in phases:
            phases[phase] = []
        phases[phase].append(f)

    # 生成 Markdown
    md = []

    # 标题
    md.append("# Live2D 集成项目进度")
    md.append("")
    md.append(f"**生成时间**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    md.append("")

    # 整体进度
    md.append("## 📊 整体进度")
    md.append("")
    md.append(f"- **总功能数**: {stats['total']}")
    md.append(f"- **已完成**: {stats['completed']} / {stats['total']} ({stats['progress_percent']:.1f}%)")
    md.append(f"- **测试通过**: {stats['passed']} / {stats['total']}")
    md.append(f"- **进行中**: {stats['in_progress']}")
    md.append(f"- **预计时间**: {stats['total_estimated']:.1f} 小时")
    md.append(f"- **实际时间**: {stats['total_actual']:.1f} 小时")
    md.append("")

    # 进度条
    progress_bar = "█" * int(stats['progress_percent'] / 5)
    progress_bar += "░" * (20 - len(progress_bar))
    md.append(f"**进度**: `{progress_bar}` {stats['progress_percent']:.1f}%")
    md.append("")

    # Phase 进度
    md.append("## 🎯 Phase 进度")
    md.append("")

    for phase_num in sorted(phases.keys()):
        phase_features = phases[phase_num]
        total = len(phase_features)
        completed = sum(1 for f in phase_features if f['implementation_status'] == 'completed')
        passed = sum(1 for f in phase_features if f['test_status'] == 'passed')

        phase_name = {
            1: "SDK 集成和基础渲染",
            2: "模型管理和配置系统",
            3: "动画和物理引擎",
            4: "事件系统和交互",
            5: "调试工具和优化",
            6: "跨平台测试和修复"
        }.get(phase_num, f"Phase {phase_num}")

        md.append(f"### Phase {phase_num}: {phase_name}")
        md.append(f"**进度**: {completed} / {total} ({completed/total*100:.1f}%) | **测试通过**: {passed} / {total}")
        md.append("")

        # 功能列表
        for f in phase_features:
            status_icon = format_status(f['implementation_status'])
            test_status = format_test_status(f['test_status'])

            md.append(f"- {status_icon} **{f['id']}** - {f['title']}")
            md.append(f"  - Priority: `{f['priority']}` | Test: {test_status}")

            if f['git_commit']:
                md.append(f"  - Commit: `{f['git_commit']}`")

            if f.get('actual_hours'):
                md.append(f"  - Time: {f['actual_hours']}h (estimated: {f['estimated_hours']}h)")

            if f.get('notes'):
                md.append(f"  - Notes: {f['notes']}")

            md.append("")

    # Git 提交历史（最近）
    md.append("## 🔄 Git 提交历史")
    md.append("")

    committed_features = [f for f in features if f['git_commit']]
    for f in committed_features[-10:]:  # 最近 10 个
        status_icon = format_status(f['implementation_status'])
        test_status = format_test_status(f['test_status'])

        md.append(f"### [{f['id']}] {f['title']}")
        md.append(f"**Commit**: `{f['git_commit']}`")
        md.append(f"**Status**: {status_icon} {f['implementation_status']} | {test_status}")

        if f.get('actual_hours'):
            md.append(f"**Duration**: {f['actual_hours']}h")

        md.append("")

    # 待完成任务
    md.append("## 📝 待完成任务")
    md.append("")

    pending_features = [f for f in features if f['implementation_status'] == 'pending']
    in_progress_features = [f for f in features if f['implementation_status'] == 'in_progress']

    if in_progress_features:
        md.append("### 进行中")
        for f in in_progress_features:
            md.append(f"- 🚧 **{f['id']}** - {f['title']}")
        md.append("")

    if pending_features:
        md.append("### 待开始")
        # 只显示前 10 个
        for f in pending_features[:10]:
            md.append(f"- ⏳ **{f['id']}** - {f['title']} ({f['estimated_hours']}h)")

        if len(pending_features) > 10:
            md.append(f"- ... 还有 {len(pending_features) - 10} 个任务")
        md.append("")

    # 统计信息
    md.append("## 📈 项目统计")
    md.append("")
    md.append(f"- **Phase 数量**: {len(phases)}")
    md.append(f"- **优先级分布**:")

    priority_counts = {}
    for f in features:
        prio = f['priority']
        priority_counts[prio] = priority_counts.get(prio, 0) + 1

    for prio in ['P0', 'P1', 'P2']:
        if prio in priority_counts:
            md.append(f"  - {prio}: {priority_counts[prio]} 项")

    md.append("")
    md.append("---")
    md.append("")
    md.append("**生成工具**: `scripts/generate_live2d_progress.py`")
    md.append(f"**数据源**: `featurelist_live2d.json` (v{data['version']})")
    md.append("")

    return "\n".join(md)

def main():
    """主函数"""
    print("📊 正在生成 Live2D 集成项目进度报告...")

    # 加载数据
    try:
        data = load_featurelist()
    except FileNotFoundError:
        print("❌ 错误: 找不到 featurelist_live2d.json")
        return 1
    except json.JSONDecodeError as e:
        print(f"❌ 错误: featurelist_live2d.json JSON 格式错误: {e}")
        return 1

    # 生成报告
    report = generate_progress_report(data)

    # 保存文件
    output_path = Path('progress_live2d.md')
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(report)

    print(f"✅ progress_live2d.md 已更新")
    print(f"   生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"   总功能数: {data['statistics']['total_features']}")
    print(f"   预计时间: {data['statistics']['total_estimated_hours']:.1f} 小时")

    return 0

if __name__ == '__main__':
    exit(main())
