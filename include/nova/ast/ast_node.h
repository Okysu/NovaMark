#pragma once

#include "nova/core/source_location.h"
#include <string>
#include <memory>
#include <vector>
#include <variant>

namespace nova {

/// @brief AST 节点类型枚举
enum class NodeType {
    // 文件级
    Program,            // 整个程序
    FrontMatter,        // YAML 元信息块
    
    // 定义
    CharDef,            // @char 定义
    ItemDef,            // @item 定义
    VarDef,             // @var 定义
    SceneDef,           // #scene 定义
    ThemeDef,           // @theme 定义
    
    // 语句
    Dialogue,           // 对话语句
    Narrator,           // 旁白语句
    InterpolatedText,   // 带插值/样式的文本
    Choice,             // 选择分支
    ChoiceOption,       // 选择选项
    Branch,             // if/else 分支
    Jump,               // -> 跳转
    Call,               // @call 调用
    Return,             // @return 返回
    Label,              // .label 子场景标签
    Wait,               // @wait 等待
    
    // 指令
    BgCommand,          // @bg
    SpriteCommand,      // @sprite
    BgmCommand,         // @bgm
    SfxCommand,         // @sfx
    GiveCommand,        // @give
    TakeCommand,        // @take
    SetCommand,         // @set
    CheckCommand,       // @check
    Ending,             // @ending
    Flag,               // @flag
    
    // 表达式
    BinaryExpr,         // 二元运算
    UnaryExpr,          // 一元运算
    Literal,            // 字面量
    Identifier,         // 标识符
    CallExpr,           // 函数调用
    DiceExpr,           // 骰子表达式
    Interpolation,      // 变量插值 {{var}}
    InlineStyle,        // 内联样式 {style:text}
};

/// @brief AST 节点基类
class AstNode {
public:
    virtual ~AstNode() = default;
    
    /// @brief 获取节点类型
    virtual NodeType type() const = 0;
    
    /// @brief 获取源码位置
    const SourceLocation& location() const { return m_location; }
    
    /// @brief 接受访问者
    template<typename Visitor>
    auto accept(Visitor& visitor) {
        return visitor.visit(*this);
    }

protected:
    explicit AstNode(SourceLocation loc) : m_location(std::move(loc)) {}
    
    SourceLocation m_location;
};

/// @brief 使用智能指针管理 AST 节点
using AstPtr = std::unique_ptr<AstNode>;

/// @brief 程序节点（根节点）
class ProgramNode : public AstNode {
public:
    explicit ProgramNode(SourceLocation loc) : AstNode(std::move(loc)) {}
    
    NodeType type() const override { return NodeType::Program; }
    
    std::vector<AstPtr>& statements() { return m_statements; }
    const std::vector<AstPtr>& statements() const { return m_statements; }
    
    void add_statement(AstPtr stmt) {
        m_statements.push_back(std::move(stmt));
    }

private:
    std::vector<AstPtr> m_statements;
};

/// @brief 字面量节点
class LiteralNode : public AstNode {
public:
    using Value = std::variant<std::monostate, std::string, double, bool>;
    
    LiteralNode(SourceLocation loc, Value val)
        : AstNode(std::move(loc)), m_value(std::move(val)) {}
    
    NodeType type() const override { return NodeType::Literal; }
    
    const Value& value() const { return m_value; }
    
    bool is_string() const { return std::holds_alternative<std::string>(m_value); }
    bool is_number() const { return std::holds_alternative<double>(m_value); }
    bool is_bool() const { return std::holds_alternative<bool>(m_value); }
    bool is_null() const { return std::holds_alternative<std::monostate>(m_value); }
    
    const std::string& as_string() const { return std::get<std::string>(m_value); }
    double as_number() const { return std::get<double>(m_value); }
    bool as_bool() const { return std::get<bool>(m_value); }

private:
    Value m_value;
};

/// @brief 标识符节点
class IdentifierNode : public AstNode {
public:
    IdentifierNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::Identifier; }
    
    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

/// @brief 二元表达式节点
class BinaryExprNode : public AstNode {
public:
    BinaryExprNode(SourceLocation loc, std::string op, AstPtr left, AstPtr right)
        : AstNode(std::move(loc)), m_op(std::move(op)), 
          m_left(std::move(left)), m_right(std::move(right)) {}
    
    NodeType type() const override { return NodeType::BinaryExpr; }
    
    const std::string& op() const { return m_op; }
    const AstNode* left() const { return m_left.get(); }
    const AstNode* right() const { return m_right.get(); }

private:
    std::string m_op;
    AstPtr m_left;
    AstPtr m_right;
};

/// @brief 一元表达式节点
class UnaryExprNode : public AstNode {
public:
    UnaryExprNode(SourceLocation loc, std::string op, AstPtr operand)
        : AstNode(std::move(loc)), m_op(std::move(op)), m_operand(std::move(operand)) {}
    
    NodeType type() const override { return NodeType::UnaryExpr; }
    
    const std::string& op() const { return m_op; }
    const AstNode* operand() const { return m_operand.get(); }

private:
    std::string m_op;
    AstPtr m_operand;
};

/// @brief 对话语句节点
class DialogueNode : public AstNode {
public:
    DialogueNode(SourceLocation loc, std::string speaker, std::string emotion, std::string text)
        : AstNode(std::move(loc)), m_speaker(std::move(speaker)), 
          m_emotion(std::move(emotion)), m_text(std::move(text)) {}
    
    NodeType type() const override { return NodeType::Dialogue; }
    
    const std::string& speaker() const { return m_speaker; }
    const std::string& emotion() const { return m_emotion; }
    const std::string& text() const { return m_text; }

private:
    std::string m_speaker;
    std::string m_emotion;
    std::string m_text;
};

/// @brief 旁白语句节点
class NarratorNode : public AstNode {
public:
    NarratorNode(SourceLocation loc, std::string text)
        : AstNode(std::move(loc)), m_text(std::move(text)) {}
    
    NodeType type() const override { return NodeType::Narrator; }
    
    const std::string& text() const { return m_text; }

private:
    std::string m_text;
};

/// @brief 场景定义节点
class SceneDefNode : public AstNode {
public:
    SceneDefNode(SourceLocation loc, std::string name, std::string title)
        : AstNode(std::move(loc)), m_name(std::move(name)), m_title(std::move(title)) {}
    
    NodeType type() const override { return NodeType::SceneDef; }
    
    const std::string& name() const { return m_name; }
    const std::string& title() const { return m_title; }

private:
    std::string m_name;
    std::string m_title;
};

/// @brief 跳转语句节点
class JumpNode : public AstNode {
public:
    JumpNode(SourceLocation loc, std::string target)
        : AstNode(std::move(loc)), m_target(std::move(target)) {}
    
    NodeType type() const override { return NodeType::Jump; }
    
    const std::string& target() const { return m_target; }

private:
    std::string m_target;
};

/// @brief 选择选项节点
class ChoiceOptionNode : public AstNode {
public:
    ChoiceOptionNode(SourceLocation loc, std::string text, std::string target, AstPtr condition)
        : AstNode(std::move(loc)), m_text(std::move(text)), 
          m_target(std::move(target)), m_condition(std::move(condition)) {}
    
    NodeType type() const override { return NodeType::ChoiceOption; }
    
    const std::string& text() const { return m_text; }
    const std::string& target() const { return m_target; }
    const AstNode* condition() const { return m_condition.get(); }

private:
    std::string m_text;
    std::string m_target;
    AstPtr m_condition;
};

/// @brief 选择分支节点
class ChoiceNode : public AstNode {
public:
    ChoiceNode(SourceLocation loc, std::string question)
        : AstNode(std::move(loc)), m_question(std::move(question)) {}
    
    NodeType type() const override { return NodeType::Choice; }
    
    const std::string& question() const { return m_question; }
    std::vector<AstPtr>& options() { return m_options; }
    const std::vector<AstPtr>& options() const { return m_options; }
    
    void add_option(AstPtr option) { m_options.push_back(std::move(option)); }

private:
    std::string m_question;
    std::vector<AstPtr> m_options;
};

/// @brief 变量定义节点
class VarDefNode : public AstNode {
public:
    VarDefNode(SourceLocation loc, std::string name, AstPtr init_value)
        : AstNode(std::move(loc)), m_name(std::move(name)), 
          m_init_value(std::move(init_value)) {}
    
    NodeType type() const override { return NodeType::VarDef; }
    
    const std::string& name() const { return m_name; }
    const AstNode* init_value() const { return m_init_value.get(); }

private:
    std::string m_name;
    AstPtr m_init_value;
};

/// @brief if/else 分支节点
class BranchNode : public AstNode {
public:
    BranchNode(SourceLocation loc, AstPtr condition)
        : AstNode(std::move(loc)), m_condition(std::move(condition)) {}
    
    NodeType type() const override { return NodeType::Branch; }
    
    const AstNode* condition() const { return m_condition.get(); }
    std::vector<AstPtr>& then_branch() { return m_then_branch; }
    const std::vector<AstPtr>& then_branch() const { return m_then_branch; }
    std::vector<AstPtr>& else_branch() { return m_else_branch; }
    const std::vector<AstPtr>& else_branch() const { return m_else_branch; }
    
    void add_then(AstPtr stmt) { m_then_branch.push_back(std::move(stmt)); }
    void add_else(AstPtr stmt) { m_else_branch.push_back(std::move(stmt)); }

private:
    AstPtr m_condition;
    std::vector<AstPtr> m_then_branch;
    std::vector<AstPtr> m_else_branch;
};

/// @brief 角色定义节点
class CharDefNode : public AstNode {
public:
    struct Property {
        std::string key;
        std::string value;
    };
    
    CharDefNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::CharDef; }
    
    const std::string& name() const { return m_name; }
    std::vector<Property>& properties() { return m_properties; }
    const std::vector<Property>& properties() const { return m_properties; }
    
    void add_property(std::string key, std::string value) {
        m_properties.push_back({std::move(key), std::move(value)});
    }

private:
    std::string m_name;
    std::vector<Property> m_properties;
};

/// @brief 物品定义节点
class ItemDefNode : public AstNode {
public:
    struct Property {
        std::string key;
        std::string value;
    };
    
    ItemDefNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::ItemDef; }
    
    const std::string& name() const { return m_name; }
    std::vector<Property>& properties() { return m_properties; }
    const std::vector<Property>& properties() const { return m_properties; }
    
    void add_property(std::string key, std::string value) {
        m_properties.push_back({std::move(key), std::move(value)});
    }

private:
    std::string m_name;
    std::vector<Property> m_properties;
};

/// @brief 命令参数
struct CommandArg {
    std::string key;
    std::string value;
};

/// @brief @bg 背景命令节点
class BgCommandNode : public AstNode {
public:
    BgCommandNode(SourceLocation loc, std::string image)
        : AstNode(std::move(loc)), m_image(std::move(image)) {}
    
    NodeType type() const override { return NodeType::BgCommand; }
    
    const std::string& image() const { return m_image; }
    std::vector<CommandArg>& args() { return m_args; }
    const std::vector<CommandArg>& args() const { return m_args; }
    
    void add_arg(std::string key, std::string value) {
        m_args.push_back({std::move(key), std::move(value)});
    }

private:
    std::string m_image;
    std::vector<CommandArg> m_args;
};

/// @brief @sprite 立绘命令节点
class SpriteCommandNode : public AstNode {
public:
    SpriteCommandNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::SpriteCommand; }
    
    const std::string& name() const { return m_name; }
    std::vector<CommandArg>& args() { return m_args; }
    const std::vector<CommandArg>& args() const { return m_args; }
    
    void add_arg(std::string key, std::string value) {
        m_args.push_back({std::move(key), std::move(value)});
    }

private:
    std::string m_name;
    std::vector<CommandArg> m_args;
};

/// @brief @bgm 背景音乐命令节点
class BgmCommandNode : public AstNode {
public:
    BgmCommandNode(SourceLocation loc, std::string file)
        : AstNode(std::move(loc)), m_file(std::move(file)) {}
    
    NodeType type() const override { return NodeType::BgmCommand; }
    
    const std::string& file() const { return m_file; }
    std::vector<CommandArg>& args() { return m_args; }
    const std::vector<CommandArg>& args() const { return m_args; }
    
    void add_arg(std::string key, std::string value) {
        m_args.push_back({std::move(key), std::move(value)});
    }

private:
    std::string m_file;
    std::vector<CommandArg> m_args;
};

/// @brief @sfx 音效命令节点
class SfxCommandNode : public AstNode {
public:
    SfxCommandNode(SourceLocation loc, std::string file)
        : AstNode(std::move(loc)), m_file(std::move(file)) {}
    
    NodeType type() const override { return NodeType::SfxCommand; }
    
    const std::string& file() const { return m_file; }
    std::vector<CommandArg>& args() { return m_args; }
    const std::vector<CommandArg>& args() const { return m_args; }
    
    void add_arg(std::string key, std::string value) {
        m_args.push_back({std::move(key), std::move(value)});
    }

private:
    std::string m_file;
    std::vector<CommandArg> m_args;
};

/// @brief @set 变量设置命令节点
class SetCommandNode : public AstNode {
public:
    SetCommandNode(SourceLocation loc, std::string name, AstPtr value)
        : AstNode(std::move(loc)), m_name(std::move(name)), m_value(std::move(value)) {}
    
    NodeType type() const override { return NodeType::SetCommand; }
    
    const std::string& name() const { return m_name; }
    const AstNode* value() const { return m_value.get(); }

private:
    std::string m_name;
    AstPtr m_value;
};

/// @brief @give 给予物品命令节点
class GiveCommandNode : public AstNode {
public:
    GiveCommandNode(SourceLocation loc, std::string item, int count)
        : AstNode(std::move(loc)), m_item(std::move(item)), m_count(count) {}
    
    NodeType type() const override { return NodeType::GiveCommand; }
    
    const std::string& item() const { return m_item; }
    int count() const { return m_count; }

private:
    std::string m_item;
    int m_count;
};

/// @brief @take 取走物品命令节点
class TakeCommandNode : public AstNode {
public:
    TakeCommandNode(SourceLocation loc, std::string item, int count)
        : AstNode(std::move(loc)), m_item(std::move(item)), m_count(count) {}
    
    NodeType type() const override { return NodeType::TakeCommand; }
    
    const std::string& item() const { return m_item; }
    int count() const { return m_count; }

private:
    std::string m_item;
    int m_count;
};

/// @brief @call 调用节点
class CallNode : public AstNode {
public:
    CallNode(SourceLocation loc, std::string target)
        : AstNode(std::move(loc)), m_target(std::move(target)) {}
    
    NodeType type() const override { return NodeType::Call; }
    
    const std::string& target() const { return m_target; }

private:
    std::string m_target;
};

/// @brief @return 返回节点
class ReturnNode : public AstNode {
public:
    explicit ReturnNode(SourceLocation loc) : AstNode(std::move(loc)) {}
    
    NodeType type() const override { return NodeType::Return; }
};

/// @brief @ending 结局触发节点
class EndingNode : public AstNode {
public:
    EndingNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::Ending; }
    
    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

/// @brief @flag 标记设置节点
class FlagNode : public AstNode {
public:
    FlagNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::Flag; }
    
    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

/// @brief 函数调用表达式节点
class CallExprNode : public AstNode {
public:
    CallExprNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::CallExpr; }
    
    const std::string& name() const { return m_name; }
    std::vector<AstPtr>& arguments() { return m_arguments; }
    const std::vector<AstPtr>& arguments() const { return m_arguments; }
    
    void add_argument(AstPtr arg) { m_arguments.push_back(std::move(arg)); }

private:
    std::string m_name;
    std::vector<AstPtr> m_arguments;
};

/// @brief 骰子表达式节点 (如 2d6+3)
class DiceExprNode : public AstNode {
public:
    DiceExprNode(SourceLocation loc, int count, int sides, int modifier)
        : AstNode(std::move(loc)), m_count(count), m_sides(sides), m_modifier(modifier) {}
    
    NodeType type() const override { return NodeType::DiceExpr; }
    
    int count() const { return m_count; }
    int sides() const { return m_sides; }
    int modifier() const { return m_modifier; }

private:
    int m_count;
    int m_sides;
    int m_modifier;
};

/// @brief .label 子场景标签节点
class LabelNode : public AstNode {
public:
    LabelNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::Label; }
    
    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

/// @brief @check 检定命令节点
class CheckCommandNode : public AstNode {
public:
    CheckCommandNode(SourceLocation loc, AstPtr condition)
        : AstNode(std::move(loc)), m_condition(std::move(condition)) {}
    
    NodeType type() const override { return NodeType::CheckCommand; }
    
    const AstNode* condition() const { return m_condition.get(); }
    std::vector<AstPtr>& success_branch() { return m_success_branch; }
    const std::vector<AstPtr>& success_branch() const { return m_success_branch; }
    std::vector<AstPtr>& failure_branch() { return m_failure_branch; }
    const std::vector<AstPtr>& failure_branch() const { return m_failure_branch; }
    
    void add_success(AstPtr stmt) { m_success_branch.push_back(std::move(stmt)); }
    void add_failure(AstPtr stmt) { m_failure_branch.push_back(std::move(stmt)); }

private:
    AstPtr m_condition;
    std::vector<AstPtr> m_success_branch;
    std::vector<AstPtr> m_failure_branch;
};

/// @brief @wait 等待节点
class WaitNode : public AstNode {
public:
    WaitNode(SourceLocation loc, double seconds)
        : AstNode(std::move(loc)), m_seconds(seconds) {}
    
    NodeType type() const override { return NodeType::Wait; }
    
    double seconds() const { return m_seconds; }

private:
    double m_seconds;
};

/// @brief @theme 主题定义节点
class ThemeDefNode : public AstNode {
public:
    struct Property {
        std::string key;
        std::string value;
    };
    
    ThemeDefNode(SourceLocation loc, std::string name)
        : AstNode(std::move(loc)), m_name(std::move(name)) {}
    
    NodeType type() const override { return NodeType::ThemeDef; }
    
    const std::string& name() const { return m_name; }
    std::vector<Property>& properties() { return m_properties; }
    const std::vector<Property>& properties() const { return m_properties; }
    
    void add_property(std::string key, std::string value) {
        m_properties.push_back({std::move(key), std::move(value)});
    }

private:
    std::string m_name;
    std::vector<Property> m_properties;
};

/// @brief YAML Front Matter 元信息块节点
class FrontMatterNode : public AstNode {
public:
    struct Property {
        std::string key;
        std::string value;
    };
    
    explicit FrontMatterNode(SourceLocation loc)
        : AstNode(std::move(loc)) {}
    
    NodeType type() const override { return NodeType::FrontMatter; }
    
    std::vector<Property>& properties() { return m_properties; }
    const std::vector<Property>& properties() const { return m_properties; }
    
    void add_property(std::string key, std::string value) {
        m_properties.push_back({std::move(key), std::move(value)});
    }
    
    std::string raw_content() const {
        std::string result;
        for (const auto& prop : m_properties) {
            result += prop.key + ": " + prop.value + "\n";
        }
        return result;
    }

private:
    std::vector<Property> m_properties;
};

/// @brief 变量插值节点 {{var}}
class InterpolationNode : public AstNode {
public:
    InterpolationNode(SourceLocation loc, std::string var_name)
        : AstNode(std::move(loc)), m_var_name(std::move(var_name)) {}
    
    NodeType type() const override { return NodeType::Interpolation; }
    
    const std::string& var_name() const { return m_var_name; }

private:
    std::string m_var_name;
};

/// @brief 内联样式节点 {style:text}
class InlineStyleNode : public AstNode {
public:
    InlineStyleNode(SourceLocation loc, std::string style, std::string text)
        : AstNode(std::move(loc)), m_style(std::move(style)), m_text(std::move(text)) {}
    
    NodeType type() const override { return NodeType::InlineStyle; }
    
    const std::string& style() const { return m_style; }
    const std::string& text() const { return m_text; }

private:
    std::string m_style;
    std::string m_text;
};

/// @brief 带插值和样式的复合文本节点
class InterpolatedTextNode : public AstNode {
public:
    struct Segment {
        enum class Type { PlainText, Interpolation, InlineStyle };
        Type type;
        std::string content;
        std::string style;
    };
    
    InterpolatedTextNode(SourceLocation loc)
        : AstNode(std::move(loc)) {}
    
    NodeType type() const override { return NodeType::InterpolatedText; }
    
    std::vector<Segment>& segments() { return m_segments; }
    const std::vector<Segment>& segments() const { return m_segments; }
    
    void add_plain_text(std::string text) {
        m_segments.push_back({Segment::Type::PlainText, std::move(text), {}});
    }
    
    void add_interpolation(std::string var_name) {
        m_segments.push_back({Segment::Type::Interpolation, std::move(var_name), {}});
    }
    
    void add_inline_style(std::string style, std::string text) {
        m_segments.push_back({Segment::Type::InlineStyle, std::move(text), std::move(style)});
    }
    
    bool is_plain_text() const {
        for (const auto& seg : m_segments) {
            if (seg.type != Segment::Type::PlainText) return false;
        }
        return true;
    }
    
    std::string to_plain_text() const {
        std::string result;
        for (const auto& seg : m_segments) {
            result += seg.content;
        }
        return result;
    }

private:
    std::vector<Segment> m_segments;
};

} // namespace nova
