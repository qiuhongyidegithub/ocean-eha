


$(function () {
    // 时间数据数组和数据更新函数
    let timeData = []; // 时间数据，使用当前分钟和秒格式
    let exportTimeData = []; // 时间数据，使用当前分钟和秒格式
    let expectedSpeedData = [];
    let nowSpeedData = [];
    let expectedDistanceData = [];
    let nowDistanceData = [];
    let expectedPressureData = [];
    let nowPressureData = [];

    let expectedtorqueData = [];
    let expectAccelerationData = [];
    let ActualcurrentData = [];
    let ActualtorqueData = [];
    let ActualpowerData = [];
    // let Actualwendu = [];
    
    let ActualRotationalspeedData = [];
    let timeStamps   = [];

    function getCurrentTime() {
        const now = new Date();
        const minutes = now.getMinutes();
        const seconds = now.getSeconds();
        return `${minutes}.${seconds < 10 ? '0' + seconds : seconds}`;
    }


function getCurrentTimeMs() {
  const now = new Date();
  const yyyy = now.getFullYear();
  const MM = String(now.getMonth() + 1).padStart(2, '0'); // 月份从 0 开始
  const dd = String(now.getDate()).padStart(2, '0');
  const hh = String(now.getHours()).padStart(2, '0');
  const mm = String(now.getMinutes()).padStart(2, '0');
  const ss = String(now.getSeconds()).padStart(2, '0');
  const ms = String(now.getMilliseconds()).padStart(3, '0');

  return `${yyyy}-${MM}-${dd} ${hh}:${mm}:${ss}.${ms}`;
}


    

    


    function calculateFontSize(number) {
        const width = window.innerWidth;
        const fontSize = width * number; // 字体大小为屏幕宽度的 2%
        return fontSize;
    }


    function updateChartData() {
        // 获取当前时间
          const now = Date.now();
        timeData.push(getCurrentTime());
        exportTimeData.push(getCurrentTimeMs());
        timeStamps.push(now);

        // 模拟更新数据 (您可以替换成实际数据更新逻辑)
        expectedSpeedData.push(variables.expectedSpeed);
        nowSpeedData.push(variables.ActualRotationalspeed);
        expectedDistanceData.push(variables.expectedDistance);
        nowDistanceData.push(variables.nowexpectedDistance);
        expectedPressureData.push(variables.expectedPressure);
        nowPressureData.push(variables.nowexpectedPressure);

        expectedtorqueData.push(variables.expectedtorque);
        expectAccelerationData.push(variables.expectAcceleration);
        ActualcurrentData.push(variables.Actualcurrent);
        ActualtorqueData.push(variables.Actualtorque);
        ActualpowerData.push(variables.Actualpower);
        // Actualwendu.push(variables.Actualwendu);
        ActualRotationalspeedData.push(variables.ActualRotationalspeed);



    


        // 如果数据长度超过 1800（即 30 分钟的数据），则移除最早的一个数据点
        if (timeData.length > 100000000) {
            timeData.shift();
            expectedSpeedData.shift();
            nowSpeedData.shift();
            expectedDistanceData.shift();
            nowDistanceData.shift();
            expectedPressureData.shift();
            nowPressureData.shift();

            expectedtorqueData.shift();
            ActualtorqueData.shift();
            expectAccelerationData.shift();
            ActualRotationalspeedData.shift();
            ActualcurrentData.shift();
          
            ActualpowerData.shift();
            // Actualwendu.shift();
        
        }
    }


    

    let colors = ['#2cec0b', '#ece10b', '#AAF487', '#47D8BE', '#ec0bca', '#DE801C'];
    
    let colors2 = ['#2cec0b', '#ece10b', '#AAF487', '#47D8BE', '#ec0bca', '#DE801C'];

    

    // 初始化图表
    function initChart() {


  
        
        var myChart = echarts.init(document.getElementById('chart1'));

        var option1 = {
            tooltip: {
                trigger: 'axis'
            },
            legend: {
                data: [
                    // { name: '设定速度', textStyle: { color: colors[0], fontWeight: 'bold' } },
                    // { name: '实际速度', textStyle: { color: colors[1], fontWeight: 'bold' } },
                    { name: '设定行程', textStyle: { color: colors[2], fontWeight: 'bold' } },
                    { name: '实际行程', textStyle: { color: colors[3], fontWeight: 'bold' } }
                    // { name: '设定压力', textStyle: { color: colors[4], fontWeight: 'bold' } },
                    // { name: '实际压力', textStyle: { color: colors[5], fontWeight: 'bold' } }
                ],
                textStyle: {
                    color: '#fff',
                   fontSize: rootFontSize * 5    // 1rem = rootFontSize px
                },
                itemWidth: calculateFontSize(0.004),
                itemHeight: calculateFontSize(0.004),
            },
            grid: {
                left: '2%',
                right: '10%',
                bottom: '3%',
                top: '13%',
                containLabel: true
            },
            xAxis: {
                type: 'category',
                data: timeData,
                axisLabel: {
                    formatter: '{value}',
                    textStyle: {
                        color: "#ebf8ac"
                    }
                },
                axisLine: {
                    lineStyle: {
                        color: '#01FCE3'
                    }
                },
                // 开启 x 轴的缩放
            },
            yAxis: {
                type: 'value',
                axisLabel: {
                    textStyle: {
                        color: "#2EC7C9"
                    }
                },
                axisLine: {
                    lineStyle: {
                        color: '#01FCE3'
                    }
                },
                // 开启 y 轴的缩放
                dataZoom: [{
                    type: 'inside', // 启用滚轮缩放
                    yAxisIndex: [1], // 仅影响 x 轴
                    start: 0, // 默认缩放起点
                    end: 100  ,// 默认缩放终点
                    zoomOnMouseWheel: true // 启用滚轮缩放
                }]
            },
            series: [
                // {
                //     name: '设定速度',
                //     type: 'line',
                //     data: expectedSpeedData,
                //     smooth: true,
                //     lineStyle: { color: '#2cec0b7B', width: 5 },
                // },
                // {
                //     name: '实际速度',
                //     type: 'line',
                //     data: nowSpeedData,
                //     smooth: true,
                //     lineStyle: { color: '#ece10b7B', width: 5 },
                // },
                {
                    name: '设定行程',
                    type: 'line',
                    data: expectedDistanceData,
                    smooth: true,
                    lineStyle: { color: '#AAF4877B', width: 2 },
                },
                {
                    name: '实际行程',
                    type: 'line',
                    data: nowDistanceData,
                    smooth: true,
                    lineStyle: { color: '#47D8BE7B', width: 2 },
                }
                // {
                //     name: '设定压力',
                //     type: 'line',
                //     data: expectedPressureData,
                //     smooth: true,
                //     lineStyle: { color: '#ec0bca7B', width: 5 },
                // },
                // {
                //     name: '实际压力',
                //     type: 'line',
                //     data: nowPressureData,
                //     smooth: true,
                //     lineStyle: { color: '#DE801C7B', width: 5 },
                // }
            ],
            // 区域缩放
            toolbox: {
                orient: 'vertical',
                right: 10,
                bottom: 10,
                feature: {
                    dataZoom: [
                        {
                            yAxisIndex: 'none' // 启用 y 轴缩放
                        },
                        {
                            xAxisIndex: 'none' // 启用 x 轴缩放
                        }
                    ],
                    restore: {}
                }
            }

        };

        var myChart2 = echarts.init(document.getElementById('chart2'));

        var option2 = {
            tooltip: {
                trigger: 'axis'
            },
            legend: {
                data: [
                    // { name: '设定扭矩', textStyle: { color: colors[0], fontWeight: 'bold' } },
                    { name: '实际扭矩', textStyle: { color: colors[1], fontWeight: 'bold' } },
                    // { name: '最大转矩', textStyle: { color: colors[2], fontWeight: 'bold' } },
                    { name: '实际转速', textStyle: { color: colors[3], fontWeight: 'bold' } },
                   { name: '电机温度', textStyle: { color: colors[4], fontWeight: 'bold' } }
                    // { name: '实际电流', textStyle: { color: colors[4], fontWeight: 'bold' } },
                    // { name: '实际电压', textStyle: { color: colors[5], fontWeight: 'bold' } }
                ],
                textStyle: {
                    color: '#fff',
                       fontSize: rootFontSize * 4.5   
                },
                itemWidth: calculateFontSize(0.004),
                itemHeight: calculateFontSize(0.004),
            },
            grid: {
                left: '3%',
                right: '10%',
                bottom: '3%',
                top: '13%',
                containLabel: true
            },
            xAxis: {
                type: 'category',
                data: timeData,
                axisLabel: {
                    formatter: '{value}',
                    textStyle: {
                        color: "#ebf8ac"
                    }
                },
                axisLine: {
                    lineStyle: {
                        color: '#01FCE3'
                    }
                },
                dataZoom: [{
                    type: 'inside',  // 使用滚轮缩放
                    xAxisIndex: 0,  // 同时控制 x 轴
                    yAxisIndex: 0,  // 同时控制 y 轴
                    start: 0,       // 默认缩放范围
                    end: 100,
                    zoomOnMouseWheel: true, // 启用滚轮缩放
                }]
  
            },
            yAxis: {
                type: 'value',
                axisLabel: {
                    textStyle: {
                        color: "#2EC7C9"
                    }
                },
                axisLine: {
                    lineStyle: {
                        color: '#01FCE3'
                    }
                },
                dataZoom: [{
                    type: 'inside',  // 使用滚轮缩放
                    xAxisIndex: 0,  // 同时控制 x 轴
                    yAxisIndex: 0,  // 同时控制 y 轴
                    start: 0,       // 默认缩放范围
                    end: 100,
                    zoomOnMouseWheel: true, // 启用滚轮缩放
                }]
            },
  
    
          
            series: [
                // {
                //     name: '设定扭矩',
                //     type: 'line',
                //     data: expectedtorqueData,
                //     smooth: true,
                //     lineStyle: { color: '#2cec0b7B', width: 5 },
                // },
                {
                    name: '实际扭矩',
                    type: 'line',
                    data: ActualtorqueData,
                    smooth: true,
                    lineStyle: { color: '#ece10b7B', width: 2 },
                },
                // {
                //     name: '最大转矩',
                //     type: 'line',
                //     data: expectAccelerationData,
                //     smooth: true,
                //     lineStyle: { color: '#AAF4877B', width: 5 },
                // },
                {
                    name: '实际转速',
                    type: 'line',
                    data: ActualRotationalspeedData,
                    smooth: true,
                    lineStyle: { color: '#47D8BE7B', width: 2 },
                }
                // {
                //     name: '实际电流',
                //     type: 'line',
                //     data: ActualcurrentData,
                //     smooth: true,
                //     lineStyle: { color: '#ec0bca7B', width: 5 },
                // },
                // {
                //     name: '电机温度',
                //     type: 'line',
                //     data: Actualwendu,
                //     smooth: true,
                //     lineStyle: { color: '#ec0bca7B', width: 5 },
                // }
            ],
            // 区域缩放
             toolbox: {
                orient: 'vertical',
                right: 10,
                bottom: 10,
                feature: {
                    dataZoom: [
                        {
                            yAxisIndex: 'none' // 启用 y 轴缩放
                        },
                        {
                            xAxisIndex: 'none' // 启用 x 轴缩放
                        }
                    ],
                    restore: {}
                }
            }
        };
        
        myChart.getDom().addEventListener('wheel', function (e) {
            let zoomFactor = e.deltaY < 0 ? 1.1 : 0.9; // 向上滚动放大，向下滚动缩小
            let yAxis = myChart.getOption().yAxis[0]; // 获取 myChart1 的 y 轴配置
            let currentZoom = yAxis.dataZoom[0].end - yAxis.dataZoom[0].start;
        
            let newStart = Math.max(0, yAxis.dataZoom[0].start - (zoomFactor - 1) * 100);
            let newEnd = Math.min(100, newStart + currentZoom * zoomFactor);
        
            myChart.setOption({
                yAxis: [{
                    dataZoom: [{
                        start: newStart,
                        end: newEnd
                    }]
                }]
            });
        
            e.preventDefault(); // 阻止默认的页面滚动行为
        });
        
        myChart2.getDom().addEventListener('wheel', function (e) {
            let zoomFactor = e.deltaY < 0 ? 1.1 : 0.9; // 向上滚动放大，向下滚动缩小
            let yAxis = myChart2.getOption().yAxis[0]; // 获取 myChart2 的 y 轴配置
            let currentZoom = yAxis.dataZoom[0].end - yAxis.dataZoom[0].start;
        
            let newStart = Math.max(0, yAxis.dataZoom[0].start - (zoomFactor - 1) * 100);
            let newEnd = Math.min(100, newStart + currentZoom * zoomFactor);
        
            myChart2.setOption({
                yAxis: [{
                    dataZoom: [{
                        start: newStart,
                        end: newEnd
                    }]
                }]
            });
        
            e.preventDefault(); // 阻止默认的页面滚动行为
        });
        
        
        myChart.setOption(option1, true);
        myChart2.setOption(option2, {notMerge:true});

// 标记是否处于“悬停暂停重绘”状态
let isPaused = false;

// 1. 数据定时推送：只往数组里 push，不动图表
const dataTimer = setInterval(() => {
  updateChartData();  
}, 200);

// 2. 重绘定时器：只有 !isPaused 时才真正调用重绘
const redrawTimer = setInterval(() => {
  if (!isPaused) {
    updateChartDataall();
  }
}, 200);

// 3. 鼠标移入/移出时只暂停或恢复“重绘”，数据还在累积
myChart.getDom().addEventListener('mouseenter', () => { isPaused = true; });
myChart.getDom().addEventListener('mouseleave', () => { isPaused = false; });

myChart2.getDom().addEventListener('mouseenter', () => { isPaused = true; });
myChart2.getDom().addEventListener('mouseleave', () => { isPaused = false; });






 
        // 自动更新数据并刷新图表
       function updateChartDataall() {
            updateChartData(); // 更新数据
        const now = Date.now();
        const threeMinutesAgo = now - 2 * 60 * 1000; // 3分钟 = 180,000毫秒 


            // 查找最近3分钟数据的起始索引
        let startIndex = 0;
    for (let i = timeStamps.length - 1; i >= 0; i--) {
        if (timeStamps[i] < threeMinutesAgo) {
            startIndex = i + 1; // 找到第一个大于等于阈值的时间点              
            break;
        }
    }
            const timeSlice = timeData.slice(startIndex); // 保留最新的 180 秒数据
            // const expectedSpeedSlice = expectedSpeedData.slice(-180);
            // const nowSpeedSlice = nowSpeedData.slice(-180);
            const expectedDistanceSlice = expectedDistanceData.slice(startIndex);
            const nowDistanceSlice = nowDistanceData.slice(startIndex);
            // const expectedPressureSlice = expectedPressureData.slice(-180);
            // const nowPressureSlice = nowPressureData.slice(-180);
        
            // const expectedtorqueSlice = expectedtorqueData.slice(-180);
            const ActualtorqueSlice = ActualtorqueData.slice(startIndex);
    
            const ActualRotationalspeedSlice = ActualRotationalspeedData.slice(startIndex);
            // const expectAccelerationSlice = expectAccelerationData.slice(-180);
            // const ActualcurrentSlice = ActualcurrentData.slice(-180);
            // const ActualpowerSlice = ActualpowerData.slice(-180);
    

        
            // 更新图表数据
 myChart.setOption({
    xAxis: { data: timeSlice },
    series: [
      { name: '设定行程', data: expectedDistanceSlice },
      { name: '实际行程', data: nowDistanceSlice }
    ]
  },  false);

  myChart2.setOption({
    xAxis: { data: timeSlice },
    series: [
      { name: '实际扭矩', data: ActualtorqueSlice },
      { name: '实际转速', data: ActualRotationalspeedSlice }
      
    ]
  },  false);
}

window.updateChartDataall = updateChartDataall;


        window.addEventListener("resize", function () {
            myChart.resize();
            myChart2.resize();
        });


        
    }




    
// 导出数据为 CSV 文件
function exportData() {
    let csvContent = "\uFEFF";  // 加上 BOM
    csvContent += "时间,设定行程,实际行程,设定转速,实际转速, 实际扭矩\n";

    for (let i = 0; i < exportTimeData.length; i++) {
        csvContent += `${exportTimeData[i]},${expectedDistanceData[i]},${nowDistanceData[i]},${expectedSpeedData[i]},${nowSpeedData[i]},${variables.Actualtorque}\n`;
    }

    const blob = new Blob([csvContent], { type: "text/csv;charset=utf-8;" });
    const link = document.createElement("a");
    const url = URL.createObjectURL(blob);
    link.setAttribute("href", url);
    link.setAttribute("download", "chart_data.csv");
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
}


// 绑定导出按钮点击事件
document.getElementById("export-button").addEventListener("click", exportData);

// 调用图表初始化函数
initChart();

});


const rootFontSize = parseFloat(
  getComputedStyle(document.documentElement).fontSize
);