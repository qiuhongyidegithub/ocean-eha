@echo off

chcp 65001
echo 等待5秒钟等待服务器开启
start "" cmd /c "chcp 65001 && http-server ./ -p 8080 --cors"

timeout /t 5 /nobreak

echo 正在打开网页 index.html...
start "" "http://localhost:8080/"

echo 正在运行 SerialPort_server.exe...
echo 等待三秒钟等待设备启动
start "" cmd /c "chcp 65001 && SerialPort_server.exe"

timeout /t 5 /nobreak
taskkill /f /im node.exe

pause
