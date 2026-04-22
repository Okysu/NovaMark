default clue_ready = False
default visited_secret = False

label start:
    "你要先做什么？"
    menu "请选择行动":
        "直接前进":
            jump main_route
        "准备后调查":
            $ clue_ready = True
            call secret_route
    return

label main_route:
    "你继续前进。"
    return

label secret_route:
    if clue_ready == True:
        $ visited_secret = True
    "你带着准备好的线索进入支线。"
    return
