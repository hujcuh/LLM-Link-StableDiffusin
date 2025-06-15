#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING

#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>
#include <cpprest/filestream.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "basefunc.h"  // 包含 Base64 解码函数

using namespace utility;   // 用于 U("...")
using namespace web;
using namespace web::http;
using namespace web::http::client;

#ifdef _WIN32   //======================================================================================string convert
// 将 std::wstring 转换为 UTF-8 std::string
std::string wstring_to_utf8(const std::wstring& wstr) { 
    if (wstr.empty())
        return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size_needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_needed, nullptr, nullptr);
    return result;
}

// 将 UTF-8 std::string 转换为 std::wstring
std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty())
        return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size_needed == 0)
        return std::wstring();
    std::wstring result(size_needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size_needed);
    if (!result.empty() && result.back() == L'\0')
        result.pop_back();
    return result;
}
#endif

// =================================================================================================大语言模型对话模块
// 发送 HTTP POST 请求到大模型接口，并解析 JSON 返回中 "content" 字段。
// 这里增加了生成参数 n_predict 和 temperature

std::string callLargeModel(const std::string& prompt, int n_predict = 256, double temperature = 0.7) {
    utility::string_t model_url = U("http://127.0.0.1:8080/completion");
    json::value payload;
    try {
        payload[U("prompt")] = json::value::string(conversions::to_string_t(prompt));
        // 添加生成参数
        payload[U("n_predict")] = json::value::number(n_predict);
        payload[U("temperature")] = json::value::number(temperature);
    }
    catch (const std::exception& e) {
        std::cerr << "大模型提示词转换异常: " << e.what() << std::endl;
        return "Error: 输入提示存在非法字符。";
    }

    // 设置超时时间为 600 秒
    http_client_config config;
    config.set_timeout(std::chrono::seconds(600));

    http_client client(model_url, config);
    http_request request(methods::POST);
    request.headers().set_content_type(U("application/json"));
    request.headers().remove(U("Expect"));
    request.set_body(payload);

    try {
        http_response response = client.request(request).get();
        if (response.status_code() != status_codes::OK) {
            std::cerr << "大模型 HTTP 请求失败, 状态码: " << response.status_code() << std::endl;
            return "Error: 大模型请求失败。";
        }
        utility::string_t resp_str = response.extract_string().get();
        std::cout << "大模型 Raw Response:\n"
            << conversions::to_utf8string(resp_str) << std::endl;
        json::value json_resp = json::value::parse(resp_str);
        if (!json_resp.has_field(U("content"))) {
            return "Error: 响应中未包含 'content' 字段。";
        }
        return conversions::to_utf8string(json_resp.at(U("content")).as_string());
    }
    catch (const std::exception& e) {
        std::cerr << "调用大模型异常: " << e.what() << std::endl;
        return "Error: 调用大模型异常。";
    }
}


// =================================================================================================sd‑webui 绘图模块
// 发送 HTTP POST 请求到 sd‑webui 接口，并解析返回的 JSON 中 "images" 数组中的第一项

bool callSdWebuiDraw(const std::string& prompt) {
    utility::string_t draw_url = U("http://127.0.0.1:7860/sdapi/v1/txt2img");
    json::value payload;
    try {
        payload[U("prompt")] = json::value::string(conversions::to_string_t(prompt));
    }
    catch (const std::exception& e) {
        std::cerr << "绘图提示词转换异常: " << e.what() << std::endl;
        return false;
    }
    payload[U("steps")] = json::value::number(20);
    payload[U("width")] = json::value::number(512);
    payload[U("height")] = json::value::number(512);
    payload[U("negative_prompt")] = json::value::string(U(""));
    payload[U("cfg_scale")] = json::value::number(7);

    http_client client(draw_url);
    http_request request(methods::POST);
    request.headers().set_content_type(U("application/json"));
    request.headers().remove(U("Expect"));
    request.set_body(payload);

    try {
        http_response response = client.request(request).get();
        if (response.status_code() != status_codes::OK) {
            std::cerr << "绘图 HTTP 请求失败, 状态码: " << response.status_code() << std::endl;
            return false;
        }
        utility::string_t resp_str = response.extract_string().get();
        std::cout << "绘图 Raw Response:\n"
            << conversions::to_utf8string(resp_str) << std::endl;
        json::value json_resp = json::value::parse(resp_str);
        if (!json_resp.has_field(U("images")) || !json_resp.at(U("images")).is_array()) {
            std::cerr << "绘图响应中缺少 images 数组或格式不正确！" << std::endl;
            return false;
        }

        utility::string_t b64_str = json_resp.at(U("images")).as_array()[0].as_string();
        // 如果图片数据中包含 Base64 前缀，则移除
        utility::string_t prefix = U("data:image/png;base64,");
        if (b64_str.find(prefix) == 0) {
            b64_str = b64_str.substr(prefix.size());
        }
#ifdef _WIN32
        std::wstring wb64_str = b64_str;
        std::string b64_std = wstring_to_utf8(wb64_str);
#else
        std::string b64_std = b64_str;
#endif
        std::vector<unsigned char> image_data = Base64::decode(b64_std);
        if (image_data.empty()) {
            std::cerr << "Base64 解码失败，未获得有效数据。" << std::endl;
            return false;
        }
        std::ofstream ofs("output.png", std::ios::binary);
        if (!ofs) {
            std::cerr << "无法创建文件 output.png" << std::endl;
            return false;
        }
        ofs.write(reinterpret_cast<const char*>(image_data.data()), image_data.size());
        ofs.close();

        std::cout << "图片已保存为 output.png" << std::endl;
#ifdef _WIN32
        system("start output.png");
#elif __APPLE__
        system("open output.png");
#else
        system("xdg-open output.png");
#endif
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "调用绘图接口异常: " << e.what() << std::endl;
        return false;
    }
}


// =======================================================================================转换绘图提示词
// 直接提取用户输入中 "draw" 后面的部分，去除左右空白

std::string transformDrawingPrompt(const std::string& conversation) {
    size_t pos = conversation.find("draw");
    if (pos != std::string::npos) {
        std::string prompt = conversation.substr(pos + 4);
        // 去掉首尾空白和换行
        while (!prompt.empty() && (prompt.front() == ' ' || prompt.front() == '\n'))
            prompt.erase(prompt.begin());
        while (!prompt.empty() && (prompt.back() == ' ' || prompt.back() == '\n'))
            prompt.pop_back();
        return prompt;
    }
    return conversation;
}


// ========================================================================================主函数：模式选择、对话及绘图整合

int main() {
#ifdef _WIN32
    // 设置控制台为 UTF-8；建议在外部终端（如 Windows Terminal 或 CMD）中运行程序
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << u8"欢迎使用大模型对话生图！" << std::endl;
    std::cout << u8"请选择模式：" << std::endl;
    std::cout << u8"1. 直接与大语言模型对话" << std::endl;
    std::cout << u8"2. 对话后转换为绘图提示词，调用 sd‑webui 绘图" << std::endl;
    std::cout << u8"请输入模式 (1 或 2): ";
    int mode = 0;
    std::cin >> mode;
    std::cin.ignore(); // 清除剩余换行

    std::string conversationHistory;
    std::string userInput;

    if (mode == 1) {
        std::cout << u8"进入对话模式，输入 'exit' 或 'quit' 结束对话。" << std::endl;
        while (true) {
            std::cout << u8"用户: ";
            std::getline(std::cin, userInput);
            if (userInput == "exit" || userInput == "quit")
                break;
            conversationHistory += u8"用户: " + userInput + "\n";
            std::string reply = callLargeModel(userInput, 128, 0.7);
            conversationHistory += u8"模型: " + reply + "\n";
            std::cout << u8"模型: " << reply << std::endl;
        }
    }
    else if (mode == 2) {
        std::cout << u8"进入对话+绘图模式，输入 'exit' 或 'quit' 结束对话。" << std::endl;
        while (true) {
            std::cout << u8"用户: ";
            std::getline(std::cin, userInput);
            if (userInput == "exit" || userInput == "quit")
                break;
            conversationHistory += u8"用户: " + userInput + "\n";
            std::string reply = callLargeModel(userInput, 128, 0.7);
            conversationHistory += u8"模型: " + reply + "\n";
            std::cout << u8"模型: " << reply << std::endl;

            // 当用户输入中包含 "draw" 时触发绘图
            if (userInput.find("draw") != std::string::npos) {
                std::string drawingPrompt = transformDrawingPrompt(conversationHistory);
                std::cout << u8"转换后的绘图提示词: " << drawingPrompt << std::endl;
                if (callSdWebuiDraw(drawingPrompt))
                    conversationHistory += "系统: 绘图成功，图片已生成。\n";
                else
                    conversationHistory += "系统: 绘图出现错误。\n";
            }
        }
    }
    else {
        std::cout << u8"无效的模式，程序退出。" << std::endl;
    }

    return 0;
}
