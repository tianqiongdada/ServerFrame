
#ifndef _WEBSOCKET_HELPER_H_
#define _WEBSOCKET_HELPER_H_

#define MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#include <netinet/in.h>  
#include <cstring>
#include "sha1.h"
#include "base64.h"
using namespace std;

//成功
#define WEBSOCKET_SUCCESS 1
//需要继续读取数据
#define WEBSOCKET_READING_DATA 0
//失败
#define WEBSOCKET_ERROR -1
//不是websocket
#define WEBSOCKET_NOT -2
//忽略
#define WEBSOCKET_IGNORE -3
//关闭
#define WEBSOCKET_CLOSE -4
//文本消息
#define WEBSOCKET_TEXT -5

#define MAX_RECVBUF_LEN					4096+32
#define MAX_SENDBUF_LEN					4096+32

//websocket状态枚举
enum WebsocketState
{
	beforCheck = 0,		// 等待校验当前连接是否websocket
	notWebSocket = 1,	//当前连接不是websocket
	handshake = 2,		// 握手期间
	transData = 3		// 数据传输期间
};

//websocket 辅助类
class WebSocketHelper
{
public:
	WebSocketHelper();
	~WebSocketHelper();

public:
	//是否需要进行websocket操作,握手，解包等
	bool NeedWebSocketOperate();
	//是否进行握手操作
	bool NeedHandshake();
	//是否完成了握手,可以传输数据
	bool CanTransData();
	//设置握手成功
	void HandShakeSuccess();
	//重置状态和数据
	void Clear();
	//重置数据
	void ClearData();
	
	/*
	websocket 握手
	@param char *recvData 本次接收的数据包
	@param char *sendData 本次握手成功要发送的数据包
	@return 成功则返回WEBSOCKET_SUCCESS,等待数据 WEBSOCKET_READING_DATA，失败WEBSOCKET_ERROR
	*/
	int HandShake(char *recvData, ushort  dwRecvLen, char *sendData);
	
	/*
	websocket 接收并解websocket数据包
	@param char *recvData 本次接收的数据包
	@param ushort  dwRecvLen 本次接收的数据包长度
	@param char *cmdData 解包后的数据
	@param ushort  *dwCmdLen 解包后的数据长度
	@return 成功则返回WEBSOCKET_SUCCESS,等待数据 WEBSOCKET_READING_DATA，失败WEBSOCKET_ERROR
	*/
	int Unpack(char *recvData,ushort  dwRecvLen, char *cmdData, ushort  *dwCmdLen);
	
	/*
	websocket 封装websocket数据包
	@param char *cmdData 命令数据包
	@param char *sendData 封装后要发送的数据包	
	@param ushort *wSendlen 封包后的数据长度
	@return 成功则返回true
	*/
	bool Pack(const char *cmdData, ushort cmdLen, char *sendData, ushort *wSendlen);

private:
	//设置websocket状态
	void ChangeState(WebsocketState state);


private:
	WebsocketState m_wsState;
	//当前已经接收到的数据
	char m_szRecvBuf[MAX_SENDBUF_LEN];
	//已经接收到的数据长度
	ushort m_wRecvedLen;	
	//是否正在接收数据
	bool m_bRecivingData;
};

#endif //#ifndef _WEBSOCKET_HELPER_H_