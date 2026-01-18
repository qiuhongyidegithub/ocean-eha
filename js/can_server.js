const express = require('express');
const ffi = require('ffi-napi');
const ref = require('ref-napi');
const Struct = require('ref-struct-napi');
const ArrayType = require('ref-array-napi');


const Uint8Array = ArrayType(ref.types.uint8);


const CAN_OBJ = Struct({
  ID: ref.types.uint32,
  TimeStamp: ref.types.uint32,
  TimeFlag: ref.types.uint8,
  SendType: ref.types.uint8,
  RemoteFlag: ref.types.uint8,
  ExternFlag: ref.types.uint8,
  DataLen: ref.types.uint8,
  Data: [Uint8Array, 8],      // 固定长度8
  Reserved: [Uint8Array, 3]   // 固定长度3
})['packed']; 



const CAN_OBJPtr = ref.refType(CAN_OBJ);


// 配置 CAN 设备
const DEV_TYPE = 4;  // USB-CAN-II 设备类型
const DEV_INDEX = 0; // 设备索引
const CAN_INDEX = 0; // CAN 通道索引

// 加载 ECanVci.dll 库
const canLib = ffi.Library('./ECanVci64.dll', {
  'OpenDevice': ['uint32', ['uint32', 'uint32', 'uint32']],
  'CloseDevice': ['uint32', ['uint32', 'uint32']],
  'InitCAN': ['uint32', ['uint32', 'uint32', 'uint32', 'pointer']],
  'StartCAN': ['uint32', ['uint32', 'uint32', 'uint32']],
  'Transmit': ['uint32', ['uint32', 'uint32', 'uint32', CAN_OBJPtr, 'uint32']],
  'Receive': ['uint32', ['uint32', 'uint32', 'uint32', CAN_OBJPtr, 'uint32', 'int']],
  'ReadErrInfo': ['uint32', ['uint32', 'uint32', 'uint32', 'pointer']]
});

// 初始化 CAN 设备
function initDevice() {
  if (canLib.OpenDevice(DEV_TYPE, DEV_INDEX, 0) !== 1) {
    console.error('打开设备失败');
    process.exit(1);
  }

  // 初始化 CAN 配置参数
  let initConfig = Buffer.alloc(12);
  initConfig.writeUInt32LE(0, 0); // AccCode
  initConfig.writeUInt32LE(0xFFFFFFFF, 4); // AccMask
  initConfig[8] = 0; // Filter
  initConfig[9] = 0; // Timing0 (500Kbps)
  initConfig[10] = 0x1C; // Timing1 (500Kbps)
  initConfig[11] = 0; // Mode

  if (canLib.InitCAN(DEV_TYPE, DEV_INDEX, CAN_INDEX, initConfig) !== 1) {
    console.error('初始化通道失败');
    canLib.CloseDevice(DEV_TYPE, DEV_INDEX);
    process.exit(1);
  }

  if (canLib.StartCAN(DEV_TYPE, DEV_INDEX, CAN_INDEX) !== 1) {
    console.error('启动通道失败');
    canLib.CloseDevice(DEV_TYPE, DEV_INDEX);
    process.exit(1);
  }

  console.log('CAN 设备初始化成功');
}

// 发送 CAN 数据
function sendCAN(id, data) {
  let frame = new CAN_OBJ();
  frame.ID = id;
  frame.TimeFlag = 0;
  frame.SendType = 0;
  frame.RemoteFlag = 0;
  frame.ExternFlag = 0;
  frame.DataLen = data.length;

  // 填充数据字段
  for (let i = 0; i < 8; i++) {
    frame.Data[i] = i < data.length ? data[i] : 0;
  }

  let result = canLib.Transmit(DEV_TYPE, DEV_INDEX, CAN_INDEX, frame.ref(), 1);
  return result === 1 ? '发送成功' : '发送失败';
}

// 接收 CAN 数据
function receiveCAN() {
  let frame = new CAN_OBJ();
  let result = canLib.Receive(DEV_TYPE, DEV_INDEX, CAN_INDEX, frame.ref(), 1, 0);

  if (result > 0) {
    let dataArr = [];
    for (let i = 0; i < frame.DataLen; i++) {
      dataArr.push(frame.Data[i]);
    }
    return { id: frame.ID, data: dataArr };
  } else {
    return { error: '未收到数据' };
  }
}

// 创建 Express Web 服务器
const app = express();
app.use(express.json());

// API 端点：发送 CAN 数据
app.post('/send_can', (req, res) => {
  const { id, data } = req.body; // 从请求中获取数据
  const result = sendCAN(id, data); // 调用发送函数
  res.json({ status: result });
});

// API 端点：接收 CAN 数据
app.get('/receive_can', (req, res) => {
  const result = receiveCAN(); // 调用接收函数
  res.json(result);
});

// 启动 HTTP 服务
const PORT = 3000;
app.listen(PORT, () => {
  console.log(`服务器运行在 http://localhost:${PORT}`);
  initDevice(); // 初始化 CAN 设备
});

// 监听进程退出时关闭 CAN 设备
process.on('SIGINT', () => {
  canLib.CloseDevice(DEV_TYPE, DEV_INDEX);
  console.log('\nCAN 设备已关闭');
  process.exit();
});



