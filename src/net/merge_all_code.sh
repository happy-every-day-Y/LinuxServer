#!/bin/bash

# 输出文件名
OUTPUT="all_code.cpp"

# 清空输出文件
echo "// ===== MERGED SOURCE FILE =====" > "$OUTPUT"
echo "// Generated on $(date)" >> "$OUTPUT"
echo >> "$OUTPUT"

# 查找并排序所有源码和头文件
find . \
  -type f \
  \( -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.txt" -o -name "*.cpp" \) \
  | sort \
  | while read -r file; do

    echo "/***************************************/" >> "$OUTPUT"
    echo "/* FILE: $file */" >> "$OUTPUT"
    echo "/***************************************/" >> "$OUTPUT"
    echo >> "$OUTPUT"

    cat "$file" >> "$OUTPUT"

    echo -e "\n\n" >> "$OUTPUT"
done

echo "Done. Output file: $OUTPUT"

