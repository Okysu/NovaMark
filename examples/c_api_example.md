---
title: C API 示例
description: 使用 NovaMark C API 构建简单的游戏渲染器
author: NovaMark Team
---

# C API 使用示例

本示例演示如何使用 NovaMark C API 加载并运行 .nvmp 游戏包。

## 完整示例代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nova/renderer/nova_c_api.h"

// 简单的渲染状态打印
void print_state(NovaState* state) {
    printf("=== 当前状态 ===\n");
    
    // 背景和音乐
    if (state->bg) printf("背景: %s\n", state->bg);
    if (state->bgm) printf("BGM: %s\n", state->bgm);
    
    // 精灵
    printf("精灵数量: %zu\n", state->spriteCount);
    for (size_t i = 0; i < state->spriteCount; i++) {
        NovaSprite* sp = (NovaSprite*)&state->sprites[i];
        printf("  [%s] url=%s pos=(%.0f,%.0f)\n", 
               sp->id, sp->url, sp->x, sp->y);
    }
    
    // 对话
    if (state->hasDialogue) {
        printf("对话:\n");
        if (state->dialogue.name) {
            printf("  说话人: %s\n", state->dialogue.name);
        }
        printf("  内容: %s\n", state->dialogue.text);
    }
    
    // 选项
    if (state->choiceCount > 0) {
        printf("选项:\n");
        for (size_t i = 0; i < state->choiceCount; i++) {
            NovaChoice* ch = (NovaChoice*)&state->choices[i];
            printf("  [%d] %s%s\n", i, ch->text, 
                   ch->disabled ? " (禁用)" : "");
        }
    }
    
    printf("================\n\n");
}

// 状态变化回调
void on_state_changed(NovaState state, void* user_data) {
    int* step_count = (int*)user_data;
    (*step_count)++;
    printf("\n[步骤 %d]\n", *step_count);
    print_state(&state);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("用法: %s <game.nvmp>\n", argv[0]);
        return 1;
    }
    
    // 创建 VM 实例
    NovaVM* vm = nova_create();
    if (!vm) {
        printf("错误: 无法创建 VM\n");
        return 1;
    }
    
    // 设置状态回调
    int step_count = 0;
    nova_set_state_callback(vm, on_state_changed, &step_count);
    
    // 加载游戏包
    printf("加载游戏包: %s\n\n", argv[1]);
    if (!nova_load_package(vm, argv[1])) {
        printf("错误: %s\n", nova_get_error(vm));
        nova_destroy(vm);
        return 1;
    }
    
    // 推进到第一个可交互状态
    nova_advance(vm);
    
    // 获取初始状态
    NovaState state = nova_get_state(vm);
    print_state(&state);
    
    // 模拟游戏流程
    char input[256];
    while (1) {
        state = nova_get_state(vm);
        
        // 检查是否结束
        if (state.choiceCount == 0 && !state.hasDialogue) {
            printf("\n游戏结束。\n");
            break;
        }
        
        // 如果有选项，让用户选择
        if (state.choiceCount > 0) {
            printf("请输入选项编号 (0-%zu): ", state.choiceCount - 1);
            if (fgets(input, sizeof(input), stdin)) {
                int choice = atoi(input);
                if (choice >= 0 && choice < (int)state.choiceCount) {
                    NovaChoice* ch = (NovaChoice*)&state.choices[choice];
                    if (!ch->disabled) {
                        printf("选择: %s\n\n", ch->text);
                        nova_choose(vm, ch->id);
                    } else {
                        printf("该选项不可用\n");
                    }
                }
            }
        } else {
            // 等待用户继续推进
            printf("按 Enter 继续...");
            fgets(input, sizeof(input), stdin);
            nova_advance(vm);
        }
    }
    
    // 清理
    nova_destroy(vm);
    printf("\nVM 已销毁，程序结束。\n");
    
    return 0;
}
```

## 编译

```bash
# 编译示例
gcc -o c_api_example c_api_example.c \
    -I/path/to/NovaMark/include \
    -L/path/to/NovaMark/build/src/renderer/Release \
    -L/path/to/NovaMark/build/src/vm/Release \
    -L/path/to/NovaMark/build/src/packer/Release \
    -lnova-renderer -lnova-vm -lnova-packer
```

## 运行

```bash
./c_api_example game.nvmp
```

## API 函数说明

### 生命周期管理

| 函数 | 说明 |
|------|------|
| `NovaVM* nova_create()` | 创建 VM 实例 |
| `void nova_destroy(NovaVM* vm)` | 销毁 VM 实例 |

### 游戏加载

| 函数 | 说明 |
|------|------|
| `int nova_load_package(NovaVM* vm, const char* path)` | 从文件加载 .nvmp |
| `int nova_load_from_buffer(NovaVM* vm, const unsigned char* data, size_t size)` | 从内存加载 |

### 游戏控制

| 函数 | 说明 |
|------|------|
| `void nova_advance(NovaVM* vm)` | 推进到下一个离散阻塞点 |
| `void nova_choose(NovaVM* vm, const char* choiceId)` | 选择选项 |

### 状态获取

| 函数 | 说明 |
|------|------|
| `NovaState nova_get_state(NovaVM* vm)` | 获取当前渲染状态 |
| `const char* nova_get_error(NovaVM* vm)` | 获取错误信息 |

### 回调设置

| 函数 | 说明 |
|------|------|
| `void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData)` | 设置状态变化回调 |

### 存档

| 函数 | 说明 |
|------|------|
| `int nova_save_snapshot_file(NovaVM* vm, const char* path)` | 保存当前运行时快照到文件 |
| `int nova_load_snapshot_file(NovaVM* vm, const char* path)` | 从快照文件恢复运行时状态 |

### 变量和背包

| 函数 | 说明 |
|------|------|
| `size_t nova_get_variable_count(NovaVM* vm)` | 获取变量数量 |
| `double nova_get_variable_number(NovaVM* vm, const char* name)` | 获取数值变量 |
| `const char* nova_get_variable_string(NovaVM* vm, const char* name)` | 获取字符串变量 |
| `size_t nova_get_inventory_count(NovaVM* vm, const char* itemId)` | 获取物品数量 |
| `int nova_has_item(NovaVM* vm, const char* itemId)` | 检查是否有物品 |
