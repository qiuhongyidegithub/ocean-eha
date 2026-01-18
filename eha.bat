@echo off

chcp 65001


echo 正在打开网页 index.html...
start "" "http://localhost:8080/"

echo 正在运行 my_project.exe...
echo 等待三秒钟等待设备启动
my_project.exe




pause