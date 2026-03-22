#include "nova/vm/vm.h"
#include "nova/vm/state.h"
#include <random>
#include <stdexcept>
#include <sstream>

namespace nova {

namespace {

/// @brief 将位置字符串映射为 x 百分比坐标，供渲染器布局使用
/// @param position 位置字符串：left, center, right
/// @return x 坐标百分比 (left=20, center=50, right=80)
double mapPositionToX(const std::string& position) {
    if (position == "left") return 20.0;
    if (position == "center") return 50.0;
    if (position == "right") return 80.0;
    return 50.0; // 默认居中
}

double safe_stod(const std::string& s, double defaultVal = 0.0) {
    try { return std::stod(s); }
    catch (...) { return defaultVal; }
}

int safe_stoi(const std::string& s, int defaultVal = 0) {
    try { return std::stoi(s); }
    catch (...) { return defaultVal; }
}

const SceneDefNode* as_scene(const AstNode* node) {
    return dynamic_cast<const SceneDefNode*>(node);
}

const DialogueNode* as_dialogue(const AstNode* node) {
    return dynamic_cast<const DialogueNode*>(node);
}

const NarratorNode* as_narrator(const AstNode* node) {
    return dynamic_cast<const NarratorNode*>(node);
}

const JumpNode* as_jump(const AstNode* node) {
    return dynamic_cast<const JumpNode*>(node);
}

const ChoiceNode* as_choice(const AstNode* node) {
    return dynamic_cast<const ChoiceNode*>(node);
}

const BranchNode* as_branch(const AstNode* node) {
    return dynamic_cast<const BranchNode*>(node);
}

const VarDefNode* as_var_def(const AstNode* node) {
    return dynamic_cast<const VarDefNode*>(node);
}

const SetCommandNode* as_set(const AstNode* node) {
    return dynamic_cast<const SetCommandNode*>(node);
}

const GiveCommandNode* as_give(const AstNode* node) {
    return dynamic_cast<const GiveCommandNode*>(node);
}

const TakeCommandNode* as_take(const AstNode* node) {
    return dynamic_cast<const TakeCommandNode*>(node);
}

const BgCommandNode* as_bg(const AstNode* node) {
    return dynamic_cast<const BgCommandNode*>(node);
}

const SpriteCommandNode* as_sprite(const AstNode* node) {
    return dynamic_cast<const SpriteCommandNode*>(node);
}

const BgmCommandNode* as_bgm(const AstNode* node) {
    return dynamic_cast<const BgmCommandNode*>(node);
}

const SfxCommandNode* as_sfx(const AstNode* node) {
    return dynamic_cast<const SfxCommandNode*>(node);
}

const EndingNode* as_ending(const AstNode* node) {
    return dynamic_cast<const EndingNode*>(node);
}

const FlagNode* as_flag(const AstNode* node) {
    return dynamic_cast<const FlagNode*>(node);
}

const CheckCommandNode* as_check(const AstNode* node) {
    return dynamic_cast<const CheckCommandNode*>(node);
}

const CallNode* as_call(const AstNode* node) {
    return dynamic_cast<const CallNode*>(node);
}

const ReturnNode* as_return(const AstNode* node) {
    return dynamic_cast<const ReturnNode*>(node);
}

const LabelNode* as_label(const AstNode* node) {
    return dynamic_cast<const LabelNode*>(node);
}

const LiteralNode* as_literal(const AstNode* node) {
    return dynamic_cast<const LiteralNode*>(node);
}

const IdentifierNode* as_identifier(const AstNode* node) {
    return dynamic_cast<const IdentifierNode*>(node);
}

const BinaryExprNode* as_binary(const AstNode* node) {
    return dynamic_cast<const BinaryExprNode*>(node);
}

const UnaryExprNode* as_unary(const AstNode* node) {
    return dynamic_cast<const UnaryExprNode*>(node);
}

const CallExprNode* as_call_expr(const AstNode* node) {
    return dynamic_cast<const CallExprNode*>(node);
}

std::string get_string_like_argument(const AstNode* node) {
    if (const auto* id = as_identifier(node)) {
        return id->name();
    }

    if (const auto* lit = as_literal(node)) {
        if (lit->is_string()) {
            return lit->as_string();
        }
    }

    return "";
}

bool is_string_like_argument(const AstNode* node) {
    return as_identifier(node) != nullptr || (as_literal(node) != nullptr && as_literal(node)->is_string());
}

const DiceExprNode* as_dice(const AstNode* node) {
    return dynamic_cast<const DiceExprNode*>(node);
}

const CharDefNode* as_char_def(const AstNode* node) {
    return dynamic_cast<const CharDefNode*>(node);
}

const ItemDefNode* as_item_def(const AstNode* node) {
    return dynamic_cast<const ItemDefNode*>(node);
}

const ChoiceOptionNode* as_choice_option(const AstNode* node) {
    return dynamic_cast<const ChoiceOptionNode*>(node);
}

const FrontMatterNode* as_front_matter(const AstNode* node) {
    return dynamic_cast<const FrontMatterNode*>(node);
}

}

NovaVM::NovaVM() {
    m_state.status = VMStatus::Running;
}

int NovaVM::consumeRuntimeStateChangeFlags() {
    const int flags = m_runtimeStateChangeFlags;
    m_runtimeStateChangeFlags = RuntimeStateChangeNone;
    return flags;
}

void NovaVM::markRuntimeStateChanged(int flags) {
    if (flags == RuntimeStateChangeNone) {
        return;
    }

    ++m_runtimeStateVersion;
    m_runtimeStateChangeFlags |= flags;
}

GameState NovaVM::captureState() const {
    GameState state;
    state.currentScene = m_currentScene;
    state.currentLabel = m_state.currentLabel;
    state.statementIndex = m_statementIndex;
    state.textConfig = m_state.textConfig;
    state.bg = m_state.bg;
    state.bgTransition = m_state.bgTransition;
    state.bgm = m_state.bgm;
    state.bgmVolume = m_state.bgmVolume;
    state.bgmLoop = m_state.bgmLoop;
    state.sprites = m_state.sprites;
    state.dialogue = m_state.dialogue;
    state.choice = m_state.choice;
    state.ending = m_state.ending;
    state.callStack = m_callStack;
    
    state.numberVariables = m_variables.getAllNumbers();
    state.stringVariables = m_variables.getAllStrings();
    state.boolVariables = m_variables.getAllBools();
    state.inventory = m_inventory.getAllItems();
    
    state.triggeredEndings = m_playthrough.endings();
    state.flags = m_playthrough.flags();
    
    return state;
}

bool NovaVM::loadSave(const SaveData& save) {
    return loadSave(save.state);
}

bool NovaVM::loadSave(const GameState& state) {
    if (!m_program) return false;
    
    auto sceneIt = m_scenes.find(state.currentScene);
    if (sceneIt == m_scenes.end()) return false;
    
    m_currentScene = state.currentScene;
    m_statementIndex = state.statementIndex;
    m_callStack = state.callStack;
    
    m_variables.loadFrom(state.numberVariables, state.stringVariables, state.boolVariables);
    m_inventory.loadFrom(state.inventory);
    m_playthrough.clear();
    
    for (const auto& ending : state.triggeredEndings) {
        m_playthrough.triggerEnding(ending);
    }
    for (const auto& flag : state.flags) {
        m_playthrough.setFlag(flag);
    }
    
    m_state.currentScene = m_currentScene;
    m_state.currentLabel = state.currentLabel;
    m_state.status = VMStatus::Running;
    m_state.textConfig = state.textConfig;
    m_state.bg = state.bg;
    m_state.bgTransition = state.bgTransition;
    m_state.bgm = state.bgm;
    m_state.bgmVolume = state.bgmVolume;
    m_state.bgmLoop = state.bgmLoop;
    m_state.sprites = state.sprites;
    m_state.dialogue = state.dialogue;
    m_state.choice = state.choice;
    m_state.ending = state.ending;
    if (state.ending.has_value()) {
        m_state.status = VMStatus::Ended;
    } else if (state.choice.has_value()) {
        m_state.status = VMStatus::WaitingChoice;
    }
    markRuntimeStateChanged(RuntimeStateChangeVariables | RuntimeStateChangeInventory);

    return true;
}

void NovaVM::load(const ProgramNode* program) {
    m_program = program;
    buildSceneIndex();
    buildDefinitionRegistry();
    m_statementIndex = 0;
    m_state.status = VMStatus::Running;
    applyFrontMatterDefaults();
    
    executeGlobalStatements();
    markRuntimeStateChanged(RuntimeStateChangeVariables | RuntimeStateChangeInventory);
}

void NovaVM::buildDefinitionRegistry() {
    m_characterDefinitions.clear();
    m_itemDefinitions.clear();

    if (!m_program) return;

    for (const auto& stmt : m_program->statements()) {
        if (const auto* ch = as_char_def(stmt.get())) {
            CharacterDefinition def;
            def.id = ch->name();
            for (const auto& prop : ch->properties()) {
                if (prop.key == "color") {
                    def.color = prop.value;
                } else if (prop.key == "description") {
                    def.description = prop.value;
                } else if (prop.key.rfind("sprite_", 0) == 0) {
                    def.sprites[prop.key.substr(7)] = prop.value;
                }
            }
            m_characterDefinitions[ch->name()] = std::move(def);
            continue;
        }

        if (const auto* item = as_item_def(stmt.get())) {
            ItemDefinition def;
            def.id = item->name();
            for (const auto& prop : item->properties()) {
                if (prop.key == "name") {
                    def.name = prop.value;
                } else if (prop.key == "description") {
                    def.description = prop.value;
                } else if (prop.key == "icon") {
                    def.icon = prop.value;
                } else if (prop.key == "default_value") {
                    def.defaultValue = prop.value;
                }
            }
            m_itemDefinitions[item->name()] = std::move(def);
        }
    }
}

std::string NovaVM::resolveCharacterSprite(const std::string& speaker, const std::string& emotion) const {
    auto it = m_characterDefinitions.find(speaker);
    if (it == m_characterDefinitions.end()) {
        return "";
    }

    if (!emotion.empty()) {
        auto spriteIt = it->second.sprites.find(emotion);
        if (spriteIt != it->second.sprites.end()) {
            return spriteIt->second;
        }
    }

    auto defaultIt = it->second.sprites.find("default");
    if (defaultIt != it->second.sprites.end()) {
        return defaultIt->second;
    }

    return "";
}

void NovaVM::applyFrontMatterDefaults() {
    m_state.textConfig = TextConfigState{};

    if (!m_program) return;

    for (const auto& stmt : m_program->statements()) {
        const auto* frontMatter = as_front_matter(stmt.get());
        if (!frontMatter) {
            if (as_scene(stmt.get())) {
                break;
            }
            continue;
        }

        for (const auto& prop : frontMatter->properties()) {
            if (prop.key == "default_font") {
                m_state.textConfig.defaultFont = prop.value;
            } else if (prop.key == "default_font_size") {
                m_state.textConfig.defaultFontSize = safe_stoi(prop.value, m_state.textConfig.defaultFontSize);
            } else if (prop.key == "default_text_speed") {
                m_state.textConfig.defaultTextSpeed = safe_stoi(prop.value, m_state.textConfig.defaultTextSpeed);
            }
        }
        break;
    }
}

void NovaVM::executeGlobalStatements() {
    if (!m_program) return;
    
    for (const auto& stmt : m_program->statements()) {
        if (auto scene = as_scene(stmt.get())) {
            break;
        }
        
        if (auto var = as_var_def(stmt.get())) {
            if (var && var->init_value()) {
                m_variables.set(var->name(), evaluateExpression(var->init_value()));
            }
        } else if (auto give = as_give(stmt.get())) {
            m_inventory.add(give->item(), static_cast<int>(evaluateAsNumber(give->count())));
        } else if (auto take = as_take(stmt.get())) {
            m_inventory.remove(take->item(), static_cast<int>(evaluateAsNumber(take->count())));
        }
    }
}

void NovaVM::load(const AstPtr& program) {
    load(dynamic_cast<const ProgramNode*>(program.get()));
}

void NovaVM::buildSceneIndex() {
    m_scenes.clear();
    m_scene_order.clear();
    m_scene_order_index.clear();
    
    if (!m_program) return;
    
    std::string currentScene;
    size_t sceneStartIndex = 0;
    
    for (size_t i = 0; i < m_program->statements().size(); ++i) {
        const auto& stmt = m_program->statements()[i];
        
        if (auto scene = as_scene(stmt.get())) {
            if (!currentScene.empty()) {
                m_scenes[currentScene].statementCount = i - sceneStartIndex;
            }
            
            currentScene = scene->name();
            sceneStartIndex = i + 1;

            m_scene_order_index[currentScene] = m_scene_order.size();
            m_scene_order.push_back(currentScene);
            
            SceneData data;
            data.name = scene->name();
            data.title = scene->title();
            data.statementStart = i + 1;
            m_scenes[scene->name()] = std::move(data);
        } else if (!currentScene.empty()) {
            if (auto label = as_label(stmt.get())) {
                m_scenes[currentScene].labels[label->name()] = i - sceneStartIndex;
            }
        }
    }
    
    if (!currentScene.empty()) {
        m_scenes[currentScene].statementCount = m_program->statements().size() - sceneStartIndex;
    }
}

void NovaVM::advance() {
    if (m_state.status == VMStatus::Ended) return;
    if (m_state.status == VMStatus::WaitingChoice) return;

    if (m_state.dialogue) {
        m_state.clearDialogue();
    }
    
    m_state.sfx.clear();
    
    if (m_currentScene.empty()) {
        if (!m_scene_order.empty()) {
            m_currentScene = m_scene_order.front();
            m_statementIndex = 0;
            m_state.currentScene = m_currentScene;
        } else {
            m_state.status = VMStatus::Ended;
            return;
        }
    }
    
    if (!m_program) {
        m_state.status = VMStatus::Ended;
        return;
    }
    
    const auto& programStatements = m_program->statements();
    std::string scene_at_start = m_currentScene;
    
    while (true) {
        auto sceneIt = m_scenes.find(m_currentScene);
        if (sceneIt == m_scenes.end()) {
            m_state.status = VMStatus::Ended;
            return;
        }
        
        const auto& sceneData = sceneIt->second;
        
        if (m_statementIndex >= sceneData.statementCount) {
            auto orderIt = m_scene_order_index.find(m_currentScene);
            if (orderIt != m_scene_order_index.end()) {
                size_t idx = orderIt->second;
                if (idx + 1 < m_scene_order.size()) {
                    m_currentScene = m_scene_order[idx + 1];
                    m_statementIndex = 0;
                    m_state.currentScene = m_currentScene;
                    m_state.currentLabel.clear();
                    scene_at_start = m_currentScene;
                    continue;
                }
            }

            m_state.status = VMStatus::Ended;
            return;
        }
        
        size_t globalIndex = sceneData.statementStart + m_statementIndex;
        if (globalIndex >= programStatements.size()) {
            m_state.status = VMStatus::Ended;
            return;
        }
        
        const auto& stmt = programStatements[globalIndex];
        m_statementIndex++;
        
        executeStatement(stmt.get());
        
        if (m_state.status == VMStatus::WaitingChoice ||
            m_state.status == VMStatus::Ended) {
            return;
        }
        
        if (m_currentScene != scene_at_start) {
            return;
        }
        
        if (m_state.dialogue || m_state.choice) {
            return;
        }
    }
}

namespace {
void select_choice_by_index(nova::NovaVM& vm, int index) {
    auto& state = const_cast<nova::NovaState&>(vm.state());
    if (state.status != nova::VMStatus::WaitingChoice || !state.choice) return;

    if (index < 0 || index >= static_cast<int>(state.choice->options.size())) return;

    nova::ChoiceOption opt = state.choice->options[index];
    state.clearChoice();
    state.clearDialogue();
    state.status = nova::VMStatus::Running;

    if (!opt.target.empty() && opt.target[0] == '.') {
        vm.jumpToLabel(opt.target.substr(1));
    } else {
        vm.jumpToScene(opt.target);
    }
}
}

bool NovaVM::choose(const std::string& choiceId) {
    if (m_state.status != VMStatus::WaitingChoice || !m_state.choice) return false;

    for (int i = 0; i < static_cast<int>(m_state.choice->options.size()); ++i) {
        if (m_state.choice->options[i].id == choiceId) {
            select_choice_by_index(*this, i);
            return true;
        }
    }

    return false;
}

void NovaVM::setEntryPoint(const std::string& sceneName) {
    m_currentScene = sceneName;
    m_statementIndex = 0;
}

bool NovaVM::jumpToScene(const std::string& sceneName) {
    auto it = m_scenes.find(sceneName);
    if (it == m_scenes.end()) return false;
    
    m_currentScene = sceneName;
    m_statementIndex = 0;
    m_state.currentScene = sceneName;
    return true;
}

bool NovaVM::jumpToLabel(const std::string& labelName) {
    if (m_currentScene.empty()) {
        if (!m_scene_order.empty()) {
            m_currentScene = m_scene_order.front();
            m_state.currentScene = m_currentScene;
        } else {
            return false;
        }
    }

    auto sceneIt = m_scenes.find(m_currentScene);
    if (sceneIt == m_scenes.end()) return false;

    std::string key = labelName;
    if (!key.empty() && key[0] == '.') {
        key = key.substr(1);
    }

    auto labelIt = sceneIt->second.labels.find(key);
    if (labelIt == sceneIt->second.labels.end()) return false;
    
    m_statementIndex = labelIt->second;
    m_state.currentLabel = key;
    return true;
}

void NovaVM::callScene(const std::string& sceneName) {
    m_callStack.push_back({m_currentScene, m_statementIndex});
    jumpToScene(sceneName);
}

void NovaVM::returnFromCall() {
    if (m_callStack.empty()) {
        m_state.status = VMStatus::Ended;
        return;
    }
    
    auto [scene, index] = m_callStack.back();
    m_callStack.pop_back();
    
    m_currentScene = scene;
    m_statementIndex = index;
    m_state.currentScene = scene;
}

void NovaVM::reset() {
    m_state.clear();
    m_state.status = VMStatus::Running;
    m_currentScene.clear();
    m_statementIndex = 0;
    m_callStack.clear();
    markRuntimeStateChanged(RuntimeStateChangeVariables | RuntimeStateChangeInventory);
}

void NovaVM::executeStatement(const AstNode* node) {
    if (!node) return;
    
    switch (node->type()) {
        case NodeType::Dialogue:
            executeDialogue(as_dialogue(node));
            break;
        case NodeType::Narrator:
            executeNarrator(as_narrator(node));
            break;
        case NodeType::Jump:
            executeJump(as_jump(node));
            break;
        case NodeType::Choice:
            executeChoice(as_choice(node));
            break;
        case NodeType::Branch:
            executeBranch(as_branch(node));
            break;
        case NodeType::VarDef: {
            auto var = as_var_def(node);
            if (var && var->init_value()) {
                m_variables.set(var->name(), evaluateExpression(var->init_value()));
                markRuntimeStateChanged(RuntimeStateChangeVariables);
            }
            break;
        }
        case NodeType::SetCommand:
            executeSet(as_set(node));
            break;
        case NodeType::GiveCommand:
            executeGive(as_give(node));
            break;
        case NodeType::TakeCommand:
            executeTake(as_take(node));
            break;
        case NodeType::BgCommand:
            executeBg(as_bg(node));
            break;
        case NodeType::SpriteCommand:
            executeSprite(as_sprite(node));
            break;
        case NodeType::BgmCommand:
            executeBgm(as_bgm(node));
            break;
        case NodeType::SfxCommand:
            executeSfx(as_sfx(node));
            break;
        case NodeType::Ending:
            executeEnding(as_ending(node));
            break;
        case NodeType::Flag:
            executeFlag(as_flag(node));
            break;
        case NodeType::CheckCommand:
            executeCheck(as_check(node));
            break;
        case NodeType::Call: {
            auto call = as_call(node);
            if (call) callScene(call->target());
            break;
        }
        case NodeType::Return:
            returnFromCall();
            break;
        case NodeType::Label:
            break;
        default:
            break;
    }
}

void NovaVM::executeDialogue(const DialogueNode* node) {
    if (!node) return;
    DialogueState diag;
    diag.isShow = true;
    diag.speaker = node->speaker();
    diag.emotion = node->emotion();
    diag.text = node->text();
    auto it = m_characterDefinitions.find(diag.speaker);
    if (it != m_characterDefinitions.end()) {
        diag.color = it->second.color;
    }
    m_state.dialogue = diag;

    const std::string spriteUrl = resolveCharacterSprite(diag.speaker, diag.emotion);
    if (!spriteUrl.empty()) {
        auto spriteIt = std::find_if(m_state.sprites.begin(), m_state.sprites.end(),
            [&](const SpriteState& s) { return s.id == diag.speaker; });
        if (spriteIt != m_state.sprites.end()) {
            spriteIt->url = spriteUrl;
        } else {
            SpriteState sprite;
            sprite.id = diag.speaker;
            sprite.url = spriteUrl;
            m_state.sprites.push_back(sprite);
        }
    }
}

void NovaVM::executeNarrator(const NarratorNode* node) {
    if (!node) return;
    m_state.dialogue = DialogueState{true, "", node->text(), "", ""};
}

void NovaVM::executeJump(const JumpNode* node) {
    if (!node) return;
    if (!node->target().empty() && node->target()[0] == '.') {
        jumpToLabel(node->target().substr(1));
    } else {
        jumpToScene(node->target());
    }
}

void NovaVM::executeChoice(const ChoiceNode* node) {
    if (!node) return;
    
    ChoiceState choice;
    choice.isShow = true;
    choice.question = node->question();
    
    int idx = 0;
    for (const auto& opt : node->options()) {
        auto optNode = as_choice_option(opt.get());
        if (optNode) {
            ChoiceOption co;
            co.id = std::to_string(idx++);
            co.text = optNode->text();
            co.target = optNode->target();
            co.disabled = optNode->condition() && !evaluateCondition(optNode->condition());
            choice.options.push_back(co);
        }
    }
    
    m_state.choice = choice;
    m_state.status = VMStatus::WaitingChoice;
}

void NovaVM::executeBranch(const BranchNode* node) {
    if (!node) return;
    
    bool cond = evaluateCondition(node->condition());
    auto& branch = cond ? node->then_branch() : node->else_branch();
    
    for (const auto& stmt : branch) {
        executeStatement(stmt.get());
        if (m_state.status == VMStatus::WaitingChoice || 
            m_state.status == VMStatus::Ended) {
            return;
        }
    }
}

void NovaVM::executeSet(const SetCommandNode* node) {
    if (!node) return;
    m_variables.set(node->name(), evaluateExpression(node->value()));
    markRuntimeStateChanged(RuntimeStateChangeVariables);
}

void NovaVM::executeGive(const GiveCommandNode* node) {
    if (!node) return;
    int count = static_cast<int>(evaluateAsNumber(node->count()));
    m_inventory.add(node->item(), count);
    markRuntimeStateChanged(RuntimeStateChangeInventory);
}

void NovaVM::executeTake(const TakeCommandNode* node) {
    if (!node) return;
    int count = static_cast<int>(evaluateAsNumber(node->count()));
    if (m_inventory.remove(node->item(), count)) {
        markRuntimeStateChanged(RuntimeStateChangeInventory);
    }
}

void NovaVM::executeBg(const BgCommandNode* node) {
    if (!node) return;
    m_state.bg = node->image();
    for (const auto& arg : node->args()) {
        if (arg.key == "transition") {
            m_state.bgTransition = arg.value;
        }
    }
}

void NovaVM::executeSprite(const SpriteCommandNode* node) {
    if (!node) return;

    const std::string spriteId = node->name();

    bool hasX = false, hasY = false, hasPosition = false;
    bool hasOpacity = false, hasZIndex = false, hasUrl = false;

    std::string url;
    double x = 0.0, y = 0.0, opacity = 1.0;
    int zIndex = 0;
    std::string position;

    for (const auto& arg : node->args()) {
        if (arg.key == "x") { x = safe_stod(arg.value, 0.0); hasX = true; }
        else if (arg.key == "y") { y = safe_stod(arg.value, 0.0); hasY = true; }
        else if (arg.key == "position") { position = arg.value; hasPosition = true; }
        else if (arg.key == "opacity") { opacity = safe_stod(arg.value, 1.0); hasOpacity = true; }
        else if (arg.key == "zIndex") { zIndex = safe_stoi(arg.value, 0); hasZIndex = true; }
        else if (arg.key == "url") { url = arg.value; hasUrl = true; }
    }

    auto it = std::find_if(m_state.sprites.begin(), m_state.sprites.end(),
        [&](const SpriteState& s) { return s.id == spriteId; });

    if (it != m_state.sprites.end()) {
        if (hasX) it->x = x;
        if (hasY) it->y = y;
        if (hasPosition) it->position = position;
        if (hasOpacity) it->opacity = opacity;
        if (hasZIndex) it->zIndex = zIndex;
        if (hasUrl) it->url = url;

        if (!hasX && hasPosition) {
            it->x = mapPositionToX(it->position);
        }
    } else {
        SpriteState sprite;
        sprite.id = spriteId;
        sprite.url = url;
        sprite.x = x;
        sprite.y = y;
        sprite.position = position;
        sprite.opacity = opacity;
        sprite.zIndex = zIndex;

        if (!hasX && hasPosition) {
            sprite.x = mapPositionToX(position);
        }

        m_state.sprites.push_back(sprite);
    }
}

void NovaVM::executeBgm(const BgmCommandNode* node) {
    if (!node) return;
    m_state.bgm = node->file();
    for (const auto& arg : node->args()) {
        if (arg.key == "volume") m_state.bgmVolume = safe_stod(arg.value, m_state.bgmVolume);
        else if (arg.key == "loop") m_state.bgmLoop = (arg.value == "true");
    }
}

void NovaVM::executeSfx(const SfxCommandNode* node) {
    if (!node) return;
    SfxState sfx;
    sfx.id = node->file();
    sfx.path = node->file();
    for (const auto& arg : node->args()) {
        if (arg.key == "volume") sfx.volume = safe_stod(arg.value, sfx.volume);
        else if (arg.key == "loop") sfx.loop = (arg.value == "true");
    }
    m_state.sfx.push_back(sfx);
}

void NovaVM::executeEnding(const EndingNode* node) {
    if (!node) return;
    m_playthrough.triggerEnding(node->name());
    m_state.ending = node->name();
    m_state.status = VMStatus::Ended;
}

void NovaVM::executeFlag(const FlagNode* node) {
    if (!node) return;
    m_playthrough.setFlag(node->name());
}

void NovaVM::executeCheck(const CheckCommandNode* node) {
    if (!node) return;
    
    bool success = evaluateCondition(node->condition());
    auto& branch = success ? node->success_branch() : node->failure_branch();
    
    for (const auto& stmt : branch) {
        executeStatement(stmt.get());
    }
}

VarValue NovaVM::evaluateExpression(const AstNode* expr) {
    if (!expr) return 0.0;
    
    if (auto lit = as_literal(expr)) {
        if (lit->is_number()) return lit->as_number();
        if (lit->is_string()) return lit->as_string();
        if (lit->is_bool()) return lit->as_bool();
        return 0.0;
    }
    
    if (auto id = as_identifier(expr)) {
        auto val = m_variables.get(id->name());
        return val.value_or(0.0);
    }
    
    if (auto bin = as_binary(expr)) {
        double left = evaluateAsNumber(bin->left());
        double right = evaluateAsNumber(bin->right());
        const auto& op = bin->op();
        
        if (op == "+") return left + right;
        if (op == "-") return left - right;
        if (op == "*") return left * right;
        if (op == "/") return right != 0 ? left / right : 0.0;
        if (op == "%") return right != 0 ? std::fmod(left, right) : 0.0;
        if (op == "and") return evaluateCondition(bin->left()) && evaluateCondition(bin->right());
        if (op == "or") return evaluateCondition(bin->left()) || evaluateCondition(bin->right());
        if (op == "<") return left < right;
        if (op == "<=") return left <= right;
        if (op == ">") return left > right;
        if (op == ">=") return left >= right;
        if (op == "==") return left == right;
        if (op == "!=") return left != right;
    }
    
    if (auto unary = as_unary(expr)) {
        if (unary->op() == "not") {
            return !evaluateCondition(unary->operand());
        }
        if (unary->op() == "-") {
            return -evaluateAsNumber(unary->operand());
        }
    }
    
    if (auto call = as_call_expr(expr)) {
        return evaluateFunctionCall(call);
    }
    
    if (auto dice = as_dice(expr)) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(1, dice->sides());
        int total = 0;
        for (int i = 0; i < dice->count(); ++i) {
            total += dist(rng);
        }
        return static_cast<double>(total + dice->modifier());
    }
    
    return 0.0;
}

bool NovaVM::evaluateCondition(const AstNode* expr) {
    if (!expr) return false;
    
    auto val = evaluateExpression(expr);
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val);
    }
    if (std::holds_alternative<double>(val)) {
        return std::get<double>(val) != 0.0;
    }
    return false;
}

double NovaVM::evaluateAsNumber(const AstNode* expr) {
    auto val = evaluateExpression(expr);
    if (std::holds_alternative<double>(val)) {
        return std::get<double>(val);
    }
    return 0.0;
}

std::string NovaVM::evaluateAsString(const AstNode* expr) {
    auto val = evaluateExpression(expr);
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    }
    return m_variables.asString("");
}

VarValue NovaVM::evaluateFunctionCall(const CallExprNode* call) {
    if (!call) return 0.0;
    
    const auto& name = call->name();
    const auto& args = call->arguments();
    
    if (name == "has_ending" && !args.empty()) {
        const std::string target = get_string_like_argument(args[0].get());
        return !target.empty() ? m_playthrough.hasEnding(target) : false;
    }
    if (name == "has_flag" && !args.empty()) {
        const std::string target = get_string_like_argument(args[0].get());
        return !target.empty() ? m_playthrough.hasFlag(target) : false;
    }
    if (name == "has_item" && !args.empty()) {
        const std::string target = get_string_like_argument(args[0].get());
        return !target.empty() ? m_inventory.has(target) : false;
    }
    if (name == "item_count" && !args.empty()) {
        const std::string target = get_string_like_argument(args[0].get());
        return !target.empty() ? static_cast<double>(m_inventory.count(target)) : 0.0;
    }
    if (name == "roll" && !args.empty()) {
        auto lit = as_literal(args[0].get());
        if (lit && lit->is_string()) {
            return evaluateDiceRoll(lit->as_string());
        }
    }
    if (name == "random" && args.size() == 2) {
        int minValue = static_cast<int>(evaluateAsNumber(args[0].get()));
        int maxValue = static_cast<int>(evaluateAsNumber(args[1].get()));
        if (minValue > maxValue) {
            std::swap(minValue, maxValue);
        }
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return static_cast<double>(dist(rng));
    }
    if (name == "chance" && args.size() == 1) {
        double probability = evaluateAsNumber(args[0].get());
        if (probability < 0.0) probability = 0.0;
        if (probability > 1.0) probability = 1.0;
        static std::mt19937 rng(std::random_device{}());
        std::bernoulli_distribution dist(probability);
        return dist(rng);
    }
    
    return 0.0;
}

double NovaVM::evaluateDiceRoll(const std::string& expr) {
    int count = 1, sides = 6, modifier = 0;
    size_t dPos = expr.find('d');
    
    if (dPos != std::string::npos) {
        if (dPos > 0) count = safe_stoi(expr.substr(0, dPos), 1);
        size_t plusPos = expr.find('+', dPos);
        size_t minusPos = expr.find('-', dPos);
        
        if (plusPos != std::string::npos) {
            sides = safe_stoi(expr.substr(dPos + 1, plusPos - dPos - 1), 6);
            modifier = safe_stoi(expr.substr(plusPos + 1), 0);
        } else if (minusPos != std::string::npos) {
            sides = safe_stoi(expr.substr(dPos + 1, minusPos - dPos - 1), 6);
            modifier = -safe_stoi(expr.substr(minusPos + 1), 0);
        } else {
            sides = safe_stoi(expr.substr(dPos + 1), 6);
        }
    }
    
    if (count < 1) count = 1;
    if (sides < 1) sides = 6;
    
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, sides);
    int total = 0;
    for (int i = 0; i < count; ++i) {
        total += dist(rng);
    }
    return static_cast<double>(total + modifier);
}

} // namespace nova
