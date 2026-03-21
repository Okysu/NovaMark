#include <gtest/gtest.h>
#include "nova/semantic/semantic_analyzer.h"
#include "nova/parser/parser.h"
#include "nova/lexer/lexer.h"
#include "nova/ast/ast_node.h"

using namespace nova;

class SemanticTest : public ::testing::Test {
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
    
    SemanticAnalysisResult analyze(const std::string& source) {
        auto result = parse(source);
        if (result.is_err()) {
            SemanticAnalysisResult err_result;
            err_result.success = false;
            return err_result;
        }
        auto program = as_program(result.unwrap());
        SemanticAnalyzer analyzer;
        return analyzer.analyze(program);
    }
};

// ============================================
// 场景定义测试
// ============================================

TEST_F(SemanticTest, SceneDefinition) {
    auto result = analyze("#scene_forest \"Forest\"\n");
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, DuplicateScene) {
    auto result = analyze(
        "#scene_forest \"Forest\"\n"
        "#scene_forest \"Another Forest\"\n"
    );
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.diagnostics.error_count(), 1u);
}

// ============================================
// 变量定义测试
// ============================================

TEST_F(SemanticTest, VariableDefinition) {
    auto result = analyze("@var hp = 100\n");
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, UndefinedVariable) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "@set hp = 100\n"
    );
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.diagnostics.error_count(), 1u);
}

// ============================================
// 角色定义测试
// ============================================

TEST_F(SemanticTest, CharacterDefinition) {
    auto result = analyze(
        "@char 林晓\n"
        "  color: #E8A0BF\n"
        "@end\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, UndefinedCharacterWarning) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "林晓: 你好\n"
    );
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.diagnostics.warning_count(), 1u);
}

// ============================================
// 物品定义测试
// ============================================

TEST_F(SemanticTest, ItemDefinition) {
    auto result = analyze(
        "@item sword\n"
        "  name: 剑\n"
        "  default_value: 1\n"
        "@end\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, UndefinedItem) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "@give sword 1\n"
    );
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.diagnostics.error_count(), 1u);
}

// ============================================
// 跳转测试
// ============================================

TEST_F(SemanticTest, JumpToDefinedScene) {
    auto result = analyze(
        "#scene_start \"Start\"\n"
        "-> scene_next\n"
        "#scene_next \"Next\"\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, JumpToUndefinedScene) {
    auto result = analyze(
        "#scene_start \"Start\"\n"
        "-> scene_undefined\n"
    );
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.diagnostics.error_count(), 1u);
}

// ============================================
// 标签测试
// ============================================

TEST_F(SemanticTest, LabelDefinition) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        ".start\n"
        "> Hello\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, JumpToLabel) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "-> .start\n"
        ".start\n"
        "> Hello\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, UndefinedLabel) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "-> .undefined\n"
    );
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.diagnostics.error_count(), 1u);
}

// ============================================
// 选择分支测试
// ============================================

TEST_F(SemanticTest, ChoiceWithValidTargets) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "? What to do?\n"
        "- [Go left] -> .left\n"
        "- [Go right] -> .right\n"
        ".left\n"
        "> You go left\n"
        ".right\n"
        "> You go right\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, EmptyChoice) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "? What to do?\n"
    );
    EXPECT_FALSE(result.success);
}

// ============================================
// 函数调用测试
// ============================================

TEST_F(SemanticTest, HasEndingFunction) {
    auto result = analyze(
        "@var ended = has_ending(good_end)\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, RollFunction) {
    auto result = analyze(
        "@var damage = roll(\"2d6\")\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, RollFunctionRejectsNonStringArgument) {
    auto result = analyze(
        "@var damage = roll(2)\n"
    );
    EXPECT_FALSE(result.success);
}

TEST_F(SemanticTest, RandomAndChanceFunctions) {
    auto result = analyze(
        "@var hp = 10\n"
        "@var x = random(1, hp)\n"
        "@var lucky = chance(0.5)\n"
    );
    EXPECT_TRUE(result.success);
}

TEST_F(SemanticTest, HasItemRejectsNumericArgument) {
    auto result = analyze(
        "@var ok = has_item(123)\n"
    );
    EXPECT_FALSE(result.success);
}

TEST_F(SemanticTest, UnknownFunction) {
    auto result = analyze(
        "@var x = unknown_func()\n"
    );
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.diagnostics.error_count(), 1u);
}

TEST_F(SemanticTest, WrongArity) {
    auto result = analyze(
        "@var x = roll(\"1d6\", 5)\n"
    );
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.diagnostics.error_count(), 1u);
}

// ============================================
// @check 检定测试
// ============================================

TEST_F(SemanticTest, CheckCommand) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "@check roll(\"1d20\") >= 15\n"
        "@success\n"
        "  > Success!\n"
        "@fail\n"
        "  > Failure!\n"
        "@endcheck\n"
    );
    for (const auto& diag : result.diagnostics.diagnostics()) {
        ADD_FAILURE() << diag.to_string();
    }
    EXPECT_TRUE(result.success) << result.diagnostics.to_string();
    EXPECT_EQ(result.diagnostics.error_count(), 0u);
}

TEST_F(SemanticTest, CheckWithMissingBranches) {
    auto result = analyze(
        "#scene_test \"Test\"\n"
        "@check roll(\"1d20\") >= 15\n"
        "@endcheck\n"
    );
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.diagnostics.warning_count(), 2u);
}

// ============================================
// 完整场景测试
// ============================================

TEST_F(SemanticTest, CompleteScenario) {
    auto result = analyze(
        "@char 林晓\n"
        "  color: #E8A0BF\n"
        "@end\n"
        "\n"
        "@item sword\n"
        "  name: 剑\n"
        "@end\n"
        "\n"
        "@var hp = 100\n"
        "@var gold = 50\n"
        "\n"
        "#scene_start \"Start\"\n"
        "> 你站在路口。\n"
        "林晓: 你好，旅行者。\n"
        "\n"
        "? 你要怎么做？\n"
        "- [买剑 (50金币)] -> .buy if gold >= 50\n"
        "- [离开] -> .leave\n"
        "\n"
        ".buy\n"
        "@take gold 50\n"
        "@give sword 1\n"
        "> 你买了一把剑。\n"
        "-> scene_next\n"
        "\n"
        ".leave\n"
        "> 你转身离开。\n"
        "-> scene_next\n"
        "\n"
        "#scene_next \"Next\"\n"
        "> 故事继续...\n"
    );
    EXPECT_TRUE(result.success);
}
