#!/usr/bin/env python3
"""
自动生成 progress.md

用法:
    python scripts/generate_progress.py

这个脚本会读取 featurelist.json 并生成人类可读的 progress.md
"""

import json
from datetime import datetime
from pathlib import Path

def load_featurelist():
    """读取 featurelist.json"""
    featurelist_path = Path(__file__).parent.parent / "featurelist.json"
    with open(featurelist_path, 'r', encoding='utf-8') as f:
        return json.load(f)

def calculate_statistics(data):
    """计算统计数据"""
    features = data['features']
    total = len(features)
    completed = sum(1 for f in features if f['test_status'] == 'passed')
    passed = sum(1 for f in features if f['test_status'] == 'passed')
    total_estimated = sum(f.get('estimated_hours', 0) for f in features)
    total_actual = sum(f.get('actual_hours', 0) for f in features if f.get('actual_hours'))

    return {
        'total_features': total,
        'completed_features': completed,
        'passed_tests': passed,
        'total_estimated_hours': total_estimated,
        'total_actual_hours': total_actual,
        'progress_percentage': (completed / total * 100) if total > 0 else 0.0
    }

def get_phase_features(features, phase):
    """获取指定 Phase 的功能"""
    return [f for f in features if f['phase'] == phase]

def generate_phase_section(phase_num, features):
    """生成 Phase 进度部分"""
    phase_features = get_phase_features(features, phase_num)
    completed = sum(1 for f in phase_features if f['test_status'] == 'passed')
    total = len(phase_features)
    status = "✅ 已完成" if completed == total else "⏳ 进行中" if completed > 0 else "⏳ 未开始"

    section = f"""
### Phase {phase_num}: {get_phase_name(phase_num)} ({completed}/{total} 完成)
**状态**: {status}

"""

    for f in sorted(phase_features, key=lambda x: x['id']):
        impl_status = f['implementation_status']
        test_status = f['test_status']

        # 选择图标
        if test_status == 'passed':
            icon = '✅'
        elif impl_status == 'completed':
            icon = '⚠️'
        elif impl_status == 'in_progress':
            icon = '🔄'
        else:
            icon = '⏳'

        section += f"- {icon} **{f['id']}** - {f['title']} ({f.get('estimated_hours', 0)}h)\n"

        # 添加提交信息
        if f.get('git_commit'):
            section += f"  - Commit: `{f['git_commit']}`\n"

        # 添加实际时间
        if f.get('actual_hours'):
            section += f"  - 实际时间: {f['actual_hours']}h\n"

        # 添加注释
        if f.get('notes'):
            section += f"  - 注: {f['notes']}\n"

        section += "\n"

    return section

def get_phase_name(phase_num):
    """获取 Phase 名称"""
    names = {
        1: "基础设施",
        2: "元数据获取",
        3: "媒体播放器",
        4: "转码功能",
        5: "应用集成"
    }
    return names.get(phase_num, f"Phase {phase_num}")

def generate_git_history(features):
    """生成 Git 提交历史"""
    section = "## 🔄 Git 提交历史\n\n"

    completed_features = [f for f in features if f.get('git_commit')]

    if not completed_features:
        section += "### 尚无提交\n\n等待编码代理开始实施...\n\n"
        return section

    for f in completed_features:
        section += f"### [{f['id']}] {f['title']}\n"
        section += f"**Commit**: `{f['git_commit']}`\n"
        section += f"**Status**: {f['implementation_status']} & {f['test_status']}\n"

        if f.get('actual_hours'):
            section += f"**Duration**: {f['actual_hours']}h\n"

        section += "\n"

    return section

def generate_progress_md():
    """生成 progress.md"""
    data = load_featurelist()
    stats = calculate_statistics(data)
    features = data['features']

    md = f"""# FFmpeg 集成项目进度

## 📊 整体进度

- **完成功能**: {stats['completed_features']} / {stats['total_features']} ({stats['progress_percentage']:.1f}%)
- **通过测试**: {stats['passed_tests']} / {stats['total_features']}
- **预计时间**: {stats['total_estimated_hours']:.1f} 小时
- **实际时间**: {stats['total_actual_hours']:.1f} 小时
- **最后更新**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

---

## 📅 项目时间线

### 最近更新

#### {datetime.now().strftime('%Y-%m-%d %H:%M')} - 自动生成
progress.md 由 `generate_progress.py` 自动生成
- 完成功能: {stats['completed_features']} / {stats['total_features']}
- 进度: {stats['progress_percentage']:.1f}%

"""

    # 添加 Phase 进度
    md += "\n---\n\n## 🎯 Phase 进度\n\n"

    for phase in range(1, 6):
        md += generate_phase_section(phase, features)

    # 添加 Git 历史
    md += "\n---\n\n"
    md += generate_git_history(features)

    # 添加快速开始
    md += """
---

## 🚀 快速开始

### 查看当前状态
```bash
# 查看功能清单
cat featurelist.json | jq '.statistics'

# 查看进度
cat progress.md

# 查看最近提交
git log --oneline -10
```

### 更新进度
```bash
# 更新 featurelist.json 后运行
python scripts/generate_progress.py

# 提交更新
git add progress.md
git commit -m "chore: update progress"
```

---

## 📊 优先级矩阵

| 优先级 | 数量 | 预计时间 | 完成数 |
|--------|------|----------|--------|
"""

    # 统计优先级
    priority_stats = {}
    for f in features:
        priority = f['priority']
        if priority not in priority_stats:
            priority_stats[priority] = {'count': 0, 'hours': 0, 'completed': 0}
        priority_stats[priority]['count'] += 1
        priority_stats[priority]['hours'] += f.get('estimated_hours', 0)
        if f['test_status'] == 'passed':
            priority_stats[priority]['completed'] += 1

    for priority in ['P0', 'P1', 'P2']:
        if priority in priority_stats:
            stats_p = priority_stats[priority]
            md += f"| {priority} | {stats_p['count']} | {stats_p['hours']:.1f}h | {stats_p['completed']} |\n"

    # 添加里程碑
    md += "\n---\n\n## 🎯 里程碑\n\n"
    phases = [
        (1, "基础设施就绪"),
        (2, "元数据提取 MVP"),
        (3, "媒体播放器完成"),
        (4, "转码功能完成"),
        (5, "项目交付")
    ]

    for phase_num, desc in phases:
        phase_features = get_phase_features(features, phase_num)
        completed = sum(1 for f in phase_features if f['test_status'] == 'passed')
        total = len(phase_features)
        icon = "✅" if completed == total else "⏳"
        md += f"- {icon} **M{phase_num}**: Phase {phase_num} 完成 - {desc} ({completed}/{total})\n"

    # 添加待解决问题
    md += "\n---\n\n## 🔍 待解决问题\n\n"

    # 查找阻塞的任务
    blocked_tasks = [f for f in features if f['implementation_status'] == 'blocked']
    if blocked_tasks:
        for task in blocked_tasks:
            md += f"### ⚠️ [{task['id']}] {task['title']}\n"
            if task.get('notes'):
                md += f"{task['notes']}\n"
            md += "\n"
    else:
        md += "### 无当前问题\n\n所有任务进展顺利...\n"

    # 添加页脚
    md += f"""
---

**文档版本**: 1.0.0
**生成时间**: {datetime.now().isoformat()}
**生成者**: generate_progress.py
"""

    return md

if __name__ == '__main__':
    md_content = generate_progress_md()

    # 写入 progress.md
    progress_path = Path(__file__).parent.parent / "progress.md"
    with open(progress_path, 'w', encoding='utf-8') as f:
        f.write(md_content)

    print(f"✅ progress.md 已更新")
    print(f"   生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
