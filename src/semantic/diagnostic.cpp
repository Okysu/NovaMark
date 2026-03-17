#include "nova/semantic/diagnostic.h"
#include <sstream>

namespace nova {

std::string Diagnostic::to_string() const {
    std::string level_str;
    switch (level) {
        case DiagnosticLevel::Error:   level_str = "error"; break;
        case DiagnosticLevel::Warning: level_str = "warning"; break;
        case DiagnosticLevel::Note:    level_str = "note"; break;
    }
    
    std::ostringstream oss;
    oss << location.to_string() << ": " << level_str << ": " << message;
    return oss.str();
}

void DiagnosticCollector::error(SemanticError err, std::string message, SourceLocation loc) {
    m_diagnostics.push_back({DiagnosticLevel::Error, err, std::move(message), std::move(loc)});
    m_error_count++;
}

void DiagnosticCollector::warning(SemanticError err, std::string message, SourceLocation loc) {
    m_diagnostics.push_back({DiagnosticLevel::Warning, err, std::move(message), std::move(loc)});
    m_warning_count++;
}

void DiagnosticCollector::note(std::string message, SourceLocation loc) {
    m_diagnostics.push_back({DiagnosticLevel::Note, SemanticError::UnreachableCode, 
                            std::move(message), std::move(loc)});
}

void DiagnosticCollector::clear() {
    m_diagnostics.clear();
    m_error_count = 0;
    m_warning_count = 0;
}

std::string DiagnosticCollector::to_string() const {
    std::ostringstream oss;
    for (const auto& diag : m_diagnostics) {
        oss << diag.to_string() << "\n";
    }
    return oss.str();
}

} // namespace nova
