# ─── KayteLlmAssistant static library ────────────────────────────────────────
# Local LLM assistant for KayteIDE. Talks to an Ollama server on localhost
# (https://ollama.com), indexes the project's C / C++ / Kayte / VB / BASIC /
# CMake sources for retrieval, and persists chat sessions under a short hash
# so they can be reopened with `KayteIDE --resume <hash>`.

add_library(KayteLlmAssistant STATIC
        src/OllamaClient.cpp
        src/CodeIndex.cpp
        src/AssistantSession.cpp
        src/AssistantPanel.cpp
        src/CliAgentClient.cpp
        src/LlmLog.cpp
        include/OllamaClient.hpp
        include/CodeIndex.hpp
        include/AssistantSession.hpp
        include/AssistantPanel.hpp
        include/CliAgentClient.hpp
        include/LlmLog.hpp
)

set_target_properties(KayteLlmAssistant PROPERTIES AUTOMOC ON)

target_include_directories(KayteLlmAssistant
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(KayteLlmAssistant
        PUBLIC  Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Concurrent
)
