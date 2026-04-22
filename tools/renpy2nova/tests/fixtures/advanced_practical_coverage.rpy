define e = Character("Eileen", color="#C0FFEE", image="eileen_default")
default affection = True
default relic = None

label start:
    scene bg room with fade
    show eileen happy at left
    with dissolve
    if affection == True:
        play music "theme.ogg" loop volume 0.75
        e "欢迎回来。"
    elif relic == None:
        play sound "door.wav" volume 0.5
        "你还没拿到遗物。"
    else:
        play voice "line.ogg"
        "这里需要人工确认音频。"
    menu "接下来要做什么？":
        "继续前进" if affection == True:
            jump next_scene
        "查看遗物" if relic != None:
            jump relic_scene
    return

label next_scene:
    show eileen concerned at right with fade
    $ relic = False
    return

label relic_scene:
    hide eileen
    return
