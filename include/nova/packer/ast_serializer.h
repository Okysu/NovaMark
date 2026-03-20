#pragma once

#include "nova/ast/ast_node.h"
#include "nova/packer/nvmp_format.h"
#include <vector>
#include <functional>

namespace nova {

/// @brief AST 序列化器，将 AST 节点序列化为字节码
class AstSerializer {
public:
    AstSerializer();
    
    /// @brief 序列化整个程序
    std::vector<uint8_t> serialize(const ProgramNode* program);
    
    /// @brief 获取收集到的资源引用
    const std::vector<std::string>& getAssetReferences() const { return m_assetRefs; }

private:
    BytecodeWriter m_writer;
    std::vector<std::string> m_assetRefs;
    
    void serializeNode(const AstNode* node);
    void serializeDialogue(const DialogueNode* node);
    void serializeNarrator(const NarratorNode* node);
    void serializeSceneDef(const SceneDefNode* node);
    void serializeJump(const JumpNode* node);
    void serializeChoice(const ChoiceNode* node);
    void serializeChoiceOption(const ChoiceOptionNode* node);
    void serializeVarDef(const VarDefNode* node);
    void serializeBranch(const BranchNode* node);
    void serializeCharDef(const CharDefNode* node);
    void serializeItemDef(const ItemDefNode* node);
    void serializeBgCommand(const BgCommandNode* node);
    void serializeSpriteCommand(const SpriteCommandNode* node);
    void serializeBgmCommand(const BgmCommandNode* node);
    void serializeSfxCommand(const SfxCommandNode* node);
    void serializeSetCommand(const SetCommandNode* node);
    void serializeGiveCommand(const GiveCommandNode* node);
    void serializeTakeCommand(const TakeCommandNode* node);
    void serializeSave(const SaveNode* node);
    void serializeCall(const CallNode* node);
    void serializeReturn(const ReturnNode* node);
    void serializeEnding(const EndingNode* node);
    void serializeFlag(const FlagNode* node);
    void serializeLabel(const LabelNode* node);
    void serializeCheckCommand(const CheckCommandNode* node);
    void serializeWait(const WaitNode* node);
    void serializeThemeDef(const ThemeDefNode* node);
    void serializeFrontMatter(const FrontMatterNode* node);
    
    void serializeExpression(const AstNode* expr);
    void serializeLiteral(const LiteralNode* node);
    void serializeIdentifier(const IdentifierNode* node);
    void serializeBinaryExpr(const BinaryExprNode* node);
    void serializeUnaryExpr(const UnaryExprNode* node);
    void serializeCallExpr(const CallExprNode* node);
    void serializeDiceExpr(const DiceExprNode* node);
    
    void recordAssetRef(const std::string& path);
};

/// @brief AST 反序列化器，从字节码重建 AST
class AstDeserializer {
public:
    AstDeserializer();
    ~AstDeserializer();
    
    /// @brief 反序列化字节码为 AST
    std::unique_ptr<ProgramNode> deserialize(const std::vector<uint8_t>& bytecode);
    
    /// @brief 检查是否有错误
    bool hasError() const { return m_hasError; }
    const std::string& errorMessage() const { return m_errorMsg; }

private:
    BytecodeReader* m_reader = nullptr;
    bool m_hasError = false;
    std::string m_errorMsg;
    
    std::unique_ptr<AstNode> deserializeNode();
    std::unique_ptr<DialogueNode> deserializeDialogue();
    std::unique_ptr<NarratorNode> deserializeNarrator();
    std::unique_ptr<SceneDefNode> deserializeSceneDef();
    std::unique_ptr<JumpNode> deserializeJump();
    std::unique_ptr<ChoiceNode> deserializeChoice();
    std::unique_ptr<VarDefNode> deserializeVarDef();
    std::unique_ptr<BranchNode> deserializeBranch();
    std::unique_ptr<CharDefNode> deserializeCharDef();
    std::unique_ptr<ItemDefNode> deserializeItemDef();
    std::unique_ptr<BgCommandNode> deserializeBgCommand();
    std::unique_ptr<SpriteCommandNode> deserializeSpriteCommand();
    std::unique_ptr<BgmCommandNode> deserializeBgmCommand();
    std::unique_ptr<SfxCommandNode> deserializeSfxCommand();
    std::unique_ptr<SetCommandNode> deserializeSetCommand();
    std::unique_ptr<GiveCommandNode> deserializeGiveCommand();
    std::unique_ptr<TakeCommandNode> deserializeTakeCommand();
    std::unique_ptr<SaveNode> deserializeSave();
    std::unique_ptr<CallNode> deserializeCall();
    std::unique_ptr<ReturnNode> deserializeReturn();
    std::unique_ptr<EndingNode> deserializeEnding();
    std::unique_ptr<FlagNode> deserializeFlag();
    std::unique_ptr<LabelNode> deserializeLabel();
    std::unique_ptr<CheckCommandNode> deserializeCheckCommand();
    std::unique_ptr<WaitNode> deserializeWait();
    std::unique_ptr<ThemeDefNode> deserializeThemeDef();
    std::unique_ptr<FrontMatterNode> deserializeFrontMatter();
    
    std::unique_ptr<AstNode> deserializeExpression();
    std::unique_ptr<LiteralNode> deserializeLiteral();
    std::unique_ptr<IdentifierNode> deserializeIdentifier();
    std::unique_ptr<BinaryExprNode> deserializeBinaryExpr();
    std::unique_ptr<UnaryExprNode> deserializeUnaryExpr();
    std::unique_ptr<CallExprNode> deserializeCallExpr();
    std::unique_ptr<DiceExprNode> deserializeDiceExpr();
    
    void setError(const std::string& msg);
};

} // namespace nova
