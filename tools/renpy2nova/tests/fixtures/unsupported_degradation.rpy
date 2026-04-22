define n = Character("Narrator")

label start:
    with dissolve
    python:
        $ score = 1
    init python:
        $ ready = True
    image eileen happy = "eileen_happy.png"
    play voice "narration.ogg"
    n "需要人工处理的内容如下。"
