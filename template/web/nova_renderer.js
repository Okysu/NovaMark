class NovaRenderer {
    constructor() {
        this.vm = null;
        this.mode = 'vn';
        this.audioContext = null;
        this.currentBgm = null;
        this.currentBgmUrl = null;
        this.assetCache = new Map();
        this.imageCache = new Map();
    }

    async init(nvmpData) {
        const runtime = await createNovaRuntime();
        this.vm = runtime;

        runtime._nova_wasm_init();

        const dataPtr = runtime._malloc(nvmpData.byteLength);
        this.writeBytes(dataPtr, new Uint8Array(nvmpData));

        const result = runtime._nova_wasm_load_package(dataPtr, nvmpData.byteLength);
        runtime._free(dataPtr);

        if (result !== 0) {
            throw new Error('Failed to load game package (code: ' + result + ')');
        }
    }

    advance() {
        this.vm._nova_wasm_advance();
    }

    selectChoice(index) {
        this.vm._nova_wasm_choose(index);
    }

    getStatus() {
        return this.vm._nova_wasm_get_status();
    }

    getTextConfig() {
        return {
            defaultFont: this.vm.UTF8ToString(this.vm._nova_wasm_get_default_font()),
            defaultFontSize: this.vm._nova_wasm_get_default_font_size(),
            defaultTextSpeed: this.vm._nova_wasm_get_default_text_speed(),
            baseBgPath: this.getString(this.vm._nova_wasm_get_base_bg_path()),
            baseSpritePath: this.getString(this.vm._nova_wasm_get_base_sprite_path()),
            baseAudioPath: this.getString(this.vm._nova_wasm_get_base_audio_path())
        };
    }

    getRuntimeState() {
        const sizePtr = this.vm._malloc(4);
        const jsonPtr = this.vm._nova_wasm_export_runtime_state_json(sizePtr);
        const size = this.vm.getValue(sizePtr, 'i32');
        this.vm._free(sizePtr);

        if (!jsonPtr || size <= 0) {
            return null;
        }

        const jsonText = this.vm.UTF8ToString(jsonPtr);
        this.vm._nova_wasm_free(jsonPtr);
        return JSON.parse(jsonText);
    }

    isEnded() {
        return this.getStatus() === 3;
    }

    async getAssetBytes(name, category = null) {
        if (this.assetCache.has(name)) {
            return this.assetCache.get(name);
        }

        const cacheKey = category ? `${category}:${name}` : name;
        if (this.assetCache.has(cacheKey)) {
            return this.assetCache.get(cacheKey);
        }

        const resolvedName = this.resolveAssetName(name, category);
        if (this.assetCache.has(resolvedName)) {
            return this.assetCache.get(resolvedName);
        }

        const namePtr = this.allocateString(resolvedName);
        const size = this.vm._nova_wasm_get_asset_size(namePtr);

        if (size === 0) {
            this.vm._free(namePtr);
            return null;
        }

        const bufferPtr = this.vm._malloc(size);
        const bytesRead = this.vm._nova_wasm_get_asset_bytes(namePtr, bufferPtr, size);

        this.vm._free(namePtr);

        if (bytesRead <= 0) {
            this.vm._free(bufferPtr);
            return null;
        }

        const bytes = this.readBytes(bufferPtr, bytesRead);
        this.vm._free(bufferPtr);

        this.assetCache.set(name, bytes);
        this.assetCache.set(cacheKey, bytes);
        this.assetCache.set(resolvedName, bytes);
        return bytes;
    }

    resolveAssetName(name, category = null) {
        if (!name) return name;
        if (name.includes('/')) return name;

        const ext = name.split('.').pop().toLowerCase();
        const textConfig = this.getTextConfig();
        const trimPrefix = (value, fallback) => {
            const normalized = (value || fallback || '').replace(/^assets\//, '');
            return normalized.endsWith('/') ? normalized : normalized + '/';
        };

        if (category === 'bg') {
            return trimPrefix(textConfig.baseBgPath, 'bg/') + name;
        }

        if (category === 'sprite') {
            return trimPrefix(textConfig.baseSpritePath, 'sprites/') + name;
        }

        if (category === 'audio') {
            return trimPrefix(textConfig.baseAudioPath, 'audio/') + name;
        }

        if (['png', 'jpg', 'jpeg', 'gif', 'webp'].includes(ext)) {
            return trimPrefix(textConfig.baseBgPath, 'bg/') + name;
        }

        if (['mp3', 'ogg', 'wav', 'flac'].includes(ext)) {
            return trimPrefix(textConfig.baseAudioPath, 'audio/') + name;
        }

        return name;
    }

    async getImageUrl(assetName, category = null) {
        const cacheKey = category ? `${category}:${assetName}` : assetName;
        if (this.imageCache.has(cacheKey)) {
            return this.imageCache.get(cacheKey);
        }

        const bytes = await this.getAssetBytes(assetName, category);
        if (!bytes) return null;

        const blob = new Blob([bytes], { type: this.getMimeType(assetName) });
        const url = URL.createObjectURL(blob);

        this.imageCache.set(cacheKey, url);
        return url;
    }

    getMimeType(filename) {
        const ext = filename.split('.').pop().toLowerCase();
        const types = {
            'png': 'image/png',
            'jpg': 'image/jpeg',
            'jpeg': 'image/jpeg',
            'gif': 'image/gif',
            'webp': 'image/webp',
            'mp3': 'audio/mpeg',
            'ogg': 'audio/ogg',
            'wav': 'audio/wav'
        };
        return types[ext] || 'application/octet-stream';
    }

    allocateString(str) {
        const len = this.vm.lengthBytesUTF8(str) + 1;
        const ptr = this.vm._malloc(len);
        this.vm.stringToUTF8(str, ptr, len);
        return ptr;
    }

    writeBytes(ptr, bytes) {
        this.vm.writeArrayToMemory(bytes, ptr);
    }

    readBytes(ptr, length) {
        return this.vm.HEAPU8.slice(ptr, ptr + length);
    }

    getString(ptr) {
        if (!ptr) {
            return '';
        }

        return this.vm.UTF8ToString(ptr);
    }

    getBackground() {
        return this.getString(this.vm._nova_wasm_get_bg());
    }

    getBgm() {
        return this.getString(this.vm._nova_wasm_get_bgm());
    }

    getBgmVolume() {
        return this.vm._nova_wasm_get_bgm_volume();
    }

    getBgmLoop() {
        return this.vm._nova_wasm_get_bgm_loop() === 1;
    }

    hasDialogue() {
        return this.vm._nova_wasm_has_dialogue() === 1;
    }

    getDialogueSpeaker() {
        return this.getString(this.vm._nova_wasm_get_dialogue_speaker());
    }

    getDialogueText() {
        return this.getString(this.vm._nova_wasm_get_dialogue_text());
    }

    getDialogueColor() {
        return this.getString(this.vm._nova_wasm_get_dialogue_color());
    }

    hasChoices() {
        return this.vm._nova_wasm_has_choices() === 1;
    }

    getChoiceCount() {
        return this.vm._nova_wasm_get_choice_count();
    }

    getChoiceText(index) {
        return this.getString(this.vm._nova_wasm_get_choice_text(index));
    }

    isChoiceDisabled(index) {
        return this.vm._nova_wasm_is_choice_disabled(index) === 1;
    }

    getSpriteCount() {
        return this.vm._nova_wasm_get_sprite_count();
    }

    getSpriteUrl(index) {
        return this.getString(this.vm._nova_wasm_get_sprite_url(index));
    }

    getSpriteX(index) {
        return this.vm._nova_wasm_get_sprite_x(index);
    }

    getSpriteY(index) {
        return this.vm._nova_wasm_get_sprite_y(index);
    }

    getSpriteOpacity(index) {
        return this.vm._nova_wasm_get_sprite_opacity(index);
    }

    getSpriteZIndex(index) {
        return this.vm._nova_wasm_get_sprite_z_index(index);
    }

    async playBgm(assetName, loop, volume) {
        if (this.currentBgmUrl === assetName) return;

        if (this.currentBgm) {
            this.currentBgm.pause();
            this.currentBgm = null;
        }

        const bytes = await this.getAssetBytes(assetName, 'audio');
        if (!bytes) return;

        if (!this.audioContext) {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        }

        const blob = new Blob([bytes], { type: this.getMimeType(assetName) });
        const url = URL.createObjectURL(blob);

        this.currentBgm = new Audio(url);
        this.currentBgm.loop = loop;
        this.currentBgm.volume = volume;
        this.currentBgmUrl = assetName;

        try {
            await this.currentBgm.play();
        } catch (e) {
            console.warn('BGM autoplay blocked, will play on user interaction');
        }
    }

    stopBgm() {
        if (this.currentBgm) {
            this.currentBgm.pause();
            this.currentBgm.currentTime = 0;
            this.currentBgm = null;
            this.currentBgmUrl = null;
        }
    }

    async playSfx(assetName) {
        const bytes = await this.getAssetBytes(assetName);
        if (!bytes) return;

        if (!this.audioContext) {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        }

        const blob = new Blob([bytes], { type: this.getMimeType(assetName) });
        const url = URL.createObjectURL(blob);

        const sfx = new Audio(url);
        sfx.volume = 1.0;

        try {
            await sfx.play();
        } catch (e) {
            console.warn('SFX play failed:', e);
        }
    }

    exportSave() {
        const sizePtr = this.vm._malloc(4);
        const jsonPtr = this.vm._nova_wasm_export_save_json(sizePtr);

        if (!jsonPtr) {
            this.vm._free(sizePtr);
            return null;
        }

        const size = this.vm.getValue(sizePtr, 'i32');
        const json = this.getString(jsonPtr);

        this.vm._free(sizePtr);
        this.vm._nova_wasm_free(jsonPtr);

        return json;
    }

    importSave(json) {
        const jsonPtr = this.allocateString(json);
        const result = this.vm._nova_wasm_import_save_json(jsonPtr, json.length);
        this.vm._free(jsonPtr);

        return result === 0;
    }

    setMode(mode) {
        this.mode = mode;
    }
}

window.NovaRenderer = NovaRenderer;
