#include "UserData.h"
CUserManager::CUserManager(IEpoller* tcp_epoller)
{
	m_pEpoller = tcp_epoller;
	memset(m_UserDataArr, 0, MAX_USER_NUM);
}

CUserManager::~CUserManager()
{
	m_pEpoller = NULL;
}

bool CUserManager::RecvData(int nIndex, char* cRcvData, uint nDataLen)
{
	if (nIndex < 0 || nIndex >= MAX_USER_NUM)
	{
		return false;
	}

	stUserData &data = m_UserDataArr[nIndex];
	if (!data.bHaveData)
	{
		return false;
	}

	//判断是否需要进行websocket操作（）
	if (data.webSocketHelper.NeedWebSocketOperate())
	{
		//是否需要握手
		if (data.webSocketHelper.NeedHandshake())
		{
			//握手
			char cWebSocketHandleData[MAX_RECVBUF_LEN] = {};
			int ret = data.webSocketHelper.HandShake(cRcvData, nDataLen, cWebSocketHandleData);
			if (WEBSOCKET_ERROR == ret)//错误，关闭连接
			{
				CloseConnect(data.pUserFlags->nUserIndex);
				return true;
			}
			if (WEBSOCKET_READING_DATA == ret)//数据不完整,等待下次接收
			{
				return true;
			}
			if (WEBSOCKET_SUCCESS == ret)//握手成功,需要下发握手协议
			{
				/*if (data.webSocketHelper.CanTransData())
				{
					ushort len = 0;
					if (!data.webSocketHelper.Pack(cRcvData, nDataLen, data.cSendData, &len))
					{
						return true;
					}
				}*/

				//向客户端下发握手协议，等客户端确认
				SendData(data.pUserFlags, cWebSocketHandleData, strlen(cWebSocketHandleData));

				//握手成功
				data.webSocketHelper.HandShakeSuccess();
				return true;
			}
		}
		else if (data.webSocketHelper.CanTransData())   //传输数据的阶段
		{
			//接收并解包数据	
			char pCmdData[MAX_RECVBUF_LEN];
			memset(pCmdData, 0, MAX_RECVBUF_LEN);
			ushort dwCmdLen = 0;
			int ret = data.webSocketHelper.Unpack(cRcvData, nDataLen, pCmdData, &dwCmdLen);
			if (WEBSOCKET_ERROR == ret)//错误，关闭连接
			{
				CloseConnect(nIndex);
				return true;
			}
			if (WEBSOCKET_CLOSE == ret)//错误，关闭连接
			{
				CloseConnect(nIndex);
				return true;
			}
			if (WEBSOCKET_READING_DATA == ret)//数据不完整,等待下次接收
			{
				//m_pEpoller->mod(data.pUserFlags->nSocketID, data.pUserFlags, EPOLLIN);	//继续投递接收消息
				LOG_INFO << "数据接收不完整";
				return true;
			}

			if (WEBSOCKET_TEXT == ret ) //文本消息, 不处理了
			{
				LOG_INFO << "文本数据";
				return true;
			}

			if (WEBSOCKET_IGNORE == ret)
			{
				LOG_INFO << "忽略内容";
				return true;
			}

			if (WEBSOCKET_SUCCESS == ret)//成功
			{
				//将数据压入数据存储区域
				memcpy(data.cRcvData + data.nRcvDataLen, pCmdData, dwCmdLen);
				data.nRcvDataLen += dwCmdLen;
				_UnpackData(nIndex);
				return true;
			}
		}
	}

	//非webSocket协议，将数据压入数据存储区域
	memcpy(data.cRcvData + data.nRcvDataLen, cRcvData, nDataLen);
	data.nRcvDataLen += nDataLen;
	_UnpackData(nIndex);
	return true;
}

stUserData* CUserManager::GetUser(int nIndex)
{
	return &m_UserDataArr[nIndex];
}

void CUserManager::_UnpackData(int nIndex)
{
	if (nIndex < 0 || nIndex >= MAX_USER_NUM)
	{
		return;
	}
	stUserData& data = m_UserDataArr[nIndex];
	if (!data.bHaveData)
	{
		return;
	}
	
	int nHeadLen = sizeof(HeadInfo); //头部信息长度
	int nGoupDataLen = 0;			 //一组数据的长度
	int nOffset = 0;
	while (data.nRcvDataLen > nHeadLen)
	{
		HeadInfo hInfo;
		memcpy(&hInfo, data.cRcvData + nOffset, nHeadLen);

		printf("[%s %d] buf:%s, TotalLen:%d, MainCmd:%u, dataLen:%u !\n", __FUNCTION__, __LINE__, data.cRcvData,
			data.nRcvDataLen, hInfo.u_nMainCmd, hInfo.u_nDataLen);

		nGoupDataLen = nHeadLen +hInfo.u_nDataLen;
		if (nGoupDataLen > data.nRcvDataLen)  //一组比存储数据长，跳出等下次接收完整
		{
			break;
		}

		char cData[MAX_RECVBUF_LEN] = {};
		memcpy(cData, data.cRcvData + (nOffset + nHeadLen), hInfo.u_nDataLen);

		nOffset += nGoupDataLen;					//偏移了一组数据
		data.nRcvDataLen -= nGoupDataLen;			//减去一组数据的长度

		//处理命令,并且获取将要发送的数据包
		char cSendBuf[MAX_SENDBUF_LEN] = {};
		uint nDataLen = 0;
		m_CmdDeal.DealCmd(hInfo.u_nMainCmd, hInfo.u_nSubCmd, cData, hInfo.u_nDataLen, cSendBuf, nDataLen);
		if (nDataLen > 0) //存在需要发送的数据
		{
			int nRet = 0;
			nRet = SendData(data.pUserFlags, cSendBuf, nDataLen);

			if (0 == nRet)
				return;

			if (nRet < 0 )   //发送出错
			{
				if (EAGAIN == errno) //如果是输出缓冲区满了，跳出重新设置接收数据
					break;
				else
					return;
			}
		}
	}

	//重新设置接入数据
	if (nOffset > 0)
		RestRcvData(nIndex, data.cRcvData + nOffset, data.nRcvDataLen);
}

void CUserManager::GetSendData(int nIndex, char* cSrcData, uint cSrcDataLen , char* cSendData, ushort &nSendDataLen)
{
	if (nIndex < 0 || nIndex >= MAX_USER_NUM)
	{
		return;
	}

	stUserData& data = m_UserDataArr[nIndex];
	if (!data.bHaveData)
	{
		return;
	}

	if (data.webSocketHelper.CanTransData())
	{
		data.webSocketHelper.Pack(cSrcData, cSrcDataLen, cSendData, &nSendDataLen);
	}
	else
	{
		memcpy(cSendData, cSrcData, cSrcDataLen);
		nSendDataLen = cSrcDataLen;
	}
}

bool CUserManager::addUser(int nSocketID, int &nIndex)
{
	bool bAdd = false;
	for (int iUserIndex = 0; iUserIndex < MAX_USER_NUM; ++iUserIndex)
	{
		if (m_UserDataArr[iUserIndex].bHaveData)
			continue;
		else
		{
			m_UserDataArr[iUserIndex].bHaveData = true;
			
			if (!m_UserDataArr[iUserIndex].pUserFlags)
				m_UserDataArr[iUserIndex].pUserFlags = new stUserFlags;
			m_UserDataArr[iUserIndex].pUserFlags->setParams(TYPE_SOCKET, nSocketID);
			m_UserDataArr[iUserIndex].pUserFlags->nSocketID = nSocketID;
			m_UserDataArr[iUserIndex].pUserFlags->nUserIndex = iUserIndex;
			nIndex = iUserIndex;
			bAdd = true;
			break;
		}
	}

	return bAdd;
}

bool CUserManager::CloseConnect(int nIndex)
{
	if (nIndex < 0 || nIndex >= MAX_USER_NUM)
	{
		LOG_INFO << "close error, nIndex is not in UserArr";
		return false;
	}

	stUserData& data = m_UserDataArr[nIndex];
	if (!data.bHaveData)
	{
		LOG_INFO << "close error, User does not have data!";
		return false;
	}

	LOG_INFO << "close conenect, socketID:" << data.pUserFlags->nSocketID;
	close(data.pUserFlags->nSocketID);
	if (m_pEpoller)
		m_pEpoller->del(data.pUserFlags->nSocketID, data.pUserFlags, EPOLLOUT);
	data.Clear();
	return true;
}

int CUserManager::SendData(stUserFlags* pUserFlags)
{
	if (!pUserFlags)
	{
		return -1;
	}

	int nSocketID = pUserFlags->nSocketID;
	int nUserIndex = pUserFlags->nUserIndex;
	stUserData data = m_UserDataArr[nUserIndex];
	if (!data.bHaveData)
	{
		return -1;
	}

	char cSendData[MAX_RECVBUF_LEN] = {};
	ushort nSendDataLen = data.nSendDataLen;
	memcpy(cSendData, data.cSendData, data.nSendDataLen);

	if (IsWebSocketTransData(nUserIndex)) //websoket
	{
		char sendData[MAX_SENDBUF_LEN];
		ushort len = 0;
		if (!m_UserDataArr[nUserIndex].webSocketHelper.Pack((const char*)cSendData, nSendDataLen, sendData, &len))
		{
			LOG_INFO << "Pack Err";
			return 0;
		}

		//覆盖将要发送的数据
		memcpy((char*)cSendData, sendData, len);
		nSendDataLen = len;
	}

	int nWriteBytes = 0;
	int nTotalWriteBytes = 0;
	int nOffset = nSendDataLen;
	while (nOffset > 0)
	{
		nWriteBytes = write(nSocketID, cSendData + nSendDataLen - nOffset, nOffset);
		printf("[%s %d] send size:%d!\n", __FUNCTION__, __LINE__, nWriteBytes);
		if (nWriteBytes < 0)
		{
			if (EAGAIN == errno) //内核输出缓冲区满了
			{
				//重新设置发送缓冲区
				RestSendData(nUserIndex, cSendData + nTotalWriteBytes, nOffset);

				//切换到输出事件
				m_pEpoller->mod(nSocketID, pUserFlags, EPOLLOUT);
				LOG_INFO << "write buf be full";
				break;
			}
			else //链接出错了，关闭链接
			{
				CloseConnect(nUserIndex);
				break;
			}
		}

		if (0 == nWriteBytes) //对端关闭
		{
			CloseConnect(nUserIndex);
			break;
		}

		nOffset -= nWriteBytes;
		nTotalWriteBytes += nWriteBytes;
	}

	//重新设置发送缓冲区
	if (nWriteBytes > 0)
	{
		RestSendData(nUserIndex, cSendData + nTotalWriteBytes, nOffset);
	}

	return nWriteBytes;
}

int CUserManager::SendData(stUserFlags* pUserFlags, char* cSendData, uint nSendDataLen)
{
	if (!pUserFlags)
	{
		return 0;
	}

	int nSocketID = pUserFlags->nSocketID;
	int nUserIndex = pUserFlags->nUserIndex;

	if (IsWebSocketTransData(nUserIndex))
	{
		char sendData[MAX_SENDBUF_LEN];
		ushort len = 0;
		if (!m_UserDataArr[nUserIndex].webSocketHelper.Pack((const char*)cSendData, nSendDataLen, sendData, &len))
		{
			LOG_INFO << "Pack Err";
			return 0;
		}

		//覆盖将要发送的数据
		memcpy((char*)cSendData, sendData, len);
		nSendDataLen = len;
	}

	int nWriteBytes = 0;
	int nTotalWriteBytes = 0;
	int nOffset = nSendDataLen;
	while (nOffset > 0)
	{
		nWriteBytes = write(nSocketID, cSendData + nSendDataLen - nOffset, nOffset);
		LOG_INFO << "SendSize:%d" << nWriteBytes;
		if (nWriteBytes < 0)
		{
			if (EAGAIN == errno) //内核输出缓冲区满了
			{
				//重新设置发送缓冲区
				RestSendData(nUserIndex, cSendData + nTotalWriteBytes, nOffset);

				//切换到输出事件
				m_pEpoller->mod(nSocketID, pUserFlags, EPOLLOUT);
				LOG_INFO << "write buf be full !";
				break;
			}
			else //链接出错了，关闭链接
			{
				CloseConnect(nUserIndex);
				break;
			}
		}

		if (0 == nWriteBytes) //对端关闭
		{
			CloseConnect(nUserIndex);
			break;
		}

		nOffset -= nWriteBytes;
		nTotalWriteBytes += nWriteBytes;
	}

	return nWriteBytes;
}

int CUserManager::SendData(stUserFlags* pUserFlags, uint uMainCmd, uint uSubCmd, char* cSendData)
{
	if (!pUserFlags)
	{
		return -1;
	}

	int nSocketID = pUserFlags->nSocketID;
	int nUserIndex = pUserFlags->nUserIndex;
	int nDataLen = strlen(cSendData);
	if (nDataLen <= 0)
	{
		LOG_ERROR << "dataLen Err!";
		return -1;
	}

	char cSendBuf[MAX_SENDBUF_LEN] = {};
	uint nSendDataLen = 0;
	PackData(uMainCmd, uSubCmd, cSendData, nDataLen, cSendBuf, nSendDataLen);

	if (IsWebSocketTransData(nUserIndex))
	{
		char sendData[MAX_SENDBUF_LEN];
		ushort len = 0;
		if (!m_UserDataArr[nUserIndex].webSocketHelper.Pack((const char*)cSendBuf, nSendDataLen, sendData, &len))
		{
			LOG_INFO << "Pack Err";
			return -1;
		}

		//覆盖将要发送的数据
		memcpy((char*)cSendBuf, sendData, len);
		nSendDataLen = len;
	}

	int nWriteBytes = 0;
	int nTotalWriteBytes = 0;
	int nOffset = nSendDataLen;
	while (nOffset > 0)
	{
		nWriteBytes = write(nSocketID, cSendBuf + nSendDataLen - nOffset, nOffset);
		if (nWriteBytes < 0)
		{
			if (EAGAIN == errno) //内核输出缓冲区满了
			{
				//重新设置发送缓冲区
				RestSendData(nUserIndex, cSendBuf + nTotalWriteBytes, nOffset);

				//切换到输出事件
				m_pEpoller->mod(nSocketID, pUserFlags, EPOLLOUT);
				LOG_INFO << "write buf is full !";
				break;
			}
			else //链接出错了，关闭链接
			{
				CloseConnect(nUserIndex);
				return -1;
			}
		}

		if (0 == nWriteBytes) //对端关闭
		{
			LOG_INFO << "客户端关闭了，fd:" << nSocketID;
			CloseConnect(nUserIndex);
			return -1;
		}

		nOffset -= nWriteBytes;
		nTotalWriteBytes += nWriteBytes;
	}

	return nWriteBytes;
}

void CUserManager::KeepLife()
{
	bool bAdd = false;
	for (int iUserIndex = 0; iUserIndex < MAX_USER_NUM; ++iUserIndex)
	{
		if (!m_UserDataArr[iUserIndex].bHaveData)
			continue;

		LOG_INFO << "ping 测试, socketID:" << m_UserDataArr[iUserIndex].pUserFlags->nSocketID;

		SendData(m_UserDataArr[iUserIndex].pUserFlags, CMD_PING, 0, "测试");
	}
}

bool CUserManager::RestRcvData(int nIndex, char* cNewData, uint nNewDataLen)
{
	if (nIndex < 0 || nIndex >= MAX_USER_NUM)
	{
		return false;
	}

	stUserData &data = m_UserDataArr[nIndex];
	if (!data.bHaveData)
	{
		return false;
	}

	memset(data.cRcvData, 0, MAX_DATA_LEN);
	memcpy(data.cRcvData, cNewData, nNewDataLen);
	data.nRcvDataLen = nNewDataLen;
	return true;
}

bool CUserManager::RestSendData(int nIndex, char* cNewData, uint nNewDataLen)
{
	if (nIndex < 0 || nIndex >= MAX_USER_NUM)
	{
		return false;
	}

	stUserData& data = m_UserDataArr[nIndex];
	if (!data.bHaveData)
	{
		return false;
	}

	memset(data.cSendData, 0, MAX_SENDBUF_LEN);
	memcpy(data.cSendData, cNewData, nNewDataLen);
	data.nSendDataLen = nNewDataLen;
	return true;
}

bool CUserManager::IsWebSocketTransData(int nIndex)
{
	if (nIndex >= MAX_USER_NUM || nIndex < 0)
		return false;

	if (m_UserDataArr[nIndex].webSocketHelper.CanTransData())
		return true;

	return false;
}
