
#define _WIN32_WINNT 0x0601  // 确保 Winsock2 支持

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#define CPPHTTPLIB_USE_POLL  // 启用 poll 支持

#include <iostream>
#include "httplib.h"
#include <windows.h>
#include <mutex>
#include <thread>  
#include "ECanVci.h"
#include <algorithm>  
#include "json.hpp"
using namespace httplib;
using json = nlohmann::json;
// 设备配置
const DWORD DEV_TYPE = 4;   // USBCAN-II 
const DWORD DEV_INDEX = 0;  // 设备索引
const DWORD CAN_INDEX = 0;  // CAN通道号
const DWORD CAN_INDEX2 = 1;  // CAN通道号
std::mutex canMutex;  // CAN设备操作互斥锁

// 初始化CAN设备
bool initDevice() {
    std::lock_guard<std::mutex> lock(canMutex);
    CloseDevice(DEV_TYPE, DEV_INDEX);
    if (OpenDevice(DEV_TYPE, DEV_INDEX, 0) != 1) {
        std::cerr << u8"打开设备失败! 错误码: " << GetLastError() << std::endl;
        return false;
    }

    INIT_CONFIG initConfig;
    initConfig.AccCode = 0;
    initConfig.AccMask = 0xFFFFFFFF;
    initConfig.Reserved = 0;
    initConfig.Filter = 0;
    initConfig.Timing0 = 0x00;  // 500Kbps
    initConfig.Timing1 = 0x1C;
    initConfig.Mode = 0;

    P_INIT_CONFIG pInitConfig = &initConfig;  
    if (InitCAN(DEV_TYPE, DEV_INDEX, CAN_INDEX,pInitConfig) != 1) {
        std::cerr << "初始化通道失败!" << std::endl;
        CloseDevice(DEV_TYPE, DEV_INDEX);
        return false;
    }

    if (StartCAN(DEV_TYPE, DEV_INDEX, CAN_INDEX) != 1) {
        std::cerr << "启动通道失败!" << std::endl;
        CloseDevice(DEV_TYPE, DEV_INDEX);
        return false;
    }

    std::cout << "CAN1设备初始化成功" << std::endl;
    return true;
}

bool initDevice2() {
    std::lock_guard<std::mutex> lock(canMutex);

    INIT_CONFIG initConfig;
    initConfig.AccCode = 0;
    initConfig.AccMask = 0xFFFFFFFF;
    initConfig.Reserved = 0;
    initConfig.Filter = 0;
    initConfig.Timing0 = 0x00;  // 500Kbps
    initConfig.Timing1 = 0x1C;
    initConfig.Mode = 0;

    P_INIT_CONFIG pInitConfig = &initConfig; 
    if (InitCAN(DEV_TYPE, DEV_INDEX, CAN_INDEX2,pInitConfig) != 1) {
        std::cerr << "初始化通道失败!" << std::endl;
        CloseDevice(DEV_TYPE, DEV_INDEX);
        return false;
    }

    if (StartCAN(DEV_TYPE, DEV_INDEX, CAN_INDEX2) != 1) {
        std::cerr << "启动通道失败!" << std::endl;
        CloseDevice(DEV_TYPE, DEV_INDEX);
        return false;
    }

    std::cout << "CAN2设备初始化成功" << std::endl;
    return true;
}

bool sendCAN( CAN_OBJ& frame) {

  

    std::lock_guard<std::mutex> lock(canMutex);
    P_CAN_OBJ pFrame = &frame;  
    if (Transmit(DEV_TYPE, DEV_INDEX, CAN_INDEX, pFrame, 1) == 1) {
        ERR_INFO errInfo;
        if (ReadErrInfo(DEV_TYPE, DEV_INDEX, CAN_INDEX, &errInfo) == 1) {
         
            std::cout << "成功发送 CAN 数据: ID = " << std::hex << frame.ID << std::endl;
        }
        else{
            std::cout << "失败发送 CAN 数据: ID = " << std::hex << frame.ID << std::endl;
            return false;
   
        }
       
    }
    return true;
}

bool receiveCAN(CAN_OBJ& frame) {
    std::lock_guard<std::mutex> lock(canMutex);
    P_CAN_OBJ pFrame = &frame; 
    ULONG received = Receive(DEV_TYPE, DEV_INDEX, CAN_INDEX, pFrame, 1, 0);
    if (received == (ULONG)-1) {
        ERR_INFO errInfo;
        if (ReadErrInfo(DEV_TYPE, DEV_INDEX, CAN_INDEX, &errInfo) == 1) {
            std::cerr << "接收失败! 错误码: 0x" << std::hex << errInfo.ErrCode << std::endl;
        }
        else
        {
            std::cout << "成功接收 CAN 数据: ID = " << std::hex << frame.ID << std::endl;
            std::cout << "成功接收 CAN 数据: DATE = " << std::hex << frame.Data << std::endl;
        }

    
        return false;
    }
    return received > 0;
}


// void continuousSend() {
//     CAN_OBJ frame ;


//     while (true) {
        
      
//         if (receiveCAN(frame)!=1) {
//             ERR_INFO errInfo;
//             if (ReadErrInfo(DEV_TYPE, DEV_INDEX, 1, &errInfo) == 1) {
//                 std::cerr << "接收失败! 错误码: 0x" << std::hex << errInfo.ErrCode << std::endl;
//             }

//         } else {
    
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 每 500ms 发送一次
//     }
// }


void continuousSend() {
    CAN_OBJ frame = {0};
    frame.ID = 0x012;   // 设置CAN ID
    frame.DataLen = 7;   // 设置数据长度
    frame.SendType = 0;  // 设置为正常发送
    frame.RemoteFlag = 0; // 数据帧
    frame.ExternFlag = 0; // 标准帧

    uint8_t counter = 0;
    while (true) {
        BYTE data[8];
       
            data[0] =0x82 ; 
            data[1] =0xdb ; 
            data[2] =0x00 ; 
            data[3] =0x00 ; 
            data[4] =0x00 ; 
            data[5] =0x00 ; 
            data[6] =0x00 ; 
            data[7] =0x00 ; 
    

     
        memcpy(frame.Data, data, frame.DataLen);

        std::cout << "准备发送 CAN 数据: ID = " << std::hex << frame.ID << " 数据: ";
        for (int i = 0; i < frame.DataLen; ++i) {
            std::cout << std::hex << (int)frame.Data[i] << " ";
        }
        std::cout << std::endl;

        // 发送数据
        if (Transmit(DEV_TYPE, DEV_INDEX, CAN_INDEX2, &frame, 1) != 1) {
            ERR_INFO errInfo;
            if (ReadErrInfo(DEV_TYPE, DEV_INDEX, 0, &errInfo) == 1) {
                std::cerr << "接收失败! 错误码: 0x" << std::hex << errInfo.ErrCode << std::endl;
            }
            if (ReadErrInfo(DEV_TYPE, DEV_INDEX, 1, &errInfo) == 1) {
                std::cerr << "发送失败! 错误码: 0x" << std::hex << errInfo.ErrCode << std::endl;
            }
        } else {
            std::cout << "成功发送 CAN 数据: ID = " << std::hex << frame.ID << std::endl;
        }

    
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 每 500ms 发送一次
    }
}


int main() {
    if (!initDevice()) return EXIT_FAILURE;
    if (!initDevice2()) return EXIT_FAILURE; 
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // std::thread sendThread(continuousSend);
    // sendThread.detach();  
    Server server;

    // 设置CORS头
    httplib::Headers headers;
    headers.emplace("Access-Control-Allow-Origin", "*");
    headers.emplace("Access-Control-Allow-Methods", "GET, PUT, OPTIONS");
    headers.emplace("Access-Control-Allow-Headers", "Content-Type");
    server.set_default_headers(headers);
  
    

    server.Put("/send_can", [](const Request& req, Response& res) {
        // 设置 CORS 头部，确保只设置一个 'Access-Control-Allow-Origin'
        res.set_header("Access-Control-Allow-Origin", "*");  // 只允许一个通配符
        res.set_header("Access-Control-Allow-Methods", "PUT, GET, OPTIONS");  // 允许的 HTTP 方法
        res.set_header("Access-Control-Allow-Headers", "Content-Type");  // 允许的请求头部

        std::cout << "Received put request at /send_can" << std::endl;  // 添加调试日志
    
     
        
        try {
            json request_body = json::parse(req.body);
            uint32_t can_id = request_body["id"].get<uint32_t>();
            std::vector<uint8_t> data = request_body["data"].get<std::vector<uint8_t>>();
    
            std::cout << "Received id: " << std::hex << can_id << std::endl;
            std::cout << "Received data: ";
            for (size_t i = 0; i < data.size(); ++i) {
                std::cout << std::hex << (int)data[i] << " ";
            }
            std::cout << std::endl;
    


            CAN_OBJ frame = {0};
            frame.SendType = 0;  // 设置为正常发送
            frame.RemoteFlag = 0; // 数据帧
            frame.ExternFlag = 0; // 标准帧
            frame.ID = can_id;
            frame.DataLen = data.size();
            memcpy(frame.Data, data.data(), frame.DataLen);

            json response;
            if (sendCAN(frame)) {
                response = {{"success", true}};
                res.set_content(response.dump(), "application/json");
                std::cout << "Received put request at /send_can" << std::endl;  // 添加调试日志
            } else {
                response = {{"success", false}};
                res.status = 500;
                res.set_content(response.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            json response = {{"error", e.what()}};
            res.status = 400;
            res.set_content(response.dump(), "application/json");
        }
    });
    
    // 处理 CAN 接收请求
    server.Get("/receive_can", [](const Request&, Response& res) {

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, PUT, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        CAN_OBJ frame;
        json response;
        // std::cout << "Received GET request at /receive_can" << std::endl;
        // 调用 receiveCAN 接收数据
        if (receiveCAN(frame)) {
     
            size_t data_length = sizeof(frame.Data) / sizeof(frame.Data[0]);  // 获取 Data 数组的实际长度
    
            // 处理数据
            response["id"] = frame.ID;
            response["data"] = std::vector<uint8_t>(frame.Data, frame.Data + data_length); // 使用实际的数据长度
        } else {
        
            response["id"] = 0;
            response["data"] = {};
        }
    
        // 返回 JSON 格式的响应
        res.set_content(response.dump(), "application/json");
    });
    
    // 启动服务器
    std::cout << "Starting server on http://localhost:8080..." << std::endl;


    server.listen("localhost", 8080); 
    std::cout << "Server is running at http://localhost:8080" << std::endl;

    // 关闭 CAN 设备连接
    CloseDevice(DEV_TYPE, DEV_INDEX);
    return EXIT_SUCCESS;
}
