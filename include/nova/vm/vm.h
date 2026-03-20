#pragma once

#include "nova/vm/state.h"
#include "nova/vm/variable.h"
#include "nova/vm/inventory.h"
#include "nova/vm/save_data.h"
#include "nova/ast/ast_node.h"
#include <cstdint>
#include <memory>
#include <functional>
#include <vector>

namespace nova {

/// @brief 场景数据
struct SceneData {
    std::string name;
    std::string title;
    size_t statementStart = 0;
    size_t statementCount = 0;
    std::unordered_map<std::string, size_t> labels;
};

struct CharacterDefinition {
    std::string id;
    std::string color;
    std::string description;
};

struct ItemDefinition {
    std::string id;
    std::string name;
    std::string description;
};

/// @brief NovaMark 虚拟机
class NovaVM {
public:
    using ChoiceCallback = std::function<void(int)>;

    enum RuntimeStateChangeFlag {
        RuntimeStateChangeNone = 0,
        RuntimeStateChangeVariables = 1 << 0,
        RuntimeStateChangeInventory = 1 << 1
    };
    
    NovaVM();
    ~NovaVM() = default;
    
    /// @brief 加载 AST 程序
    void load(const ProgramNode* program);
    void load(const AstPtr& program);
    
    /// @brief 获取当前渲染状态
    const NovaState& state() const { return m_state; }

    void setTextConfig(const TextConfigState& config) { m_state.textConfig = config; }

    /// @brief 消费当前对话（渲染层展示后调用）
    void consumeDialogue();
    
    /// @brief 捕获当前游戏状态
    GameState captureState() const;
    
    /// @brief 从存档恢复游戏状态
    bool loadSave(const SaveData& save);
    
    /// @brief 从存档恢复游戏状态
    bool loadSave(const GameState& state);
    
    /// @brief 执行一步（直到需要用户输入）
    void step();
    
    /// @brief 执行一步（advance 的别名，用于 C API）
    void advance() { step(); }
    
    /// @brief 执行到下一个等待点
    void run();
    
    /// @brief 选择选项（按索引）
    void selectChoice(int index);
    
    /// @brief 选择选项（按 ID）
    bool selectChoiceById(const std::string& choiceId);
    
    /// @brief 获取当前场景名
    const std::string& currentScene() const { return m_currentScene; }
    
    /// @brief 获取当前语句索引
    size_t statementIndex() const { return m_statementIndex; }
    
    /// @brief 设置场景入口
    void setEntryPoint(const std::string& sceneName);
    
    /// @brief 跳转到场景
    bool jumpToScene(const std::string& sceneName);
    
    /// @brief 跳转到标签
    bool jumpToLabel(const std::string& labelName);
    
    /// @brief 调用场景（保存返回点）
    void callScene(const std::string& sceneName);
    
    /// @brief 返回调用点
    void returnFromCall();
    
    /// @brief 变量管理器
    VariableManager& variables() { return m_variables; }
    const VariableManager& variables() const { return m_variables; }
    
    /// @brief 背包管理器
    Inventory& inventory() { return m_inventory; }
    const Inventory& inventory() const { return m_inventory; }

    const std::unordered_map<std::string, CharacterDefinition>& characterDefinitions() const { return m_characterDefinitions; }
    const std::unordered_map<std::string, ItemDefinition>& itemDefinitions() const { return m_itemDefinitions; }
    
    /// @brief 周目状态
    PlaythroughState& playthrough() { return m_playthrough; }
    const PlaythroughState& playthrough() const { return m_playthrough; }
    
    /// @brief 存档管理器
    SaveManager& saves() { return m_saveManager; }
    const SaveManager& saves() const { return m_saveManager; }
    
    /// @brief 重置 VM
    void reset();
    
    /// @brief 设置延迟回调
    void setDelayCallback(std::function<void(double)> callback) {
        m_delayCallback = std::move(callback);
    }

    uint64_t runtimeStateVersion() const { return m_runtimeStateVersion; }
    int consumeRuntimeStateChangeFlags();

private:
    NovaState m_state;
    VariableManager m_variables;
    Inventory m_inventory;
    PlaythroughState m_playthrough;
    SaveManager m_saveManager;
    
    const ProgramNode* m_program = nullptr;
    std::unordered_map<std::string, SceneData> m_scenes;
    std::unordered_map<std::string, CharacterDefinition> m_characterDefinitions;
    std::unordered_map<std::string, ItemDefinition> m_itemDefinitions;

    std::vector<std::string> m_scene_order;
    std::unordered_map<std::string, size_t> m_scene_order_index;
    
    std::string m_currentScene;
    size_t m_statementIndex = 0;
    
    std::vector<std::pair<std::string, size_t>> m_callStack;
    
    std::function<void(double)> m_delayCallback;
    uint64_t m_runtimeStateVersion = 0;
    int m_runtimeStateChangeFlags = RuntimeStateChangeNone;
    
    void buildSceneIndex();
    void buildDefinitionRegistry();
    void executeGlobalStatements();
    void applyFrontMatterDefaults();
    void executeStatement(const AstNode* node);
    void executeDialogue(const DialogueNode* node);
    void executeNarrator(const NarratorNode* node);
    void executeJump(const JumpNode* node);
    void executeChoice(const ChoiceNode* node);
    void executeBranch(const BranchNode* node);
    void executeSet(const SetCommandNode* node);
    void executeGive(const GiveCommandNode* node);
    void executeTake(const TakeCommandNode* node);
    void executeBg(const BgCommandNode* node);
    void executeSprite(const SpriteCommandNode* node);
    void executeBgm(const BgmCommandNode* node);
    void executeSfx(const SfxCommandNode* node);
    void executeEnding(const EndingNode* node);
    void executeFlag(const FlagNode* node);
    void executeSave(const SaveNode* node);
    void executeWait(const WaitNode* node);
    void executeCheck(const CheckCommandNode* node);
    
    VarValue evaluateExpression(const AstNode* expr);
    bool evaluateCondition(const AstNode* expr);
    double evaluateAsNumber(const AstNode* expr);
    std::string evaluateAsString(const AstNode* expr);
    VarValue evaluateFunctionCall(const CallExprNode* call);
    double evaluateDiceRoll(const std::string& expr);
    void markRuntimeStateChanged(int flags);
};

} // namespace nova
