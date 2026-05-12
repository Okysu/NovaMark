#include <gtest/gtest.h>
#include "nova/vm/vm.h"
#include "nova/vm/state.h"
#include "nova/vm/game_state.h"
#include "nova/vm/serializer.h"
#include "nova/parser/parser.h"
#include "nova/lexer/lexer.h"

using namespace nova;

class NovaStateV2Test : public ::testing::Test {
protected:
    Result<AstPtr> parse(const std::string& source) {
        Lexer lexer(source, "<test>");
        auto tokens_result = lexer.tokenize();
        if (tokens_result.is_err()) return Result<AstPtr>(tokens_result.error());
        Parser parser(std::move(tokens_result).unwrap());
        return parser.parse();
    }

    const ProgramNode* as_program(const AstPtr& ast) {
        return dynamic_cast<const ProgramNode*>(ast.get());
    }
};

/// EndingState 结构体值语义
TEST_F(NovaStateV2Test, EndingStateValueEquality) {
    EndingState a{"good_ending", true};
    EndingState b{"good_ending", true};
    EndingState c{"bad_ending", true};
    EndingState d{"good_ending", false};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a == d);
    EXPECT_EQ(a.title, "good_ending");
    EXPECT_TRUE(a.reached);
}

/// @ending 指令正确填充 EndingState（有标题时用标题）
TEST_F(NovaStateV2Test, EndingInstructionFillsEndingStateWithTitle) {
    auto result = parse(
        "@var score = 0\n"
        "#scene_start \"Start\"\n"
        "@ending perfect \"完美结局\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(as_program(result.unwrap()));
    vm.advance();

    ASSERT_TRUE(vm.state().ending.has_value());
    EXPECT_EQ(vm.state().ending->title, "完美结局");
    EXPECT_TRUE(vm.state().ending->reached);
    EXPECT_EQ(vm.state().status, VMStatus::Ended);
}

/// @ending 指令无标题时用 ending ID
TEST_F(NovaStateV2Test, EndingInstructionUsesIdAsTitleWhenNoQuotedTitle) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@ending test_end\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(as_program(result.unwrap()));
    vm.advance();

    ASSERT_TRUE(vm.state().ending.has_value());
    EXPECT_EQ(vm.state().ending->title, "test_end");
    EXPECT_TRUE(vm.state().ending->reached);
}

/// flags 在 advance() 时从 PlaythroughState 同步到 NovaState
TEST_F(NovaStateV2Test, FlagsSyncedToRenderState) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@flag discovered_secret\n"
        "@flag met_king\n"
        "> Hello\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(as_program(result.unwrap()));
    // 第一次 advance 执行 @flag 语句，停在 "Hello" 对话
    vm.advance();
    // 此时 flags 已在 PlaythroughState 中，但 NovaState 只在 advance 开头同步
    // 再次 advance 时会重新同步
    vm.advance();

    EXPECT_EQ(vm.state().flags.size(), 2u);
    EXPECT_NE(std::find(vm.state().flags.begin(), vm.state().flags.end(), "discovered_secret"), vm.state().flags.end());
    EXPECT_NE(std::find(vm.state().flags.begin(), vm.state().flags.end(), "met_king"), vm.state().flags.end());

    EXPECT_TRUE(vm.playthrough().hasFlag("discovered_secret"));
    EXPECT_TRUE(vm.playthrough().hasFlag("met_king"));
}

/// GameState stateVersion 默认为 2
TEST_F(NovaStateV2Test, GameStateDefaultsToVersion2) {
    GameState state;
    state.clear();
    EXPECT_EQ(state.stateVersion, 2);
}

/// captureState 包含 stateVersion = 2
TEST_F(NovaStateV2Test, CaptureStateIncludesVersion) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Hello\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(as_program(result.unwrap()));
    vm.advance();

    auto state = vm.captureState();
    EXPECT_EQ(state.stateVersion, 2);
}

/// 存档 JSON 序列化包含 stateVersion
TEST_F(NovaStateV2Test, SerializedStateJsonHasVersion) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Hello\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(as_program(result.unwrap()));
    vm.advance();

    auto state = vm.captureState();
    std::string json = GameStateSerializer::serialize(state);

    auto j = nlohmann::json::parse(json);
    EXPECT_EQ(j["stateVersion"], 2);
}

/// v1 存档（ending 为字符串）可正确迁移到 v2
TEST_F(NovaStateV2Test, V1EndingStringMigratesToV2) {
    std::string v1Json = R"({
        "currentScene": "scene_start",
        "statementIndex": 0,
        "stateVersion": 1,
        "ending": "old_ending",
        "endingTitle": "Old Title",
        "callStack": [],
        "numberVariables": {},
        "stringVariables": {},
        "boolVariables": {},
        "inventory": {},
        "triggeredEndings": ["old_ending"],
        "flags": []
    })";

    GameState state;
    ASSERT_TRUE(GameStateSerializer::deserialize(v1Json, state));
    ASSERT_TRUE(state.ending.has_value());
    EXPECT_EQ(state.ending->title, "Old Title");
    EXPECT_TRUE(state.ending->reached);
    EXPECT_EQ(state.stateVersion, 2);  // 应被升级为 v2
}

/// NovaState 中的 ending 和 flags 在 clear() 时被正确清除
TEST_F(NovaStateV2Test, ClearResetsEndingAndFlags) {
    NovaState state;
    state.ending = EndingState{"test", true};
    state.flags.push_back("flag1");
    state.flags.push_back("flag2");

    state.clear();

    EXPECT_FALSE(state.ending.has_value());
    EXPECT_TRUE(state.flags.empty());
}
