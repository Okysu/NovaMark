# **NovaMark 引擎架构与渲染指南 (v1.0)**

本文档定义了 NovaMark 引擎的底层架构、编译打包流程以及多语言渲染器的接入规范。NovaMark 采用了\*\*“C++ 核心虚拟机 (VM) \+ 多端哑渲染器 (Dumb Renderer)”\*\*的顶级架构设计，确保核心逻辑的绝对一致性与跨平台发布的极简性。

## **1\. 核心架构概述**

NovaMark 引擎严格遵循 **状态驱动 (State-Driven)** 的理念：

* **NovaVM (C++ 核心):** 负责读取编译后的 AST，管理所有内存、变量运算、条件判断、音视频播放逻辑、时间线控制。它是一个黑盒状态机。  
* **Renderer (各端渲染器):** 无论是 Web, iOS, 鸿蒙还是桌面端，前端不包含任何“游戏逻辑”。前端每一帧只向 VM 询问：“当前画面应该是什么样？”，然后如实将 VM 返回的数据结构绘制在屏幕上。前端只负责两件事：**画图**和**传递用户点击输入**。

## **2\. 工程管理与多 .nvm 文件支持**

在实际的游戏开发中，剧本可能会多达数十万字，必须拆分成多个 .nvm 文件。

### **2.1 目录结构标准**

一个典型的 NovaMark 工程目录如下：

MyGameProject/  
├── project.yaml          \# 全局配置文件（定义入口场景等）  
├── scripts/              \# 剧本文件夹  
│   ├── 00\_init.nvm       \# 定义全局变量、@char、@item  
│   ├── 01\_prologue.nvm   \# 序章场景  
│   └── 02\_forest.nvm     \# 森林场景  
└── assets/               \# 资源文件夹  
    ├── bg/  
    ├── sprites/  
    └── audio/

### **2.2 跨文件引用与全局命名空间**

* **场景 ID 全局唯一：** 引擎在编译时，会遍历 scripts/ 下的所有 .nvm 文件。所有的 \#scene\_id 会被提取到一个**全局场景注册表**中。  
* **无缝跳转：** 因此，你可以在 01\_prologue.nvm 中直接使用 \-\> scene\_forest 跳转到 02\_forest.nvm 中定义的场景，开发者完全不需要写繁琐的 import 语句。  
* **变量共享：** 所有的 @var 和 @item 也是全局生效的。

## **3\. 编译与打包机制 (The Packer)**

为了保护资源不被轻易提取、剧本不被篡改，以及提升加载速度，引擎提供了一个构建工具：nova-cli build。

### **3.1 编译流程**

1. **词法与语法分析 (Parsing):** 将所有 .nvm 文件转化为一棵巨大的抽象语法树 (AST)。  
2. **语义检查 (Validation):** 检查是否存在死链接（跳转到不存在的场景）、未定义的变量或角色。  
3. **资源打包 (Asset Bundling):** 压缩图片和音频资源。  
4. **生成 NVMP 文件:** 将 AST 字节码和所有二进制资源打包成一个专有的 .nvmp (NovaMark Package) 归档文件。

### **3.2 NVMP 文件结构 (虚拟文件系统)**

.nvmp 是一个二进制文件，内部结构如下：

* **\[File Header\]**: 魔法数字 (如 NOVA), 版本号。  
* **\[Index Table\]**: 资源字典索引（记录图片、音频在包内的偏移量 Offset 和长度 Length）。  
* **\[AST Bytecode\]**: 编译后的剧本逻辑字节码。  
* **\[Binary Data\]**: 连续的压缩后的图片、音频二进制流。

运行时，C++ VM 直接通过内存映射 (mmap) 读取这个文件，实现**极速加载**。

## **4\. 数据契约 (Data Contract)**

C++ 核心向各语言渲染器暴露的通用数据结构（这里以 JSON/TypeScript Interface 形式表达）：

interface NovaState {  
  // 1\. 媒体与背景  
  bg: string | null;           // 当前背景图路径  
  bgm: string | null;          // 当前 BGM 路径  
  sfx: Array\<{id: string, path: string, loop: boolean}\>; // 正在播放的音效  
    
  // 2\. 画面元素  
  sprites: Array\<{  
    id: string; url: string; x: number; y: number; opacity: number; zIndex: number;  
  }\>;  
    
  // 3\. 对话与选择  
  dialogue: { isShow: boolean; name: string; text: string; color: string } | null;  
  choices: Array\<{ id: string; text: string; disabled: boolean }\>;  
}

## **5\. 多语言渲染器示例**

以下示例展示了如何在不同的编程语言/框架中接入 C++ VM 提供的绑定接口。

### **5.1 Web 端 (TypeScript \+ React)**

利用 WebAssembly (WASM) 运行 C++ 核心。

import React, { useEffect, useState } from 'react';  
import { NovaVM } from 'nova-wasm'; // C++ 编译出的 WASM 模块

export const WebRenderer \= () \=\> {  
  const \[state, setState\] \= useState\<NovaState | null\>(null);

  useEffect(() \=\> {  
    // 初始化 VM 并加载打包好的游戏文件  
    NovaVM.loadPackage('/game.nvmp');  
      
    // 注册帧更新回调  
    NovaVM.onUpdate((newState: NovaState) \=\> {  
      setState(newState);  
    });  
    NovaVM.start();  
  }, \[\]);

  if (\!state) return \<div\>正在加载游戏...\</div\>;

  return (  
    \<div className="game-container" onClick={() \=\> NovaVM.next()}\>  
      {/\* 渲染背景 \*/}  
      {state.bg && \<img className="layer-bg" src={state.bg} /\>}  
        
      {/\* 渲染立绘 \*/}  
      {state.sprites.map(sp \=\> (  
        \<img key={sp.id} src={sp.url} style={{ left: sp.x, top: sp.y, opacity: sp.opacity, position: 'absolute' }} /\>  
      ))}  
        
      {/\* 渲染对话框 \*/}  
      {state.dialogue?.isShow && (  
        \<div className="dialogue-box"\>  
          \<div className="name" style={{ color: state.dialogue.color }}\>{state.dialogue.name}\</div\>  
          \<div className="text"\>{state.dialogue.text}\</div\>  
        \</div\>  
      )}  
        
      {/\* 渲染选项 \*/}  
      \<div className="choices"\>  
        {state.choices.map(c \=\> (  
          \<button key={c.id} onClick={(e) \=\> { e.stopPropagation(); NovaVM.makeChoice(c.id); }}\>  
            {c.text}  
          \</button\>  
        ))}  
      \</div\>  
    \</div\>  
  );  
};

### **5.2 鸿蒙端 (ArkTS)**

通过 Node-API (NAPI) 调用 C++ 底层动态库 (libnova.so)。

import { NovaVM } from 'libnova.so'; // 导入 C++ NAPI 接口

@Entry  
@Component  
struct ArkTSRenderer {  
  @State state: NovaState \= NovaVM.getInitialState();

  aboutToAppear() {  
    NovaVM.loadPackage('rawfile://game.nvmp');  
    NovaVM.onUpdate((newState) \=\> {  
      this.state \= newState;  
    });  
  }

  build() {  
    Stack() {  
      if (this.state.bg) { Image(this.state.bg).width('100%').height('100%') }  
        
      ForEach(this.state.sprites, (sp) \=\> {  
        Image(sp.url).position({x: sp.x, y: sp.y}).opacity(sp.opacity)  
      })

      if (this.state.dialogue?.isShow) {  
        Column() {  
          Text(this.state.dialogue.name).fontColor(this.state.dialogue.color)  
          Text(this.state.dialogue.text).fontColor(Color.White)  
        }  
        .position({ x: 20, y: '70%' })  
        .onClick(() \=\> NovaVM.next()) // 点击继续  
      }  
    }.width('100%').height('100%')  
  }  
}

### **5.3 桌面端 (Go \+ Ebitengine)**

通过 cgo 调用 C++ 动态库。Go 非常适合以极低的性能开销打包独立桌面版游戏。

package main

import (  
    "\[github.com/hajimehoshi/ebiten/v2\](https://github.com/hajimehoshi/ebiten/v2)"  
    "\[github.com/hajimehoshi/ebiten/v2/ebitenutil\](https://github.com/hajimehoshi/ebiten/v2/ebitenutil)"  
    "nova\_cgo\_binding" // 包装了 C++ 调用的包  
)

type Game struct {  
    State nova\_cgo\_binding.NovaState  
}

func (g \*Game) Update() error {  
    // 处理用户输入  
    if ebiten.IsMouseButtonPressed(ebiten.MouseButtonLeft) {  
        nova\_cgo\_binding.Next() // 发送继续指令  
    }  
    // 获取最新状态  
    g.State \= nova\_cgo\_binding.GetState()  
    return nil  
}

func (g \*Game) Draw(screen \*ebiten.Image) {  
    // 1\. 绘制背景  
    if g.State.Bg \!= "" {  
        bgImg, \_, \_ := ebitenutil.NewImageFromFile(g.State.Bg)  
        screen.DrawImage(bgImg, nil)  
    }

    // 2\. 绘制立绘  
    for \_, sp := range g.State.Sprites {  
        img, \_, \_ := ebitenutil.NewImageFromFile(sp.Url)  
        op := \&ebiten.DrawImageOptions{}  
        op.GeoM.Translate(float64(sp.X), float64(sp.Y))  
        op.ColorM.Scale(1, 1, 1, float64(sp.Opacity))  
        screen.DrawImage(img, op)  
    }

    // 3\. 绘制对话框  
    if g.State.Dialogue \!= nil && g.State.Dialogue.IsShow {  
        ebitenutil.DebugPrintAt(screen, g.State.Dialogue.Name+": "+g.State.Dialogue.Text, 20, 400\)  
    }  
}

func (g \*Game) Layout(outsideWidth, outsideHeight int) (int, int) { return 800, 600 }

func main() {  
    nova\_cgo\_binding.LoadPackage("game.nvmp")  
    ebiten.RunGame(\&Game{})  
}

### **5.4 桌面端/树莓派 (Python \+ Pygame)**

利用 ctypes 或 pybind11 提供 Python 绑定，非常适合创作者用来做快速原型测试。

import pygame  
import novavm\_py \# C++ 编译出的 Python 扩展模块

pygame.init()  
screen \= pygame.display.set\_mode((800, 600))  
novavm\_py.load\_package("game.nvmp")

running \= True  
while running:  
    \# 1\. 传递输入到 C++ 引擎  
    for event in pygame.event.get():  
        if event.type \== pygame.QUIT:  
            running \= False  
        elif event.type \== pygame.MOUSEBUTTONDOWN:  
            novavm\_py.next() \# 通知引擎玩家点击了屏幕  
              
    \# 2\. 从引擎获取当前帧的数据字典  
    state \= novavm\_py.get\_state()  
      
    \# 3\. 渲染循环 (只负责绘图)  
    screen.fill((0, 0, 0))  
      
    \# 画背景  
    if state\["bg"\]:  
        bg\_surf \= pygame.image.load(state\["bg"\])  
        screen.blit(bg\_surf, (0, 0))  
          
    \# 画立绘  
    for sp in state\["sprites"\]:  
        sprite\_surf \= pygame.image.load(sp\["url"\])  
        sprite\_surf.set\_alpha(sp\["opacity"\] \* 255\)  
        screen.blit(sprite\_surf, (sp\["x"\], sp\["y"\]))  
          
    \# 画对话框  
    dialogue \= state.get("dialogue")  
    if dialogue and dialogue\["is\_show"\]:  
        font \= pygame.font.Font(None, 36\)  
        text\_surf \= font.render(f'{dialogue\["name"\]}: {dialogue\["text"\]}', True, (255, 255, 255))  
        screen.blit(text\_surf, (50, 450))  
          
    pygame.display.flip()

pygame.quit()

## **6\. 总结**

通过上述架构，NovaMark 将复杂的 **剧本解析、变量流转、存档管理、音视频时间轴** 全部封锁在 C++ 的 NovaVM 中。

这使得无论你未来想要发布到 Steam (C++ / Go), 手机端 (ArkTS / Swift), 还是微信小游戏 (JS/WASM)，你都**只需手写不到 200 行的 UI 渲染代码**。所有平台的逻辑和表现绝对保持 100% 同步！
