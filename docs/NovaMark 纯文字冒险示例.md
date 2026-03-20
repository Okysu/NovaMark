## **title: 黑曜石密室 (纯文字冒险演示) author: NovaMark 示例 version: 1.0 default\_font: "Noto Serif SC" text\_speed: normal**

# **1\. 角色定义 (不包含任何 sprite 立绘)**

@char 旁白

color: \#AAAAAA

italic: true

@end

@char 哥布林商人

color: \#4CAF50

font: "黑体"

@end

@char 玩家

color: \#55AAFF

@end

# **2\. 物品定义 (仅包含描述)**

@item gold\_coin

name: "金币"

desc: "地下城通用的货币，闪闪发光。"

@end

@item lockpick

name: "生锈的开锁工具"

desc: "一根生锈的铁丝，很容易折断。"

@end

# **3\. 变量与运行时状态设置**

@var hp \= 20

@var str \= 12 \# 力量属性

# **\----------------------------------------**

# **游戏主场景开始**

# **\----------------------------------------**

\#scene\_start "黑曜石密室"

你推开沉重的石门，伴随着刺耳的摩擦声，一股霉味扑面而来。

这里没有精美的立绘，也没有绚丽的背景图，只有无尽的黑暗和你的想象力。

@wait 1s

@give lockpick 1

@give gold\_coin 10

{黄:\[系统\]} 获得 金币 x10，生锈的开锁工具 x1。画面右上角的金币数量更新了。

借着微弱的火把光芒，你看到角落里蹲着一个矮小的身影。房间正中央放着一个生锈的铁箱。

哥布林商人: 嘿，陌生的冒险者！想买点关于这个地牢的情报吗？或者……你想试试运气，打开中间那个箱子？

玩家: 只有我们两个人吗？这里连个背景图都没有。

哥布林商人: 嘿嘿，真正的冒险都在脑子里！

.main\_menu

? 你要怎么做？

* \[检查铁箱\] \-\> .check\_chest if not has\_flag(chest\_resolved)  
* \[询问情报 (花费 5 金币)\] \-\> .talk\_goblin if item\_count(gold\_coin) \>= 5  
* \[攻击哥布林\] \-\> .attack\_goblin  
* \[离开密室，深入地下城\] \-\> scene\_next

.check\_chest

铁箱上挂着一把厚重的黄铜锁，锁芯已经被锈迹填满。

if has\_item(lockpick)

玩家: 试试刚才捡到的开锁工具吧。

\> 你将铁丝探入锁孔，屏住呼吸……  
\> (正在进行力量检定：1d20 \+ 力量(12) \>= 18\)

@wait 1.5s  
@check roll("1d20") \+ str \>= 18  
  成功:  
    \> {big:咔哒！} {波浪:锁开了！}  
    \> 你在里面发现了一大袋金币！  
    @give gold\_coin 50  
    @take lockpick 1  
    \> {黄:\[系统\]} 获得 金币 x50，失去 开锁工具 x1。  
    @flag chest\_resolved  
  失败:  
    \> 咔嚓一声，开锁工具承受不住你的力量，断在了锁芯里。  
    @take lockpick 1  
    \> {红:\[系统\]} 失去 开锁工具 x1。  
    \> 看来这箱子彻底打不开了。  
    @flag chest\_resolved  
@end\_check

else

\> 锁眼太小，你没有开锁工具，靠徒手根本打不开，只能干瞪眼。

endif

\-\> .main\_menu

.talk\_goblin

@take gold\_coin 5

哥布林商人: 嘿嘿，金币的味道真好。（你的金币减少了）

哥布林商人: 听好了，前面的通道里藏着一只喜欢火焰的怪物。

哥布林商人: 千万、千万不要在它面前举起火把！

你默默记下了这条宝贵的情报。

\-\> .main\_menu

.attack\_goblin

你拔出长剑，突然向哥布林发难！

哥布林商人: 你这疯子！

哥布林敏捷地向后一跃，顺手向你掷出一瓶绿色的毒液。

@var damage \= roll("1d6") \+ 2

@set hp \= hp \- damage

{抖动:毒液溅射到了你身上，造成了 {{damage}} 点伤害！}（当前HP: {{hp}}）

if hp \<= 0

\> 你倒在了冰冷的石板上。

\-\> ending\_death

else

\> 哥布林趁机钻进了一条隐蔽的裂缝，消失不见了。

@flag goblin\_fled

\-\> .main\_menu

endif

\#scene\_next "黑暗深渊"

你绕过密室的障碍，走进了更深沉的黑暗中……

本次纯文字演示到此结束。

@ending normal

\#ending\_death "死亡结局"

贪婪与鲁莽让你付出了代价。

你的旅途到此为止了。

@ending bad
