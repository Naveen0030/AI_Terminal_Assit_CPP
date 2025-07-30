#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

using json = nlohmann::json;

// Color utility class for cross-platform colored output
class ColorUtils {
private:
    static bool colors_enabled;
    
public:
    // ANSI color codes
    static const std::string RESET;
    static const std::string BOLD;
    static const std::string DIM;
    
    // Text colors
    static const std::string RED;
    static const std::string GREEN;
    static const std::string YELLOW;
    static const std::string BLUE;
    static const std::string MAGENTA;
    static const std::string CYAN;
    static const std::string WHITE;
    static const std::string GRAY;
    
    // Background colors
    static const std::string BG_RED;
    static const std::string BG_GREEN;
    static const std::string BG_BLUE;
    
    static void initColors() {
#ifdef _WIN32
        // Enable ANSI escape sequences on Windows 10+
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        colors_enabled = true;
#else
        // Check if stdout is a terminal
        colors_enabled = isatty(STDOUT_FILENO);
#endif
    }
    
    static std::string colorize(const std::string& text, const std::string& color) {
        if (!colors_enabled) return text;
        return color + text + RESET;
    }
    
    static bool areColorsEnabled() {
        return colors_enabled;
    }
};

// Static member definitions
bool ColorUtils::colors_enabled = false;
const std::string ColorUtils::RESET = "\033[0m";
const std::string ColorUtils::BOLD = "\033[1m";
const std::string ColorUtils::DIM = "\033[2m";
const std::string ColorUtils::RED = "\033[31m";
const std::string ColorUtils::GREEN = "\033[32m";
const std::string ColorUtils::YELLOW = "\033[33m";
const std::string ColorUtils::BLUE = "\033[34m";
const std::string ColorUtils::MAGENTA = "\033[35m";
const std::string ColorUtils::CYAN = "\033[36m";
const std::string ColorUtils::WHITE = "\033[37m";
const std::string ColorUtils::GRAY = "\033[90m";
const std::string ColorUtils::BG_RED = "\033[41m";
const std::string ColorUtils::BG_GREEN = "\033[42m";
const std::string ColorUtils::BG_BLUE = "\033[44m";

class OllamaAssistant {
private:
    std::string api_url;
    std::string model_name;
    std::vector<json> conversation_history;
    CURL* curl;
    
    struct WriteCallback {
        std::string data;
    };
    
    static size_t WriteCallbackFunc(void* contents, size_t size, size_t nmemb, WriteCallback* userp) {
        size_t totalSize = size * nmemb;
        userp->data.append((char*)contents, totalSize);
        return totalSize;
    }
    
public:
    OllamaAssistant(const std::string& model = "llama3.2") 
        : api_url("http://localhost:11434/api/chat"), model_name(model) {
        curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize libcurl");
        }
        
        // Initialize conversation with system message
        conversation_history.push_back({
            {"role", "system"},
            {"content", "You are a helpful terminal assistant. Provide clear, concise responses focused on programming and technical help."}
        });
    }
    
    ~OllamaAssistant() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }
    
    bool checkOllamaConnection() {
        CURL* test_curl = curl_easy_init();
        if (!test_curl) return false;
        
        WriteCallback response;
        curl_easy_setopt(test_curl, CURLOPT_URL, "http://localhost:11434/api/tags");
        curl_easy_setopt(test_curl, CURLOPT_WRITEFUNCTION, WriteCallbackFunc);
        curl_easy_setopt(test_curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(test_curl, CURLOPT_TIMEOUT, 5L);
        
        CURLcode res = curl_easy_perform(test_curl);
        curl_easy_cleanup(test_curl);
        
        return res == CURLE_OK;
    }
    
    std::vector<std::string> getAvailableModels() {
        std::vector<std::string> models;
        
        WriteCallback response;
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/tags");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            try {
                json response_json = json::parse(response.data);
                if (response_json.contains("models")) {
                    for (const auto& model : response_json["models"]) {
                        if (model.contains("name")) {
                            models.push_back(model["name"]);
                        }
                    }
                }
            } catch (const json::exception& e) {
                // If parsing fails, return empty vector
            }
        }
        
        return models;
    }
    
    void setModel(const std::string& model) {
        model_name = model;
        std::cout << ColorUtils::colorize("✅ Model changed to: ", ColorUtils::GREEN) 
                 << ColorUtils::colorize(model_name, ColorUtils::BOLD + ColorUtils::CYAN) << "\n" << std::endl;
    }
    
    std::string getCurrentModel() const {
        return model_name;
    }
    
    std::string sendMessage(const std::string& message) {
        // Add user message to conversation history
        conversation_history.push_back({
            {"role", "user"},
            {"content", message}
        });
        
        // Prepare JSON payload for Ollama chat API
        json payload = {
            {"model", model_name},
            {"messages", conversation_history},
            {"stream", false}  // Get complete response, not streaming
        };
        
        std::string json_string = payload.dump();
        
        // Set up HTTP headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // Set up response callback
        WriteCallback response;
        
        // Configure curl options
        curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);  // Longer timeout for local processing
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        
        if (res != CURLE_OK) {
            throw std::runtime_error("HTTP request failed: " + std::string(curl_easy_strerror(res)) + 
                                   "\nMake sure Ollama is running: ollama serve");
        }
        
        // Check HTTP response code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code != 200) {
            throw std::runtime_error("Ollama API request failed with HTTP " + std::to_string(response_code) + 
                                   ": " + response.data + 
                                   "\nMake sure the model '" + model_name + "' is installed: ollama pull " + model_name);
        }
        
        // Parse JSON response
        try {
            json response_json = json::parse(response.data);
            
            if (response_json.contains("error")) {
                throw std::runtime_error("Ollama Error: " + response_json["error"].get<std::string>());
            }
            
            if (!response_json.contains("message") || !response_json["message"].contains("content")) {
                throw std::runtime_error("Invalid Ollama API response: no message content found");
            }
            
            std::string assistant_reply = response_json["message"]["content"];
            
            // Add assistant's response to conversation history
            conversation_history.push_back({
                {"role", "assistant"},
                {"content", assistant_reply}
            });
            
            return assistant_reply;
            
        } catch (const json::exception& e) {
            throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
        }
    }
    
    void clearConversation() {
        conversation_history.clear();
        conversation_history.push_back({
            {"role", "system"},
            {"content", "You are a helpful terminal assistant. Provide clear, concise responses focused on programming and technical help."}
        });
        std::cout << ColorUtils::colorize("✅ Conversation history cleared.", ColorUtils::GREEN) << "\n" << std::endl;
    }
    
    void showConversationHistory() {
        std::cout << "\n" << ColorUtils::colorize("=== Conversation History ===", ColorUtils::BOLD + ColorUtils::CYAN) << std::endl;
        
        if (conversation_history.size() <= 1) {
            std::cout << ColorUtils::colorize("No conversation history yet.", ColorUtils::GRAY) << std::endl;
        } else {
            for (size_t i = 1; i < conversation_history.size(); ++i) { // Skip system message
                const auto& msg = conversation_history[i];
                std::string role = msg["role"];
                std::string content = msg["content"];
                
                if (role == "user") {
                    std::cout << ColorUtils::colorize("👤 You: ", ColorUtils::BOLD + ColorUtils::BLUE) 
                             << content << std::endl;
                } else if (role == "assistant") {
                    std::cout << ColorUtils::colorize("🦙 Ollama: ", ColorUtils::BOLD + ColorUtils::GREEN) 
                             << content << std::endl;
                }
                std::cout << std::endl;
            }
        }
        std::cout << ColorUtils::colorize("========================", ColorUtils::CYAN) << "\n" << std::endl;
    }
    
    size_t getConversationLength() const {
        return conversation_history.size() - 1; // Exclude system message
    }
};

class TerminalInterface {
private:
    std::unique_ptr<OllamaAssistant> assistant;
    
    void printHelp() {
        std::cout << "\n" << ColorUtils::colorize("=== 🦙 Ollama Terminal Assistant ===", ColorUtils::BOLD + ColorUtils::MAGENTA) << std::endl;
        std::cout << ColorUtils::colorize("Available Commands:", ColorUtils::BOLD + ColorUtils::CYAN) << std::endl;
        
        std::cout << ColorUtils::colorize("  /help", ColorUtils::YELLOW) 
                 << "     - Show this help message" << std::endl;
        std::cout << ColorUtils::colorize("  /clear", ColorUtils::YELLOW) 
                 << "    - Clear conversation history" << std::endl;
        std::cout << ColorUtils::colorize("  /history", ColorUtils::YELLOW) 
                 << "  - Show conversation history" << std::endl;
        std::cout << ColorUtils::colorize("  /models", ColorUtils::YELLOW) 
                 << "   - List available models" << std::endl;
        std::cout << ColorUtils::colorize("  /model", ColorUtils::YELLOW) 
                 << "    - Change current model" << std::endl;
        std::cout << ColorUtils::colorize("  /status", ColorUtils::YELLOW) 
                 << "   - Check Ollama connection" << std::endl;
        std::cout << ColorUtils::colorize("  /quit", ColorUtils::YELLOW) 
                 << "     - Exit the application" << std::endl;
        std::cout << ColorUtils::colorize("  /exit", ColorUtils::YELLOW) 
                 << "     - Exit the application" << std::endl;
        
        std::cout << "\n" << ColorUtils::colorize("💡 Just type your message and press Enter to chat!", ColorUtils::GREEN) << std::endl;
        std::cout << ColorUtils::colorize("   Ask programming questions, get help, or have a conversation!", ColorUtils::DIM) << std::endl;
        std::cout << ColorUtils::colorize("   Current model: ", ColorUtils::DIM) 
                 << ColorUtils::colorize(assistant->getCurrentModel(), ColorUtils::BOLD + ColorUtils::CYAN) << std::endl;
        std::cout << ColorUtils::colorize("================================", ColorUtils::MAGENTA) << "\n" << std::endl;
    }
    
    void printWelcome() {
        std::cout << ColorUtils::colorize("🦙 Ollama Terminal Assistant", ColorUtils::BOLD + ColorUtils::MAGENTA) << std::endl;
        std::cout << ColorUtils::colorize("✨ Your FREE local AI assistant - No API keys needed!", ColorUtils::CYAN) << std::endl;
        std::cout << ColorUtils::colorize("🚀 Running locally with model: ", ColorUtils::GREEN) 
                 << ColorUtils::colorize(assistant->getCurrentModel(), ColorUtils::BOLD + ColorUtils::CYAN) << std::endl;
        std::cout << ColorUtils::colorize("Type '/help' for commands or start chatting!", ColorUtils::GREEN) << std::endl;
        std::cout << ColorUtils::colorize("========================================", ColorUtils::MAGENTA) << "\n" << std::endl;
    }
    
    std::string getInput() {
        std::string input;
        std::cout << ColorUtils::colorize("👤 You: ", ColorUtils::BOLD + ColorUtils::BLUE);
        std::getline(std::cin, input);
        return input;
    }
    
    void showThinking() {
        std::cout << ColorUtils::colorize("🦙 Thinking", ColorUtils::YELLOW) << std::flush;
        for (int i = 0; i < 3; ++i) {
            std::cout << ColorUtils::colorize(".", ColorUtils::YELLOW) << std::flush;
            std::cout.flush();
        }
    }
    
    void clearThinking() {
        std::cout << "\r" << std::string(15, ' ') << "\r" << std::flush;
    }
    
    bool isCommand(const std::string& input) {
        return !input.empty() && input[0] == '/';
    }
    
    bool handleCommand(const std::string& command) {
        if (command == "/help") {
            printHelp();
            return true;
        } else if (command == "/clear") {
            assistant->clearConversation();
            return true;
        } else if (command == "/history") {
            assistant->showConversationHistory();
            return true;
        } else if (command == "/models") {
            showAvailableModels();
            return true;
        } else if (command == "/model") {
            changeModel();
            return true;
        } else if (command == "/status") {
            checkStatus();
            return true;
        } else if (command == "/quit" || command == "/exit") {
            std::cout << ColorUtils::colorize("👋 Goodbye! Thanks for using Ollama Terminal Assistant!", ColorUtils::GREEN) << std::endl;
            return false;
        } else {
            std::cout << ColorUtils::colorize("❌ Unknown command: ", ColorUtils::RED) << command << std::endl;
            std::cout << ColorUtils::colorize("💡 Type '/help' for available commands.", ColorUtils::YELLOW) << "\n" << std::endl;
            return true;
        }
    }
    
    void showAvailableModels() {
        std::cout << ColorUtils::colorize("🔍 Fetching available models...", ColorUtils::YELLOW) << std::endl;
        
        try {
            auto models = assistant->getAvailableModels();
            if (models.empty()) {
                std::cout << ColorUtils::colorize("❌ No models found. Install a model first:", ColorUtils::RED) << std::endl;
                std::cout << ColorUtils::colorize("   ollama pull llama3.2", ColorUtils::CYAN) << std::endl;
                std::cout << ColorUtils::colorize("   ollama pull codellama", ColorUtils::CYAN) << std::endl;
            } else {
                std::cout << ColorUtils::colorize("📋 Available Models:", ColorUtils::BOLD + ColorUtils::CYAN) << std::endl;
                for (size_t i = 0; i < models.size(); ++i) {
                    std::string marker = (models[i] == assistant->getCurrentModel()) ? "➤ " : "  ";
                    std::string color = (models[i] == assistant->getCurrentModel()) ? 
                                      ColorUtils::BOLD + ColorUtils::GREEN : ColorUtils::WHITE;
                    std::cout << ColorUtils::colorize(marker + std::to_string(i + 1) + ". " + models[i], color) << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << ColorUtils::colorize("❌ Error fetching models: ", ColorUtils::RED) << e.what() << std::endl;
        }
        std::cout << std::endl;
    }
    
    void changeModel() {
        try {
            auto models = assistant->getAvailableModels();
            if (models.empty()) {
                std::cout << ColorUtils::colorize("❌ No models available. Install one first:", ColorUtils::RED) << std::endl;
                std::cout << ColorUtils::colorize("   ollama pull llama3.2", ColorUtils::CYAN) << std::endl;
                return;
            }
            
            std::cout << ColorUtils::colorize("📋 Available Models:", ColorUtils::BOLD + ColorUtils::CYAN) << std::endl;
            for (size_t i = 0; i < models.size(); ++i) {
                std::string marker = (models[i] == assistant->getCurrentModel()) ? "➤ " : "  ";
                std::string color = (models[i] == assistant->getCurrentModel()) ? 
                                  ColorUtils::BOLD + ColorUtils::GREEN : ColorUtils::WHITE;
                std::cout << ColorUtils::colorize(marker + std::to_string(i + 1) + ". " + models[i], color) << std::endl;
            }
            
            std::cout << ColorUtils::colorize("Enter model number (or press Enter to cancel): ", ColorUtils::YELLOW);
            std::string input;
            std::getline(std::cin, input);
            
            if (input.empty()) return;
            
            try {
                int choice = std::stoi(input);
                if (choice >= 1 && choice <= static_cast<int>(models.size())) {
                    assistant->setModel(models[choice - 1]);
                } else {
                    std::cout << ColorUtils::colorize("❌ Invalid choice!", ColorUtils::RED) << std::endl;
                }
            } catch (const std::exception&) {
                std::cout << ColorUtils::colorize("❌ Invalid input!", ColorUtils::RED) << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << ColorUtils::colorize("❌ Error: ", ColorUtils::RED) << e.what() << std::endl;
        }
        std::cout << std::endl;
    }
    
    void checkStatus() {
        std::cout << ColorUtils::colorize("🔍 Checking Ollama connection...", ColorUtils::YELLOW) << std::endl;
        
        if (assistant->checkOllamaConnection()) {
            std::cout << ColorUtils::colorize("✅ Ollama is running and accessible!", ColorUtils::GREEN) << std::endl;
            std::cout << ColorUtils::colorize("📡 Server: http://localhost:11434", ColorUtils::CYAN) << std::endl;
            std::cout << ColorUtils::colorize("🤖 Current model: ", ColorUtils::CYAN) 
                     << ColorUtils::colorize(assistant->getCurrentModel(), ColorUtils::BOLD + ColorUtils::GREEN) << std::endl;
        } else {
            std::cout << ColorUtils::colorize("❌ Cannot connect to Ollama!", ColorUtils::RED) << std::endl;
            std::cout << ColorUtils::colorize("💡 Make sure Ollama is running:", ColorUtils::YELLOW) << std::endl;
            std::cout << ColorUtils::colorize("   ollama serve", ColorUtils::CYAN) << std::endl;
        }
        std::cout << std::endl;
    }
    
public:
    TerminalInterface(const std::string& model_name = "llama3.2") {
        try {
            assistant = std::make_unique<OllamaAssistant>(model_name);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to initialize Ollama assistant: " + std::string(e.what()));
        }
    }
    
    bool initializeConnection() {
        std::cout << ColorUtils::colorize("🔍 Checking Ollama connection...", ColorUtils::YELLOW) << std::endl;
        
        if (!assistant->checkOllamaConnection()) {
            std::cout << ColorUtils::colorize("❌ Cannot connect to Ollama!", ColorUtils::RED) << std::endl;
            std::cout << ColorUtils::colorize("💡 Please make sure Ollama is running:", ColorUtils::YELLOW) << std::endl;
            std::cout << ColorUtils::colorize("   ollama serve", ColorUtils::BOLD + ColorUtils::CYAN) << std::endl;
            std::cout << ColorUtils::colorize("   Then run this program again.", ColorUtils::YELLOW) << std::endl;
            return false;
        }
        
        std::cout << ColorUtils::colorize("✅ Connected to Ollama successfully!", ColorUtils::GREEN) << std::endl;
        return true;
    }
    
    void run() {
        if (!initializeConnection()) {
            return;
        }
        
        printWelcome();
        
        while (true) {
            try {
                std::string input = getInput();
                
                // Handle empty input
                if (input.empty()) {
                    continue;
                }
                
                // Handle commands
                if (isCommand(input)) {
                    if (!handleCommand(input)) {
                        break; // Exit command was executed
                    }
                    continue;
                }
                
                // Send message to Ollama
                showThinking();
                std::string response = assistant->sendMessage(input);
                clearThinking();
                
                std::cout << ColorUtils::colorize("🦙 Ollama: ", ColorUtils::BOLD + ColorUtils::GREEN) 
                         << response << "\n" << std::endl;
                
            } catch (const std::exception& e) {
                clearThinking();
                std::cout << ColorUtils::colorize("❌ Error: ", ColorUtils::BOLD + ColorUtils::RED) 
                         << e.what() << "\n" << std::endl;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    // Initialize colors
    ColorUtils::initColors();
    
    // Initialize libcurl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    std::string model_name = "llama3.2"; // Default model
    
    // Get model name from command line argument
    if (argc > 1) {
        model_name = argv[1];
    }
    
    try {
        TerminalInterface terminal(model_name);
        terminal.run();
    } catch (const std::exception& e) {
        std::cerr << ColorUtils::colorize("💥 Fatal error: ", ColorUtils::BOLD + ColorUtils::RED) 
                 << e.what() << std::endl;
        curl_global_cleanup();
        return 1;
    }
    
    curl_global_cleanup();
    return 0;
}

/*
=== COMPLETE OLLAMA SETUP AND USAGE GUIDE ===

🦙 OLLAMA SETUP (since you already have it installed):

1️⃣ START OLLAMA SERVER:
ollama serve

2️⃣ INSTALL A MODEL (in another terminal):
ollama pull llama3.2          # Recommended: Fast & Good
ollama pull codellama         # For coding tasks
ollama pull llama3.2:1b       # Smaller/faster model
ollama pull phi3:mini         # Very fast lightweight model

3️⃣ VERIFY INSTALLATION:
ollama list                   # See installed models
curl http://localhost:11434   # Test server connection

📦 COMPILE THE C++ PROGRAM:

Ubuntu/Debian:
sudo apt-get update
sudo apt-get install build-essential libcurl4-openssl-dev nlohmann-json3-dev
g++ -std=c++17 -o ollama_assistant main.cpp -lcurl -pthread

macOS:
brew install curl nlohmann-json
g++ -std=c++17 -o ollama_assistant main.cpp -lcurl -pthread

🚀 RUN THE PROGRAM:

Default model (llama3.2):
./ollama_assistant

Specific model:
./ollama_assistant codellama
./ollama_assistant phi3:mini

🎯 AVAILABLE COMMANDS:
/help     - Show help and commands
/models   - List all installed models
/model    - Change current model
/status   - Check Ollama connection
/clear    - Clear conversation history
/history  - View conversation history
/quit     - Exit application

💡 RECOMMENDED MODELS FOR DIFFERENT TASKS:

🔧 Programming Help:
ollama pull codellama         # Best for code
ollama pull deepseek-coder    # Great for coding

💬 General Chat:
ollama pull llama3.2          # Balanced & good
ollama pull phi3:mini         # Fast & lightweight

📚 Detailed Explanations:
ollama pull llama3.1:8b       # More detailed responses

⚡ SPEED COMPARISON:
phi3:mini     - Fastest, good quality
llama3.2:1b   - Fast, decent quality  
llama3.2      - Balanced (recommended)
codellama     - Best for programming
llama3.1:8b   - Slowest but most detailed

🔧 TROUBLESHOOTING:

❌ "Cannot connect to Ollama":
- Run: ollama serve
- Check: curl http://localhost:11434

❌ "Model not found":
- Install: ollama pull llama3.2
- List: ollama list

❌ Compilation errors:
- Install dependencies as shown above
- Use C++17: g++ -std=c++17

🌟 ADVANTAGES OVER CHATGPT API:
✅ Completely FREE - No costs ever
✅ 100% Private - Data stays on your machine
✅ No internet required after setup
✅ No rate limits or usage restrictions
✅ Multiple models to choose from
✅ Customizable system prompts
✅ Fast local responses

🎨 FEATURES:
✅ Full colored terminal output
✅ Multi-turn conversations with memory
✅ Model switching on-the-fly
✅ Connection status checking
✅ Model management
✅ Error handling & recovery
✅ Cross-platform support
✅ Professional terminal interface

Start chatting with your FREE local AI assistant! 🦙✨
*/