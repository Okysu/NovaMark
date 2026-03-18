# NovaMark 源代码

# 核心库
add_subdirectory(core)

# 词法分析器
add_subdirectory(lexer)

# 语法分析器
add_subdirectory(parser)

# AST 节点定义
add_subdirectory(ast)

# 语义分析器
add_subdirectory(semantic)

# 虚拟机
add_subdirectory(vm)

# 打包工具
add_subdirectory(packer)

# 渲染器接口
add_subdirectory(renderer)

# CLI 工具
add_subdirectory(cli)
