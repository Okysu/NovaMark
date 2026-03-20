class NovaRenderer {
    constructor() {
        this.vm = null;
        this.mode = 'vn';
        this.audioContext = null;
        this.currentBgm = null;
        this.currentBgmUrl = null;
        this.sfxPool = new Map();
        this.assetCache = new Map();
        this.imageCache = new Map();
    }

    async init(nvmpData) {
        const runtime = await createNovaRuntime();
        this.vm = runtime;
        
        runtime._nova_wasm_init();
        
        const dataPtr = runtime._malloc(nvmpData.byteLength);
        runtime.HEAPU8.set(new Uint8Array(nvmpData), dataPtr);
        
        const result = runtime._nova_wasm_load_package(dataPtr, nvmpData.byteLength);
        runtime._free(dataPtr);
        
        if (result !== 0) {
            throw new Error('Failed to load game package');
        }
        
        this.vm = runtime;
    }

    start() {
        this.vm._nova_wasm_start();
        this.render();
    }

    step() {
        this.vm._nova_wasm_next();
        this.render();
    }

    selectChoice(index) {
        this.vm._nova_wasm_select_choice(index);
        this.render();
    }

    getStatus() {
        return this.vm._nova_wasm_get_status();
    }

    isEnded() {
        return this.getStatus() === 3;
    }

    async getAssetBytes(name) {
        if (this.assetCache.has(name)) {
            return this.assetCache.get(name);
        }
        
        const namePtr = this.allocateString(name);
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
        
        const bytes = this.vm.HEAPU8.slice(bufferPtr, bufferPtr + bytesRead);
        this.vm._free(bufferPtr);
        
        this.assetCache.set(name, bytes);
        return bytes;
    }

    async getImageUrl(assetName) {
        if (this.imageCache.has(assetName)) {
            return this.imageCache.get(assetName);
        }
        
        const bytes = await this.getAssetBytes(assetName);
        if (!bytes) return null;
        
        const blob = new Blob([bytes], { type: this.getMimeType(assetName) });
        const url = URL.createObjectURL(blob);
        
        this.imageCache.set(assetName, url);
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
        const len = lengthBytesUTF8(str) + 1;
        const ptr = this.vm._malloc(len);
        stringToUTF8(str, ptr, len);
        return ptr;
    }

    getString(ptr) {
        return UTF8ToString(ptr);
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
        
        const bytes = await this.getAssetBytes(assetName);
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
        
        if (result === 0) {
            this.render();
            return true;
        }
        return false;
    }

    setMode(mode) {
        this.mode = mode;
    }

    render() {
    }
}

window.NovaRenderer = NovaRenderer;
