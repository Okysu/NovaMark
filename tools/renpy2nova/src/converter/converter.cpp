#include "renpy2nova/converter/converter.h"

#include "renpy2nova/analyzer/renpy_analyzer.h"
#include "renpy2nova/emitter/novamark_emitter.h"
#include "renpy2nova/lexer/renpy_lexer.h"

#include <utility>

namespace nova::renpy2nova {
namespace {

ConversionReport merge_reports(ConversionReport first, const ConversionReport& second) {
    for (const auto& diagnostic : second.diagnostics()) {
        first.add(diagnostic.severity, diagnostic.message, diagnostic.line, diagnostic.column);
    }

    for (const auto& entry : second.entries()) {
        first.add_entry(entry);
    }

    return first;
}

} // namespace

ConversionResult<ConversionOutput> Converter::convert(const ConversionInput& input) const {
    RenpyLexer lexer;
    auto token_result = lexer.tokenize(input.source);
    if (token_result.is_err()) {
        return ConversionResult<ConversionOutput>(std::move(token_result.report()));
    }

    RenpyAnalyzer analyzer;
    auto project_result = analyzer.analyze(std::move(token_result.unwrap()));
    if (project_result.is_err()) {
        return ConversionResult<ConversionOutput>(merge_reports(std::move(token_result.report()), project_result.report()));
    }

    NovamarkEmitter emitter;
    auto emit_result = emitter.emit(project_result.unwrap());
    if (emit_result.is_err()) {
        ConversionReport report = merge_reports(std::move(token_result.report()), project_result.report());
        return ConversionResult<ConversionOutput>(merge_reports(std::move(report), emit_result.report()));
    }

    ConversionReport report = merge_reports(std::move(token_result.report()), project_result.report());
    report = merge_reports(std::move(report), emit_result.report());
    ConversionOutput output{emit_result.unwrap(), std::move(report)};
    (void)input.source_name;
    return ConversionResult<ConversionOutput>(std::move(output));
}

} // namespace nova::renpy2nova
