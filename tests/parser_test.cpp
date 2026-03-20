#include <gtest/gtest.h>
#include "nova/parser/parser.h"
#include "nova/lexer/lexer.h"
#include "nova/ast/ast_node.h"

using namespace nova;

class ParserTest : public ::testing::Test {
protected:
    Result<AstPtr> parse(const std::string& source) {
        Lexer lexer(source, "<test>");
        auto tokens_result = lexer.tokenize();
        if (tokens_result.is_err()) {
            return Result<AstPtr>(tokens_result.error());
        }
        Parser parser(tokens_result.unwrap());
        return parser.parse();
    }
    
    const ProgramNode* as_program(const AstPtr& ast) {
        return dynamic_cast<const ProgramNode*>(ast.get());
    }
    
    const NarratorNode* as_narrator(const AstNode* node) {
        return dynamic_cast<const NarratorNode*>(node);
    }
    
    const DialogueNode* as_dialogue(const AstNode* node) {
        return dynamic_cast<const DialogueNode*>(node);
    }
    
    const SceneDefNode* as_scene_def(const AstNode* node) {
        return dynamic_cast<const SceneDefNode*>(node);
    }
    
    const JumpNode* as_jump(const AstNode* node) {
        return dynamic_cast<const JumpNode*>(node);
    }
    
    const ChoiceNode* as_choice(const AstNode* node) {
        return dynamic_cast<const ChoiceNode*>(node);
    }
    
    const VarDefNode* as_var_def(const AstNode* node) {
        return dynamic_cast<const VarDefNode*>(node);
    }
    
    const LiteralNode* as_literal(const AstNode* node) {
        return dynamic_cast<const LiteralNode*>(node);
    }
    
    const IdentifierNode* as_identifier(const AstNode* node) {
        return dynamic_cast<const IdentifierNode*>(node);
    }
    
    const BinaryExprNode* as_binary_expr(const AstNode* node) {
        return dynamic_cast<const BinaryExprNode*>(node);
    }
    
    const EndingNode* as_ending(const AstNode* node) {
        return dynamic_cast<const EndingNode*>(node);
    }
    
    const FlagNode* as_flag(const AstNode* node) {
        return dynamic_cast<const FlagNode*>(node);
    }
    
    const CallExprNode* as_call_expr(const AstNode* node) {
        return dynamic_cast<const CallExprNode*>(node);
    }
    
    const LabelNode* as_label(const AstNode* node) {
        return dynamic_cast<const LabelNode*>(node);
    }
    
    const CheckCommandNode* as_check_command(const AstNode* node) {
        return dynamic_cast<const CheckCommandNode*>(node);
    }
    
    const WaitNode* as_wait(const AstNode* node) {
        return dynamic_cast<const WaitNode*>(node);
    }
    
    const ThemeDefNode* as_theme_def(const AstNode* node) {
        return dynamic_cast<const ThemeDefNode*>(node);
    }
    
    const FrontMatterNode* as_front_matter(const AstNode* node) {
        return dynamic_cast<const FrontMatterNode*>(node);
    }
};

// ============================================
// Empty Program Tests
// ============================================

TEST_F(ParserTest, EmptyProgram) {
    auto result = parse("");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements().size(), 0u);
}

TEST_F(ParserTest, OnlyNewlines) {
    auto result = parse("\n\n\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements().size(), 0u);
}

// ============================================
// Narrator Tests
// ============================================

TEST_F(ParserTest, SimpleNarrator) {
    auto result = parse("> Hello, world!");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto narrator = as_narrator(program->statements()[0].get());
    ASSERT_NE(narrator, nullptr);
    EXPECT_EQ(narrator->text(), "Hello, world!");
}

TEST_F(ParserTest, NarratorWithChineseText) {
    auto result = parse("> 月光透过树梢洒落，林间寂静得出奇。");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto narrator = as_narrator(program->statements()[0].get());
    ASSERT_NE(narrator, nullptr);
    EXPECT_EQ(narrator->text(), "月光透过树梢洒落，林间寂静得出奇。");
}

TEST_F(ParserTest, EmptyNarrator) {
    auto result = parse(">\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto narrator = as_narrator(program->statements()[0].get());
    ASSERT_NE(narrator, nullptr);
    EXPECT_EQ(narrator->text(), "");
}

// ============================================
// Dialogue Tests
// ============================================

TEST_F(ParserTest, SimpleDialogue) {
    auto result = parse("林晓: 你好世界");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto dialogue = as_dialogue(program->statements()[0].get());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_EQ(dialogue->speaker(), "林晓");
    EXPECT_EQ(dialogue->emotion(), "");
    EXPECT_EQ(dialogue->text(), "你好世界");
}

TEST_F(ParserTest, DialogueWithEmotion) {
    auto result = parse("林晓[happy]: 太好了，我们出发吧！");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto dialogue = as_dialogue(program->statements()[0].get());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_EQ(dialogue->speaker(), "林晓");
    EXPECT_EQ(dialogue->emotion(), "happy");
    EXPECT_EQ(dialogue->text(), "太好了，我们出发吧！");
}

TEST_F(ParserTest, DialogueWithEnglishSpeaker) {
    auto result = parse("Alice: Hello there!");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto dialogue = as_dialogue(program->statements()[0].get());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_EQ(dialogue->speaker(), "Alice");
    EXPECT_EQ(dialogue->emotion(), "");
    EXPECT_EQ(dialogue->text(), "Hello there!");
}

TEST_F(ParserTest, DialogueEmptyText) {
    auto result = parse("Bob:\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto dialogue = as_dialogue(program->statements()[0].get());
    ASSERT_NE(dialogue, nullptr);
    EXPECT_EQ(dialogue->speaker(), "Bob");
    EXPECT_EQ(dialogue->text(), "");
}

// ============================================
// Scene Definition Tests
// ============================================

TEST_F(ParserTest, SceneDefWithChineseName) {
    auto result = parse("#forest \"幽暗的森林\"");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto scene = as_scene_def(program->statements()[0].get());
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->name(), "forest");
    EXPECT_EQ(scene->title(), "幽暗的森林");
}

TEST_F(ParserTest, SceneDefWithoutTitle) {
    auto result = parse("#dungeon");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto scene = as_scene_def(program->statements()[0].get());
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->name(), "dungeon");
    EXPECT_EQ(scene->title(), "");
}

// ============================================
// Jump Tests
// ============================================

TEST_F(ParserTest, SimpleJump) {
    auto result = parse("-> next_scene");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto jump = as_jump(program->statements()[0].get());
    ASSERT_NE(jump, nullptr);
    EXPECT_EQ(jump->target(), "next_scene");
}

TEST_F(ParserTest, JumpWithChineseTarget) {
    auto result = parse("-> 结局_好");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto jump = as_jump(program->statements()[0].get());
    ASSERT_NE(jump, nullptr);
    EXPECT_EQ(jump->target(), "结局_好");
}

// ============================================
// Choice Tests
// ============================================

TEST_F(ParserTest, SimpleChoice) {
    auto result = parse(
        "? 你要怎么做？\n"
        "- [选项A] -> scene_a\n"
        "- [选项B] -> scene_b\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto choice = as_choice(program->statements()[0].get());
    ASSERT_NE(choice, nullptr);
    EXPECT_EQ(choice->question(), "你要怎么做？");
    ASSERT_EQ(choice->options().size(), 2u);
}

TEST_F(ParserTest, ChoiceWithCondition) {
    auto result = parse(
        "? 你想买吗？\n"
        "- [买下] -> buy_scene if gold >= 50\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto choice = as_choice(program->statements()[0].get());
    ASSERT_NE(choice, nullptr);
    ASSERT_EQ(choice->options().size(), 1u);
}

// ============================================
// Variable Definition Tests
// ============================================

TEST_F(ParserTest, VarDefWithoutInit) {
    auto result = parse("@var hp");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    EXPECT_EQ(var_def->name(), "hp");
    EXPECT_EQ(var_def->init_value(), nullptr);
}

TEST_F(ParserTest, VarDefWithNumberInit) {
    auto result = parse("@var hp = 100");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    EXPECT_EQ(var_def->name(), "hp");
    ASSERT_NE(var_def->init_value(), nullptr);
    
    auto lit = as_literal(var_def->init_value());
    ASSERT_NE(lit, nullptr);
    EXPECT_TRUE(lit->is_number());
    EXPECT_EQ(lit->as_number(), 100.0);
}

TEST_F(ParserTest, VarDefWithStringInit) {
    auto result = parse("@var name = \"Player\"");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    EXPECT_EQ(var_def->name(), "name");
    ASSERT_NE(var_def->init_value(), nullptr);
    
    auto lit = as_literal(var_def->init_value());
    ASSERT_NE(lit, nullptr);
    EXPECT_TRUE(lit->is_string());
    EXPECT_EQ(lit->as_string(), "Player");
}

TEST_F(ParserTest, VarDefWithBoolInit) {
    auto result = parse("@var is_alive = true");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    
    auto lit = as_literal(var_def->init_value());
    ASSERT_NE(lit, nullptr);
    EXPECT_TRUE(lit->is_bool());
    EXPECT_EQ(lit->as_bool(), true);
}

// ============================================
// Expression Tests
// ============================================

TEST_F(ParserTest, NumberLiteral) {
    auto result = parse("@var x = 42");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    auto var_def = as_var_def(program->statements()[0].get());
    auto lit = as_literal(var_def->init_value());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->as_number(), 42.0);
}

TEST_F(ParserTest, FloatLiteral) {
    auto result = parse("@var pi = 3.14");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    auto var_def = as_var_def(program->statements()[0].get());
    auto lit = as_literal(var_def->init_value());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->as_number(), 3.14);
}

TEST_F(ParserTest, IdentifierExpr) {
    auto result = parse("@var y = x");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    auto var_def = as_var_def(program->statements()[0].get());
    auto ident = as_identifier(var_def->init_value());
    ASSERT_NE(ident, nullptr);
    EXPECT_EQ(ident->name(), "x");
}

TEST_F(ParserTest, BinaryAddition) {
    auto result = parse("@var sum = a + b");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    auto var_def = as_var_def(program->statements()[0].get());
    auto bin = as_binary_expr(var_def->init_value());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op(), "+");
}

TEST_F(ParserTest, BinaryMultiplication) {
    auto result = parse("@var product = a * b");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    auto var_def = as_var_def(program->statements()[0].get());
    auto bin = as_binary_expr(var_def->init_value());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op(), "*");
}

TEST_F(ParserTest, ComparisonExpression) {
    auto result = parse("@var cond = a >= b");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    auto var_def = as_var_def(program->statements()[0].get());
    auto bin = as_binary_expr(var_def->init_value());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op(), ">=");
}

TEST_F(ParserTest, ParenthesizedExpression) {
    auto result = parse("@var result = (a + b) * c");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    auto var_def = as_var_def(program->statements()[0].get());
    auto bin = as_binary_expr(var_def->init_value());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op(), "*");
}

TEST_F(ParserTest, OperatorPrecedence) {
    auto result = parse("@var x = a + b * c");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    auto var_def = as_var_def(program->statements()[0].get());
    auto bin = as_binary_expr(var_def->init_value());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op(), "+");
    auto right = as_binary_expr(bin->right());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->op(), "*");
}

// ============================================
// Multiple Statements Tests
// ============================================

TEST_F(ParserTest, MultipleStatements) {
    auto result = parse(
        "#intro \"开始\"\n"
        "> 欢迎来到游戏\n"
        "Alice: 你好！\n"
        "-> next_scene\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 4u);
    
    EXPECT_NE(as_scene_def(program->statements()[0].get()), nullptr);
    EXPECT_NE(as_narrator(program->statements()[1].get()), nullptr);
    EXPECT_NE(as_dialogue(program->statements()[2].get()), nullptr);
    EXPECT_NE(as_jump(program->statements()[3].get()), nullptr);
}

TEST_F(ParserTest, ComplexScript) {
    auto result = parse(
        "#forest \"Forest\"\n"
        "@var hp = 100\n"
        "> You enter the forest\n"
        "Alice: Hello\n"
        "? What now\n"
        "- [Go] -> deep\n"
        "- [Back] -> town\n"
    );
    ASSERT_TRUE(result.is_ok()) << "Parse failed";
    auto program = as_program(result.unwrap());
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements().size(), 5u);
}

// ============================================
// If/Endif Tests
// ============================================

TEST_F(ParserTest, SimpleIf) {
    auto result = parse(
        "if hp >= 0\n"
        "> You are alive\n"
        "endif\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_EQ(program->statements().size(), 1u);
}

TEST_F(ParserTest, IfWithElse) {
    auto result = parse(
        "if hp >= 0\n"
        "> Alive\n"
        "else\n"
        "> Dead\n"
        "endif\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_EQ(program->statements().size(), 1u);
}

TEST_F(ParserTest, IfWithComplexCondition) {
    auto result = parse(
        "if gold >= 50\n"
        "> You can buy\n"
        "endif\n"
    );
    ASSERT_TRUE(result.is_ok());
}

// ============================================
// Command Tests
// ============================================

TEST_F(ParserTest, BgCommand) {
    auto result = parse("@bg forest_bg\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, BgCommandWithArgs) {
    auto result = parse("@bg forest_bg transition:fade duration:1\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, SpriteCommand) {
    auto result = parse("@sprite Alice show\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, SpriteCommandWithArgs) {
    auto result = parse("@sprite Alice move x:100 y:200\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, BgmCommand) {
    auto result = parse("@bgm theme_music\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, BgmCommandWithLoop) {
    auto result = parse("@bgm theme_music loop:true volume:0.8\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, SfxCommand) {
    auto result = parse("@sfx click_sound\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, SetCommand) {
    auto result = parse("@set hp = hp - 10\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, GiveCommand) {
    auto result = parse("@give gold_coin 50\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, TakeCommand) {
    auto result = parse("@take torch 1\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, SaveCommand) {
    auto result = parse("@save \"Before boss\"\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, CallCommand) {
    auto result = parse("@call sub_scene\n");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, ReturnCommand) {
    auto result = parse("@return\n");
    ASSERT_TRUE(result.is_ok());
}

// ============================================
// Multi-Playthrough System Tests
// ============================================

TEST_F(ParserTest, EndingCommand) {
    auto result = parse("@ending ending_bad_01\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto ending = as_ending(program->statements()[0].get());
    ASSERT_NE(ending, nullptr);
    EXPECT_EQ(ending->name(), "ending_bad_01");
}

TEST_F(ParserTest, EndingCommandWithChineseName) {
    auto result = parse("@ending 坏结局_01\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto ending = as_ending(program->statements()[0].get());
    ASSERT_NE(ending, nullptr);
    EXPECT_EQ(ending->name(), "坏结局_01");
}

TEST_F(ParserTest, FlagCommand) {
    auto result = parse("@flag chest_resolved\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto flag = as_flag(program->statements()[0].get());
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->name(), "chest_resolved");
}

TEST_F(ParserTest, FlagCommandWithChineseName) {
    auto result = parse("@flag 宝箱已打开\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto flag = as_flag(program->statements()[0].get());
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->name(), "宝箱已打开");
}

TEST_F(ParserTest, HasEndingFunction) {
    auto result = parse("@var seen = has_ending(ending_bad_01)\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    
    auto call = as_call_expr(var_def->init_value());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->name(), "has_ending");
    ASSERT_EQ(call->arguments().size(), 1u);
    
    auto arg = as_identifier(call->arguments()[0].get());
    ASSERT_NE(arg, nullptr);
    EXPECT_EQ(arg->name(), "ending_bad_01");
}

TEST_F(ParserTest, HasFlagFunction) {
    auto result = parse("@var resolved = has_flag(chest_resolved)\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    
    auto call = as_call_expr(var_def->init_value());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->name(), "has_flag");
}

TEST_F(ParserTest, HasItemFunction) {
    auto result = parse("@var has_key = has_item(golden_key)\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    
    auto call = as_call_expr(var_def->init_value());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->name(), "has_item");
}

TEST_F(ParserTest, ItemCountFunction) {
    auto result = parse("@var coins = item_count(gold_coin)\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    
    auto call = as_call_expr(var_def->init_value());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->name(), "item_count");
}

TEST_F(ParserTest, HasEndingInIfCondition) {
    auto result = parse(
        "if has_ending(ending_good_01)\n"
        "> You have seen the good ending before.\n"
        "endif\n"
    );
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, NotHasFlagInIfCondition) {
    auto result = parse(
        "if not has_flag(chest_opened)\n"
        "> The chest is still closed.\n"
        "endif\n"
    );
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, ItemCountComparison) {
    auto result = parse(
        "if item_count(gold_coin) >= 100\n"
        "> You are rich!\n"
        "endif\n"
    );
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, FunctionWithMultipleArgs) {
    auto result = parse("@var x = custom_func(a, b, c)\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto var_def = as_var_def(program->statements()[0].get());
    auto call = as_call_expr(var_def->init_value());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->name(), "custom_func");
    ASSERT_EQ(call->arguments().size(), 3u);
}

// ============================================
// Label Tests
// ============================================

TEST_F(ParserTest, SimpleLabel) {
    auto result = parse(".main_menu\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto label = as_label(program->statements()[0].get());
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->name(), "main_menu");
}

TEST_F(ParserTest, LabelWithChineseName) {
    auto result = parse(".商店入口\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto label = as_label(program->statements()[0].get());
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->name(), "商店入口");
}

// ============================================
// Check Command Tests
// ============================================

TEST_F(ParserTest, CheckCommandSimple) {
    auto result = parse(
        "@check roll(\"1d20\") >= 15\n"
        "> Success!\n"
        "endcheck\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto check_cmd = as_check_command(program->statements()[0].get());
    ASSERT_NE(check_cmd, nullptr);
    EXPECT_NE(check_cmd->condition(), nullptr);
    EXPECT_EQ(check_cmd->success_branch().size(), 1u);
    EXPECT_EQ(check_cmd->failure_branch().size(), 0u);
}

TEST_F(ParserTest, CheckCommandWithSuccessFail) {
    auto result = parse(
        "@check roll(\"1d20\") >= 15\n"
        "success\n"
        "> You succeeded!\n"
        "fail\n"
        "> You failed.\n"
        "endcheck\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto check_cmd = as_check_command(program->statements()[0].get());
    ASSERT_NE(check_cmd, nullptr);
    EXPECT_EQ(check_cmd->success_branch().size(), 1u);
    EXPECT_EQ(check_cmd->failure_branch().size(), 1u);
}

TEST_F(ParserTest, CheckCommandWithVariable) {
    auto result = parse(
        "@check str + roll(\"1d20\") >= 18\n"
        "success\n"
        "> Critical hit!\n"
        "fail\n"
        "> Missed.\n"
        "endcheck\n"
    );
    ASSERT_TRUE(result.is_ok());
}

TEST_F(ParserTest, CheckCommandFailFirst) {
    auto result = parse(
        "@check luck >= 10\n"
        "fail\n"
        "> Bad luck.\n"
        "success\n"
        "> Lucky!\n"
        "endcheck\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto check_cmd = as_check_command(program->statements()[0].get());
    ASSERT_NE(check_cmd, nullptr);
}

// ============================================
// Roll Function Tests
// ============================================

TEST_F(ParserTest, RollFunction) {
    auto result = parse("@var roll_result = roll(\"1d20\")\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    
    auto call = as_call_expr(var_def->init_value());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->name(), "roll");
    ASSERT_EQ(call->arguments().size(), 1u);
}

TEST_F(ParserTest, RollInComparison) {
    auto result = parse("@var hit = roll(\"1d20\") >= 15\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto var_def = as_var_def(program->statements()[0].get());
    ASSERT_NE(var_def, nullptr);
    
    auto bin = as_binary_expr(var_def->init_value());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op(), ">=");
}

TEST_F(ParserTest, RollWithModifier) {
    auto result = parse("@var damage = roll(\"2d6\") + 3\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    
    auto var_def = as_var_def(program->statements()[0].get());
    auto bin = as_binary_expr(var_def->init_value());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op(), "+");
}

// ============================================
// Wait Command Tests
// ============================================

TEST_F(ParserTest, WaitCommandWithNumber) {
    auto result = parse("@wait 1.5\n");
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto wait = as_wait(program->statements()[0].get());
    ASSERT_NE(wait, nullptr);
    EXPECT_DOUBLE_EQ(wait->seconds(), 1.5);
}

TEST_F(ParserTest, WaitCommandWithText) {
    auto result = parse("@wait 2s\n");
    ASSERT_TRUE(result.is_ok());
}

// ============================================
// Theme Definition Tests
// ============================================

TEST_F(ParserTest, ThemeDef) {
    auto result = parse(
        "@theme dark_forest\n"
        "dialog_bg: #1a1a2e\n"
        "text_color: #AAAAAA\n"
        "@end\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_EQ(program->statements().size(), 1u);
    
    auto theme = as_theme_def(program->statements()[0].get());
    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->name(), "dark_forest");
    ASSERT_EQ(theme->properties().size(), 2u);
}

// ============================================
// Front Matter Tests
// ============================================

TEST_F(ParserTest, FrontMatterSimple) {
    auto result = parse(
        "---\n"
        "title: Test Game\n"
        "version: 1.0\n"
        "---\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_GE(program->statements().size(), 1u);
    
    auto fm = as_front_matter(program->statements()[0].get());
    ASSERT_NE(fm, nullptr);
    ASSERT_GE(fm->properties().size(), 2u);
}

TEST_F(ParserTest, FrontMatterWithScene) {
    auto result = parse(
        "---\n"
        "title: My Game\n"
        "---\n"
        "#intro \"Introduction\"\n"
        "> Welcome!\n"
    );
    ASSERT_TRUE(result.is_ok());
    auto program = as_program(result.unwrap());
    ASSERT_GE(program->statements().size(), 2u);
}

// ============================================
// Error Tests
// ============================================

TEST_F(ParserTest, ErrorUnexpectedToken) {
    auto result = parse("12345");
    EXPECT_TRUE(result.is_err());
}

TEST_F(ParserTest, ErrorMissingColon) {
    auto result = parse("Alice 你好");
    EXPECT_TRUE(result.is_err());
}

TEST_F(ParserTest, ErrorMissingTarget) {
    auto result = parse("-> ");
    EXPECT_TRUE(result.is_err());
}

TEST_F(ParserTest, ErrorUnknownDirective) {
    auto result = parse("@unknown_directive");
    EXPECT_TRUE(result.is_err());
}

TEST_F(ParserTest, ErrorSceneDefMissingName) {
    auto result = parse("#");
    EXPECT_TRUE(result.is_err());
}
