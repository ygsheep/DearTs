#!/bin/bash

# 创建输出目录
mkdir -p "D:/develop/CPlusPlus/Dear_SDL/DearTsd/docs/diagrams"

# 从 markdown 文件中提取 mermaid 图表并生成
cd "D:/develop/CPlusPlus/Dear_SDL/DearTsd/docs"

# 查找所有 mermaid 代码块并编号
counter=1
in_mermaid=0
current_diagram=""

while IFS= read -r line; do
    if [[ "$line" =~ ^\`\`\`mermaid ]]; then
        in_mermaid=1
        current_diagram=""
        filename=$(printf "%02d" $counter)
        continue
    elif [[ "$line" =~ ^\`\`\` ]] && [ $in_mermaid -eq 1 ]; then
        in_mermaid=0

        # 保存 mermaid 代码到文件
        echo "$current_diagram" > "diagrams/${filename}-diagram.mmd"

        # 生成 PNG
        mmdc -i "diagrams/${filename}-diagram.mmd" -o "diagrams/${filename}-diagram.png" -b transparent -s 2

        counter=$((counter + 1))
        continue
    fi

    if [ $in_mermaid -eq 1 ]; then
        current_diagram="$current_diagram$line"$'\n'
    fi
done < "architecture.md"

echo "Generated $((counter - 1)) diagrams in diagrams/"
