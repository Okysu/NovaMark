let renderer = null;
let saveData = null;
let lastChatSignature = null;
let isEndingShown = false;

/**
 * 打字机效果类
 * 基于 textConfig.defaultTextSpeed 实现逐字显示
 */
class TypewriterEffect {
    constructor() {
        this.element = null;
        this.text = '';
        this.currentIndex = 0;
        this.isTyping = false;
        this.timer = null;
        this.onComplete = null;
        // 默认速度：50ms/字符（对应 textSpeed = 50）
        // textSpeed 越小越快，0 = 瞬间显示
        this.speed = 50;
    }

    /**
     * 开始打字效果
     * @param {HTMLElement} element - 目标 DOM 元素
     * @param {string} text - 完整文本
     * @param {number} textSpeed - 文字速度（毫秒/字符），0 表示瞬间显示
     * @param {Function} onComplete - 完成回调
     */
    start(element, text, textSpeed, onComplete) {
        this.stop();
        this.element = element;
        this.text = text;
        this.currentIndex = 0;
        this.isTyping = true;
        this.onComplete = onComplete;
        this.speed = textSpeed > 0 ? textSpeed : 0;

        // 速度为 0 时瞬间显示
        if (this.speed === 0) {
            this.element.textContent = this.text;
            this.isTyping = false;
            if (this.onComplete) this.onComplete();
            return;
        }

        this.element.textContent = '';
        this.tick();
    }

    tick() {
        if (!this.isTyping) return;

        if (this.currentIndex < this.text.length) {
            this.currentIndex++;
            this.element.textContent = this.text.slice(0, this.currentIndex);
            this.timer = setTimeout(() => this.tick(), this.speed);
        } else {
            this.isTyping = false;
            if (this.onComplete) this.onComplete();
        }
    }

    /**
     * 立即补全当前文字
     * @returns {boolean} 是否执行了补全操作
     */
    complete() {
        if (!this.isTyping) return false;

        if (this.timer) {
            clearTimeout(this.timer);
            this.timer = null;
        }
        this.element.textContent = this.text;
        this.isTyping = false;
        if (this.onComplete) this.onComplete();
        return true;
    }

    stop() {
        if (this.timer) {
            clearTimeout(this.timer);
            this.timer = null;
        }
        this.isTyping = false;
    }

    isInProgress() {
        return this.isTyping;
    }
}

// 全局打字机实例
const typewriter = new TypewriterEffect();

const $ = id => document.getElementById(id);

function getDebugChoices() {
    if (!renderer || !renderer.hasChoices()) {
        return [];
    }

    const count = renderer.getChoiceCount();
    const choices = [];
    for (let i = 0; i < count; i++) {
        choices.push({
            index: i,
            text: renderer.getChoiceText(i),
            disabled: renderer.isChoiceDisabled(i)
        });
    }
    return choices;
}

function escapeHtml(value) {
    return String(value)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

function buildStatusChips(runtimeState) {
    if (!runtimeState) {
        return [];
    }

    const chips = [];
    const numbers = runtimeState.variables?.numbers || {};
    const strings = runtimeState.variables?.strings || {};
    const bools = runtimeState.variables?.bools || {};
    const inventory = runtimeState.inventory || {};
    const inventoryItems = Array.isArray(runtimeState.inventoryItems) ? runtimeState.inventoryItems : [];

    for (const [key, value] of Object.entries(numbers)) {
        chips.push({ label: key, value: String(value) });
    }
    for (const [key, value] of Object.entries(strings)) {
        chips.push({ label: key, value: String(value) });
    }
    for (const [key, value] of Object.entries(bools)) {
        chips.push({ label: key, value: value ? 'true' : 'false' });
    }
    if (inventoryItems.length > 0) {
        for (const item of inventoryItems) {
            chips.push({ label: item.name || item.id, value: String(item.count) });
        }
    } else {
        for (const [key, value] of Object.entries(inventory)) {
            chips.push({ label: `item:${key}`, value: String(value) });
        }
    }
    return chips;
}

function renderStatusBar() {
    if (!renderer) {
        return;
    }

    const runtimeState = renderer.getRuntimeState();
    const chips = buildStatusChips(runtimeState);
    const html = chips.map(chip => (
        `<div class="status-chip"><span class="status-chip-label">${escapeHtml(chip.label)}</span><span class="status-chip-value">${escapeHtml(chip.value)}</span></div>`
    )).join('');

    const vnBar = $('vn-status-bar');
    const chatBar = $('chat-status-bar');
    vnBar.innerHTML = html;
    chatBar.innerHTML = html;
    vnBar.classList.toggle('hidden', chips.length === 0 || renderer.mode !== 'vn');
    chatBar.classList.toggle('hidden', chips.length === 0 || renderer.mode !== 'chat');
}

function getDebugSnapshot() {
    if (!renderer) {
        return null;
    }

    const snapshot = {
        mode: renderer.mode,
        status: renderer.getStatus(),
        isEnded: renderer.isEnded(),
        hasDialogue: renderer.hasDialogue(),
        hasChoices: renderer.hasChoices(),
        textConfig: renderer.getTextConfig(),
        runtimeState: renderer.getRuntimeState(),
        background: renderer.getBackground(),
        bgm: renderer.getBgm(),
        dialogue: null,
        choices: getDebugChoices(),
        sprites: []
    };

    if (renderer.hasDialogue()) {
        snapshot.dialogue = {
            speaker: renderer.getDialogueSpeaker(),
            text: renderer.getDialogueText(),
            color: renderer.getDialogueColor()
        };
    }

    const spriteCount = renderer.getSpriteCount();
    for (let i = 0; i < spriteCount; i++) {
        snapshot.sprites.push({
            url: renderer.getSpriteUrl(i),
            x: renderer.getSpriteX(i),
            y: renderer.getSpriteY(i),
            opacity: renderer.getSpriteOpacity(i),
            zIndex: renderer.getSpriteZIndex(i)
        });
    }

    return snapshot;
}

function installDebugAPI() {
    if (typeof window === 'undefined') {
        return;
    }

    window.novaDebug = {
        status() {
            if (!renderer) {
                return null;
            }

            return {
                mode: renderer.mode,
                status: renderer.getStatus(),
                isEnded: renderer.isEnded(),
                hasDialogue: renderer.hasDialogue(),
                hasChoices: renderer.hasChoices(),
                textConfig: renderer.getTextConfig(),
                runtimeState: renderer.getRuntimeState()
            };
        },

        snapshot() {
            return getDebugSnapshot();
        },

        runtimeState() {
            if (!renderer) {
                return null;
            }
            return renderer.getRuntimeState();
        },

        choices() {
            return getDebugChoices();
        },

        render() {
            if (!renderer) {
                return false;
            }
            renderGame();
            return true;
        },

        advance() {
            if (!renderer || renderer.isEnded()) {
                return false;
            }
            renderer.advance();
            return this.snapshot();
        },

        select(index) {
            if (!renderer || !renderer.hasChoices()) {
                return false;
            }
            if (index < 0 || index >= renderer.getChoiceCount()) {
                return false;
            }
            renderer.selectChoice(index);
            return this.snapshot();
        },

        choose(index) {
            if (!renderer || !renderer.hasChoices()) {
                return false;
            }
            handleChoice(index);
            return this.snapshot();
        }
    };
}

installDebugAPI();

document.addEventListener('DOMContentLoaded', () => {
    const nvmpInput = $('nvmp-file');
    const saveInput = $('save-file');
    const startBtn = $('start-btn');
    const loadingDiv = $('loading');
    const errorDiv = $('error');
    
    nvmpInput.addEventListener('change', () => {
        startBtn.disabled = !nvmpInput.files.length;
    });
    
    saveInput.addEventListener('change', async () => {
        if (saveInput.files.length) {
            const file = saveInput.files[0];
            saveData = await file.text();
        }
    });
    
    startBtn.addEventListener('click', async () => {
        if (!nvmpInput.files.length) return;
        
        startBtn.disabled = true;
        loadingDiv.classList.remove('hidden');
        errorDiv.classList.add('hidden');
        
        try {
            const nvmpFile = nvmpInput.files[0];
            const nvmpData = await nvmpFile.arrayBuffer();
            
            renderer = new NovaRenderer();
            await renderer.init(nvmpData);
            
            lastChatSignature = null;
            isEndingShown = false;
            
            if (saveData) {
                renderer.importSave(saveData);
            }
            
            showGameScreen();
            renderer.advance();
            
            renderGame();
            
        } catch (err) {
            errorDiv.textContent = 'Error: ' + err.message;
            errorDiv.classList.remove('hidden');
            startBtn.disabled = false;
        } finally {
            loadingDiv.classList.add('hidden');
        }
    });
    
    $('vn-mode-btn').addEventListener('click', () => setMode('vn'));
    $('chat-mode-btn').addEventListener('click', () => setMode('chat'));
    
    $('vn-dialogue').addEventListener('click', handleAdvance);
    $('vn-background').addEventListener('click', handleAdvance);
    $('chat-messages').addEventListener('click', handleAdvance);
    
    $('save-btn').addEventListener('click', handleSave);
    $('menu-btn').addEventListener('click', () => $('menu-overlay').classList.remove('hidden'));
    $('resume-btn').addEventListener('click', () => $('menu-overlay').classList.add('hidden'));
    $('save-menu-btn').addEventListener('click', () => {
        $('menu-overlay').classList.add('hidden');
        handleSave();
    });
    $('load-menu-btn').addEventListener('click', () => {
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = '.json';
        input.onchange = async () => {
            const file = input.files[0];
            const json = await file.text();
            renderer.importSave(json);
            $('menu-overlay').classList.add('hidden');
        };
        input.click();
    });
    $('new-game-btn').addEventListener('click', () => {
        location.reload();
    });
});

function showGameScreen() {
    $('start-screen').classList.add('hidden');
    $('game-screen').classList.remove('hidden');
}

function setMode(mode) {
    renderer.setMode(mode);
    
    $('vn-mode-btn').classList.toggle('active', mode === 'vn');
    $('chat-mode-btn').classList.toggle('active', mode === 'chat');
    
    $('vn-mode').classList.toggle('hidden', mode !== 'vn');
    $('chat-mode').classList.toggle('hidden', mode !== 'chat');

    renderGame();
}

function handleAdvance() {
    if (!renderer || renderer.isEnded()) return;
    if (renderer.hasChoices()) return;

    if (typewriter.isInProgress()) {
        typewriter.complete();
        return;
    }
    
    if (chatTypewriter.isInProgress()) {
        chatTypewriter.complete();
        return;
    }

    renderer.advance();
    renderGame();
}

function handleChoice(index) {
    if (!renderer) return;
    
    typewriter.stop();
    chatTypewriter.stop();
    
    if (renderer.mode === 'chat') {
        const choiceText = renderer.getChoiceText(index);
        addChatMessage(choiceText, 'player');
    }
    
    renderer.selectChoice(index);

    renderer.advance();
    renderGame();
}

function handleSave() {
    const json = renderer.exportSave();
    if (!json) return;
    
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    
    const a = document.createElement('a');
    a.href = url;
    a.download = `novamark_save_${Date.now()}.json`;
    a.click();
    
    URL.revokeObjectURL(url);
}

async function renderGame() {
    if (!renderer) return;
    
    if (renderer.mode === 'vn') {
        await renderVNMode();
    } else {
        await renderChatMode();
    }

    renderStatusBar();
    
    const bgm = renderer.getBgm();
    if (bgm) {
        await renderer.playBgm(bgm, renderer.getBgmLoop(), renderer.getBgmVolume());
    } else {
        renderer.stopBgm();
    }
    
    if (renderer.isEnded() && !isEndingShown) {
        isEndingShown = true;
        showEnding();
    }
}

// 当前对话签名，用于检测对话是否变化
let currentDialogueSignature = null;
let previousSpriteKeys = new Set();

function getSpriteOwner(url, runtimeState) {
    if (!url || !runtimeState?.characterDefinitions) return null;
    const defs = runtimeState.characterDefinitions;
    for (const [id, def] of Object.entries(defs)) {
        const sprites = def?.sprites || {};
        for (const spriteUrl of Object.values(sprites)) {
            if (spriteUrl === url) {
                return id;
            }
        }
    }
    return null;
}

function getSpriteLane(x) {
    if (x <= 25) return 'left';
    if (x >= 75) return 'right';
    return 'center';
}

async function renderVNMode() {
    const bg = renderer.getBackground();
    if (bg) {
        const url = await renderer.getImageUrl(bg, 'bg');
        if (url) {
            $('vn-background').style.backgroundImage = `url(${url})`;
        }
    } else {
        $('vn-background').style.backgroundImage = '';
    }
    
    const spritesDiv = $('vn-sprites');
    spritesDiv.innerHTML = '';
    const runtimeState = renderer.getRuntimeState();
    const currentSpeaker = renderer.getDialogueSpeaker();
    const currentSpriteKeys = new Set();
    
    const spriteCount = renderer.getSpriteCount();
    for (let i = 0; i < spriteCount; i++) {
        const spriteAsset = renderer.getSpriteUrl(i);
        const url = await renderer.getImageUrl(spriteAsset, 'sprite');
        if (!url) continue;
         
        const img = document.createElement('img');
        img.src = url;
        img.className = 'vn-sprite';
        const lane = getSpriteLane(renderer.getSpriteX(i));
        const owner = getSpriteOwner(spriteAsset, runtimeState);
        const isSpeaking = owner && currentSpeaker && owner === currentSpeaker;
        const spriteKey = `${owner || 'unknown'}:${spriteAsset}:${lane}`;
        currentSpriteKeys.add(spriteKey);

        img.classList.add(`vn-sprite--${lane}`);
        img.classList.toggle('is-speaking', Boolean(isSpeaking));
        img.classList.toggle('is-dimmed', Boolean(currentSpeaker && owner && owner !== currentSpeaker));
        img.classList.toggle('is-entering', !previousSpriteKeys.has(spriteKey));
        img.classList.toggle('is-forward', Boolean(isSpeaking));
        img.dataset.owner = owner || '';
        img.style.opacity = renderer.getSpriteOpacity(i);
        img.style.zIndex = renderer.getSpriteZIndex(i);
        spritesDiv.appendChild(img);
    }
    previousSpriteKeys = currentSpriteKeys;
    
    const dialogueDiv = $('vn-dialogue');
    if (renderer.hasDialogue()) {
        dialogueDiv.classList.remove('hidden');
        
        const speaker = renderer.getDialogueSpeaker();
        const text = renderer.getDialogueText();
        const color = renderer.getDialogueColor();
        const signature = `${speaker}\0${text}\0${color}`;

        if (speaker) {
            $('vn-speaker').textContent = speaker;
            $('vn-speaker').style.color = color || '#fff';
            $('vn-speaker').classList.remove('hidden');
        } else {
            $('vn-speaker').textContent = '';
            $('vn-speaker').classList.add('hidden');
        }

        // 检测对话是否变化，变化时启动打字机效果
        if (signature !== currentDialogueSignature) {
            currentDialogueSignature = signature;
            const textConfig = renderer.getTextConfig();
            typewriter.start(
                $('vn-text'),
                text,
                textConfig.defaultTextSpeed || 50,
                () => {
                    $('vn-indicator').classList.toggle('hidden', renderer.hasChoices());
                }
            );
        } else if (!typewriter.isInProgress()) {
            // 打字机已完成，显示指示器
            $('vn-indicator').classList.toggle('hidden', renderer.hasChoices());
        }
    } else {
        dialogueDiv.classList.add('hidden');
        currentDialogueSignature = null;
        typewriter.stop();
    }
    
    const choicesDiv = $('vn-choices');
    choicesDiv.innerHTML = '';
    
    if (renderer.hasChoices()) {
        choicesDiv.classList.remove('hidden');
        
        const count = renderer.getChoiceCount();
        for (let i = 0; i < count; i++) {
            const btn = document.createElement('button');
            btn.className = 'vn-choice';
            btn.textContent = renderer.getChoiceText(i);
            btn.disabled = renderer.isChoiceDisabled(i);
            btn.onclick = () => handleChoice(i);
            choicesDiv.appendChild(btn);
        }
    } else {
        choicesDiv.classList.add('hidden');
    }
}

const chatTypewriter = new TypewriterEffect();

async function renderChatMode() {
    if (renderer.hasDialogue()) {
        const speaker = renderer.getDialogueSpeaker();
        const text = renderer.getDialogueText();
        const color = renderer.getDialogueColor();
        const signature = `${speaker}\u0000${text}\u0000${color}`;
        
        if (signature !== lastChatSignature) {
            lastChatSignature = signature;
            
            const messagesDiv = $('chat-messages');
            const msgDiv = document.createElement('div');
            msgDiv.className = speaker ? 'chat-message dialogue' : 'chat-message narrator';
            
            if (speaker) {
                const speakerDiv = document.createElement('div');
                speakerDiv.className = 'speaker';
                speakerDiv.textContent = speaker;
                if (color) speakerDiv.style.color = color;
                msgDiv.appendChild(speakerDiv);
            }
            
            const textDiv = document.createElement('div');
            msgDiv.appendChild(textDiv);
            messagesDiv.appendChild(msgDiv);
            messagesDiv.scrollTop = messagesDiv.scrollHeight;
            
            const textConfig = renderer.getTextConfig();
            chatTypewriter.start(
                textDiv,
                text,
                textConfig.defaultTextSpeed || 50,
                null
            );
        }
    }
    
    const choicesDiv = $('chat-choices');
    choicesDiv.innerHTML = '';
    
    if (renderer.hasChoices()) {
        const count = renderer.getChoiceCount();
        for (let i = 0; i < count; i++) {
            const btn = document.createElement('button');
            btn.className = 'chat-choice';
            btn.textContent = renderer.getChoiceText(i);
            btn.disabled = renderer.isChoiceDisabled(i);
            btn.onclick = () => handleChoice(i);
            choicesDiv.appendChild(btn);
        }
    }
}

function addChatMessage(text, type, speaker = null, color = null) {
    const messagesDiv = $('chat-messages');
    
    const msgDiv = document.createElement('div');
    msgDiv.className = `chat-message ${type}`;
    
    if (type === 'dialogue' && speaker) {
        const speakerDiv = document.createElement('div');
        speakerDiv.className = 'speaker';
        speakerDiv.textContent = speaker;
        if (color) speakerDiv.style.color = color;
        msgDiv.appendChild(speakerDiv);
    }
    
    const textDiv = document.createElement('div');
    textDiv.textContent = text;
    msgDiv.appendChild(textDiv);
    
    messagesDiv.appendChild(msgDiv);
    messagesDiv.scrollTop = messagesDiv.scrollHeight;
}

function showEnding() {
    const overlay = document.createElement('div');
    overlay.className = 'overlay';
    overlay.innerHTML = `
        <div class="menu-content">
            <h2>The End</h2>
            <p>Thank you for playing!</p>
            <button onclick="location.reload()">Play Again</button>
        </div>
    `;
    document.getElementById('app').appendChild(overlay);
}
