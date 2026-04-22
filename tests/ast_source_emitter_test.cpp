#include "nova/ast/ast_snapshot.h"
#include "nova/lexer/lexer.h"
#include "nova/packer/packer.h"
#include "nova/parser/parser.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using namespace nova;

namespace {

std::unique_ptr<ProgramNode> parse_program(const std::string& source, const std::string& file = "test.nvm") {
    Lexer lexer(source, file);
    auto tokens = lexer.tokenize();
    EXPECT_TRUE(tokens.is_ok()) << tokens.error().message;
    Parser parser(tokens.unwrap());
    auto ast = parser.parse();
    EXPECT_TRUE(ast.is_ok()) << ast.error().message;
    auto ptr = std::move(ast).unwrap();
    auto* program = dynamic_cast<ProgramNode*>(ptr.release());
    return std::unique_ptr<ProgramNode>(program);
}

nlohmann::json render_source_files(const std::string& source, const std::string& file = "test.nvm") {
    auto program = parse_program(source, file);
    auto snapshot = export_ast_snapshot_string(program.get());
    return nlohmann::json::parse(ast_snapshot_to_source_files_json(snapshot));
}

std::string first_content(const nlohmann::json& files) {
    EXPECT_TRUE(files.contains("files"));
    EXPECT_FALSE(files.at("files").empty());
    return files.at("files").at(0).at("content").get<std::string>();
}

} // namespace

TEST(AstSourceEmitterTest, EmitsBasicSceneDialogueAndJump) {
    auto files = render_source_files(
        "#scene_start \"开始\"\n"
        "> 你好\n"
        "林晓[happy]: 很高兴见到你\n"
        "-> .next\n"
        ".next\n"
        "@ending good \"好结局\"\n"
    );

    EXPECT_EQ(first_content(files),
        "#scene_start \"开始\"\n"
        "> 你好\n"
        "林晓[happy]: 很高兴见到你\n"
        "-> .next\n"
        ".next\n"
        "@ending good \"好结局\"\n");
}

TEST(AstSourceEmitterTest, EmitsStateCommandsAndExpressions) {
    auto files = render_source_files(
        "@var score = 1 + 2 * 3\n"
        "@set score = (score + 1) * 2\n"
        "@flag answered\n"
        "@give potion score + 1\n"
        "@take coin item_count(\"coin\") - 1\n"
    );

    EXPECT_EQ(first_content(files),
        "@var score = 1 + 2 * 3\n"
        "@set score = (score + 1) * 2\n"
        "@flag answered\n"
        "@give potion score + 1\n"
        "@take coin item_count(\"coin\") - 1\n");
}

TEST(AstSourceEmitterTest, EmitsChoiceSingleLineAndBlockStyle) {
    auto files = render_source_files(
        "? 请选择\n"
        "- [继续] -> .next if has_item(\"key\")\n"
        "- [交出钥匙]\n"
        "  @take key 1\n"
        "  @give coin 5\n"
        "  -> .next\n"
        ".next\n"
        "> 完成\n"
    );

    EXPECT_EQ(first_content(files),
        "? 请选择\n"
        "- [继续] -> .next if has_item(\"key\")\n"
        "- [交出钥匙]\n"
        "  @take key 1\n"
        "  @give coin 5\n"
        "  -> .next\n"
        ".next\n"
        "> 完成\n");
}

TEST(AstSourceEmitterTest, EmitsBranchAndCheckCommand) {
    auto files = render_source_files(
        "if score <= 3\n"
        "  > 状态良好\n"
        "else\n"
        "  > 建议休息\n"
        "endif\n"
        "@check roll(\"2d6\") >= 8\n"
        "@success\n"
        "  @set score = score + 1\n"
        "@fail\n"
        "  @flag failed_check\n"
        "@endcheck\n"
    );

    EXPECT_EQ(first_content(files),
        "if score <= 3\n"
        "  > 状态良好\n"
        "else\n"
        "  > 建议休息\n"
        "endif\n"
        "@check roll(\"2d6\") >= 8\n"
        "@success\n"
        "  @set score = score + 1\n"
        "@fail\n"
        "  @flag failed_check\n"
        "@endcheck\n");
}

TEST(AstSourceEmitterTest, EmitsDefinitionsMediaAndFrontMatter) {
    auto files = render_source_files(
        "---\n"
        "title: Demo\n"
        "---\n"
        "@char alice\n"
        "  name: Alice\n"
        "@end\n"
        "@item potion\n"
        "  name: Potion\n"
        "@end\n"
        "@theme default\n"
        "  color: #fff\n"
        "@end\n"
        "@bg room.png fade:true\n"
        "@sprite alice position:left\n"
        "@bgm theme.mp3 loop:true\n"
        "@sfx click.wav\n"
        "@call side_scene\n"
        "@return\n"
    );

    EXPECT_EQ(first_content(files),
        "---\n"
        "title: Demo\n"
        "---\n"
        "@char alice\n"
        "  name: Alice\n"
        "@end\n"
        "@item potion\n"
        "  name: Potion\n"
        "@end\n"
        "@theme default\n"
        "  color: #fff\n"
        "@end\n"
        "@bg room.png fade\n"
        "@sprite alice position:left\n"
        "@bgm theme.mp3 loop\n"
        "@sfx click.wav\n"
        "@call side_scene\n"
        "@return\n");
}

TEST(AstSourceEmitterTest, GroupsTopLevelNodesByLocationFile) {
    std::vector<MemoryScript> scripts = {
        {"b.nvm", "#scene_b \"B\"\n> B\n"},
        {"a.nvm", "#scene_a \"A\"\n> A\n"}
    };
    auto snapshot = export_ast_snapshot_string_from_scripts(scripts);
    auto files = nlohmann::json::parse(ast_snapshot_to_source_files_json(snapshot));

    ASSERT_EQ(files.at("files").size(), 2u);
    EXPECT_EQ(files.at("files").at(0).at("file"), "b.nvm");
    EXPECT_EQ(files.at("files").at(0).at("content"), "#scene_b \"B\"\n> B\n");
    EXPECT_EQ(files.at("files").at(1).at("file"), "a.nvm");
    EXPECT_EQ(files.at("files").at(1).at("content"), "#scene_a \"A\"\n> A\n");
}

TEST(AstSourceEmitterTest, RenderedOutputCanParseAgain) {
    auto files = render_source_files(
        "@var score = 0\n"
        "#scene_start \"Start\"\n"
        "? 请选择\n"
        "- [有时]\n"
        "  @set score = score + 1\n"
        "  -> .result\n"
        ".result\n"
        "@check score > 0\n"
        "@success\n"
        "  > 成功\n"
        "@fail\n"
        "  > 失败\n"
        "@endcheck\n"
    );

    auto rendered = first_content(files);
    auto reparsed = parse_program(rendered, "rendered.nvm");
    ASSERT_NE(reparsed, nullptr);
    EXPECT_FALSE(reparsed->statements().empty());
}

TEST(AstSourceEmitterTest, EmptyProgramReturnsEmptyFilesArray) {
    auto files = nlohmann::json::parse(ast_snapshot_to_source_files_json(R"({"version":1,"root":{"type":"Program","children":[]}})"));
    ASSERT_TRUE(files.contains("files"));
    EXPECT_TRUE(files.at("files").empty());
}
