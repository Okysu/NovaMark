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

#include <cstdint>
#include <fstream>

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

TEST_F(VMTest, ProjectMetadataParsesBaseAssetPaths) {
    auto meta = GameMetadata::from_project_file(
        "name: tide_library_demo\n"
        "scripts_path: .\n"
        "assets_path: assets\n"
        "base_bg_path: bg/\n"
        "base_sprite_path: sprites/\n"
        "base_audio_path: audio/\n"
    );

    ASSERT_TRUE(meta.valid);
    EXPECT_EQ(meta.base_bg_path, "bg/");
    EXPECT_EQ(meta.base_sprite_path, "sprites/");
    EXPECT_EQ(meta.base_audio_path, "audio/");
}

TEST_F(VMTest, VMExecuteNarrator) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Hello World\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();

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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    
    ASSERT_TRUE(vm.state().choice.has_value());
    ASSERT_FALSE(vm.state().choice->options.empty());
    vm.choose(vm.state().choice->options[0].id);
    vm.advance();
    
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
    vm.advance();
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

    vm.advance();
    EXPECT_EQ(vm.state().currentScene, "scene_a");
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "In A");

    vm.advance();
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();

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
    vm.advance();
    
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
    vm.advance();

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
    vm.advance();

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
    vm.advance();

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
    vm.advance();

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
    vm.advance();

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
    vm.advance();

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
    vm.advance();
    ASSERT_FALSE(vm.state().sprites.empty());
    EXPECT_EQ(vm.state().sprites.front().url, "linxiao_default.png");

        vm.advance();
    ASSERT_FALSE(vm.state().sprites.empty());
    EXPECT_EQ(vm.state().sprites.front().url, "linxiao_happy.png");
}

TEST_F(VMTest, VMItemDefinitionRegistryIncludesIcon) {
    auto result = parse(
        "@item potion\n"
        "  name: \"治疗药水\"\n"
        "  icon: potion.png\n"
        "  default_value: 3\n"
        "@end\n"
        "#scene_start \"Start\"\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());

    auto it = vm.itemDefinitions().find("potion");
    ASSERT_NE(it, vm.itemDefinitions().end());
    EXPECT_EQ(it->second.icon, "potion.png");
    EXPECT_EQ(it->second.defaultValue, "3");
}

TEST_F(VMTest, VMSpritePositionStringPassThrough) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite 林晓 url:linxiao.png position:left\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_FALSE(vm.state().sprites.empty());
    ASSERT_TRUE(vm.state().sprites.front().position.has_value());
    EXPECT_EQ(*vm.state().sprites.front().position, "left");
}

// ============================================
// Serializer Tests
// ============================================

TEST_F(VMTest, SerializeGameState) {
    GameState state;
    state.currentScene = "forest";
    state.currentLabel = "branch_a";
    state.statementIndex = 42;
    state.bg = "forest.png";
    state.bgm = "theme.mp3";
    state.bgmVolume = 0.5;
    state.bgmLoop = false;
    state.ending = "good_ending";
    state.dialogue = DialogueState{true, "Alice", "Hello", "happy", "#fff"};
    state.choice = ChoiceState{true, "Choose", {ChoiceOption{"c1", "A", ".a", false}}};
    SpriteState sprite;
    sprite.id = "Alice";
    sprite.url = "alice.png";
    sprite.position = "left";
    sprite.opacity = 1.0;
    sprite.zIndex = 1;
    state.sprites = {sprite};
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
    EXPECT_EQ(restored.currentLabel, "branch_a");
    EXPECT_EQ(restored.statementIndex, 42);
    ASSERT_TRUE(restored.bg.has_value());
    EXPECT_EQ(*restored.bg, "forest.png");
    ASSERT_TRUE(restored.bgm.has_value());
    EXPECT_EQ(*restored.bgm, "theme.mp3");
    EXPECT_DOUBLE_EQ(restored.bgmVolume, 0.5);
    EXPECT_FALSE(restored.bgmLoop);
    ASSERT_TRUE(restored.ending.has_value());
    EXPECT_EQ(*restored.ending, "good_ending");
    ASSERT_TRUE(restored.dialogue.has_value());
    EXPECT_EQ(restored.dialogue->text, "Hello");
    ASSERT_TRUE(restored.choice.has_value());
    EXPECT_EQ(restored.choice->question, "Choose");
    ASSERT_EQ(restored.sprites.size(), 1u);
    ASSERT_TRUE(restored.sprites[0].position.has_value());
    EXPECT_EQ(*restored.sprites[0].position, "left");
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
    state.currentTheme = "dark_forest";
    state.themeProperties = {{"dialog_bg", "#1a1a2e"}};
    
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
    ASSERT_TRUE(restored.state.currentTheme.has_value());
    EXPECT_EQ(*restored.state.currentTheme, "dark_forest");
    EXPECT_EQ(restored.state.themeProperties["dialog_bg"], "#1a1a2e");
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
    std::optional<std::string> currentTheme = "dark_forest";
    std::unordered_map<std::string, std::string> themeProperties = {{"dialog_bg", "#000"}};
    
    GameState state = GameStateSerializer::captureState(
        "battle", "camp", 15,
        TextConfigState{}, std::nullopt, std::nullopt, std::nullopt, 1.0, true,
        currentTheme, themeProperties,
        {}, std::nullopt, std::nullopt, std::nullopt,
        callStack, vars, inv, endings, flags
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
    std::string newLabel;
    size_t newIndex = 0;
    TextConfigState textConfig;
    std::optional<std::string> bg;
    std::optional<std::string> bgTransition;
    std::optional<std::string> bgm;
    double bgmVolume = 1.0;
    bool bgmLoop = true;
    std::optional<std::string> newCurrentTheme;
    std::unordered_map<std::string, std::string> newThemeProperties;
    std::vector<SpriteState> sprites;
    std::optional<DialogueState> dialogue;
    std::optional<ChoiceState> choice;
    std::optional<std::string> ending;
    std::vector<std::pair<std::string, size_t>> newCallStack;
    std::unordered_set<std::string> newEndings;
    std::unordered_set<std::string> newFlags;
    
    GameStateSerializer::restoreState(
        state, newScene, newLabel, newIndex,
        textConfig, bg, bgTransition, bgm, bgmVolume, bgmLoop,
        newCurrentTheme, newThemeProperties,
        sprites, dialogue, choice, ending,
        newCallStack, newVars, newInv, newEndings, newFlags
    );
    
    EXPECT_EQ(newScene, "battle");
    EXPECT_EQ(newLabel, "camp");
    EXPECT_EQ(newIndex, 15);
    EXPECT_DOUBLE_EQ(newVars.asNumber("hp"), 100.0);
    EXPECT_EQ(newVars.asString("name"), "Hero");
    EXPECT_TRUE(newVars.asBool("alive"));
    EXPECT_EQ(newInv.count("sword"), 1);
    EXPECT_EQ(newInv.count("potion"), 5);
    EXPECT_TRUE(newEndings.count("bad_ending"));
    EXPECT_TRUE(newFlags.count("talked_to_wizard"));
    ASSERT_TRUE(newCurrentTheme.has_value());
    EXPECT_EQ(*newCurrentTheme, "dark_forest");
    EXPECT_EQ(newThemeProperties["dialog_bg"], "#000");
}

TEST_F(VMTest, SaveToFileAndLoad) {
    GameState state;
    state.currentScene = "test_scene";
    state.numberVariables = {{"test_var", 123.0}};
    
    SaveData save;
    save.saveId = "test_save";
    save.label = "Test";
    save.state = state;
    
    std::string filepath = "test_save.nvs";
    ASSERT_TRUE(GameStateSerializer::saveToFile(filepath, save));
    
    SaveData loaded;
    ASSERT_TRUE(GameStateSerializer::loadFromFile(filepath, loaded));
    
    EXPECT_EQ(loaded.saveId, "test_save");
    EXPECT_EQ(loaded.state.currentScene, "test_scene");
    EXPECT_DOUBLE_EQ(loaded.state.numberVariables["test_var"], 123.0);
    
    std::remove(filepath.c_str());
}

TEST_F(VMTest, SerializeSaveDataBinary) {
    GameState state;
    state.currentScene = "binary_scene";
    state.statementIndex = 7;
    state.numberVariables = {{"hp", 88.0}};
    state.stringVariables = {{"name", "BinaryHero"}};
    state.boolVariables = {{"alive", true}};
    state.inventory = {{"potion", 2}};
    state.triggeredEndings = {"true_end"};
    state.flags = {"opened_gate"};
    state.currentTheme = "binary_theme";
    state.themeProperties = {{"text_color", "#ffffff"}};
    SpriteState sprite;
    sprite.id = "binary_hero";
    sprite.url = "binary.png";
    sprite.x = "72%";
    sprite.opacity = 0.75;
    state.sprites = {sprite};

    SaveData save;
    save.saveId = "save_bin";
    save.label = "Binary Save";
    save.screenshot = "binary.png";
    save.timestamp = std::chrono::system_clock::now();
    save.state = state;

    auto bytes = GameStateSerializer::serializeSaveBinary(save);
    ASSERT_FALSE(bytes.empty());

    SaveData restored;
    ASSERT_TRUE(GameStateSerializer::deserializeSaveBinary(bytes, restored));
    EXPECT_EQ(restored.saveId, "save_bin");
    EXPECT_EQ(restored.label, "Binary Save");
    EXPECT_EQ(restored.screenshot, "binary.png");
    EXPECT_EQ(restored.state.currentScene, "binary_scene");
    EXPECT_EQ(restored.state.statementIndex, 7u);
    EXPECT_DOUBLE_EQ(restored.state.numberVariables["hp"], 88.0);
    EXPECT_EQ(restored.state.stringVariables["name"], "BinaryHero");
    EXPECT_TRUE(restored.state.boolVariables["alive"]);
    EXPECT_EQ(restored.state.inventory["potion"], 2);
    EXPECT_TRUE(restored.state.triggeredEndings.count("true_end"));
    EXPECT_TRUE(restored.state.flags.count("opened_gate"));
    ASSERT_TRUE(restored.state.currentTheme.has_value());
    EXPECT_EQ(*restored.state.currentTheme, "binary_theme");
    EXPECT_EQ(restored.state.themeProperties["text_color"], "#ffffff");
    ASSERT_EQ(restored.state.sprites.size(), 1u);
    EXPECT_EQ(restored.state.sprites[0].id, "binary_hero");
    ASSERT_TRUE(restored.state.sprites[0].url.has_value());
    EXPECT_EQ(*restored.state.sprites[0].url, "binary.png");
    ASSERT_TRUE(restored.state.sprites[0].x.has_value());
    EXPECT_EQ(*restored.state.sprites[0].x, "72%");
    EXPECT_FALSE(restored.state.sprites[0].y.has_value());
    ASSERT_TRUE(restored.state.sprites[0].opacity.has_value());
    EXPECT_DOUBLE_EQ(*restored.state.sprites[0].opacity, 0.75);
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

TEST_F(VMTest, NvmpAssetRoundTripPreservesExactBytes) {
    BytecodeWriter writer;
    writer.writeByte(static_cast<uint8_t>(OpCode::NodeProgram));
    writer.writeU32(0);
    writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));

    AssetBundler bundler;
    std::vector<uint8_t> original = {0xFF, 0xD8, 0xFF, 0xE1, 0x12, 0x34, 0x56, 0x78};
    const std::string assetPath = "temp_asset_single.jpg";
    {
        std::ofstream out(assetPath, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out.write(reinterpret_cast<const char*>(original.data()), static_cast<std::streamsize>(original.size()));
    }
    bundler.addFile(assetPath);

    NvmpWriter nvmpWriter;
    nvmpWriter.setBytecode(writer.data());
    nvmpWriter.setAssets(bundler);

    auto buffer = nvmpWriter.writeToBuffer();
    ASSERT_FALSE(buffer.empty());

    NvmpReader reader;
    ASSERT_TRUE(reader.loadFromBuffer(buffer));

    std::vector<uint8_t> extracted;
    ASSERT_TRUE(reader.getAsset("temp_asset_single.jpg", extracted));
    EXPECT_EQ(extracted, original);

    std::remove(assetPath.c_str());
}

TEST_F(VMTest, NvmpMultipleAssetsPreserveCorrectPayloadMapping) {
    BytecodeWriter writer;
    writer.writeByte(static_cast<uint8_t>(OpCode::NodeProgram));
    writer.writeU32(0);
    writer.writeByte(static_cast<uint8_t>(OpCode::EndNode));

    AssetBundler bundler;
    std::vector<uint8_t> bg = {0xFF, 0xD8, 0xFF, 0xE1, 0xAA, 0xBB};
    std::vector<uint8_t> sprite = {0x89, 0x50, 0x4E, 0x47, 0x01, 0x02};
    std::vector<uint8_t> audio = {0x4F, 0x67, 0x67, 0x53, 0x10, 0x20};

    const std::string bgPath = "temp_bg_test.jpg";
    const std::string spritePath = "temp_sprite_test.png";
    const std::string audioPath = "temp_audio_test.ogg";
    {
        std::ofstream out(bgPath, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out.write(reinterpret_cast<const char*>(bg.data()), static_cast<std::streamsize>(bg.size()));
    }
    {
        std::ofstream out(spritePath, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out.write(reinterpret_cast<const char*>(sprite.data()), static_cast<std::streamsize>(sprite.size()));
    }
    {
        std::ofstream out(audioPath, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out.write(reinterpret_cast<const char*>(audio.data()), static_cast<std::streamsize>(audio.size()));
    }

    bundler.addFile(bgPath);
    bundler.addFile(spritePath);
    bundler.addFile(audioPath);

    NvmpWriter nvmpWriter;
    nvmpWriter.setBytecode(writer.data());
    nvmpWriter.setAssets(bundler);

    auto buffer = nvmpWriter.writeToBuffer();
    ASSERT_FALSE(buffer.empty());

    NvmpReader reader;
    ASSERT_TRUE(reader.loadFromBuffer(buffer));

    std::vector<uint8_t> bgOut;
    std::vector<uint8_t> spriteOut;
    std::vector<uint8_t> audioOut;
    ASSERT_TRUE(reader.getAsset(bgPath, bgOut));
    ASSERT_TRUE(reader.getAsset(spritePath, spriteOut));
    ASSERT_TRUE(reader.getAsset(audioPath, audioOut));

    EXPECT_EQ(bgOut, bg);
    EXPECT_EQ(spriteOut, sprite);
    EXPECT_EQ(audioOut, audio);

    std::remove(bgPath.c_str());
    std::remove(spritePath.c_str());
    std::remove(audioPath.c_str());
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
    
    vm.advance();
    EXPECT_EQ(vm.state().status, VMStatus::Running);
    EXPECT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "First");
    
    vm.advance();
    EXPECT_EQ(vm.state().status, VMStatus::Running);
    EXPECT_EQ(vm.state().dialogue->text, "Second");
    
    vm.advance();
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
    
    vm.advance();
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    EXPECT_TRUE(vm.state().choice.has_value());
    EXPECT_EQ(vm.state().choice->options.size(), 1);
    
    ASSERT_TRUE(vm.state().choice.has_value());
    ASSERT_FALSE(vm.state().choice->options.empty());
    vm.choose(vm.state().choice->options[0].id);
    vm.advance();
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
    EXPECT_EQ(vm.currentScene(), "scene_dungeon");
    
    vm.jumpToLabel("hall");
    vm.advance();
    
    auto savedState = vm.captureState();
    EXPECT_EQ(savedState.currentScene, "scene_dungeon");
    ASSERT_TRUE(savedState.dialogue.has_value());
    EXPECT_EQ(savedState.dialogue->text, "Hall");
    
    NovaVM vm2;
    vm2.load(result.unwrap());
    EXPECT_TRUE(vm2.loadSave(savedState));
    EXPECT_EQ(vm2.currentScene(), "scene_dungeon");
    EXPECT_DOUBLE_EQ(vm2.variables().asNumber("hp"), 100.0);
    EXPECT_DOUBLE_EQ(vm2.variables().asNumber("gold"), 50.0);
    EXPECT_EQ(vm2.inventory().count("key"), 1);
    ASSERT_TRUE(vm2.state().dialogue.has_value());
    EXPECT_EQ(vm2.state().dialogue->text, "Hall");
}

TEST_F(VMTest, VMLoadSaveWithEndingsAndFlags) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Text\n"
    );
    ASSERT_TRUE(result.is_ok());
    
    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();
    
    vm.playthrough().triggerEnding("good_ending");
    vm.playthrough().setFlag("met_king");
    
    auto savedState = vm.captureState();
    EXPECT_TRUE(savedState.triggeredEndings.count("good_ending"));
    EXPECT_TRUE(savedState.flags.count("met_king"));
    
    NovaVM vm2;
    vm2.load(result.unwrap());
    vm2.advance();
    EXPECT_TRUE(vm2.loadSave(savedState));
    EXPECT_TRUE(vm2.playthrough().hasEnding("good_ending"));
    EXPECT_TRUE(vm2.playthrough().hasFlag("met_king"));
}

TEST_F(VMTest, VMLoadSaveReplacesExistingPlaythroughState) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "> Text\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM sourceVm;
    sourceVm.load(result.unwrap());
    sourceVm.advance();
    sourceVm.playthrough().triggerEnding("ending_a");
    sourceVm.playthrough().setFlag("flag_a");
    auto savedState = sourceVm.captureState();

    NovaVM restoredVm;
    restoredVm.load(result.unwrap());
    restoredVm.advance();
    restoredVm.playthrough().triggerEnding("ending_b");
    restoredVm.playthrough().setFlag("flag_b");

    ASSERT_TRUE(restoredVm.loadSave(savedState));
    EXPECT_TRUE(restoredVm.playthrough().hasEnding("ending_a"));
    EXPECT_FALSE(restoredVm.playthrough().hasEnding("ending_b"));
    EXPECT_TRUE(restoredVm.playthrough().hasFlag("flag_a"));
    EXPECT_FALSE(restoredVm.playthrough().hasFlag("flag_b"));
}

TEST_F(VMTest, VMGiveTakeSupportExpressions) {
    auto result = parse(
        "@var bonus = 1\n"
        "#scene_start \"Start\"\n"
        "@give money 1 + bonus\n"
        "@take money bonus\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    EXPECT_EQ(vm.inventory().count("money"), 1);
}

TEST_F(VMTest, VMChooseById) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "? Choose:\n"
        "- [Left] -> .left\n"
        "- [Right] -> .right\n"
        ".left\n"
        "> Left path\n"
        ".right\n"
        "> Right path\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();
    ASSERT_TRUE(vm.state().choice.has_value());
    ASSERT_GE(vm.state().choice->options.size(), 2u);

    const std::string choiceId = vm.state().choice->options[1].id;
    EXPECT_TRUE(vm.choose(choiceId));
    vm.advance();

    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "Right path");
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    
    EXPECT_FALSE(vm.choose("missing_choice"));
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);

    ASSERT_TRUE(vm.state().choice.has_value());
    ASSERT_FALSE(vm.state().choice->options.empty());
    EXPECT_TRUE(vm.choose(vm.state().choice->options[0].id));
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
    vm.advance();
    
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
    vm.advance();
    
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
    
    vm.advance();
    EXPECT_EQ(vm.currentScene(), "scene_intro");
    EXPECT_EQ(vm.state().dialogue->text, "Welcome");
        
    vm.advance();
    EXPECT_EQ(vm.state().status, VMStatus::WaitingChoice);
    
    ASSERT_TRUE(vm.state().choice.has_value());
    ASSERT_FALSE(vm.state().choice->options.empty());
    vm.choose(vm.state().choice->options[0].id);
    vm.advance();
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
    
    vm.advance();
    EXPECT_EQ(vm.currentScene(), "scene_a");
    
    vm.advance();
    EXPECT_EQ(vm.currentScene(), "scene_b");
    
    vm.advance();
    EXPECT_EQ(vm.currentScene(), "scene_c");
    
    vm.advance();
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
    
    vm.advance();
    EXPECT_EQ(vm.currentScene(), "scene_main");
    EXPECT_EQ(vm.state().dialogue->text, "Starting");
        
    vm.advance();
    EXPECT_EQ(vm.currentScene(), "scene_shop");
    
    vm.advance();
    EXPECT_EQ(vm.state().dialogue->text, "In shop");
        
    vm.advance();
    EXPECT_EQ(vm.currentScene(), "scene_main");
    
    vm.advance();
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
    vm.advance();
    
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
    vm.advance();
    
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
    vm.advance();
    
    EXPECT_TRUE(vm.variables().asBool("not_result"));
}

// ============================================
// Sprite Position/Merge Behavior Tests
// ============================================

TEST_F(VMTest, VMSpritePositionLeftDoesNotInjectX) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite alice url:alice.png position:left\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    ASSERT_TRUE(vm.state().sprites[0].position.has_value());
    EXPECT_EQ(*vm.state().sprites[0].position, "left");
    EXPECT_FALSE(vm.state().sprites[0].x.has_value());
}

TEST_F(VMTest, VMSpritePositionRightDoesNotInjectX) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite bob url:bob.png position:right\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    ASSERT_TRUE(vm.state().sprites[0].position.has_value());
    EXPECT_EQ(*vm.state().sprites[0].position, "right");
    EXPECT_FALSE(vm.state().sprites[0].x.has_value());
}

TEST_F(VMTest, VMSpritePositionCenterDoesNotInjectX) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite charlie url:charlie.png position:center\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    ASSERT_TRUE(vm.state().sprites[0].position.has_value());
    EXPECT_EQ(*vm.state().sprites[0].position, "center");
    EXPECT_FALSE(vm.state().sprites[0].x.has_value());
}

TEST_F(VMTest, VMSpritePreservesRawCoordinateStrings) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite aria url:aria.png x:70 y:100\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    ASSERT_TRUE(vm.state().sprites[0].x.has_value());
    ASSERT_TRUE(vm.state().sprites[0].y.has_value());
    EXPECT_EQ(*vm.state().sprites[0].x, "70");
    EXPECT_EQ(*vm.state().sprites[0].y, "100");
}

TEST_F(VMTest, VMSpriteUrlUpdateDoesNotResetPosition) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite diana url:diana_normal.png position:left\n"
        "@sprite diana url:diana_happy.png\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    ASSERT_TRUE(vm.state().sprites[0].url.has_value());
    EXPECT_EQ(*vm.state().sprites[0].url, "diana_happy.png");
    ASSERT_TRUE(vm.state().sprites[0].position.has_value());
    EXPECT_EQ(*vm.state().sprites[0].position, "left");
    EXPECT_FALSE(vm.state().sprites[0].x.has_value());
}

TEST_F(VMTest, VMSpriteUrlUpdateDoesNotResetExplicitX) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite eve url:eve_a.png x:35\n"
        "@sprite eve url:eve_b.png\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    ASSERT_TRUE(vm.state().sprites[0].url.has_value());
    EXPECT_EQ(*vm.state().sprites[0].url, "eve_b.png");
    ASSERT_TRUE(vm.state().sprites[0].x.has_value());
    EXPECT_EQ(*vm.state().sprites[0].x, "35");
}

TEST_F(VMTest, VMSpriteExplicitXCoexistsWithPosition) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite frank url:frank.png position:left x:45\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    ASSERT_TRUE(vm.state().sprites[0].position.has_value());
    EXPECT_EQ(*vm.state().sprites[0].position, "left");
    ASSERT_TRUE(vm.state().sprites[0].x.has_value());
    EXPECT_EQ(*vm.state().sprites[0].x, "45");
}

TEST_F(VMTest, VMSpriteHideRemovesSpriteFromState) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite iris url:iris.png position:left\n"
        "> shown\n"
        "@sprite iris hide\n"
        "> hidden\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    EXPECT_EQ(vm.state().sprites[0].id, "iris");
    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "shown");

    vm.advance();

    EXPECT_TRUE(vm.state().sprites.empty());
    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "hidden");
}

TEST_F(VMTest, VMSpriteShowUsesCharacterDefaultSprite) {
    auto result = parse(
        "@char iris\n"
        "  sprite_default: iris_default.png\n"
        "@end\n"
        "#scene_start \"Start\"\n"
        "@sprite iris show position:left\n"
        "> shown\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    EXPECT_EQ(vm.state().sprites[0].id, "iris");
    ASSERT_TRUE(vm.state().sprites[0].url.has_value());
    EXPECT_EQ(*vm.state().sprites[0].url, "iris_default.png");
    ASSERT_TRUE(vm.state().sprites[0].position.has_value());
    EXPECT_EQ(*vm.state().sprites[0].position, "left");
    EXPECT_FALSE(vm.state().sprites[0].x.has_value());
    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "shown");
}

TEST_F(VMTest, VMDialogueManagedSpritesClearOnSceneJump) {
    auto result = parse(
        "@char iris\n"
        "  sprite_default: iris_default.png\n"
        "  sprite_happy: iris_happy.png\n"
        "@end\n"
        "#scene_start \"Start\"\n"
        "iris[happy]: hello\n"
        "-> scene_next\n"
        "#scene_next \"Next\"\n"
        "> arrived\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_EQ(vm.state().sprites.size(), 1u);
    EXPECT_EQ(vm.state().sprites[0].id, "iris");
    ASSERT_TRUE(vm.state().sprites[0].url.has_value());
    EXPECT_EQ(*vm.state().sprites[0].url, "iris_happy.png");
    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "hello");

    vm.advance();

    EXPECT_TRUE(vm.state().sprites.empty());
    EXPECT_FALSE(vm.state().dialogue.has_value());

    vm.advance();

    EXPECT_TRUE(vm.state().sprites.empty());
    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "arrived");
}

TEST_F(VMTest, VMExplicitSpritesClearOnSceneJump) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@sprite iris show url:iris_default.png position:left\n"
        "-> scene_next\n"
        "#scene_next \"Next\"\n"
        "> arrived\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    EXPECT_TRUE(vm.state().sprites.empty());
    EXPECT_FALSE(vm.state().dialogue.has_value());

    vm.advance();

    EXPECT_TRUE(vm.state().sprites.empty());
    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "arrived");
}

TEST_F(VMTest, VMBgmStopClearsCurrentTrack) {
    auto result = parse(
        "#scene_start \"Start\"\n"
        "@bgm theme.ogg loop:true volume:0.4\n"
        "> started\n"
        "@bgm stop\n"
        "> stopped\n"
    );
    ASSERT_TRUE(result.is_ok());

    NovaVM vm;
    vm.load(result.unwrap());
    vm.advance();

    ASSERT_TRUE(vm.state().bgm.has_value());
    EXPECT_EQ(*vm.state().bgm, "theme.ogg");
    EXPECT_TRUE(vm.state().bgmLoop);
    EXPECT_DOUBLE_EQ(vm.state().bgmVolume, 0.4);
    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "started");

    vm.advance();

    EXPECT_FALSE(vm.state().bgm.has_value());
    EXPECT_TRUE(vm.state().bgmLoop);
    EXPECT_DOUBLE_EQ(vm.state().bgmVolume, 1.0);
    ASSERT_TRUE(vm.state().dialogue.has_value());
    EXPECT_EQ(vm.state().dialogue->text, "stopped");
}
