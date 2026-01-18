#include <winsock2.h>
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
static const int DEFAULT_RX_TIMEOUT_MS =100;    // 默认等待回包超时

// 串口句柄
static HANDLE g_hSerial = INVALID_HANDLE_VALUE;

// 互斥锁（保护串口读写）
static std::mutex g_rs485Mutex;

// 条件变量和标志位，用于线程同步
static std::condition_variable g_cv;
static bool g_dataReady = false;

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

    // 启用软件流控制
    dcb.BaudRate = BAUD_RATE;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = EVENPARITY;  // 设置偶校验

    if (!SetCommState(hSerial, &dcb)) {
        std::cerr << "[RS485] SetCommState 失败! err=" << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    std::cout << "[RS485] 串口初始化成功: " << RS485_PORT << std::endl;
    return hSerial;
}

//
static void purgeSerial(HANDLE hSerial) {
    std::lock_guard<std::mutex> lock(g_rs485Mutex);
    PurgeComm(hSerial, PURGE_TXCLEAR);  // 仅清除发送缓冲区
}

// 发送 RS485 数据的函数 (逻辑保持不变，依赖上面的 purgeSerial 修复)
static bool sendRS485(HANDLE hSerial, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(g_rs485Mutex);

    // 每次发送数据前清空串口缓冲区 (现在只会清空TX，不会误删RX了)
    PurgeComm(hSerial, PURGE_TXCLEAR);

    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(hSerial, data.data(), (DWORD)data.size(), &bytesWritten, NULL);

    if (!ok) {
        DWORD error = GetLastError();
        std::cerr << "[RS485] 发送失败! err=" << error << std::endl;
        return false;
    }

    if (bytesWritten != data.size()) {
        std::cerr << "[RS485] 发送字节数不一致! written=" << bytesWritten
                  << " expected=" << data.size() << std::endl;
        return false;
    }
    FlushFileBuffers(hSerial);

    // 打印发送的数据 (保留原打印)
    std::cout << "[RS485] 发送的数据: ";
    for (auto byte : data) {
        std::cout << "0x" << std::hex << (int)byte << " ";
    }
    std::cout << std::dec << std::endl;

    return true;
}

// 读取 RS485 数据 (保留了你的全部打印逻辑，只去掉了有害的 purgeSerial)
static bool receiveRS485(HANDLE hSerial, std::vector<uint8_t>& out, int timeout_ms = DEFAULT_RX_TIMEOUT_MS) {
    std::lock_guard<std::mutex> lock(g_rs485Mutex);

    out.clear();
    DWORD start = GetTickCount();
    uint8_t cishu=0; // 稍微改大一点点buf防止溢出，不影响逻辑

    // 先寻找帧头 'AA'
    uint8_t frame_start = 0xAA;
      uint8_t frame_start2 = 0xA1;

    while ((int)(GetTickCount() - start) < timeout_ms) {
        DWORD bytesRead = 0;
        uint8_t buf[40]; // 稍微改大一点点buf防止溢出，不影响逻辑

        BOOL ok = ReadFile(hSerial, buf, sizeof(buf), &bytesRead, NULL);
        if (!ok) {
            // 读失败不一定报错，可能是超时，这里保留你的原逻辑
            // std::cerr << "[RS485] 接收失败! err=" << GetLastError() << std::endl;
            return false;
        }

        if (bytesRead > 0) {
            // 打印接收到的原始数据 (保留原打印)
            std::cout << "[RS485] 接收到的原始数据: ";
            for (DWORD i = 0; i < bytesRead; ++i) {
                std::cout << "0x" << std::hex << (int)buf[i] << " ";
            }
            std::cout << std::dec << std::endl;

            for (DWORD i = 0; i < bytesRead; ++i) {
                // 如果找到帧头 'AA'
                if (buf[i] == frame_start&&cishu==0)
                {
                        cishu=1;
                        continue;
                }
                if (buf[i] == frame_start&& cishu==1) {
                    
                    // 读取数据长度
                    if (i + 1 < bytesRead) {
                        uint8_t length = buf[i + 1];
                        
                        // 确保这一帧数据在缓冲区里是完整的
                        // 如果数据还没收全，这里会跳过，等待下一次循环读全
                        if (i + 2 + length + 1 < bytesRead) {
                            
                            out.push_back(buf[i]);         // AA
                            out.push_back(buf[i + 1]);     // Length
                            std::cout << "[RS485] 数据长度: " << (int)length << std::endl;

                            // 获取 ID（两个字节）
                            uint16_t id = (buf[i + 2] << 8) | buf[i + 3];
                            out.push_back(buf[i + 2]);
                            out.push_back(buf[i + 3]);

                            // 获取数据部分
                            // 注意：buf + i + 4 是起点，长度是 length - 2 (减去ID的2字节)
                            std::vector<uint8_t> data(buf + i + 4, buf + i + 4 + length - 2);
                            out.insert(out.end(), data.begin(), data.end());

                            // 打印解析出的 ID 和数据部分 (保留原打印)
                            std::cout << "[RS485] ID: 0x" << std::hex << id << std::dec << std::endl;
                            std::cout << "[RS485] 数据部分: ";
                            for (size_t j = 0; j < data.size(); ++j) {
                                std::cout << "0x" << std::hex << (int)data[j] << " ";
                            }
                            std::cout << std::dec << std::endl;

                            // 检查是否以 '00 55' 结尾
                            if (buf[i + 2 + length ] == 0x00 && buf[i + 2 + length + 1] == 0x55) {
                                std::cout << "[RS485] 数据帧以 '00 55' 结尾，接收成功！" << std::endl;
                                
                                // 【关键修复】接收成功后，不要清空缓冲区！
                                // purgeSerial(hSerial);  <--- 这一行被删掉了
                                  return true;
                            } else {
                                std::cout << "[RS485] 数据帧未以 '00 55' 结尾，丢弃数据。" << std::endl;
                                
                                // 【关键修复】校验失败也不要清空，可能下一包数据紧贴着
                                // purgeSerial(hSerial);  <--- 这一行被删掉了
                       
                            }
                        }
                    }
                }
            }
               
 
        }
        // 如果没有收到数据，可以短暂休息一下，避免CPU 100%

    }

    // std::cout << "[RS485] 超时未接收到完整数据帧。" << std::endl;
    return false; // 超时没数据
}

// 每秒发送一次数据
static void sendDataPeriodically() {
    std::vector<uint8_t> testData = {0xAA, 0x04, 0x00, 0x11, 0x01, 0x00, 0x00, 0x55};  // 示例数据
    auto last_time = std::chrono::steady_clock::now();  // 获取当前时间

    // 每隔 1 秒发送一次数据
    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_time = current_time - last_time;

        // 判断是否已过去 1 秒
        if (elapsed_time.count() >= 1.0) {
            if (sendRS485(g_hSerial, testData)) {
                std::cout << "发送成功！" << std::endl;
            } else {
                std::cerr << "发送失败！" << std::endl;
            }

            last_time = current_time; // 更新时间戳，开始新的一秒
        }
    }
}

void timedReceiveRS485() {
    std::vector<uint8_t> rxData;
    auto lastReceiveTime = std::chrono::steady_clock::now();  // 上次接收时间
    const std::chrono::milliseconds receiveInterval(20);      // 每 20 毫秒接收一次

    while (true) {
        auto currentTime = std::chrono::steady_clock::now(); // 当前时间
        std::chrono::milliseconds timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastReceiveTime); // 时间差

        // 如果时间差大于等于设定的接收间隔（20ms），进行数据接收
        if (timeElapsed >= receiveInterval) {
            bool success = receiveRS485(g_hSerial, rxData, 20);  // 每 20 毫秒接收一次

            if (success) {
                std::cout << "接收到数据: ";
                for (auto byte : rxData) {
                    std::cout << "0x" << std::hex << (int)byte << " ";
                }
                std::cout << std::dec << std::endl;  // 打印接收到的数据
            } else {
            }

            lastReceiveTime = currentTime;  // 更新上次接收时间
        }
    }
}

// 启动 HTTP 服务器
int main() {
    g_hSerial = initRS485Device();
    if (g_hSerial == INVALID_HANDLE_VALUE) {
        return EXIT_FAILURE;
    }

    Server server;

    // 启动发送和接收线程
    // std::thread sendThread(sendDataPeriodically);
    // std::thread receiveThread(timedReceiveRS485);

    // sendThread.detach();  // 分离发送线程
    // receiveThread.detach();  // 分离接收线程

    // 设置 CORS 头
    httplib::Headers headers;
    headers.emplace("Access-Control-Allow-Origin", "*");
    headers.emplace("Access-Control-Allow-Methods", "GET, PUT, POST, OPTIONS");
    headers.emplace("Access-Control-Allow-Headers", "Content-Type");
    server.set_default_headers(headers);

    // 接收 RS485 数据的路由
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

    // 发送 RS485 数据的路由
    server.Put("/send_rs485", [](const Request& req, Response& res) {
        try {
            // 获取请求体数据
            auto body = req.body;
            json j = json::parse(body);

            // 处理 ID 的转换
            uint16_t id = static_cast<uint8_t>(j["id"][0].get<int>()) << 8 | static_cast<uint8_t>(j["id"][1].get<int>());

            // 获取数据部分
            std::vector<uint8_t> data = j["data"].get<std::vector<uint8_t>>();

            // 打印接收到的数据
            for (auto byte : data) {
                std::cout << "0x" << std::hex << (int)byte << " ";
            }
            std::cout << std::dec << std::endl;

            // 清空串口缓冲区，避免旧数据影响
            purgeSerial(g_hSerial);

            // 调用发送函数将数据发送到串口三次
            for (int i = 0; i < 4; ++i) {
                bool success = sendRS485(g_hSerial, data);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
       
 
                if (!success) {
                    std::cerr << "[RS485] 发送失败!" << std::endl;
                    break;
                }
            }

            // 返回响应
            res.set_content("{\"status\": \"success\"}", "application/json");
        } catch (const std::exception& e) {
            std::cerr << "[RS485] 处理请求时出错: " << e.what() << std::endl;
            res.status = 400;
            res.set_content("{\"error\": \"Invalid request\"}", "application/json");
        }
    });

    // 启动 HTTP 服务器
    std::cout << "Starting server on http://localhost:8080..." << std::endl;
    server.listen("localhost", 8080);

    // 关闭串口
    CloseHandle(g_hSerial);
    return EXIT_SUCCESS;
}
