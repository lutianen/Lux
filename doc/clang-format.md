# clang-format

### USAGE
`clang-format [options] [<file> ...]`

- `clang-fromat -i <file>` - 格式化结果直接写到相应的文件中

    `clang-format -i LuxUtils/src/* LuxUtils/include/LuxUtils/*`

### .clang-format
``` clang-format
# 用于所有没有特殊指定配置的选项
BasedOnStyle: Google
# 这种格式针对的是语言
Language: Cpp
# 用于缩进的列数
IndentWidth: 4
# 如果为 true, 分析最常见的格式化文件中“&”和“\*”的对齐方式
DerivePointerAlignment: true
# 指针和引用的对其方式
PointerAlignment: Left
# 访问修饰符的缩进或者向外伸展
AccessModifierOffset: -4
FixNamespaceComments: true
# 如果为true, 在模板声明“template<...>”后总是换行
AlwaysBreakTemplateDeclarations: true
# 限制列
ColumnLimit: 80
# 如果为true, 对齐注释
AlignTrailingComments: true
# 在结果文件中使用制表符字符的方式
UseTab: Never
```