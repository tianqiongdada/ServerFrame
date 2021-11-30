
#include "WebSocketHelper.h"
WebSocketHelper::WebSocketHelper()
{
	Clear();
}

void WebSocketHelper::Clear()
{
	ChangeState(beforCheck);
	ClearData();	
}

void WebSocketHelper::ClearData()
{	
	memset(m_szRecvBuf,0,sizeof(m_szRecvBuf));	
	m_wRecvedLen = 0;	
	m_bRecivingData = false;
}

WebSocketHelper::~WebSocketHelper()
{
}

void WebSocketHelper::ChangeState(WebsocketState state)
{
	m_wsState = state;
}

bool WebSocketHelper::CanTransData()
{
	if(m_wsState == transData)
		return true;
	return false;
}

void WebSocketHelper::HandShakeSuccess()
{
	//开始传输数据
	ChangeState(transData);
	//清空数据
	ClearData();
}

bool WebSocketHelper::NeedWebSocketOperate()
{
	//已经明确不是websocket连接，则不需要进行websocket操作。
	if(m_wsState == notWebSocket)
		return false;
	return true;
}

bool WebSocketHelper::NeedHandshake()
{
	//只有未做websocket判断，或者正在握手阶段才进行握手
	if(m_wsState != beforCheck && m_wsState != handshake)
		return false;
	return true;
}

int WebSocketHelper::HandShake(char *recvData, ushort  dwRecvLen, char *sendData)
{	
	//未做类型判断，或者正在握手阶段才进行握手
	if(m_wsState != beforCheck && m_wsState != handshake)
	{
		Clear();
		return WEBSOCKET_ERROR;
	}
	
	/*//websocket 握手请求
	GET /chat HTTP/1.1
	Host: server.example.com
	Upgrade: websocket
	Connection: Upgrade
	Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
	Origin: http://example.com
	Sec-WebSocket-Protocol: chat, superchat
	Sec-WebSocket-Version: 13
	*/

	if(m_wsState == beforCheck)
	{
		//判断当前请求是否websockt握手请求,这里连接初始数据包中有GET就进行握手校验
		if(strstr(recvData, "GET"))
		{
			ChangeState(handshake);			
		}
		else //非websocket
		{
			ChangeState(notWebSocket);
			ClearData();
			return WEBSOCKET_NOT;
		}
	}
	
	if(m_wsState == handshake)
	{
		//开始接收数据
		m_bRecivingData = true;
		//复制数据
		memcpy(m_szRecvBuf+m_wRecvedLen,recvData,dwRecvLen);
		m_wRecvedLen += dwRecvLen;
		//判断数据是否完整
		if(!strstr(m_szRecvBuf,"\r\n\r\n"))
		{
			return WEBSOCKET_READING_DATA;//等待下一次接收
		}
		//数据已经完整接收
		m_bRecivingData = false;
	}
	
	//获取Sec-WebSocket-Key
	const char *Sec_WebSocket_Key = "Sec-WebSocket-Key: ";//注意，后面有空格
	char * keyStr = NULL;
	keyStr = strstr(m_szRecvBuf, Sec_WebSocket_Key);
	if (!keyStr)
	{
		Clear();
		return WEBSOCKET_ERROR;
	}
	char * lineStr = strstr(keyStr, "\r");
	if (!lineStr)
	{
		Clear();
		return WEBSOCKET_ERROR;
	}
	*lineStr = '\0';
	char key[128];
	size_t len = strlen(keyStr) - strlen(Sec_WebSocket_Key);
	strncpy(key, keyStr + strlen(Sec_WebSocket_Key), len);
	key[len] = '\0';	
	
	string serverKey = key;
	serverKey += MAGIC_KEY;

	//sha1
	SHA1 sha;
	uint message_digest[5];
	sha.Reset();
	
	sha << serverKey.c_str();
	sha.Result(message_digest);
	for (int i = 0; i < 5; i++) {
		message_digest[i] = htonl(message_digest[i]);
	}
	string responseKey = base64_encode(reinterpret_cast<const unsigned char*>(message_digest), 20);	
	responseKey += "\r\n";
	

	/*//websocket响应请求
	HTTP/1.1 101 Switching Protocols
	Upgrade: websocket
	Connection: Upgrade
	Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
	Sec-WebSocket-Protocol: chat
	*/
	strcat(sendData, "HTTP/1.1 101 Switching Protocols\r\n");
	strcat(sendData, "Connection: upgrade\r\n");
	strcat(sendData, "Sec-WebSocket-Accept: ");	
	strcat(sendData, responseKey.c_str());
	strcat(sendData, "Upgrade: websocket\r\n\r\n");	
	
	return WEBSOCKET_SUCCESS;
}

int WebSocketHelper::Unpack(char *recvData,ushort  dwRecvLen, char *cmdData, ushort  *dwCmdLen)
{
	//是否已经完成握手，进入websocket数据传输状态
	if(m_wsState != transData)
	{
		Clear();
		return WEBSOCKET_ERROR;
	}
	//数据太长
	if(m_wRecvedLen + dwRecvLen > MAX_RECVBUF_LEN)
	{
		Clear();
		return WEBSOCKET_ERROR;
	}
	
	//开始接收数据
	m_bRecivingData = true;
	//复制数据
	memcpy(m_szRecvBuf+m_wRecvedLen,recvData,dwRecvLen);
	m_wRecvedLen += dwRecvLen;
	//判断数据是否完整
	if (m_wRecvedLen < 2)  
	{  
		//等待下一次接收
		return WEBSOCKET_READING_DATA;
	}  

	// 检查扩展位并忽略  	
	/*
	if ((frameData[0] & 0x70) != 0x0)  
	{  
		//无效数据，清空
		Clear();
		return WEBSOCKET_ERROR;
	} 
	*/
  
	// fin位: 为1表示已接收完整报文, 为0表示继续监听后续报文 (目前只处理1帧的情况)
	if ((m_szRecvBuf[0] & 0x80) != 0x80)  
	{  
		//无效数据，清空
		Clear();
		return WEBSOCKET_ERROR;
	}  
  
	// mask位, 为1表示数据被加密 (客户端发送必须有掩码)
	if ((m_szRecvBuf[1] & 0x80) != 0x80)  
	{  
		//无效数据，清空
		Clear();
		return WEBSOCKET_ERROR;
	}  

	//获取数据长度
	ushort  packageLengh = 0;//包长度
	ushort payloadLength = 0;  
	unsigned char payloadFieldExtraBytes = 0;  
	payloadLength = static_cast<ushort >(m_szRecvBuf[1] & 0x7f);
	packageLengh = payloadLength + 6;//长度125以下时包的长度
	if (payloadLength == 0x7e)  //若是126表示，后两个byte取无符号16位整数值，是负载长度
	{
		if (m_wRecvedLen < 4)  
		{  
			//下次继续接收 
			return WEBSOCKET_READING_DATA;
		}  
		ushort payloadLength16b = 0;  
		payloadFieldExtraBytes = 2;  
		memcpy(&payloadLength16b, &m_szRecvBuf[2], payloadFieldExtraBytes);  
		payloadLength = ntohs(payloadLength16b);  
		packageLengh = payloadLength + 8;//2位头，2位长度 4位masking 
		if(packageLengh > MAX_RECVBUF_LEN) //数据过长 2位头，2位长度 4位masking				
		{
			// 数据过长,暂不支持,无效数据，清空
			Clear();
			return WEBSOCKET_ERROR;
		}
		if (m_wRecvedLen < packageLengh)  
		{
			//继续接收
			return WEBSOCKET_READING_DATA;
		}
	}  
	else if (payloadLength == 0x7f)  //127表示后8个 byte，取64位无符号整数值，是负载长度
	{			 
		// 数据过长,暂不支持
		Clear();
		return WEBSOCKET_ERROR;
	}
	
	if (m_wRecvedLen < packageLengh)  
	{
		//继续接收
		return WEBSOCKET_READING_DATA;
	}

	
	//数据已经完整接收
	m_bRecivingData = false;	


	/*//https://tools.ietf.org/html/rfc6455#page-27
	 0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+

	1byte
		1bit: frame-fin，x0表示该message后续还有frame；x1表示是message的最后一个frame
		3bit: 分别是frame-rsv1、frame-rsv2和frame-rsv3，通常都是x0
		4bit: frame-opcode，x0表示是延续frame；x1表示文本frame；x2表示二进制frame；x3-7保留给非控制frame；x8表示关 闭连接；x9表示ping；
		xA表示pong；xB-F保留给控制frame
	2byte
		1bit: Mask，1表示该frame包含掩码；0表示无掩码
		7bit、7bit+2byte、7bit+8byte: 7bit取整数值，若在0-125之间，则是负载数据长度；若是126表示，后两个byte取无符号16位整数值，是负载长度；
			127表示后8个 byte，取64位无符号整数值，是负载长度
	3-6byte: 这里假定负载长度在0-125之间，并且Mask为1，则这4个byte是掩码
	7-end byte: 长度是上面取出的负载长度，包括扩展数据和应用数据两部分，通常没有扩展数据；若Mask为1，则此数据需要解码，
			解码规则为- 1-4byte掩码循环和数据byte做异或操作。

	客户端向服务器传输的数据帧必须进行掩码处理：服务器若接收到未经过掩码处理的数据帧，则必须主动关闭连接。
	*/	
       
    // 操作码   
    unsigned char opcode = static_cast<unsigned char >(m_szRecvBuf[0] & 0x0f);  
    if (opcode == 0x1)  //文本暂不处理
    {  
		ClearData();
	    return WEBSOCKET_TEXT;		
    } 
	else if(opcode == 0x2) //二进制
	{
		// 数据解码  
		if (payloadLength > 0)  
		{  
			// header: 2字节, masking key: 4字节  
			const char *maskingKey = &m_szRecvBuf[2 + payloadFieldExtraBytes]; 
			memcpy(cmdData, &m_szRecvBuf[2 + payloadFieldExtraBytes + 4], payloadLength);  
			for (int i = 0; i < payloadLength; i++)  
			{  
				cmdData[i] = cmdData[i] ^ maskingKey[i % 4];  
			}  
		}  
		*dwCmdLen = payloadLength;

		ClearData();
		return WEBSOCKET_SUCCESS;
	}
	else if (opcode == 0x8)//关闭连接
    {  
        Clear();
		return WEBSOCKET_CLOSE;
    } 
	else if (opcode == 0x9 || opcode == 0xA)  
    {  
        // ping/pong帧暂不处理  
		ClearData();
		return WEBSOCKET_IGNORE;
    } 
    else  
    {  
		ClearData();
        return WEBSOCKET_IGNORE;
    }      
}

bool WebSocketHelper::Pack(const char *cmdData, ushort cmdLen, char *sendData, ushort *wSendlen)
{
	/*
	服务器向客户端传输的数据帧一定不能进行掩码处理。客户端若接收到经过掩码处理的数据帧，则必须主动关闭连接。
	
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
	 1byte
		1bit: frame-fin，x0表示该message后续还有frame；x1表示是message的最后一个frame
		3bit: 分别是frame-rsv1、frame-rsv2和frame-rsv3，通常都是x0
		4bit: frame-opcode，x0表示是延续frame；x1表示文本frame；x2表示二进制frame；x3-7保留给非控制frame；x8表示关 闭连接；x9表示ping；
		xA表示pong；xB-F保留给控制frame
	2byte
		1bit: Mask，1表示该frame包含掩码；0表示无掩码
		7bit、7bit+2byte、7bit+8byte: 7bit取整数值，若在0-125之间，则是负载数据长度；若是126表示，后两个byte取无符号16位整数值，是负载长度；
			127表示后8个 byte，取64位无符号整数值，是负载长度
	3-6byte: 这里假定负载长度在0-125之间，并且Mask为1，则这4个byte是掩码
	7-end byte: 长度是上面取出的负载长度，包括扩展数据和应用数据两部分，通常没有扩展数据；若Mask为1，则此数据需要解码，
			解码规则为- 1-4byte掩码循环和数据byte做异或操作。
	*/
		
    if(cmdLen < 126)
    { 
        sendData[0] = (unsigned char)0x82;//1000 0010 发送二进制
        sendData[1] = (unsigned char)cmdLen;
		memcpy(sendData+2, cmdData, cmdLen);        
		*wSendlen = cmdLen + 2;
    }
    else if(cmdLen < 0xFFFF)
    {  
        sendData[0] = (unsigned char)0x82;//1000 0010
        sendData[1] = (unsigned char)0x7e;//若是126表示，后两个byte取无符号16位整数值，是负载长度
		ushort payloadLength16b = htons(cmdLen);
		memcpy(sendData+2, &payloadLength16b, 2);  
        memcpy(sendData+4, cmdData, cmdLen); 
		*wSendlen = cmdLen + 4;
    }
    else
    {
        // 暂不处理超长内容
		return false;
    }

    return true;
}
