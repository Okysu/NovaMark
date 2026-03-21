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
        if (arg.key == "url") {
            recordAssetRef(arg.value);
        }
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

// ============================================
// AstDeserializer
// ============================================

AstDeserializer::AstDeserializer() {}

AstDeserializer::~AstDeserializer() {
    delete m_reader;
}

std::unique_ptr<ProgramNode> AstDeserializer::deserialize(const std::vector<uint8_t>& bytecode) {
    delete m_reader;
    m_reader = new BytecodeReader(bytecode);
    m_hasError = false;
    m_errorMsg.clear();
    
    uint8_t op = m_reader->readByte();
    if (op != static_cast<uint8_t>(OpCode::NodeProgram)) {
        setError("Expected NodeProgram at start");
        return nullptr;
    }
    
    auto program = std::make_unique<ProgramNode>(SourceLocation{});
    
    uint32_t stmtCount = m_reader->readU32();
    for (uint32_t i = 0; i < stmtCount && !m_hasError; ++i) {
        auto stmt = deserializeNode();
        if (stmt) {
            program->add_statement(std::move(stmt));
        }
    }
    
    if (m_hasError) {
        return nullptr;
    }
    return program;
}

std::unique_ptr<AstNode> AstDeserializer::deserializeNode() {
    if (m_hasError) return nullptr;
    
    uint8_t op = m_reader->readByte();
    auto opCode = static_cast<OpCode>(op);
    
    switch (opCode) {
        case OpCode::NodeDialogue:
            return deserializeDialogue();
        case OpCode::NodeNarrator:
            return deserializeNarrator();
        case OpCode::NodeSceneDef:
            return deserializeSceneDef();
        case OpCode::NodeJump:
            return deserializeJump();
        case OpCode::NodeChoice:
            return deserializeChoice();
        case OpCode::NodeVarDef:
            return deserializeVarDef();
        case OpCode::NodeBranch:
            return deserializeBranch();
        case OpCode::NodeCharDef:
            return deserializeCharDef();
        case OpCode::NodeItemDef:
            return deserializeItemDef();
        case OpCode::NodeBgCommand:
            return deserializeBgCommand();
        case OpCode::NodeSpriteCommand:
            return deserializeSpriteCommand();
        case OpCode::NodeBgmCommand:
            return deserializeBgmCommand();
        case OpCode::NodeSfxCommand:
            return deserializeSfxCommand();
        case OpCode::NodeSetCommand:
            return deserializeSetCommand();
        case OpCode::NodeGiveCommand:
            return deserializeGiveCommand();
        case OpCode::NodeTakeCommand:
            return deserializeTakeCommand();
        case OpCode::NodeCall:
            return deserializeCall();
        case OpCode::NodeReturn:
            return deserializeReturn();
        case OpCode::NodeEnding:
            return deserializeEnding();
        case OpCode::NodeFlag:
            return deserializeFlag();
        case OpCode::NodeLabel:
            return deserializeLabel();
        case OpCode::NodeCheckCommand:
            return deserializeCheckCommand();
        case OpCode::NodeWait:
            return deserializeWait();
        case OpCode::NodeThemeDef:
            return deserializeThemeDef();
        case OpCode::NodeFrontMatter:
            return deserializeFrontMatter();
        case OpCode::EndNode:
            return nullptr;
        default:
            setError("Unknown node opcode: " + std::to_string(op));
            return nullptr;
    }
}

std::unique_ptr<DialogueNode> AstDeserializer::deserializeDialogue() {
    std::string speaker = m_reader->readString();
    std::string emotion = m_reader->readString();
    std::string text = m_reader->readString();
    
    return std::make_unique<DialogueNode>(SourceLocation{}, std::move(speaker), 
                                           std::move(emotion), std::move(text));
}

std::unique_ptr<NarratorNode> AstDeserializer::deserializeNarrator() {
    std::string text = m_reader->readString();
    return std::make_unique<NarratorNode>(SourceLocation{}, std::move(text));
}

std::unique_ptr<SceneDefNode> AstDeserializer::deserializeSceneDef() {
    std::string name = m_reader->readString();
    std::string title = m_reader->readString();
    return std::make_unique<SceneDefNode>(SourceLocation{}, std::move(name), std::move(title));
}

std::unique_ptr<JumpNode> AstDeserializer::deserializeJump() {
    std::string target = m_reader->readString();
    return std::make_unique<JumpNode>(SourceLocation{}, std::move(target));
}

std::unique_ptr<ChoiceNode> AstDeserializer::deserializeChoice() {
    std::string question = m_reader->readString();
    auto node = std::make_unique<ChoiceNode>(SourceLocation{}, std::move(question));
    
    uint32_t optCount = m_reader->readU32();
    for (uint32_t i = 0; i < optCount && !m_hasError; ++i) {
        uint8_t op = m_reader->readByte();
        if (op != static_cast<uint8_t>(OpCode::NodeChoiceOption)) {
            setError("Expected NodeChoiceOption");
            break;
        }
        
        std::string text = m_reader->readString();
        std::string target = m_reader->readString();
        
        std::unique_ptr<AstNode> condition;
        uint8_t hasCond = m_reader->readByte();
        if (hasCond) {
            condition = deserializeExpression();
        }
        
        node->add_option(std::make_unique<ChoiceOptionNode>(SourceLocation{}, 
            std::move(text), std::move(target), std::move(condition)));
    }
    
    return node;
}

std::unique_ptr<VarDefNode> AstDeserializer::deserializeVarDef() {
    std::string name = m_reader->readString();
    
    std::unique_ptr<AstNode> initValue;
    uint8_t hasInit = m_reader->readByte();
    if (hasInit) {
        initValue = deserializeExpression();
    }
    
    return std::make_unique<VarDefNode>(SourceLocation{}, std::move(name), std::move(initValue));
}

std::unique_ptr<BranchNode> AstDeserializer::deserializeBranch() {
    auto condition = deserializeExpression();
    auto node = std::make_unique<BranchNode>(SourceLocation{}, std::move(condition));
    
    uint32_t thenCount = m_reader->readU32();
    for (uint32_t i = 0; i < thenCount && !m_hasError; ++i) {
        auto stmt = deserializeNode();
        if (stmt) {
            node->add_then(std::move(stmt));
        }
    }
    
    uint32_t elseCount = m_reader->readU32();
    for (uint32_t i = 0; i < elseCount && !m_hasError; ++i) {
        auto stmt = deserializeNode();
        if (stmt) {
            node->add_else(std::move(stmt));
        }
    }
    
    return node;
}

std::unique_ptr<CharDefNode> AstDeserializer::deserializeCharDef() {
    std::string name = m_reader->readString();
    auto node = std::make_unique<CharDefNode>(SourceLocation{}, std::move(name));
    
    uint32_t propCount = m_reader->readU32();
    for (uint32_t i = 0; i < propCount; ++i) {
        std::string key = m_reader->readString();
        std::string value = m_reader->readString();
        node->add_property(std::move(key), std::move(value));
    }
    
    return node;
}

std::unique_ptr<ItemDefNode> AstDeserializer::deserializeItemDef() {
    std::string name = m_reader->readString();
    auto node = std::make_unique<ItemDefNode>(SourceLocation{}, std::move(name));
    
    uint32_t propCount = m_reader->readU32();
    for (uint32_t i = 0; i < propCount; ++i) {
        std::string key = m_reader->readString();
        std::string value = m_reader->readString();
        node->add_property(std::move(key), std::move(value));
    }
    
    return node;
}

std::unique_ptr<BgCommandNode> AstDeserializer::deserializeBgCommand() {
    std::string image = m_reader->readString();
    auto node = std::make_unique<BgCommandNode>(SourceLocation{}, std::move(image));
    
    uint32_t argCount = m_reader->readU32();
    for (uint32_t i = 0; i < argCount; ++i) {
        std::string key = m_reader->readString();
        std::string value = m_reader->readString();
        node->add_arg(std::move(key), std::move(value));
    }
    
    return node;
}

std::unique_ptr<SpriteCommandNode> AstDeserializer::deserializeSpriteCommand() {
    std::string name = m_reader->readString();
    auto node = std::make_unique<SpriteCommandNode>(SourceLocation{}, std::move(name));
    
    uint32_t argCount = m_reader->readU32();
    for (uint32_t i = 0; i < argCount; ++i) {
        std::string key = m_reader->readString();
        std::string value = m_reader->readString();
        node->add_arg(std::move(key), std::move(value));
    }
    
    return node;
}

std::unique_ptr<BgmCommandNode> AstDeserializer::deserializeBgmCommand() {
    std::string file = m_reader->readString();
    auto node = std::make_unique<BgmCommandNode>(SourceLocation{}, std::move(file));
    
    uint32_t argCount = m_reader->readU32();
    for (uint32_t i = 0; i < argCount; ++i) {
        std::string key = m_reader->readString();
        std::string value = m_reader->readString();
        node->add_arg(std::move(key), std::move(value));
    }
    
    return node;
}

std::unique_ptr<SfxCommandNode> AstDeserializer::deserializeSfxCommand() {
    std::string file = m_reader->readString();
    auto node = std::make_unique<SfxCommandNode>(SourceLocation{}, std::move(file));
    
    uint32_t argCount = m_reader->readU32();
    for (uint32_t i = 0; i < argCount; ++i) {
        std::string key = m_reader->readString();
        std::string value = m_reader->readString();
        node->add_arg(std::move(key), std::move(value));
    }
    
    return node;
}

std::unique_ptr<SetCommandNode> AstDeserializer::deserializeSetCommand() {
    std::string name = m_reader->readString();
    auto value = deserializeExpression();
    return std::make_unique<SetCommandNode>(SourceLocation{}, std::move(name), std::move(value));
}

std::unique_ptr<GiveCommandNode> AstDeserializer::deserializeGiveCommand() {
    std::string item = m_reader->readString();
    uint32_t count = m_reader->readU32();
    return std::make_unique<GiveCommandNode>(SourceLocation{}, std::move(item), static_cast<int>(count));
}

std::unique_ptr<TakeCommandNode> AstDeserializer::deserializeTakeCommand() {
    std::string item = m_reader->readString();
    uint32_t count = m_reader->readU32();
    return std::make_unique<TakeCommandNode>(SourceLocation{}, std::move(item), static_cast<int>(count));
}

std::unique_ptr<CallNode> AstDeserializer::deserializeCall() {
    std::string target = m_reader->readString();
    return std::make_unique<CallNode>(SourceLocation{}, std::move(target));
}

std::unique_ptr<ReturnNode> AstDeserializer::deserializeReturn() {
    return std::make_unique<ReturnNode>(SourceLocation{});
}

std::unique_ptr<EndingNode> AstDeserializer::deserializeEnding() {
    std::string name = m_reader->readString();
    return std::make_unique<EndingNode>(SourceLocation{}, std::move(name));
}

std::unique_ptr<FlagNode> AstDeserializer::deserializeFlag() {
    std::string name = m_reader->readString();
    return std::make_unique<FlagNode>(SourceLocation{}, std::move(name));
}

std::unique_ptr<LabelNode> AstDeserializer::deserializeLabel() {
    std::string name = m_reader->readString();
    return std::make_unique<LabelNode>(SourceLocation{}, std::move(name));
}

std::unique_ptr<CheckCommandNode> AstDeserializer::deserializeCheckCommand() {
    auto condition = deserializeExpression();
    auto node = std::make_unique<CheckCommandNode>(SourceLocation{}, std::move(condition));
    
    uint32_t successCount = m_reader->readU32();
    for (uint32_t i = 0; i < successCount && !m_hasError; ++i) {
        auto stmt = deserializeNode();
        if (stmt) {
            node->add_success(std::move(stmt));
        }
    }
    
    uint32_t failureCount = m_reader->readU32();
    for (uint32_t i = 0; i < failureCount && !m_hasError; ++i) {
        auto stmt = deserializeNode();
        if (stmt) {
            node->add_failure(std::move(stmt));
        }
    }
    
    return node;
}

std::unique_ptr<WaitNode> AstDeserializer::deserializeWait() {
    double seconds = m_reader->readDouble();
    return std::make_unique<WaitNode>(SourceLocation{}, seconds);
}

std::unique_ptr<ThemeDefNode> AstDeserializer::deserializeThemeDef() {
    std::string name = m_reader->readString();
    auto node = std::make_unique<ThemeDefNode>(SourceLocation{}, std::move(name));
    
    uint32_t propCount = m_reader->readU32();
    for (uint32_t i = 0; i < propCount; ++i) {
        std::string key = m_reader->readString();
        std::string value = m_reader->readString();
        node->add_property(std::move(key), std::move(value));
    }
    
    return node;
}

std::unique_ptr<FrontMatterNode> AstDeserializer::deserializeFrontMatter() {
    auto node = std::make_unique<FrontMatterNode>(SourceLocation{});
    
    uint32_t propCount = m_reader->readU32();
    for (uint32_t i = 0; i < propCount; ++i) {
        std::string key = m_reader->readString();
        std::string value = m_reader->readString();
        node->add_property(std::move(key), std::move(value));
    }
    
    return node;
}

std::unique_ptr<AstNode> AstDeserializer::deserializeExpression() {
    if (m_hasError) return nullptr;
    
    uint8_t op = m_reader->readByte();
    auto opCode = static_cast<OpCode>(op);
    
    switch (opCode) {
        case OpCode::NodeLiteral:
            return deserializeLiteral();
        case OpCode::NodeIdentifier:
            return deserializeIdentifier();
        case OpCode::NodeBinaryExpr:
            return deserializeBinaryExpr();
        case OpCode::NodeUnaryExpr:
            return deserializeUnaryExpr();
        case OpCode::NodeCallExpr:
            return deserializeCallExpr();
        case OpCode::NodeDiceExpr:
            return deserializeDiceExpr();
        case OpCode::EndNode:
            return nullptr;
        default:
            setError("Unknown expression opcode: " + std::to_string(op));
            return nullptr;
    }
}

std::unique_ptr<LiteralNode> AstDeserializer::deserializeLiteral() {
    uint8_t typeByte = m_reader->readByte();
    auto type = static_cast<OpCode>(typeByte);
    
    switch (type) {
        case OpCode::LiteralNull:
            return std::make_unique<LiteralNode>(SourceLocation{}, LiteralNode::Value{std::monostate{}});
        case OpCode::LiteralString: {
            std::string str = m_reader->readString();
            return std::make_unique<LiteralNode>(SourceLocation{}, LiteralNode::Value{std::move(str)});
        }
        case OpCode::LiteralNumber: {
            double num = m_reader->readDouble();
            return std::make_unique<LiteralNode>(SourceLocation{}, LiteralNode::Value{num});
        }
        case OpCode::LiteralBool: {
            uint8_t b = m_reader->readByte();
            return std::make_unique<LiteralNode>(SourceLocation{}, LiteralNode::Value{b != 0});
        }
        default:
            setError("Unknown literal type: " + std::to_string(typeByte));
            return nullptr;
    }
}

std::unique_ptr<IdentifierNode> AstDeserializer::deserializeIdentifier() {
    std::string name = m_reader->readString();
    return std::make_unique<IdentifierNode>(SourceLocation{}, std::move(name));
}

std::unique_ptr<BinaryExprNode> AstDeserializer::deserializeBinaryExpr() {
    std::string op = m_reader->readString();
    auto left = deserializeExpression();
    auto right = deserializeExpression();
    return std::make_unique<BinaryExprNode>(SourceLocation{}, std::move(op), 
                                             std::move(left), std::move(right));
}

std::unique_ptr<UnaryExprNode> AstDeserializer::deserializeUnaryExpr() {
    std::string op = m_reader->readString();
    auto operand = deserializeExpression();
    return std::make_unique<UnaryExprNode>(SourceLocation{}, std::move(op), std::move(operand));
}

std::unique_ptr<CallExprNode> AstDeserializer::deserializeCallExpr() {
    std::string name = m_reader->readString();
    auto node = std::make_unique<CallExprNode>(SourceLocation{}, std::move(name));
    
    uint32_t argCount = m_reader->readU32();
    for (uint32_t i = 0; i < argCount && !m_hasError; ++i) {
        auto arg = deserializeExpression();
        if (arg) {
            node->add_argument(std::move(arg));
        }
    }
    
    return node;
}

std::unique_ptr<DiceExprNode> AstDeserializer::deserializeDiceExpr() {
    uint32_t count = m_reader->readU32();
    uint32_t sides = m_reader->readU32();
    uint32_t modifier = m_reader->readU32();
    return std::make_unique<DiceExprNode>(SourceLocation{}, 
        static_cast<int>(count), static_cast<int>(sides), static_cast<int>(modifier));
}

void AstDeserializer::setError(const std::string& msg) {
    m_hasError = true;
    m_errorMsg = msg;
}

} // namespace nova
