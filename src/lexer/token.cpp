#include "nova/lexer/token.h"

namespace nova {

std::string Token::to_string() const {
    std::string result = token_type_str(type);
    
    if (!value.empty()) {
        result += "(\"" + value + "\")";
    }
    
    result += " @ " + location.to_string();
    return result;
}

} // namespace nova
