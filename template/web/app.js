let renderer = null;
let saveData = null;

const $ = id => document.getElementById(id);

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
            
            if (saveData) {
                renderer.importSave(saveData);
            }
            
            showGameScreen();
            renderer.start();
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
    
    if (mode === 'chat') {
        $('chat-messages').innerHTML = '';
    }
    
    renderGame();
}

function handleAdvance() {
    if (!renderer || renderer.isEnded()) return;
    if (renderer.hasChoices()) return;
    
    renderer.step();
    renderGame();
}

function handleChoice(index) {
    if (!renderer) return;
    
    if (renderer.mode === 'chat') {
        const choiceText = renderer.getChoiceText(index);
        addChatMessage(choiceText, 'player');
    }
    
    renderer.selectChoice(index);
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
    
    const bgm = renderer.getBgm();
    if (bgm) {
        await renderer.playBgm(bgm, renderer.getBgmLoop(), renderer.getBgmVolume());
    } else {
        renderer.stopBgm();
    }
    
    if (renderer.isEnded()) {
        showEnding();
    }
}

async function renderVNMode() {
    const bg = renderer.getBackground();
    if (bg) {
        const url = await renderer.getImageUrl(bg);
        if (url) {
            $('vn-background').style.backgroundImage = `url(${url})`;
        }
    } else {
        $('vn-background').style.backgroundImage = '';
    }
    
    const spritesDiv = $('vn-sprites');
    spritesDiv.innerHTML = '';
    
    const spriteCount = renderer.getSpriteCount();
    for (let i = 0; i < spriteCount; i++) {
        const url = await renderer.getImageUrl(renderer.getSpriteUrl(i));
        if (!url) continue;
        
        const img = document.createElement('img');
        img.src = url;
        img.className = 'vn-sprite';
        img.style.left = `${renderer.getSpriteX(i)}%`;
        img.style.opacity = renderer.getSpriteOpacity(i);
        img.style.zIndex = renderer.getSpriteZIndex(i);
        spritesDiv.appendChild(img);
    }
    
    const dialogueDiv = $('vn-dialogue');
    if (renderer.hasDialogue()) {
        dialogueDiv.classList.remove('hidden');
        
        const speaker = renderer.getDialogueSpeaker();
        const text = renderer.getDialogueText();
        const color = renderer.getDialogueColor();
        
        $('vn-speaker').textContent = speaker;
        $('vn-speaker').style.color = color || '#fff';
        $('vn-text').textContent = text;
        $('vn-indicator').classList.toggle('hidden', renderer.hasChoices());
    } else {
        dialogueDiv.classList.add('hidden');
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

async function renderChatMode() {
    if (renderer.hasDialogue()) {
        const speaker = renderer.getDialogueSpeaker();
        const text = renderer.getDialogueText();
        const color = renderer.getDialogueColor();
        
        if (speaker) {
            addChatMessage(text, 'dialogue', speaker, color);
        } else {
            addChatMessage(text, 'narrator');
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
