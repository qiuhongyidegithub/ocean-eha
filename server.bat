@echo off

chcp 65001
echo 等待5秒钟等待服务器开启
http-server ./ -p 8080 --cors

pause
