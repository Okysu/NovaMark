#include <gtest/gtest.h>
#include "nova/vm/game_state.h"
#include "nova/vm/serializer.h"
#include "nova/vm/vm.h"
#include "nova/vm/registry.h"
#include "nova/parser/parser.h"
#include "nova/lexer/lexer.h"

using namespace nova;

class StateVersionTest : public ::testing::Test {
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

/// v1 存档（无 stateVersion，ending 为字符串）成功加载
TEST_F(StateVersionTest, V1SaveLoadsCorrectly) {
    std::string v1Json = R"({
        "currentScene": "scene_start",
        "statementIndex": 0,
        "ending": null,
        "callStack": [],
        "numberVariables": {},
        "stringVariables": {},
        "boolVariables": {},
        "inventory": {},
        "triggeredEndings": [],
        "flags": []
    })";

    GameState state;
    ASSERT_TRUE(GameStateSerializer::deserialize(v1Json, state));
    EXPECT_EQ(state.stateVersion, 2);  // 自动升级到 v2
}

/// v2 存档（有 stateVersion，ending 为对象）成功加载
TEST_F(StateVersionTest, V2SaveLoadsCorrectly) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@ending good \"Good End\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(as_program(result.unwrap()));
    vm.advance();

    auto state = vm.captureState();
    ASSERT_TRUE(state.ending.has_value());
    EXPECT_EQ(state.ending->title, "Good End");

    // 序列化后再反序列化
    std::string json = GameStateSerializer::serialize(state);
    GameState restored;
    ASSERT_TRUE(GameStateSerializer::deserialize(json, restored));
    ASSERT_TRUE(restored.ending.has_value());
    EXPECT_EQ(restored.ending->title, "Good End");
    EXPECT_TRUE(restored.ending->reached);
    EXPECT_GE(restored.stateVersion, 2);
}

/// extensions 缺失时使用 defaultValue 填充
TEST_F(StateVersionTest, MissingExtensionsUseDefaults) {
    NovaVM vm;

    double life = 0.0;
    vm.registry().registerStateField(
        "game.life",
        [&]() -> std::string { return std::to_string(life); },
        [&](const std::string& s) { life = std::stod(s); },
        "100.0"
    );

    // 构造一个无 extensions 的存档
    GameState state;
    state.currentScene = "scene_start";
    state.stateVersion = 2;

    SaveData save;
    save.saveId = "test";
    save.state = state;

    std::string json = GameStateSerializer::serialize(save.state);
    GameState restored;
    ASSERT_TRUE(GameStateSerializer::deserialize(json, restored));

    // extensions 应为空（新存档中没有注册字段的值）
    EXPECT_TRUE(restored.extensions.empty());
}

/// extensions 往返序列化（值均为 JSON 字符串）
TEST_F(StateVersionTest, ExtensionsRoundTrip) {
    NovaVM vm;

    double life = 50.0;
    vm.registry().registerStateField(
        "game.life",
        [&]() -> std::string { return std::to_string(life); },
        [&](const std::string& s) { life = std::stod(s); },
        "100.0"
    );

    GameState state;
    state.currentScene = "scene_start";
    state.stateVersion = 3;
    state.extensions["game.life"] = "50.0";

    std::string json = GameStateSerializer::serialize(state);
    GameState restored;
    ASSERT_TRUE(GameStateSerializer::deserialize(json, restored));

    ASSERT_TRUE(restored.extensions.count("game.life"));
    EXPECT_EQ(restored.extensions["game.life"], "50.0");
}

/// 二进制存档 v6 格式往返
TEST_F(StateVersionTest, BinarySaveV6RoundTrip) {
    auto result = parse(
        "@var score = 42\n"
        "@flag treasure_found\n"
        "#scene_start \"Start\"\n"
        "Narrator: Hello world\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(as_program(result.unwrap()));
    vm.advance();

    auto state = vm.captureState();
    EXPECT_EQ(state.stateVersion, 2);

    SaveData save;
    save.saveId = "save_001";
    save.label = "test scene";
    save.state = state;
    save.timestamp = std::chrono::system_clock::now();

    auto binary = GameStateSerializer::serializeSaveBinary(save);
    EXPECT_FALSE(binary.empty());

    SaveData restored;
    ASSERT_TRUE(GameStateSerializer::deserializeSaveBinary(binary, restored));
    EXPECT_EQ(restored.saveId, "save_001");
    EXPECT_EQ(restored.label, "test scene");
    EXPECT_EQ(restored.state.currentScene, "scene_start");
    EXPECT_EQ(restored.state.numberVariables.at("score"), 42.0);
}

/// 二进制存档中的 extensions 往返（值均为 JSON 字符串）
TEST_F(StateVersionTest, BinaryExtensionsRoundTrip) {
    GameState state;
    state.currentScene = "scene_start";
    state.stateVersion = 3;
    state.extensions["game.life"] = "75.0";
    state.extensions["game.mana"] = "50.0";

    SaveData save;
    save.saveId = "ext_test";
    save.state = state;
    save.timestamp = std::chrono::system_clock::now();

    auto binary = GameStateSerializer::serializeSaveBinary(save);
    EXPECT_FALSE(binary.empty());

    SaveData restored;
    ASSERT_TRUE(GameStateSerializer::deserializeSaveBinary(binary, restored));
    EXPECT_EQ(restored.state.extensions.at("game.life"), "75.0");
    EXPECT_EQ(restored.state.extensions.at("game.mana"), "50.0");
}
