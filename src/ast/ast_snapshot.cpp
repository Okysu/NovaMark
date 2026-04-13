#include "nova/ast/ast_snapshot.h"

#include <nlohmann/json.hpp>

namespace nova {

namespace {

using json = nlohmann::json;

json location_json(const SourceLocation& location) {
    return {
        {"file", location.file},
        {"line", location.line},
        {"column", location.column}
    };
}

json command_args_json(const std::vector<CommandArg>& args) {
    json result = json::array();
    for (const auto& arg : args) {
        result.push_back({{"key", arg.key}, {"value", arg.value}});
    }
    return result;
}

template <typename Property>
json properties_json(const std::vector<Property>& properties) {
    json result = json::array();
    for (const auto& prop : properties) {
        result.push_back({{"key", prop.key}, {"value", prop.value}});
    }
    return result;
}

const char* node_type_name(NodeType type) {
    switch (type) {
        case NodeType::Program: return "Program";
        case NodeType::FrontMatter: return "FrontMatter";
        case NodeType::CharDef: return "CharDef";
        case NodeType::ItemDef: return "ItemDef";
        case NodeType::VarDef: return "VarDef";
        case NodeType::SceneDef: return "SceneDef";
        case NodeType::ThemeDef: return "ThemeDef";
        case NodeType::Dialogue: return "Dialogue";
        case NodeType::Narrator: return "Narrator";
        case NodeType::InterpolatedText: return "InterpolatedText";
        case NodeType::Choice: return "Choice";
        case NodeType::ChoiceOption: return "ChoiceOption";
        case NodeType::Branch: return "Branch";
        case NodeType::Jump: return "Jump";
        case NodeType::Call: return "Call";
        case NodeType::Return: return "Return";
        case NodeType::Label: return "Label";
        case NodeType::BgCommand: return "BgCommand";
        case NodeType::SpriteCommand: return "SpriteCommand";
        case NodeType::BgmCommand: return "BgmCommand";
        case NodeType::SfxCommand: return "SfxCommand";
        case NodeType::GiveCommand: return "GiveCommand";
        case NodeType::TakeCommand: return "TakeCommand";
        case NodeType::SetCommand: return "SetCommand";
        case NodeType::CheckCommand: return "CheckCommand";
        case NodeType::Ending: return "Ending";
        case NodeType::Flag: return "Flag";
        case NodeType::BinaryExpr: return "BinaryExpr";
        case NodeType::UnaryExpr: return "UnaryExpr";
        case NodeType::Literal: return "Literal";
        case NodeType::Identifier: return "Identifier";
        case NodeType::CallExpr: return "CallExpr";
        case NodeType::DiceExpr: return "DiceExpr";
        case NodeType::Interpolation: return "Interpolation";
        case NodeType::InlineStyle: return "InlineStyle";
    }
    return "Unknown";
}

json literal_value_json(const LiteralNode::Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return nullptr;
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    return std::get<bool>(value);
}

json node_json(const AstNode* node);

json children_json(const std::vector<AstPtr>& nodes) {
    json result = json::array();
    for (const auto& child : nodes) {
        result.push_back(node_json(child.get()));
    }
    return result;
}

json node_json(const AstNode* node) {
    if (!node) {
        return nullptr;
    }

    json result = {
        {"type", node_type_name(node->type())},
        {"location", location_json(node->location())}
    };

    switch (node->type()) {
        case NodeType::Program: {
            auto* program = dynamic_cast<const ProgramNode*>(node);
            result["children"] = children_json(program->statements());
            break;
        }
        case NodeType::Literal: {
            auto* literal = dynamic_cast<const LiteralNode*>(node);
            result["value"] = literal_value_json(literal->value());
            break;
        }
        case NodeType::Identifier: {
            auto* identifier = dynamic_cast<const IdentifierNode*>(node);
            result["name"] = identifier->name();
            break;
        }
        case NodeType::BinaryExpr: {
            auto* expr = dynamic_cast<const BinaryExprNode*>(node);
            result["op"] = expr->op();
            result["left"] = node_json(expr->left());
            result["right"] = node_json(expr->right());
            break;
        }
        case NodeType::UnaryExpr: {
            auto* expr = dynamic_cast<const UnaryExprNode*>(node);
            result["op"] = expr->op();
            result["operand"] = node_json(expr->operand());
            break;
        }
        case NodeType::Dialogue: {
            auto* dialogue = dynamic_cast<const DialogueNode*>(node);
            result["speaker"] = dialogue->speaker();
            result["emotion"] = dialogue->emotion();
            result["text"] = dialogue->text();
            break;
        }
        case NodeType::Narrator: {
            auto* narrator = dynamic_cast<const NarratorNode*>(node);
            result["text"] = narrator->text();
            break;
        }
        case NodeType::SceneDef: {
            auto* scene = dynamic_cast<const SceneDefNode*>(node);
            result["name"] = scene->name();
            result["title"] = scene->title();
            break;
        }
        case NodeType::Jump: {
            auto* jump = dynamic_cast<const JumpNode*>(node);
            result["target"] = jump->target();
            break;
        }
        case NodeType::ChoiceOption: {
            auto* option = dynamic_cast<const ChoiceOptionNode*>(node);
            result["text"] = option->text();
            result["target"] = option->target();
            result["condition"] = node_json(option->condition());
            result["body"] = children_json(option->body());
            break;
        }
        case NodeType::Choice: {
            auto* choice = dynamic_cast<const ChoiceNode*>(node);
            result["question"] = choice->question();
            result["options"] = children_json(choice->options());
            break;
        }
        case NodeType::VarDef: {
            auto* varDef = dynamic_cast<const VarDefNode*>(node);
            result["name"] = varDef->name();
            result["init"] = node_json(varDef->init_value());
            break;
        }
        case NodeType::Branch: {
            auto* branch = dynamic_cast<const BranchNode*>(node);
            result["condition"] = node_json(branch->condition());
            result["then"] = children_json(branch->then_branch());
            result["else"] = children_json(branch->else_branch());
            break;
        }
        case NodeType::CharDef: {
            auto* charDef = dynamic_cast<const CharDefNode*>(node);
            result["name"] = charDef->name();
            result["properties"] = properties_json(charDef->properties());
            break;
        }
        case NodeType::ItemDef: {
            auto* itemDef = dynamic_cast<const ItemDefNode*>(node);
            result["name"] = itemDef->name();
            result["properties"] = properties_json(itemDef->properties());
            break;
        }
        case NodeType::BgCommand: {
            auto* bg = dynamic_cast<const BgCommandNode*>(node);
            result["image"] = bg->image();
            result["args"] = command_args_json(bg->args());
            break;
        }
        case NodeType::SpriteCommand: {
            auto* sprite = dynamic_cast<const SpriteCommandNode*>(node);
            result["name"] = sprite->name();
            result["args"] = command_args_json(sprite->args());
            break;
        }
        case NodeType::BgmCommand: {
            auto* bgm = dynamic_cast<const BgmCommandNode*>(node);
            result["file"] = bgm->file();
            result["args"] = command_args_json(bgm->args());
            break;
        }
        case NodeType::SfxCommand: {
            auto* sfx = dynamic_cast<const SfxCommandNode*>(node);
            result["file"] = sfx->file();
            result["args"] = command_args_json(sfx->args());
            break;
        }
        case NodeType::SetCommand: {
            auto* set = dynamic_cast<const SetCommandNode*>(node);
            result["name"] = set->name();
            result["value"] = node_json(set->value());
            break;
        }
        case NodeType::GiveCommand: {
            auto* give = dynamic_cast<const GiveCommandNode*>(node);
            result["item"] = give->item();
            result["count"] = node_json(give->count());
            break;
        }
        case NodeType::TakeCommand: {
            auto* take = dynamic_cast<const TakeCommandNode*>(node);
            result["item"] = take->item();
            result["count"] = node_json(take->count());
            break;
        }
        case NodeType::Call: {
            auto* call = dynamic_cast<const CallNode*>(node);
            result["target"] = call->target();
            break;
        }
        case NodeType::Return:
            break;
        case NodeType::Ending: {
            auto* ending = dynamic_cast<const EndingNode*>(node);
            result["name"] = ending->name();
            result["title"] = ending->title();
            break;
        }
        case NodeType::Flag: {
            auto* flag = dynamic_cast<const FlagNode*>(node);
            result["name"] = flag->name();
            break;
        }
        case NodeType::CallExpr: {
            auto* call = dynamic_cast<const CallExprNode*>(node);
            result["name"] = call->name();
            result["arguments"] = children_json(call->arguments());
            break;
        }
        case NodeType::DiceExpr: {
            auto* dice = dynamic_cast<const DiceExprNode*>(node);
            result["count"] = dice->count();
            result["sides"] = dice->sides();
            result["modifier"] = dice->modifier();
            break;
        }
        case NodeType::Label: {
            auto* label = dynamic_cast<const LabelNode*>(node);
            result["name"] = label->name();
            break;
        }
        case NodeType::CheckCommand: {
            auto* check = dynamic_cast<const CheckCommandNode*>(node);
            result["condition"] = node_json(check->condition());
            result["success"] = children_json(check->success_branch());
            result["failure"] = children_json(check->failure_branch());
            break;
        }
        case NodeType::ThemeDef: {
            auto* theme = dynamic_cast<const ThemeDefNode*>(node);
            result["name"] = theme->name();
            result["properties"] = properties_json(theme->properties());
            break;
        }
        case NodeType::FrontMatter: {
            auto* frontMatter = dynamic_cast<const FrontMatterNode*>(node);
            result["properties"] = properties_json(frontMatter->properties());
            result["rawContent"] = frontMatter->raw_content();
            break;
        }
        case NodeType::Interpolation: {
            auto* interpolation = dynamic_cast<const InterpolationNode*>(node);
            result["varName"] = interpolation->var_name();
            break;
        }
        case NodeType::InlineStyle: {
            auto* inlineStyle = dynamic_cast<const InlineStyleNode*>(node);
            result["style"] = inlineStyle->style();
            result["text"] = inlineStyle->text();
            break;
        }
        case NodeType::InterpolatedText: {
            auto* text = dynamic_cast<const InterpolatedTextNode*>(node);
            json segments = json::array();
            for (const auto& segment : text->segments()) {
                const char* segmentType = "PlainText";
                if (segment.type == InterpolatedTextNode::Segment::Type::Interpolation) {
                    segmentType = "Interpolation";
                } else if (segment.type == InterpolatedTextNode::Segment::Type::InlineStyle) {
                    segmentType = "InlineStyle";
                }
                segments.push_back({
                    {"type", segmentType},
                    {"content", segment.content},
                    {"style", segment.style}
                });
            }
            result["segments"] = std::move(segments);
            result["plainText"] = text->to_plain_text();
            break;
        }
    }

    return result;
}

}

json export_ast_snapshot_json(const ProgramNode* program) {
    return {
        {"version", 1},
        {"root", node_json(program)}
    };
}

std::string export_ast_snapshot_string(const ProgramNode* program) {
    return export_ast_snapshot_json(program).dump();
}

}
