#include <gtest/gtest.h>
#include "nova/vm/vm.h"
#include "nova/vm/variable.h"
#include "nova/vm/inventory.h"
#include "nova/vm/save_data.h"
#include "nova/vm/serializer.h"
#include "nova/packer/packer.h"
#include "nova/packer/ast_serializer.h"
#include "nova/packer/nvmp_format.h"
#include "nova/parser/parser.h"
#include "nova/lexer/lexer.h"
#include "nova/ast/ast_node.h"

using namespace nova;

class VMTest : public ::testing::Test {
protected:
    Result<AstPtr> parse(const std::string& source) {
        Lexer lexer(source, "<test>");
        auto tokens_result = lexer.tokenize();
        if (tokens_result.is_err()) {
            return Result<AstPtr>(tokens_result.error());
        }
        Parser parser(std::move(tokens_result).unwrap());
        return parser.parse();
    }
    
    const ProgramNode* as_program(const AstPtr& ast) {
        return dynamic_cast<const ProgramNode*>(ast.get());
    }
};

// ============================================
// VariableManager Tests
// ============================================

TEST_F(VMTest, VariableSetAndGet) {
    VariableManager vm;
    vm.set("hp", 100.0);
    EXPECT_TRUE(vm.exists("hp"));
    EXPECT_DOUBLE_EQ(vm.asNumber("hp"), 100.0);
}

TEST_F(VMTest, VariableOperations) {
    VariableManager vm;
    vm.set("gold", 50.0);
    vm.add("gold", 10.0);
    EXPECT_DOUBLE_EQ(vm.asNumber("gold"), 60.0);
    
    vm.subtract("gold", 5.0);
    EXPECT_DOUBLE_EQ(vm.asNumber("gold"), 55.0);
}

TEST_F(VMTest, VariableStringType) {
    VariableManager vm;
    vm.set("name", std::string("Hero"));
    EXPECT_EQ(vm.asString("name"), "Hero");
}

TEST_F(VMTest, VariableBoolType) {
    VariableManager vm;
    vm.set("alive", true);
    EXPECT_TRUE(vm.asBool("alive"));
}

// ============================================
// Inventory Tests
// ============================================

TEST_F(VMTest, InventoryAdd) {
    Inventory inv;
    inv.add("sword", 1);
    EXPECT_TRUE(inv.has("sword"));
    EXPECT_EQ(inv.count("sword"), 1);
}

TEST_F(VMTest, InventoryMultiple) {
    Inventory inv;
    inv.add("coin", 50);
    inv.add("coin", 25);
    EXPECT_EQ(inv.count("coin"), 75);
}

TEST_F(VMTest, InventoryRemove) {
    Inventory inv;
    inv.add("potion", 5);
    EXPECT_TRUE(inv.remove("potion", 2));
    EXPECT_EQ(inv.count("potion"), 3);
}

TEST_F(VMTest, InventoryRemoveMoreThanHas) {
    Inventory inv;
    inv.add("key", 1);
    EXPECT_FALSE(inv.remove("key", 5));
    EXPECT_EQ(inv.count("key"), 1);
}

TEST_F(VMTest, InventoryCheckRequirement) {
    Inventory inv;
    inv.add("gold", 100);
    EXPECT_TRUE(inv.checkRequirement("gold", 50));
    EXPECT_FALSE(inv.checkRequirement("gold", 200));
}

// ============================================
// PlaythroughState Tests
// ============================================

TEST_F(VMTest, PlaythroughEnding) {
    PlaythroughState pt;
    EXPECT_FALSE(pt.hasEnding("ending_bad"));
    pt.triggerEnding("ending_bad");
    EXPECT_TRUE(pt.hasEnding("ending_bad"));
}

TEST_F(VMTest, PlaythroughFlag) {
    PlaythroughState pt;
    EXPECT_FALSE(pt.hasFlag("chest_opened"));
    pt.setFlag("chest_opened");
    EXPECT_TRUE(pt.hasFlag("chest_opened"));
    
    pt.clearFlag("chest_opened");
    EXPECT_FALSE(pt.hasFlag("chest_opened"));
}

// ============================================
// VM Basic Tests
// ============================================

TEST_F(VMTest, VMLoadProgram) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Hello World\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    EXPECT_NE(vm.state().status, VMStatus::Ended);
}

TEST_F(VMTest, VMReadsDefaultTextConfigFromFrontMatter) {
    auto result = parse(
        "---\n"
        "default_font: SourceHanSansCN-Regular.ttf\n"
        "default_font_size: 28\n"
        "default_text_speed: 60\n"
        "---\n"
        "#scene_start \"Start\"\n"
        "> Hello\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());

    EXPECT_EQ(vm.state().textConfig.defaultFont, "SourceHanSansCN-Regular.ttf");
    EXPECT_EQ(vm.state().textConfig.defaultFontSize, 28);
    EXPECT_EQ(vm.state().textConfig.defaultTextSpeed, 60);
}

TEST_F(VMTest, VMExecuteNarrator) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Hello World\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "Hello World");
}

TEST_F(VMTest, VMExecuteDialogue) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "林晓: 你好\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->speaker, "林晓");
    EXPECT_EQ(vm.state().dialogue->text, "你好");
}

TEST_F(VMTest, VMDialogueColorInheritsFromCharacterDefinition) {
    auto result = parse(
        "@char 林晓\n"
        "color: #E8A0BF\n"
        "@end\n"
        "#scene_start \"Start\"\n"
        "林晓: 你好\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();

    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->color, "#E8A0BF");
}

TEST_F(VMTest, VMBuildsItemDefinitionRegistry) {
    auto result = parse(
        "@item healing_potion\n"
        "name: 治疗药水\n"
        "description: 恢复生命值\n"
        "@end\n"
        "#scene_start \"Start\"\n"
        "> Hello\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());

    const auto& items = vm.itemDefinitions();
    ASSERT_TRUE(items.count("healing_potion"));
    EXPECT_EQ(items.at("healing_potion").name, "治疗药水");
    EXPECT_EQ(items.at("healing_potion").description, "恢复生命值");
}

TEST_F(VMTest, VMExecuteVarDef) {
    auto result = parse(
        "@var hp = 100\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.variables().exists("hp"));
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("hp"), 100.0);
}

TEST_F(VMTest, VMExecuteSetCommand) {
    auto result = parse(
        "@var gold = 50\n"
        "#scene_start \"Start\"\n"
        "@set gold = 100\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("gold"), 100.0);
}

TEST_F(VMTest, VMExecuteGiveTake) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@give sword 1\n"
        "@give coin 50\n"
        "@take coin 20\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.inventory().has("sword"));
    EXPECT_EQ(vm.inventory().count("coin"), 30);
}

TEST_F(VMTest, VMExecuteJump) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "-> scene_next\n"
        "#scene_next \"Next\"\n"
        "> Jumped!\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    vm.step();
    
    EXPECT_EQ(vm.state().currentScene, "scene_next");
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "Jumped!");
}

TEST_F(VMTest, VMExecuteChoice) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "? What to do?\n"
        "- [Option A] -> .a\n"
        "- [Option B] -> .b\n"
        ".a\n"
        "> Chose A\n"
        ".b\n"
        "> Chose B\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    EXPECT_TRUE(vm.state().choice.has_value());
    EXPECT_EQ(vm.state().choice->options.size(), 2u);
}

TEST_F(VMTest, VMSelectChoice) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "? What to do?\n"
        "- [Option A] -> .a\n"
        ".a\n"
        "> Chose A\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    
    vm.selectChoice(0);
    vm.step();
    
    EXPECT_EQ(vm.state().status, VMStatus::Running);
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "Chose A");
}

TEST_F(VMTest, VMJumpToLabelAcceptsDotPrefix) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        ".a\n"
        "> Hit\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());

    ASSERT_TRUE(vm.jumpToLabel(".a"));
    vm.step();
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "Hit");
}

TEST_F(VMTest, VMContinuesToNextSceneInOrder) {
    auto result = parse(
        "#scene_a \"A\"\n"
        "> In A\n"
        "#scene_b \"B\"\n"
        "> In B\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());

    vm.step();
    EXPECT_EQ(vm.state().currentScene, "scene_a");
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "In A");

    vm.step();
    EXPECT_EQ(vm.state().currentScene, "scene_b");
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "In B");
}

TEST_F(VMTest, VMExecuteBranchTrue) {
    auto result = parse(
        "@var flag = true\n"
        "#scene_start \"Start\"\n"
        "if flag\n"
        "  > True branch\n"
        "endif\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "True branch");
}

TEST_F(VMTest, VMExecuteBgCommand) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@bg \"forest.jpg\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.state().bg.has_value());
}

TEST_F(VMTest, VMExecuteEnding) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@ending bad_ending\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.playthrough().hasEnding("bad_ending"));
    EXPECT_TRUE(vm.state().ending.has_value());
    EXPECT_EQ(vm.state().ending.value(), "bad_ending");
}

TEST_F(VMTest, VMExecuteFlag) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@flag chest_opened\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.playthrough().hasFlag("chest_opened"));
}

TEST_F(VMTest, VMFunctionHasItem) {
    auto result = parse(
        "@give key 1\n"
        "@var has_key = has_item(key)\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.variables().asBool("has_key"));
}

TEST_F(VMTest, VMFunctionHasItemWithStringLiteral) {
    auto result = parse(
        "@give magic_stone 1\n"
        "@var can_use_stone = has_item(\"magic_stone\")\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();

    EXPECT_TRUE(vm.variables().asBool("can_use_stone"));
}

TEST_F(VMTest, VMFunctionItemCount) {
    auto result = parse(
        "@give gold 50\n"
        "@var coins = item_count(gold)\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("coins"), 50.0);
}

TEST_F(VMTest, VMFunctionItemCountWithStringLiteral) {
    auto result = parse(
        "@give magic_stone 2\n"
        "@var stone_count = item_count(\"magic_stone\")\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();

    EXPECT_DOUBLE_EQ(vm.variables().asNumber("stone_count"), 2.0);
}

TEST_F(VMTest, VMFunctionHasFlag) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@flag met_spirit\n"
        "@var met = has_flag(\"met_spirit\")\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();

    EXPECT_TRUE(vm.variables().asBool("met"));
}

TEST_F(VMTest, VMFunctionHasEnding) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@ending good_end\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();

    EXPECT_TRUE(vm.playthrough().hasEnding("good_end"));
}

TEST_F(VMTest, VMFunctionRandomRange) {
    auto result = parse(
        "@var hp = 10\n"
        "@var r = random(1, hp)\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();

    double value = vm.variables().asNumber("r");
    EXPECT_GE(value, 1.0);
    EXPECT_LE(value, 10.0);
}

TEST_F(VMTest, VMFunctionChanceReturnsBool) {
    auto result = parse(
        "@var lucky = chance(0.5)\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();

    EXPECT_TRUE(vm.variables().exists("lucky"));
}

TEST_F(VMTest, VMLogicalAndOrExpressions) {
    auto result = parse(
        "@var hp = 80\n"
        "@var agi = 12\n"
        "@var luck = 1\n"
        "@var ok = hp >= 50 and (agi > 10 or luck > 5)\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();

    EXPECT_TRUE(vm.variables().asBool("ok"));
}

TEST_F(VMTest, VMDialogueUsesSpriteDefaultAndEmotionSprite) {
    auto result = parse(
        "@char 林晓\n"
        "  sprite_default: linxiao_default.png\n"
        "  sprite_happy: linxiao_happy.png\n"
        "@end\n"
        "#scene_start \"Start\"\n"
        "林晓: 你好\n"
        "林晓[happy]: 太好了\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    ASSERT_FALSE(vm.state().sprites.empty());
    EXPECT_EQ(vm.state().sprites.front().url, "linxiao_default.png");

    vm.consumeDialogue();
    vm.step();
    ASSERT_FALSE(vm.state().sprites.empty());
    EXPECT_EQ(vm.state().sprites.front().url, "linxiao_happy.png");
}

TEST_F(VMTest, VMItemDefinitionRegistryIncludesIcon) {
    auto result = parse(
        "@item potion\n"
        "  name: \"治疗药水\"\n"
        "  icon: potion.png\n"
        "@end\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());

    auto it = vm.itemDefinitions().find("potion");
    ASSERT_NE(it, vm.itemDefinitions().end());
    EXPECT_EQ(it->second.icon, "potion.png");
}

TEST_F(VMTest, VMSpritePositionStringPassThrough) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite 林晓 url:linxiao.png position:left\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();

    ASSERT_FALSE(vm.state().sprites.empty());
    EXPECT_EQ(vm.state().sprites.front().position, "left");
}

// ============================================
// Serializer Tests
// ============================================

TEST_F(VMTest, SerializeGameState) {
    GameState state;
    state.currentScene = "forest";
    state.statementIndex = 42;
    state.callStack = {{"village", 10}};
    state.numberVariables = {{"hp", 100.0}, {"gold", 50.0}};
    state.stringVariables = {{"name", "Alice"}};
    state.boolVariables = {{"has_key", true}};
    state.inventory = {{"sword", 1}, {"potion", 3}};
    state.triggeredEndings = {"good_ending"};
    state.flags = {"met_king"};
    
    std::string json = GameStateSerializer::serialize(state);
    
    GameState restored;
    ASSERT_TRUE(GameStateSerializer::deserialize(json, restored));
    
    EXPECT_EQ(restored.currentScene, "forest");
    EXPECT_EQ(restored.statementIndex, 42);
    EXPECT_EQ(restored.callStack.size(), 1);
    EXPECT_EQ(restored.callStack[0].first, "village");
    EXPECT_DOUBLE_EQ(restored.numberVariables["hp"], 100.0);
    EXPECT_DOUBLE_EQ(restored.numberVariables["gold"], 50.0);
    EXPECT_EQ(restored.stringVariables["name"], "Alice");
    EXPECT_TRUE(restored.boolVariables["has_key"]);
    EXPECT_EQ(restored.inventory["sword"], 1);
    EXPECT_EQ(restored.inventory["potion"], 3);
    EXPECT_TRUE(restored.triggeredEndings.count("good_ending"));
    EXPECT_TRUE(restored.flags.count("met_king"));
}

TEST_F(VMTest, SerializeSaveData) {
    GameState state;
    state.currentScene = "castle";
    state.statementIndex = 5;
    state.numberVariables = {{"mp", 80.0}};
    
    SaveData save;
    save.saveId = "save_001";
    save.label = "Chapter 3";
    save.timestamp = std::chrono::system_clock::now();
    save.screenshot = "screenshot.png";
    save.state = state;
    
    std::string json = GameStateSerializer::serializeSave(save);
    
    SaveData restored;
    ASSERT_TRUE(GameStateSerializer::deserializeSave(json, restored));
    
    EXPECT_EQ(restored.saveId, "save_001");
    EXPECT_EQ(restored.label, "Chapter 3");
    EXPECT_EQ(restored.screenshot, "screenshot.png");
    EXPECT_EQ(restored.state.currentScene, "castle");
    EXPECT_EQ(restored.state.statementIndex, 5);
    EXPECT_DOUBLE_EQ(restored.state.numberVariables["mp"], 80.0);
}

TEST_F(VMTest, CaptureAndRestoreState) {
    VariableManager vars;
    vars.set("hp", 100.0);
    vars.set("name", std::string("Hero"));
    vars.set("alive", true);
    
    Inventory inv;
    inv.add("sword", 1);
    inv.add("potion", 5);
    
    std::unordered_set<std::string> endings = {"bad_ending"};
    std::unordered_set<std::string> flags = {"talked_to_wizard"};
    
    std::vector<std::pair<std::string, size_t>> callStack = {{"dungeon", 20}};
    
    GameState state = GameStateSerializer::captureState(
        "battle", 15, callStack, vars, inv, endings, flags
    );
    
    EXPECT_EQ(state.currentScene, "battle");
    EXPECT_EQ(state.statementIndex, 15);
    EXPECT_DOUBLE_EQ(state.numberVariables["hp"], 100.0);
    EXPECT_EQ(state.stringVariables["name"], "Hero");
    EXPECT_TRUE(state.boolVariables["alive"]);
    EXPECT_EQ(state.inventory["sword"], 1);
    EXPECT_EQ(state.inventory["potion"], 5);
    EXPECT_TRUE(state.triggeredEndings.count("bad_ending"));
    EXPECT_TRUE(state.flags.count("talked_to_wizard"));
    
    VariableManager newVars;
    Inventory newInv;
    std::string newScene;
    size_t newIndex = 0;
    std::vector<std::pair<std::string, size_t>> newCallStack;
    std::unordered_set<std::string> newEndings;
    std::unordered_set<std::string> newFlags;
    
    GameStateSerializer::restoreState(
        state, newScene, newIndex, newCallStack, newVars, newInv, newEndings, newFlags
    );
    
    EXPECT_EQ(newScene, "battle");
    EXPECT_EQ(newIndex, 15);
    EXPECT_DOUBLE_EQ(newVars.asNumber("hp"), 100.0);
    EXPECT_EQ(newVars.asString("name"), "Hero");
    EXPECT_TRUE(newVars.asBool("alive"));
    EXPECT_EQ(newInv.count("sword"), 1);
    EXPECT_EQ(newInv.count("potion"), 5);
    EXPECT_TRUE(newEndings.count("bad_ending"));
    EXPECT_TRUE(newFlags.count("talked_to_wizard"));
}

TEST_F(VMTest, SaveToFileAndLoad) {
    GameState state;
    state.currentScene = "test_scene";
    state.numberVariables = {{"test_var", 123.0}};
    
    SaveData save;
    save.saveId = "test_save";
    save.label = "Test";
    save.state = state;
    
    std::string filepath = "test_save.json";
    ASSERT_TRUE(GameStateSerializer::saveToFile(filepath, save));
    
    SaveData loaded;
    ASSERT_TRUE(GameStateSerializer::loadFromFile(filepath, loaded));
    
    EXPECT_EQ(loaded.saveId, "test_save");
    EXPECT_EQ(loaded.state.currentScene, "test_scene");
    EXPECT_DOUBLE_EQ(loaded.state.numberVariables["test_var"], 123.0);
    
    std::remove(filepath.c_str());
}

// ============================================
// Packer Tests
// ============================================

TEST_F(VMTest, BytecodeWriterReader) {
    BytecodeWriter writer;
    writer.writeByte(0x42);
    writer.writeU16(0x1234);
    writer.writeU32(0xDEADBEEF);
    writer.writeU64(0x0123456789ABCDEF);
    writer.writeString("Hello");
    
    auto data = writer.data();
    EXPECT_EQ(data.size(), 1 + 2 + 4 + 8 + 4 + 5);
    
    BytecodeReader reader(data);
    EXPECT_EQ(reader.readByte(), 0x42);
    EXPECT_EQ(reader.readU16(), 0x1234);
    EXPECT_EQ(reader.readU32(), 0xDEADBEEF);
    EXPECT_EQ(reader.readU64(), 0x0123456789ABCDEFULL);
    EXPECT_EQ(reader.readString(), "Hello");
}

TEST_F(VMTest, AstSerializeDialogue) {
    SourceLocation loc("<test>", 1, 1);
    auto program = std::make_unique<ProgramNode>(loc);
    
    auto dialogue = std::make_unique<DialogueNode>(loc, "Alice", "happy", "Hello World");
    program->add_statement(std::move(dialogue));
    
    AstSerializer serializer;
    auto bytecode = serializer.serialize(program.get());
    
    EXPECT_FALSE(bytecode.empty());
    EXPECT_EQ(bytecode[0], static_cast<uint8_t>(OpCode::NodeProgram));
}

TEST_F(VMTest, AstSerializeComplexProgram) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@bg forest.png\n"
        "-> village\n"
    );
    ASSERT_TRUE(result.is_ok()) << "Parse failed";
    
    auto& astPtr = result.unwrap();
    auto program = dynamic_cast<ProgramNode*>(astPtr.get());
    ASSERT_TRUE(program);
    
    AstSerializer serializer;
    auto bytecode = serializer.serialize(program);
    
    EXPECT_FALSE(bytecode.empty());
    
    auto refs = serializer.getAssetReferences();
    EXPECT_EQ(refs.size(), 1);
    EXPECT_EQ(refs[0], "forest.png") << "Parser should preserve full filename including extension";
}

TEST_F(VMTest, NvmpWriteAndRead) {
    BytecodeWriter writer;
    writer.writeByte(static_cast<uint8_t>(OpCode::NodeProgram));
    writer.writeU32(0);
    writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));
    auto bytecode = writer.data();
    
    NvmpWriter nvmpWriter;
    nvmpWriter.setBytecode(bytecode);
    
    auto buffer = nvmpWriter.writeToBuffer();
    EXPECT_FALSE(buffer.empty());
    
    NvmpReader reader;
    ASSERT_TRUE(reader.loadFromBuffer(buffer));
    
    auto& readBytecode = reader.bytecode();
    EXPECT_EQ(readBytecode.size(), bytecode.size());
}

TEST_F(VMTest, NvmpWriteAndReadFile) {
    BytecodeWriter writer;
    writer.writeByte(static_cast<uint8_t>(OpCode::NodeProgram));
    writer.writeU32(0);
    writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));
    auto bytecode = writer.data();
    
    std::string filepath = "test.nvmp";
    
    NvmpWriter nvmpWriter;
    nvmpWriter.setBytecode(bytecode);
    ASSERT_TRUE(nvmpWriter.writeToFile(filepath));
    
    NvmpReader reader;
    ASSERT_TRUE(reader.loadFromFile(filepath));
    
    auto& readBytecode = reader.bytecode();
    EXPECT_EQ(readBytecode.size(), bytecode.size());
    
    std::remove(filepath.c_str());
}

// ============================================
// VM State Transition Tests
// ============================================

TEST_F(VMTest, VMStatusTransitions) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> First\n"
        "> Second\n"
        "@ending test_end\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    EXPECT_EQ(vm.state().status, VMStatus::Running);
    
    vm.load(result.unwrap());
    EXPECT_EQ(vm.state().status, VMStatus::Running);
    
    vm.step();
    EXPECT_EQ(vm.state().status, VMStatus::Running);
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "First");
    
    vm.step();
    EXPECT_EQ(vm.state().status, VMStatus::Running);
    EXPECT_EQ(vm.state().dialogue->text, "Second");
    
    vm.step();
    EXPECT_EQ(vm.state().status, VMStatus::Ended);
    EXPECT_TRUE(vm.state().ending.has_value());
    EXPECT_EQ(*vm.state().ending, "test_end");
}

TEST_F(VMTest, VMWaitingChoiceTransition) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "? Choose:\n"
        "- [A] -> .a\n"
        ".a\n"
        "> Done\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    
    vm.step();
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    EXPECT_TRUE(vm.state().choice.has_value());
    EXPECT_EQ(vm.state().choice->options.size(), 1);
    
    vm.selectChoice(0);
    vm.step();
    EXPECT_EQ(vm.state().status, VMStatus::Running);
}

TEST_F(VMTest, VMReset) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Text\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    EXPECT_TRUE(vm.state().dialogue.has_value());
    
    vm.reset();
    EXPECT_EQ(vm.state().status, VMStatus::Running);
    EXPECT_FALSE(vm.state().dialogue.has_value());
    EXPECT_TRUE(vm.currentScene().empty());
}

// ============================================
// Save/Load Flow Tests
// ============================================

TEST_F(VMTest, VMCaptureState) {
    auto result = parse(
        "@var hp = 100\n"
        "@var name = \"Hero\"\n"
        "@give sword 1\n"
        "#scene_forest \"Forest\"\n"
        ".camp\n"
        "> At camp\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    auto state = vm.captureState();
    EXPECT_EQ(state.currentScene, "scene_forest");
    EXPECT_DOUBLE_EQ(state.numberVariables["hp"], 100.0);
    EXPECT_EQ(state.stringVariables["name"], "Hero");
    EXPECT_EQ(state.inventory["sword"], 1);
}

TEST_F(VMTest, VMLoadSaveRestoresState) {
    auto result = parse(
        "@var hp = 100\n"
        "@var gold = 50\n"
        "@give key 1\n"
        "#scene_dungeon \"Dungeon\"\n"
        ".entrance\n"
        "> Entrance\n"
        ".hall\n"
        "> Hall\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_EQ(vm.currentScene(), "scene_dungeon");
    
    vm.jumpToLabel("hall");
    vm.step();
    
    auto savedState = vm.captureState();
    EXPECT_EQ(savedState.currentScene, "scene_dungeon");
    
    NovaVM vm2;
    vm2.load(result.unwrap());
    EXPECT_TRUE(vm2.loadSave(savedState));
    EXPECT_EQ(vm2.currentScene(), "scene_dungeon");
    EXPECT_DOUBLE_EQ(vm2.variables().asNumber("hp"), 100.0);
    EXPECT_DOUBLE_EQ(vm2.variables().asNumber("gold"), 50.0);
    EXPECT_EQ(vm2.inventory().count("key"), 1);
}

TEST_F(VMTest, VMLoadSaveWithEndingsAndFlags) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Text\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    vm.playthrough().triggerEnding("good_ending");
    vm.playthrough().setFlag("met_king");
    
    auto savedState = vm.captureState();
    EXPECT_TRUE(savedState.triggeredEndings.count("good_ending"));
    EXPECT_TRUE(savedState.flags.count("met_king"));
    
    NovaVM vm2;
    vm2.load(result.unwrap());
    vm2.step();
    EXPECT_TRUE(vm2.loadSave(savedState));
    EXPECT_TRUE(vm2.playthrough().hasEnding("good_ending"));
    EXPECT_TRUE(vm2.playthrough().hasFlag("met_king"));
}

// ============================================
// Edge Case Tests
// ============================================

TEST_F(VMTest, VMDivisionByZero) {
    auto result = parse(
        "@var x = 10\n"
        "@var zero = 0\n"
        "@var result = x / zero\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("result"), 0.0);
}

TEST_F(VMTest, VMModuloByZero) {
    auto result = parse(
        "@var x = 10\n"
        "@var zero = 0\n"
        "@var result = x % zero\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("result"), 0.0);
}

TEST_F(VMTest, VMInvalidDiceExpression) {
    auto result = parse(
        "@var r1 = roll(\"invalid\")\n"
        "@var r2 = roll(\"0d6\")\n"
        "@var r3 = roll(\"1d0\")\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_NO_THROW(vm.variables().asNumber("r1"));
    EXPECT_NO_THROW(vm.variables().asNumber("r2"));
    EXPECT_NO_THROW(vm.variables().asNumber("r3"));
}

TEST_F(VMTest, VMSelectChoiceInvalidIndex) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "? Choose:\n"
        "- [A] -> .a\n"
        ".a\n"
        "> Done\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    
    vm.selectChoice(-1);
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    
    vm.selectChoice(999);
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    
    vm.selectChoice(0);
    EXPECT_EQ(vm.state().status, VMStatus::Running);
}

TEST_F(VMTest, VMJumpToInvalidLabel) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Start\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    EXPECT_FALSE(vm.jumpToLabel("nonexistent"));
    EXPECT_EQ(vm.currentScene(), "scene_start");
}

TEST_F(VMTest, VMJumpToInvalidScene) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Start\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.step();
    
    EXPECT_FALSE(vm.jumpToScene("nonexistent_scene"));
    EXPECT_EQ(vm.currentScene(), "scene_start");
}

// ============================================
// Multi-Scene Flow Tests
// ============================================

TEST_F(VMTest, VMMultiSceneWithLabels) {
    auto result = parse(
        "#scene_intro \"Intro\"\n"
        "> Welcome\n"
        ".menu\n"
        "? What to do?\n"
        "- [Go to forest] -> scene_forest\n"
        "- [Stay] -> .stay\n"
        ".stay\n"
        "> You stayed.\n"
        "#scene_forest \"Forest\"\n"
        "> In the forest.\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    
    vm.step();
    EXPECT_EQ(vm.currentScene(), "scene_intro");
    EXPECT_EQ(vm.state().dialogue->text, "Welcome");
    vm.consumeDialogue();
    
    vm.step();
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    
    vm.selectChoice(0);
    vm.step();
    EXPECT_EQ(vm.currentScene(), "scene_forest");
    EXPECT_EQ(vm.state().dialogue->text, "In the forest.");
}

TEST_F(VMTest, VMSceneAutoContinue) {
    auto result = parse(
        "#scene_a \"A\"\n"
        "> Scene A\n"
        "#scene_b \"B\"\n"
        "> Scene B\n"
        "#scene_c \"C\"\n"
        "> Scene C\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    
    vm.step();
    EXPECT_EQ(vm.currentScene(), "scene_a");
    
    vm.step();
    EXPECT_EQ(vm.currentScene(), "scene_b");
    
    vm.step();
    EXPECT_EQ(vm.currentScene(), "scene_c");
    
    vm.step();
    EXPECT_EQ(vm.state().status, VMStatus::Ended);
}

TEST_F(VMTest, VMCallAndReturn) {
    auto result = parse(
        "#scene_main \"Main\"\n"
        "> Starting\n"
        "@call scene_shop\n"
        "> Back to main\n"
        "#scene_shop \"Shop\"\n"
        "> In shop\n"
        "@return\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    
    vm.step();
    EXPECT_EQ(vm.currentScene(), "scene_main");
    EXPECT_EQ(vm.state().dialogue->text, "Starting");
    vm.consumeDialogue();
    
    vm.step();
    EXPECT_EQ(vm.currentScene(), "scene_shop");
    
    vm.step();
    EXPECT_EQ(vm.state().dialogue->text, "In shop");
    vm.consumeDialogue();
    
    vm.step();
    EXPECT_EQ(vm.currentScene(), "scene_main");
    
    vm.step();
    EXPECT_EQ(vm.state().dialogue->text, "Back to main");
}

// ============================================
// Expression Evaluation Tests
// ============================================

TEST_F(VMTest, VMExpressionEvaluation) {
    auto result = parse(
        "@var a = 10\n"
        "@var b = 3\n"
        "@var sum = a + b\n"
        "@var diff = a - b\n"
        "@var prod = a * b\n"
        "@var quot = a / b\n"
        "@var mod = a % b\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("sum"), 13.0);
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("diff"), 7.0);
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("prod"), 30.0);
    EXPECT_NEAR(vm.variables().asNumber("quot"), 3.333, 0.01);
    EXPECT_DOUBLE_EQ(vm.variables().asNumber("mod"), 1.0);
}

TEST_F(VMTest, VMComparisonExpressions) {
    auto result = parse(
        "@var a = 10\n"
        "@var b = 5\n"
        "@var gt = a >= b\n"
        "@var lt = b < a\n"
        "@var eq = a == 10\n"
        "@var neq = a != b\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.variables().asBool("gt"));
    EXPECT_TRUE(vm.variables().asBool("lt"));
    EXPECT_TRUE(vm.variables().asBool("eq"));
    EXPECT_TRUE(vm.variables().asBool("neq"));
}

TEST_F(VMTest, VMLogicalExpressions) {
    auto result = parse(
        "@var a = true\n"
        "@var b = false\n"
        "@var not_result = not b\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.run();
    
    EXPECT_TRUE(vm.variables().asBool("not_result"));
}
