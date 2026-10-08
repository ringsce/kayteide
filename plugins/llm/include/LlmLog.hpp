#pragma once

#include <QString>

namespace Kayte::Llm::LlmLog {

// Append-only activity log for the assistant:
//   <AppData>/llm.log   (macOS: ~/Library/Application Support/ringsce/KayteIDE/llm.log)
// Records sign-in checks, which provider/model answered, timings, costs and
// errors. It never contains passwords, API keys or tokens: Claude and Codex
// keep their own credentials (Claude Code / Codex CLI storage).
QString path();
void    write(const QString &category, const QString &message);

} // namespace Kayte::Llm::LlmLog
