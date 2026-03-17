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