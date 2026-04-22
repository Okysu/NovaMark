default affinity = True
default seen_side = None

label start:
    "你想做什么？"
    menu "请选择路线":
        "前往主线" if affinity == True:
            jump main_route
        "查看支线" if seen_side == None:
            call side_story
    return

label main_route:
    if affinity == True:
        $ affinity = False
    else:
        $ seen_side = True
    "你进入了主线。"
    return

label side_story:
    play sound "branch.wav"
    "这里是支线。"
    return
