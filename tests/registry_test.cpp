#include <gtest/gtest.h>
#include "nova/vm/registry.h"
#include "nova/vm/state.h"
#include "nova/vm/variable.h"

using namespace nova;

class RegistryTest : public ::testing::Test {
protected:
    Registry registry;
};

/// 注册自定义指令成功
TEST_F(RegistryTest, RegisterCustomDirective) {
    bool called = false;
    DirectiveResult expected;
    expected.handled = true;

    auto handler = [&](const std::string& name,
                       const std::vector<std::pair<std::string, std::string>>& args,
                       NovaState& state) -> DirectiveResult {
        called = true;
        EXPECT_EQ(name, "custom_effect");
        EXPECT_EQ(args.size(), 1u);
        EXPECT_EQ(args[0].first, "intensity");
        EXPECT_EQ(args[0].second, "high");
        return expected;
    };

    ASSERT_TRUE(registry.registerDirective("custom_effect", handler));

    NovaState state;
    std::vector<std::pair<std::string, std::string>> args = {{"intensity", "high"}};
    auto* found = registry.findDirective("custom_effect");
    ASSERT_NE(found, nullptr);

    auto result = (*found)("custom_effect", args, state);
    EXPECT_TRUE(called);
    EXPECT_TRUE(result.handled);
}

/// 注册同名指令（非 override）失败
TEST_F(RegistryTest, DuplicateDirectiveRegistrationFails) {
    auto handler = [](const std::string&, const auto&, NovaState&) -> DirectiveResult {
        return {true, false};
    };

    ASSERT_TRUE(registry.registerDirective("my_cmd", handler, false));
    ASSERT_FALSE(registry.registerDirective("my_cmd", handler, false));
}

/// override=true 覆写已注册指令
TEST_F(RegistryTest, OverrideDirectiveSucceeds) {
    auto handler1 = [](const std::string&, const auto&, NovaState&) -> DirectiveResult {
        return {true, false};
    };
    auto handler2 = [](const std::string&, const auto&, NovaState&) -> DirectiveResult {
        return {true, true};  // 不同的返回值
    };

    ASSERT_TRUE(registry.registerDirective("my_cmd", handler1));
    ASSERT_TRUE(registry.registerDirective("my_cmd", handler2, true));

    auto* found = registry.findDirective("my_cmd");
    ASSERT_NE(found, nullptr);
    NovaState s;
    auto result = (*found)("my_cmd", {}, s);
    EXPECT_TRUE(result.advanceAgain);  // handler2 的特性
}

/// 注册自定义函数成功
TEST_F(RegistryTest, RegisterCustomFunction) {
    bool called = false;
    auto handler = [&](const std::vector<VarValue>& args) -> VarValue {
        called = true;
        EXPECT_EQ(args.size(), 2u);
        EXPECT_EQ(std::get<double>(args[0]), 10.0);
        EXPECT_EQ(std::get<double>(args[1]), 20.0);
        return 30.0;
    };

    ASSERT_TRUE(registry.registerFunction("my_add", handler));

    auto* found = registry.findFunction("my_add");
    ASSERT_NE(found, nullptr);
    auto result = (*found)({10.0, 20.0});
    EXPECT_TRUE(called);
    EXPECT_EQ(std::get<double>(result), 30.0);
}

/// 注册同名函数（非 override）失败
TEST_F(RegistryTest, DuplicateFunctionRegistrationFails) {
    auto handler = [](const std::vector<VarValue>&) -> VarValue { return 0.0; };

    ASSERT_TRUE(registry.registerFunction("my_func", handler, false));
    ASSERT_FALSE(registry.registerFunction("my_func", handler, false));
}

/// override=true 覆写内置函数
TEST_F(RegistryTest, OverrideBuiltinFunction) {
    auto handler = [](const std::vector<VarValue>&) -> VarValue {
        return 42.0;
    };

    ASSERT_TRUE(registry.isBuiltinFunction("random"));
    ASSERT_TRUE(registry.registerFunction("random", handler, true));

    auto* found = registry.findFunction("random");
    ASSERT_NE(found, nullptr);
    auto result = (*found)({1.0, 100.0});
    EXPECT_EQ(std::get<double>(result), 42.0);
}

/// 注册状态字段成功
TEST_F(RegistryTest, RegisterStateField) {
    double stored = 0.0;
    auto serialize = [&]() -> nlohmann::json {
        return stored;
    };
    auto deserialize = [&](const nlohmann::json& j) {
        stored = j.get<double>();
    };

    ASSERT_TRUE(registry.registerStateField("test.score", serialize, deserialize, 0.0));

    auto* entry = registry.findStateField("test.score");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->key, "test.score");
}

/// 序列化 extensions
TEST_F(RegistryTest, SerializeExtensions) {
    double life = 100.0;
    auto serLife = [&]() -> nlohmann::json { return life; };
    auto desLife = [&](const nlohmann::json& j) { life = j.get<double>(); };

    ASSERT_TRUE(registry.registerStateField("game.life", serLife, desLife, 100.0));

    auto ext = registry.serializeExtensions();
    EXPECT_EQ(ext["game.life"], 100.0);
}

/// 反序列化 extensions
TEST_F(RegistryTest, DeserializeExtensions) {
    double life = 0.0;
    auto serLife = [&]() -> nlohmann::json { return life; };
    auto desLife = [&](const nlohmann::json& j) { life = j.get<double>(); };

    ASSERT_TRUE(registry.registerStateField("game.life", serLife, desLife, 0.0));

    nlohmann::json ext;
    ext["game.life"] = 75.0;
    registry.deserializeExtensions(ext);
    EXPECT_EQ(life, 75.0);
}

/// getDefaultExtensions 返回默认值
TEST_F(RegistryTest, DefaultExtensions) {
    double life = 0.0;
    auto serLife = [&]() -> nlohmann::json { return life; };
    auto desLife = [&](const nlohmann::json& j) { life = j.get<double>(); };

    ASSERT_TRUE(registry.registerStateField("game.life", serLife, desLife, 100.0));

    auto defaults = registry.getDefaultExtensions();
    EXPECT_EQ(defaults["game.life"], 100.0);
    EXPECT_EQ(life, 0.0);  // 未修改实际值
}

/// 空名称注册失败
TEST_F(RegistryTest, EmptyNameRegistrationFails) {
    auto handler = [](const std::string&, const auto&, NovaState&) -> DirectiveResult {
        return {true, false};
    };
    EXPECT_FALSE(registry.registerDirective("", handler));
    EXPECT_FALSE(registry.registerFunction("", [](const auto&) -> VarValue { return 0.0; }));
    EXPECT_FALSE(registry.registerStateField("", nullptr, nullptr, 0));
}

/// isBuiltin 检查
TEST_F(RegistryTest, BuiltinChecks) {
    EXPECT_TRUE(registry.isBuiltinDirective("bg"));
    EXPECT_TRUE(registry.isBuiltinDirective("sprite"));
    EXPECT_TRUE(registry.isBuiltinDirective("set"));
    EXPECT_FALSE(registry.isBuiltinDirective("nonexistent"));

    EXPECT_TRUE(registry.isBuiltinFunction("has_item"));
    EXPECT_TRUE(registry.isBuiltinFunction("random"));
    EXPECT_FALSE(registry.isBuiltinFunction("nonexistent"));
}

/// getRegisteredDirectiveNames 返回正确的注册名列表
TEST_F(RegistryTest, GetRegisteredDirectiveNames) {
    auto handler = [](const std::string&, const auto&, NovaState&) -> DirectiveResult {
        return {true, false};
    };
    registry.registerDirective("custom_a", handler);
    registry.registerDirective("custom_b", handler);

    auto names = registry.getRegisteredDirectiveNames();
    EXPECT_EQ(names.size(), 2u);
    EXPECT_NE(std::find(names.begin(), names.end(), "custom_a"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "custom_b"), names.end());
}
