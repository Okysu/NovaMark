#include <gtest/gtest.h>
#include "nova/renderer/nova_c_api.h"
#include "nova/packer/ast_serializer.h"
#include "nova/parser/parser.h"
#include "nova/lexer/lexer.h"

using namespace nova;

class CApiTest : public ::testing::Test {
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

TEST_F(CApiTest, VariableNameEnumerationSorted) {
    auto result = parse(
        "@var beta = 1\n"
        "@var alpha = 2\n"
        "@var gamma = 3\n"
        "#scene_start \"Start\"\n"
        "> Ready\n"
    );
    ASSERT_TRUE(result.is_ok());

    AstSerializer serializer;
    auto bytecode = serializer.serialize(as_program(result.unwrap()));
    ASSERT_FALSE(bytecode.empty());

    ::NovaVM* cVm = nova_create();
    ASSERT_NE(cVm, nullptr);

    ASSERT_EQ(nova_load_from_buffer(cVm, bytecode.data(), bytecode.size()), 1);
    nova_advance(cVm);

    ASSERT_EQ(nova_get_variable_count(cVm), 3u);
    EXPECT_STREQ(nova_get_variable_name(cVm, 0), "alpha");
    EXPECT_STREQ(nova_get_variable_name(cVm, 1), "beta");
    EXPECT_STREQ(nova_get_variable_name(cVm, 2), "gamma");
    EXPECT_EQ(nova_get_variable_name(cVm, 3), nullptr);

    nova_destroy(cVm);
}
