#include <windows.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include "httplib.h"
#include "json.hpp"

using namespace httplib;
using json = nlohmann::json;

// 配置区
static const char* RS485_PORT = "\\\\.\\COM10"; // 你的串口号（COM10 及以上必须这样写）
static const DWORD BAUD_RATE = CBR_38400;        // 波特率
static const int DEFAULT_RX_TIMEOUT_MS = 300;    // 默认等待回包超时

// 串口句柄
static HANDLE g_hSerial = INVALID_HANDLE_VALUE;

// 互斥锁（保护串口读写）
static std::mutex g_rs485Mutex;

// 打印十六进制
static void printHex(const std::vector<uint8_t>& data, const std::string& prefix = "") {
    if (!prefix.empty()) std::cout << prefix;
    for (auto b : data) {
        std::cout << std::hex << std::uppercase << (int)b << " ";
    }
    std::cout << std::dec << std::endl;
}

// 初始化 RS485 串口
static HANDLE initRS485Device() {
    std::lock_guard<std::mutex> lock(g_rs485Mutex);

    HANDLE hSerial = CreateFileA(
        RS485_PORT,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "[RS485] 打开串口失败! err=" << GetLastError() << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    // 配置串口参数
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);

    if (!GetCommState(hSerial, &dcb)) {
        std::cerr << "[RS485] GetCommState 失败! err=" << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    dcb.BaudRate = BAUD_RATE;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    // dcb.Parity = EVENPARITY;  // 设置偶校验

    if (!SetCommState(hSerial, &dcb)) {
        std::cerr << "[RS485] SetCommState 失败! err=" << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    std::cout << "[RS485] 串口初始化成功: " << RS485_PORT << std::endl;
    return hSerial;
}

// 清空串口缓冲（避免读到上一包残留）
static void purgeSerial(HANDLE hSerial) {
    std::lock_guard<std::mutex> lock(g_rs485Mutex);
    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

// 发送 RS485 数据
static bool sendRS485(HANDLE hSerial, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(g_rs485Mutex);

    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(hSerial, data.data(), (DWORD)data.size(), &bytesWritten, NULL);

    if (!ok) {
        std::cerr << "[RS485] 发送失败! err=" << GetLastError() << std::endl;
        return false;
    }

    if (bytesWritten != data.size()) {
        std::cerr << "[RS485] 发送字节数不一致! written=" << bytesWritten
                  << " expected=" << data.size() << std::endl;
        return false;
    }

    return true;
}

// 读取 RS485 数据
static bool receiveRS485(HANDLE hSerial, std::vector<uint8_t>& out, int timeout_ms = DEFAULT_RX_TIMEOUT_MS) {
    std::lock_guard<std::mutex> lock(g_rs485Mutex);

    out.clear();
    DWORD start = GetTickCount();

    // 先寻找帧头 'AA'
    uint8_t frame_start = 0xAA;

    while ((int)(GetTickCount() - start) < timeout_ms) {
        DWORD bytesRead = 0;
        uint8_t buf[256];

        BOOL ok = ReadFile(hSerial, buf, sizeof(buf), &bytesRead, NULL);
        if (!ok) {
            std::cerr << "[RS485] 接收失败! err=" << GetLastError() << std::endl;
            return false;
        }

        // 打印接收到的原始数据
        std::cout << "[RS485] 接收到的原始数据: ";
        for (DWORD i = 0; i < bytesRead; ++i) {
            std::cout << "0x" << std::hex << (int)buf[i] << " ";
        }
        std::cout << std::dec << std::endl;

        for (DWORD i = 0; i < bytesRead; ++i) {
            // 如果找到帧头 'AA'
            if (buf[i] == frame_start) {
                out.push_back(buf[i]);

                // 读取数据长度
                if (i + 1 < bytesRead) {
                    uint8_t length = buf[i + 1];  // 数据长度
                    out.push_back(buf[i + 1]);  // 添加长度字段
                    std::cout << "[RS485] 数据长度: " << (int)length << std::endl;

                    // 读取 ID + 数据部分 + '00 55'
                    if (i + 1 + length <= bytesRead) {
                        // 读取 ID（2 字节）
                        out.push_back(buf[i + 2]);  // ID 第一个字节
                        out.push_back(buf[i + 3]);  // ID 第二个字节

                        // 打印解析出的 ID 部分
                        std::cout << "[RS485] ID: 0x" << std::hex << (int)buf[i + 2] << (int)buf[i + 3] << std::endl;

                        // 读取数据部分（2 字节）
                        out.push_back(buf[i + 4]);  // 数据第一个字节
                        out.push_back(buf[i + 5]);  // 数据第二个字节

                        // 打印数据部分
                        std::cout << "[RS485] 数据部分: 0x" << std::hex << (int)out[out.size() - 2] << " 0x" << (int)out[out.size() - 1] << std::dec << std::endl;

                        // 检查帧尾是否以 '00 55' 结尾
                        if (buf[i + 6] == 0x00 && buf[i + 7] == 0x55) {
                            std::cout << "[RS485] 数据帧以 '00 55' 结尾，接收成功！" << std::endl;
                            return true;
                        } else {
                            std::cout << "[RS485] 数据帧未以 '00 55' 结尾，丢弃数据。" << std::endl;
                        }
                    }
                }
            }
        }
        Sleep(10);
    }

    std::cout << "[RS485] 超时未接收到完整数据帧。" << std::endl;
    return false; // 超时没数据
}





int main() {
    g_hSerial = initRS485Device();
    if (g_hSerial == INVALID_HANDLE_VALUE) {
        return EXIT_FAILURE;
    }

    Server server;

    // 全局 CORS 设置
    httplib::Headers headers;
    headers.emplace("Access-Control-Allow-Origin", "*");
    headers.emplace("Access-Control-Allow-Methods", "GET, PUT, POST, OPTIONS");
    headers.emplace("Access-Control-Allow-Headers", "Content-Type");
    server.set_default_headers(headers);

    // ======= 接收 RS485 数据的路由 =======
    server.Get("/receive_rs485", [](const Request&, Response& res) {
        std::vector<uint8_t> rxData;
        bool success = receiveRS485(g_hSerial, rxData);

        json resp;
        resp["success"] = success;
        resp["data"] = rxData;

        if (success) {
            res.set_content(resp.dump(), "application/json");
        } else {
            res.status = 500;
            resp["error"] = "Failed to receive valid data";
            res.set_content(resp.dump(), "application/json");
        }
    });

    // 启动 HTTP 服务器
    std::cout << "Starting server on http://localhost:8080..." << std::endl;
    server.listen("localhost", 8080);

    // 关闭串口
    CloseHandle(g_hSerial);
    return EXIT_SUCCESS;
}
