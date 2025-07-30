# 🦙 Ollama C++ AI Terminal Assistant

A powerful, FREE, and completely local AI assistant built in C++ that runs entirely on your machine!

---

### Demo Highlights:

- 🚀 Instant Setup - From installation to first chat in under 2 minutes
- 💬 Natural Conversations - Multi-turn programming discussions with context memory
- 🎨 Beautiful Interface - Colored terminal output with emojis and clear formatting
- ⚡ Model Switching - Change AI models on-the-fly (/model command)
- 🔧 Programming Help - Real-time code assistance, debugging, and explanations
- 📊 System Commands - Built-in tools for model management and status checking

---

## ✨ Features

### 🎯 *Core Capabilities*
- 🤖 *Multi-turn Conversations* - Maintains context throughout your session
- 🎨 *Colored Terminal Interface* - Beautiful, professional-looking output
- 🔄 *Model Management* - Switch between different AI models instantly  
- 💾 *Conversation History* - View and manage your chat history
- 🔧 *Programming Focus* - Optimized for coding help and technical discussions
- 🚀 *Cross-Platform* - Works on Linux, macOS, and Windows

### 💰 *Why Choose This Over ChatGPT API?*
| Feature | Ollama C++ Assistant | ChatGPT API |
|---------|---------------------|-------------|
| *Cost* | 🟢 *100% FREE* | 🔴 Pay per token |
| *Privacy* | 🟢 *Completely Local* | 🔴 Data sent to OpenAI |
| *Internet* | 🟢 *Works Offline* | 🔴 Requires connection |
| *Speed* | 🟢 *No Network Latency* | 🔴 Network dependent |
| *Rate Limits* | 🟢 *Unlimited Usage* | 🔴 API rate limits |
| *Models* | 🟢 *Multiple Models* | 🔴 Limited to OpenAI models |

### 🎮 *Interactive Commands*
- /help - Show all available commands
- /models - List installed AI models  
- /model - Switch to a different model
- /status - Check Ollama connection
- /clear - Clear conversation history
- /history - View conversation history
- /quit - Exit the application

---

## 🚀 Quick Start

### Prerequisites
- *Ollama* installed on your system
- *C++17* compatible compiler
- *libcurl* and *nlohmann-json* libraries

### 1️⃣ Install Ollama (if not already installed)

bash
# Linux/macOS
curl -fsSL https://ollama.ai/install.sh | sh

# Windows (PowerShell)
iex (irm ollama.ai/install.ps1)

### 2️⃣ Start Ollama & Install Models
bash
# Start Ollama server (keep this running)
ollama serve

# In another terminal, install models:
ollama pull llama3.2          # Recommended for general use
ollama pull codellama         # Best for programming tasks  
ollama pull phi3:mini         # Lightweight & fast


### 4️⃣ Compile & Run

### 🚀 Setup Guide (Windows)

#### 1. ✅ Install [Ollama](https://ollama.com/download)
After installation, run it once:
bash
ollama run llama3

This downloads and runs the model locally via http://localhost:11434.

ℹ Ollama must be running in the background before starting the assistant.
