# **NovaMark 语法规范 v0.3**

NovaMark 是一套为文字游戏/视觉小说设计的标记语言。v0.3 引入了物品数据定义与常驻 UI (HUD) 追踪系统，让游戏状态的可视化变得简单优雅。

## **目录**

1. 文件结构  
2. 元信息块 (Front Matter)  
3. 角色与物品定义 (新增 @item)  
4. 场景声明  
5. 旁白与叙述  
6. 角色对话  
7. 内联文本样式  
8. 媒体控制 (含动态坐标与循环音效)  
9. 变量与常驻 UI 系统 (新增 @ui track)  
10. 条件判断  
11. 选择分支  
12. 跳转与子场景  
13. 随机与跑团系统  
14. 存档点  
15. 自定义样式  

## **1\. 文件结构**

一个 .nvm 文件由以下部分组成：

\[元信息块\]          ← 只能出现一次，在文件最顶部  
\[数据定义块\]        ← 包含 @char 和 @item，集中定义  
\[场景块 ...\]        ← 一个或多个场景，游戏主体

## **2\. 元信息块 (Front Matter)**

建议在此处配置基础资源路径。

\---  
title: 失落的灯塔  
version: 0.3  
base\_bg\_path: "assets/bg/"         
base\_sprite\_path: "assets/sprites/"  
base\_audio\_path: "assets/audio/"  
base\_icon\_path: "assets/icons/"     \# 新增：统一图标路径  
default\_font: "Noto Serif SC"  
\---

## **3\. 角色与物品定义**

除了定义角色，现在可以使用 @item 块来定义物品的元数据。引擎可以在渲染背包界面时自动调用这些信息。

@char 林晓  
  color: \#E8A0BF  
  sprite\_default: linxiao\_normal.png  
@end

@item torch  
  name: "老旧的火把"  
  icon: torch\_icon.png  
  desc: "可以照亮黑暗，但能燃烧的时间不多了。"  
  max\_stack: 5        \# 最大堆叠数量（可选）  
@end

@item gold\_coin  
  name: "金币"  
  icon: gold\_coin.png  
  desc: "大陆通用的货币，闪闪发光。"  
@end

## **4\. 场景声明**

\#scene\_forest "幽暗的森林"

## **5\. 旁白与叙述**

\> 月光透过树梢洒落，林间寂静得出奇。

## **6\. 角色对话**

林晓\[happy\]: 太好了，我们出发吧！

## **7\. 内联文本样式**

* {红:红色文字}  
* 变量插值：当前生命：{{hp}}

## **8\. 媒体控制**

@bg forest\_night.jpg transition:slide\_left  
@sprite 林晓 move x:70% y:100px duration:1s   
@bgm main\_theme.mp3 loop:true volume:0.8

## **9\. 变量与常驻 UI 系统 (HUD)**

复杂的 RPG 需要将数值（如生命值、金钱）常驻显示在画面边缘。NovaMark 提供了强大的 @ui track 指令，**引擎会在数值发生变化时自动刷新 UI**。

**定义常驻 UI 组件：**

你可以将 UI 锚定在屏幕特定位置，并绑定变量或物品数量。

@var hp \= 100  
@var max\_hp \= 100

\# 在左上角显示生命值  
@ui track hp\_bar  
  content: "HP: {{hp}} / {{max\_hp}}"  
  position: top\_left  
  color: \#FF5555  
  icon: heart.png  
  show: true          \# 默认是否显示  
@end

\# 在右上角追踪物品数量  
@ui track money\_hud  
  content: "{{item\_count(gold\_coin)}}"  
  position: top\_right  
  icon: gold\_coin.png \# 调用上方 base\_icon\_path  
  show: false         \# 游戏开始时先隐藏  
@end

**运行时控制 UI 的显示/隐藏：**

\> 你终于走出了森林，来到了繁华的小镇。  
@ui show money\_hud    \# 进入小镇后，显示金币数量  
@ui hide hp\_bar       \# 剧情对话时，隐藏血条

**变量运算与物品发放：**

@set hp \= hp \- 10     \# 引擎会自动更新 hp\_bar 的画面  
@give torch 1         \# 背包系统增加火把  
@take gold\_coin 50    \# money\_hud 上的数字会自动刷新

## **10\. 条件判断**

if item\_count(gold\_coin) \>= 50 and (str \> 10 or agi \> 15\)  
  \> 你付了钱，并且凭借矫健的身手挤进了人群。  
  @take gold\_coin 50  
endif

## **11\. 选择分支**

? 你要怎么做？  
\- \[买下这把剑 (50金币)\] \-\> scene\_buy   if item\_count(gold\_coin) \>= 50  
\- \[转身离开\] \-\> .leave

## **12\. 跳转与子场景**

\-\> scene\_next            
@call scene\_combat       
@return                

## **13\. 随机与跑团系统**

@var damage \= roll("2d6") \+ str  
@check roll("1d20") \+ agi \>= 15  
  成功: ...  
  失败: ...  
@end\_check

## **14\. 存档点**

@save "进入 Boss 房间前"

## **15\. 自定义样式与文字特效**

@theme dark\_forest  
  dialog\_bg: \#1a1a2e  
@end  
林晓: {shake:不要……}过来！
