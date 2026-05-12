/// @file examples/registry_demo.cpp
/// @brief 注册重载系统完整示例
///
/// 演示三种注册方式：
/// 1. 自定义函数注册 → NovaMark 脚本中调用 custom_skill_check()
/// 2. 自定义状态字段 → 扩展存档的 extensions
/// 3. C API 注册（通过回调桥接）

#include "nova/vm/vm.h"
#include "nova/parser/parser.h"
#include "nova/lexer/lexer.h"
#include <iostream>
#include <cassert>

using namespace nova;

int main() {
    // ======== 示例 1: 自定义函数注册 ========
    // 脚本中使用 custom_difficulty(score) 计算难度等级
    auto script = R"(
        @var score = 85
        @var rank = custom_difficulty(score)

        #scene_start "Start"
        Narrator: 你的综合评分为 {{score}}，难度等级为 {{rank}}。
    )";

    Lexer lexer(script, "<demo>");
    auto tokens = lexer.tokenize();
    assert(tokens.is_ok());

    Parser parser(std::move(tokens).unwrap());
    auto ast = parser.parse();
    assert(ast.is_ok());

    NovaVM vm;

    // 注册自定义函数：custom_difficulty(score) → 返回难度等级字符串
    vm.registry().registerFunction("custom_difficulty",
        [](const std::vector<VarValue>& args) -> VarValue {
            if (args.empty()) return "unknown";
            double score = std::get<double>(args[0]);
            if (score >= 90) return std::string("S");
            if (score >= 80) return std::string("A");
            if (score >= 70) return std::string("B");
            return std::string("C");
        }
    );

    auto* program = dynamic_cast<ProgramNode*>(ast.unwrap().get());
    vm.load(program);
    vm.advance();

    std::cout << "=== 示例 1: 自定义函数 ===" << std::endl;
    std::cout << "Dialogue: " << vm.state().dialogue->text << std::endl;
    std::cout << "Expected:  你的综合评分为 85，难度等级为 A。" << std::endl;

    // ======== 示例 2: 自定义状态字段 ========
    double playerExp = 1500.0;

    vm.registry().registerStateField(
        "com.example.player_exp",         // 推荐命名空间前缀
        [&]() -> nlohmann::json { return playerExp; },      // 序列化
        [&](const nlohmann::json& j) { playerExp = j.get<double>(); }, // 反序列化
        0.0                                // 默认值
    );

    // 模拟存档 → 修改 exp → 读档
    auto saved = vm.captureState();
    playerExp = 2500.0;  // 模拟游戏中经验值变化
    vm.loadSave(saved);  // 从旧存档恢复

    std::cout << "\n=== 示例 2: 自定义状态字段 ===" << std::endl;
    std::cout << "Player EXP after loadSave: " << playerExp << std::endl;
    std::cout << "Expected: 2500.0 (恢复了存档中的值)" << std::endl;

    // ======== 示例 3: 覆写内置函数 ========
    NovaVM vm2;
    Lexer lexer2("@var val = random(1, 100)\n#scene_start \"S\"\n> {{val}}\n", "<demo3>");
    auto tokens2 = lexer2.tokenize();
    Parser parser2(std::move(tokens2).unwrap());
    auto ast2 = parser2.parse();
    auto* program2 = dynamic_cast<ProgramNode*>(ast2.unwrap().get());

    // 覆写 random() 为返回固定值 42
    vm2.registry().registerFunction("random",
        [](const std::vector<VarValue>&) -> VarValue { return 42.0; },
        true  // override = true
    );

    vm2.load(program2);
    vm2.advance();

    std::cout << "\n=== 示例 3: 覆写内置函数 random() ===" << std::endl;
    std::cout << "Dialogue: " << vm2.state().dialogue->text << std::endl;
    std::cout << "Expected:  42 (random(1,100) 被覆写为固定返回 42)" << std::endl;

    return 0;
}
