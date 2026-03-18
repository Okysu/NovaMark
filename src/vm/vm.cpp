#include "nova/vm/vm.h"
#include "nova/vm/state.h"
#include <random>
#include <stdexcept>
#include <sstream>

namespace nova {

namespace {

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

const SaveNode* as_save(const AstNode* node) {
    return dynamic_cast<const SaveNode*>(node);
}

const WaitNode* as_wait(const AstNode* node) {
    return dynamic_cast<const WaitNode*>(node);
}

const UiCommandNode* as_ui_cmd(const AstNode* node) {
    return dynamic_cast<const UiCommandNode*>(node);
}

const UiTrackNode* as_ui_track(const AstNode* node) {
    return dynamic_cast<const UiTrackNode*>(node);
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

}

NovaVM::NovaVM() {
    m_state.status = VMStatus::Running;
}

void NovaVM::consumeDialogue() {
    m_state.clearDialogue();
}

GameState NovaVM::captureState() const {
    GameState state;
    state.currentScene = m_currentScene;
    state.statementIndex = m_statementIndex;
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
    
    for (const auto& ending : state.triggeredEndings) {
        m_playthrough.triggerEnding(ending);
    }
    for (const auto& flag : state.flags) {
        m_playthrough.setFlag(flag);
    }
    
    m_state.currentScene = m_currentScene;
    m_state.status = VMStatus::Running;
    
    return true;
}

void NovaVM::load(const ProgramNode* program) {
    m_program = program;
    buildSceneIndex();
    m_statementIndex = 0;
    m_state.status = VMStatus::Running;
    
    executeGlobalStatements();
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
            m_inventory.add(give->item(), give->count());
        } else if (auto take = as_take(stmt.get())) {
            m_inventory.remove(take->item(), take->count());
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

void NovaVM::step() {
    if (m_state.status == VMStatus::Ended) return;
    if (m_state.status == VMStatus::WaitingChoice) return;
    
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
            m_state.status == VMStatus::WaitingDelay ||
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

void NovaVM::run() {
    while (m_state.status == VMStatus::Running) {
        step();
    }
}

void NovaVM::selectChoice(int index) {
    if (m_state.status != VMStatus::WaitingChoice || !m_state.choice) return;
    
    if (index < 0 || index >= static_cast<int>(m_state.choice->options.size())) return;
    
    ChoiceOption opt = m_state.choice->options[index];
    m_state.clearChoice();
    m_state.clearDialogue();  // 清除旧对话，避免重复显示
    m_state.status = VMStatus::Running;
    
    if (!opt.target.empty() && opt.target[0] == '.') {
        jumpToLabel(opt.target.substr(1));
    } else {
        jumpToScene(opt.target);
    }
}

bool NovaVM::selectChoiceById(const std::string& choiceId) {
    if (m_state.status != VMStatus::WaitingChoice || !m_state.choice) return false;
    
    for (int i = 0; i < static_cast<int>(m_state.choice->options.size()); ++i) {
        if (m_state.choice->options[i].id == choiceId) {
            selectChoice(i);
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
        case NodeType::Save:
            executeSave(as_save(node));
            break;
        case NodeType::Wait:
            executeWait(as_wait(node));
            break;
        case NodeType::UiCommand:
            executeUiCommand(as_ui_cmd(node));
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
    m_state.dialogue = diag;
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
}

void NovaVM::executeGive(const GiveCommandNode* node) {
    if (!node) return;
    m_inventory.add(node->item(), node->count());
}

void NovaVM::executeTake(const TakeCommandNode* node) {
    if (!node) return;
    m_inventory.remove(node->item(), node->count());
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
    
    SpriteState sprite;
    sprite.id = node->name();
    
    for (const auto& arg : node->args()) {
        if (arg.key == "x") sprite.x = safe_stod(arg.value, sprite.x);
        else if (arg.key == "y") sprite.y = safe_stod(arg.value, sprite.y);
        else if (arg.key == "opacity") sprite.opacity = safe_stod(arg.value, sprite.opacity);
        else if (arg.key == "zIndex") sprite.zIndex = safe_stoi(arg.value, sprite.zIndex);
        else if (arg.key == "url") sprite.url = arg.value;
    }
    
    auto it = std::find_if(m_state.sprites.begin(), m_state.sprites.end(),
        [&](const SpriteState& s) { return s.id == sprite.id; });
    
    if (it != m_state.sprites.end()) {
        *it = sprite;
    } else {
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

void NovaVM::executeSave(const SaveNode* node) {
    if (!node) return;
    
    GameState state;
    state.currentScene = m_currentScene;
    state.statementIndex = m_statementIndex;
    state.callStack = m_callStack;
    state.numberVariables = m_variables.getAllNumbers();
    state.stringVariables = m_variables.getAllStrings();
    state.boolVariables = m_variables.getAllBools();
    state.inventory = m_inventory.getAllItems();
    state.triggeredEndings = m_playthrough.endings();
    state.flags = m_playthrough.flags();
    
    m_saveManager.createSave(node->label(), state);
}

void NovaVM::executeWait(const WaitNode* node) {
    if (!node) return;
    if (m_delayCallback) {
        m_delayCallback(node->seconds());
    }
}

void NovaVM::executeUiCommand(const UiCommandNode* node) {
    if (!node) return;
    
    auto it = std::find_if(m_state.huds.begin(), m_state.huds.end(),
        [&](const HudState& h) { return h.id == node->target(); });
    
    if (it != m_state.huds.end()) {
        it->show = (node->action() == UiCommandNode::Action::Show);
    } else {
        HudState hud;
        hud.id = node->target();
        hud.show = (node->action() == UiCommandNode::Action::Show);
        m_state.huds.push_back(hud);
    }
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
        auto id = as_identifier(args[0].get());
        return id ? m_playthrough.hasEnding(id->name()) : false;
    }
    if (name == "has_flag" && !args.empty()) {
        auto id = as_identifier(args[0].get());
        return id ? m_playthrough.hasFlag(id->name()) : false;
    }
    if (name == "has_item" && !args.empty()) {
        auto id = as_identifier(args[0].get());
        return id ? m_inventory.has(id->name()) : false;
    }
    if (name == "item_count" && !args.empty()) {
        auto id = as_identifier(args[0].get());
        return id ? static_cast<double>(m_inventory.count(id->name())) : 0.0;
    }
    if (name == "roll" && !args.empty()) {
        auto lit = as_literal(args[0].get());
        if (lit && lit->is_string()) {
            return evaluateDiceRoll(lit->as_string());
        }
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
