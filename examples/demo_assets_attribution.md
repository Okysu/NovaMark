# demo 资源说明

本示例当前使用了多张真实图片、若干真实音频，以及一套本地生成的立绘占位图，均位于 `examples/assets/` 下，并通过 NovaMark 项目模式打包进 `.nvmp`。

## 一、背景图资源

### 1. rainy_street.jpg

- 文件位置: `assets/bg/rainy_street.jpg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Raining_street_in_the_night_-_panoramio.jpg
- 原图直链: https://upload.wikimedia.org/wikipedia/commons/0/07/Raining_street_in_the_night_-_panoramio.jpg
- 作者: Harri Hedman
- 许可: CC BY 3.0
- 场景用途: 雨夜港城街道 / 序章开场

### 2. library_interior.jpg

- 文件位置: `assets/bg/library_interior.jpg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Library_Interior_(5604862859).jpg
- 原图直链: https://upload.wikimedia.org/wikipedia/commons/c/c8/Library_Interior_%285604862859%29.jpg
- 作者: Francisco Anzola
- 许可: CC BY 2.0
- 场景用途: 潮汐图书馆主厅

### 3. sea_dawn.jpg

- 文件位置: `assets/bg/sea_dawn.jpg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Sky_Clouds_Sea.jpg
- 原图直链: https://upload.wikimedia.org/wikipedia/commons/a/ae/Sky_Clouds_Sea.jpg
- 作者: Unknown author（经 Wikimedia Commons 标记为 CC0）
- 许可: CC0 1.0 Universal Public Domain Dedication
- 场景用途: 海边天光 / 记忆片段切换

### 4. bookshop_night.jpg

- 文件位置: `assets/bg/bookshop_night.jpg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Bookshop_interior_by_night_(30248939176).jpg
- 原图直链: https://upload.wikimedia.org/wikipedia/commons/1/14/Bookshop_interior_by_night_%2830248939176%29.jpg
- 作者: James Petts
- 许可: CC BY-SA 2.0
- 场景用途: 夜间书店内景 / 序章深化场景

### 5. corridor.jpg

- 文件位置: `assets/bg/corridor.jpg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Interior_corridor_in_the_Normandie..jpg
- 原图直链: https://upload.wikimedia.org/wikipedia/commons/c/ca/Interior_corridor_in_the_Normandie..jpg
- 作者: Avezink
- 许可: CC BY-SA 4.0
- 场景用途: 侧馆长廊 / 隐门与结局场景

### 6. harbour_dawn.jpg

- 文件位置: `assets/bg/harbour_dawn.jpg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Harbour_@_dawn_-_50083824036.jpg
- 原图直链: https://upload.wikimedia.org/wikipedia/commons/f/fb/Harbour_%40_dawn_-_50083824036.jpg
- 作者: Pasi Mammela
- 许可: CC BY-SA 2.0
- 场景用途: 记忆章节中的港口黎明

### 7. pigeon_point.jpg

- 文件位置: `assets/bg/pigeon_point.jpg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Pigeon_Point_Lighthouse_(2016).jpg
- 原图直链: https://upload.wikimedia.org/wikipedia/commons/b/bb/Pigeon_Point_Lighthouse_%282016%29.jpg
- 作者: Frank Schulenburg
- 许可: CC BY-SA 4.0
- 场景用途: 灯塔章节主背景

## 二、音频资源

### 1. rain.ogg

- 文件位置: `assets/audio/rain.ogg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Sound_of_rain.ogg
- 原始文件: https://upload.wikimedia.org/wikipedia/commons/8/8a/Sound_of_rain.ogg
- 作者: Effib
- 许可: CC BY-SA 3.0
- 场景用途: 雨夜街道 / 普通结局环境音

### 2. ocean_waves.ogg

- 文件位置: `assets/audio/ocean_waves.ogg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Oceanwavescrushing.ogg
- 原始文件: https://upload.wikimedia.org/wikipedia/commons/f/f1/Oceanwavescrushing.ogg
- 作者: Luftrum
- 许可: CC BY 3.0
- 场景用途: 图书馆深海穹顶 / 记忆章节 / 灯塔章节环境音

### 3. bell.ogg

- 文件位置: `assets/audio/sfx/bell.ogg`
- 文件来源: Wikimedia Commons
- 页面: https://commons.wikimedia.org/wiki/File:Synthetic_bell_sound.ogg
- 原始文件: https://upload.wikimedia.org/wikipedia/commons/4/4f/Synthetic_bell_sound.ogg
- 作者: Achim55
- 许可: CC0 1.0 Universal Public Domain Dedication
- 场景用途: 祁星登场、观潮钟、点灯时的提示音

## 三、立绘占位资源

以下 PNG 并非外部下载素材，而是本仓库内通过脚本生成的“视觉小说风格占位图”，用于展示 NovaMark 的 `@char` 情绪预设与 `角色[emotion]: 台词` 的切换能力。

### 生成脚本

- 脚本路径: `examples/scripts/generate_placeholder_sprites.ps1`
- 生成方式: 使用 PowerShell + `System.Drawing` 本地绘制简化人物占位图

### 当前实际打包文件

```text
assets/sprites/
├── shenyan_normal.png
├── shenyan_pensive.png
├── shenyan_determined.png
├── qixing_normal.png
├── qixing_mysterious.png
├── qixing_gentle.png
├── curator_normal.png
└── curator_solemn.png
```

### 用途说明

- `shenyan_*`: 主角沈砚的默认、沉思、坚定三种状态
- `qixing_*`: 祁星的默认、神秘、柔和三种状态
- `curator_*`: 馆长的默认、庄重两种状态

## 四、备注

- 这些图片与音频仅用于仓库中的示例小说演示。
- 含 `CC BY` / `CC BY-SA` 的资源在分发示例时建议保留本说明文件，以满足署名与来源追踪要求。
- 含 `CC BY-SA` 的资源若被修改或再分发，应注意对应的 ShareAlike 义务。
