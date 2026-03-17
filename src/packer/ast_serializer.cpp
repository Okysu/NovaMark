#include "nova/packer/ast_serializer.h"
#include <cstring>

namespace nova {

AstSerializer::AstSerializer() {}

std::vector<uint8_t> AstSerializer::serialize(const ProgramNode* program) {
    m_writer.clear();
    m_assetRefs.clear();
    
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeProgram));
    
    uint32_t stmtCount = static_cast<uint32_t>(program->statements().size());
    m_writer.writeU32(stmtCount);
    
    for (const auto& stmt : program->statements()) {
        serializeNode(stmt.get());
    }
    
    m_writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));
    
    return m_writer.data();
}

void AstSerializer::serializeNode(const AstNode* node) {
    if (!node) {
        m_writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));
        return;
    }
    
    switch (node->type()) {
        case NodeType::Dialogue:
            serializeDialogue(dynamic_cast<const DialogueNode*>(node));
            break;
        case NodeType::Narrator:
            serializeNarrator(dynamic_cast<const NarratorNode*>(node));
            break;
        case NodeType::SceneDef:
            serializeSceneDef(dynamic_cast<const SceneDefNode*>(node));
            break;
        case NodeType::Jump:
            serializeJump(dynamic_cast<const JumpNode*>(node));
            break;
        case NodeType::Choice:
            serializeChoice(dynamic_cast<const ChoiceNode*>(node));
            break;
        case NodeType::VarDef:
            serializeVarDef(dynamic_cast<const VarDefNode*>(node));
            break;
        case NodeType::Branch:
            serializeBranch(dynamic_cast<const BranchNode*>(node));
            break;
        case NodeType::CharDef:
            serializeCharDef(dynamic_cast<const CharDefNode*>(node));
            break;
        case NodeType::ItemDef:
            serializeItemDef(dynamic_cast<const ItemDefNode*>(node));
            break;
        case NodeType::BgCommand:
            serializeBgCommand(dynamic_cast<const BgCommandNode*>(node));
            break;
        case NodeType::SpriteCommand:
            serializeSpriteCommand(dynamic_cast<const SpriteCommandNode*>(node));
            break;
        case NodeType::BgmCommand:
            serializeBgmCommand(dynamic_cast<const BgmCommandNode*>(node));
            break;
        case NodeType::SfxCommand:
            serializeSfxCommand(dynamic_cast<const SfxCommandNode*>(node));
            break;
        case NodeType::SetCommand:
            serializeSetCommand(dynamic_cast<const SetCommandNode*>(node));
            break;
        case NodeType::GiveCommand:
            serializeGiveCommand(dynamic_cast<const GiveCommandNode*>(node));
            break;
        case NodeType::TakeCommand:
            serializeTakeCommand(dynamic_cast<const TakeCommandNode*>(node));
            break;
        case NodeType::Save:
            serializeSave(dynamic_cast<const SaveNode*>(node));
            break;
        case NodeType::Call:
            serializeCall(dynamic_cast<const CallNode*>(node));
            break;
        case NodeType::Return:
            serializeReturn(dynamic_cast<const ReturnNode*>(node));
            break;
        case NodeType::Ending:
            serializeEnding(dynamic_cast<const EndingNode*>(node));
            break;
        case NodeType::Flag:
            serializeFlag(dynamic_cast<const FlagNode*>(node));
            break;
        case NodeType::Label:
            serializeLabel(dynamic_cast<const LabelNode*>(node));
            break;
        case NodeType::CheckCommand:
            serializeCheckCommand(dynamic_cast<const CheckCommandNode*>(node));
            break;
        case NodeType::Wait:
            serializeWait(dynamic_cast<const WaitNode*>(node));
            break;
        case NodeType::UiCommand:
            serializeUiCommand(dynamic_cast<const UiCommandNode*>(node));
            break;
        case NodeType::ThemeDef:
            serializeThemeDef(dynamic_cast<const ThemeDefNode*>(node));
            break;
        case NodeType::FrontMatter:
            serializeFrontMatter(dynamic_cast<const FrontMatterNode*>(node));
            break;
        default:
            m_writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));
            break;
    }
}

void AstSerializer::serializeDialogue(const DialogueNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeDialogue));
    m_writer.writeString(node->speaker());
    m_writer.writeString(node->emotion());
    m_writer.writeString(node->text());
}

void AstSerializer::serializeNarrator(const NarratorNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeNarrator));
    m_writer.writeString(node->text());
}

void AstSerializer::serializeSceneDef(const SceneDefNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeSceneDef));
    m_writer.writeString(node->name());
    m_writer.writeString(node->title());
}

void AstSerializer::serializeJump(const JumpNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeJump));
    m_writer.writeString(node->target());
}

void AstSerializer::serializeChoice(const ChoiceNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeChoice));
    m_writer.writeString(node->question());
    
    uint32_t optCount = static_cast<uint32_t>(node->options().size());
    m_writer.writeU32(optCount);
    
    for (const auto& opt : node->options()) {
        serializeChoiceOption(dynamic_cast<const ChoiceOptionNode*>(opt.get()));
    }
}

void AstSerializer::serializeChoiceOption(const ChoiceOptionNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeChoiceOption));
    m_writer.writeString(node->text());
    m_writer.writeString(node->target());
    
    if (node->condition()) {
        m_writer.writeByte(1);
        serializeExpression(node->condition());
    } else {
        m_writer.writeByte(0);
    }
}

void AstSerializer::serializeVarDef(const VarDefNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeVarDef));
    m_writer.writeString(node->name());
    
    if (node->init_value()) {
        m_writer.writeByte(1);
        serializeExpression(node->init_value());
    } else {
        m_writer.writeByte(0);
    }
}

void AstSerializer::serializeBranch(const BranchNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeBranch));
    
    serializeExpression(node->condition());
    
    uint32_t thenCount = static_cast<uint32_t>(node->then_branch().size());
    m_writer.writeU32(thenCount);
    for (const auto& stmt : node->then_branch()) {
        serializeNode(stmt.get());
    }
    
    uint32_t elseCount = static_cast<uint32_t>(node->else_branch().size());
    m_writer.writeU32(elseCount);
    for (const auto& stmt : node->else_branch()) {
        serializeNode(stmt.get());
    }
}

void AstSerializer::serializeCharDef(const CharDefNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeCharDef));
    m_writer.writeString(node->name());
    
    uint32_t propCount = static_cast<uint32_t>(node->properties().size());
    m_writer.writeU32(propCount);
    for (const auto& prop : node->properties()) {
        m_writer.writeString(prop.key);
        m_writer.writeString(prop.value);
    }
}

void AstSerializer::serializeItemDef(const ItemDefNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeItemDef));
    m_writer.writeString(node->name());
    
    uint32_t propCount = static_cast<uint32_t>(node->properties().size());
    m_writer.writeU32(propCount);
    for (const auto& prop : node->properties()) {
        m_writer.writeString(prop.key);
        m_writer.writeString(prop.value);
    }
}

void AstSerializer::serializeBgCommand(const BgCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeBgCommand));
    m_writer.writeString(node->image());
    recordAssetRef(node->image());
    
    uint32_t argCount = static_cast<uint32_t>(node->args().size());
    m_writer.writeU32(argCount);
    for (const auto& arg : node->args()) {
        m_writer.writeString(arg.key);
        m_writer.writeString(arg.value);
    }
}

void AstSerializer::serializeSpriteCommand(const SpriteCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeSpriteCommand));
    m_writer.writeString(node->name());
    
    uint32_t argCount = static_cast<uint32_t>(node->args().size());
    m_writer.writeU32(argCount);
    for (const auto& arg : node->args()) {
        m_writer.writeString(arg.key);
        m_writer.writeString(arg.value);
    }
}

void AstSerializer::serializeBgmCommand(const BgmCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeBgmCommand));
    m_writer.writeString(node->file());
    recordAssetRef(node->file());
    
    uint32_t argCount = static_cast<uint32_t>(node->args().size());
    m_writer.writeU32(argCount);
    for (const auto& arg : node->args()) {
        m_writer.writeString(arg.key);
        m_writer.writeString(arg.value);
    }
}

void AstSerializer::serializeSfxCommand(const SfxCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeSfxCommand));
    m_writer.writeString(node->file());
    recordAssetRef(node->file());
    
    uint32_t argCount = static_cast<uint32_t>(node->args().size());
    m_writer.writeU32(argCount);
    for (const auto& arg : node->args()) {
        m_writer.writeString(arg.key);
        m_writer.writeString(arg.value);
    }
}

void AstSerializer::serializeSetCommand(const SetCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeSetCommand));
    m_writer.writeString(node->name());
    serializeExpression(node->value());
}

void AstSerializer::serializeGiveCommand(const GiveCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeGiveCommand));
    m_writer.writeString(node->item());
    m_writer.writeU32(static_cast<uint32_t>(node->count()));
}

void AstSerializer::serializeTakeCommand(const TakeCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeTakeCommand));
    m_writer.writeString(node->item());
    m_writer.writeU32(static_cast<uint32_t>(node->count()));
}

void AstSerializer::serializeSave(const SaveNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeSave));
    m_writer.writeString(node->label());
}

void AstSerializer::serializeCall(const CallNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeCall));
    m_writer.writeString(node->target());
}

void AstSerializer::serializeReturn(const ReturnNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeReturn));
}

void AstSerializer::serializeEnding(const EndingNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeEnding));
    m_writer.writeString(node->name());
}

void AstSerializer::serializeFlag(const FlagNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeFlag));
    m_writer.writeString(node->name());
}

void AstSerializer::serializeLabel(const LabelNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeLabel));
    m_writer.writeString(node->name());
}

void AstSerializer::serializeCheckCommand(const CheckCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeCheckCommand));
    serializeExpression(node->condition());
    
    uint32_t successCount = static_cast<uint32_t>(node->success_branch().size());
    m_writer.writeU32(successCount);
    for (const auto& stmt : node->success_branch()) {
        serializeNode(stmt.get());
    }
    
    uint32_t failureCount = static_cast<uint32_t>(node->failure_branch().size());
    m_writer.writeU32(failureCount);
    for (const auto& stmt : node->failure_branch()) {
        serializeNode(stmt.get());
    }
}

void AstSerializer::serializeWait(const WaitNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeWait));
    m_writer.writeDouble(node->seconds());
}

void AstSerializer::serializeUiCommand(const UiCommandNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeUiCommand));
    m_writer.writeByte(static_cast<uint8_t>(node->action()));
    m_writer.writeString(node->target());
}

void AstSerializer::serializeThemeDef(const ThemeDefNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeThemeDef));
    m_writer.writeString(node->name());
    
    uint32_t propCount = static_cast<uint32_t>(node->properties().size());
    m_writer.writeU32(propCount);
    for (const auto& prop : node->properties()) {
        m_writer.writeString(prop.key);
        m_writer.writeString(prop.value);
    }
}

void AstSerializer::serializeFrontMatter(const FrontMatterNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeFrontMatter));
    
    uint32_t propCount = static_cast<uint32_t>(node->properties().size());
    m_writer.writeU32(propCount);
    for (const auto& prop : node->properties()) {
        m_writer.writeString(prop.key);
        m_writer.writeString(prop.value);
    }
}

void AstSerializer::serializeExpression(const AstNode* expr) {
    if (!expr) {
        m_writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));
        return;
    }
    
    switch (expr->type()) {
        case NodeType::Literal:
            serializeLiteral(dynamic_cast<const LiteralNode*>(expr));
            break;
        case NodeType::Identifier:
            serializeIdentifier(dynamic_cast<const IdentifierNode*>(expr));
            break;
        case NodeType::BinaryExpr:
            serializeBinaryExpr(dynamic_cast<const BinaryExprNode*>(expr));
            break;
        case NodeType::UnaryExpr:
            serializeUnaryExpr(dynamic_cast<const UnaryExprNode*>(expr));
            break;
        case NodeType::CallExpr:
            serializeCallExpr(dynamic_cast<const CallExprNode*>(expr));
            break;
        case NodeType::DiceExpr:
            serializeDiceExpr(dynamic_cast<const DiceExprNode*>(expr));
            break;
        default:
            m_writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));
            break;
    }
}

void AstSerializer::serializeLiteral(const LiteralNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeLiteral));
    
    if (node->is_null()) {
        m_writer.writeByte(static_cast<uint8_t>(OpCode::LiteralNull));
    } else if (node->is_string()) {
        m_writer.writeByte(static_cast<uint8_t>(OpCode::LiteralString));
        m_writer.writeString(node->as_string());
    } else if (node->is_number()) {
        m_writer.writeByte(static_cast<uint8_t>(OpCode::LiteralNumber));
        m_writer.writeDouble(node->as_number());
    } else if (node->is_bool()) {
        m_writer.writeByte(static_cast<uint8_t>(OpCode::LiteralBool));
        m_writer.writeByte(node->as_bool() ? 1 : 0);
    }
}

void AstSerializer::serializeIdentifier(const IdentifierNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeIdentifier));
    m_writer.writeString(node->name());
}

void AstSerializer::serializeBinaryExpr(const BinaryExprNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeBinaryExpr));
    m_writer.writeString(node->op());
    serializeExpression(node->left());
    serializeExpression(node->right());
}

void AstSerializer::serializeUnaryExpr(const UnaryExprNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeUnaryExpr));
    m_writer.writeString(node->op());
    serializeExpression(node->operand());
}

void AstSerializer::serializeCallExpr(const CallExprNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeCallExpr));
    m_writer.writeString(node->name());
    
    uint32_t argCount = static_cast<uint32_t>(node->arguments().size());
    m_writer.writeU32(argCount);
    for (const auto& arg : node->arguments()) {
        serializeExpression(arg.get());
    }
}

void AstSerializer::serializeDiceExpr(const DiceExprNode* node) {
    m_writer.writeByte(static_cast<uint8_t>(OpCode::NodeDiceExpr));
    m_writer.writeU32(static_cast<uint32_t>(node->count()));
    m_writer.writeU32(static_cast<uint32_t>(node->sides()));
    m_writer.writeU32(static_cast<uint32_t>(node->modifier()));
}

void AstSerializer::recordAssetRef(const std::string& path) {
    if (!path.empty()) {
        m_assetRefs.push_back(path);
    }
}

} // namespace nova