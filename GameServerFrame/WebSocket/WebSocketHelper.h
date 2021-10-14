
#ifndef _WEBSOCKET_HELPER_H_
#define _WEBSOCKET_HELPER_H_

#define MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#include <netinet/in.h>  
#include <cstring>
#include "sha1.h"
#include "base64.h"
using namespace std;

//�ɹ�
#define WEBSOCKET_SUCCESS 1
//��Ҫ������ȡ����
#define WEBSOCKET_READING_DATA 0
//ʧ��
#define WEBSOCKET_ERROR -1
//����websocket
#define WEBSOCKET_NOT -2
//����
#define WEBSOCKET_IGNORE -3
//�ر�
#define WEBSOCKET_CLOSE -4
//�ı���Ϣ
#define WEBSOCKET_TEXT -5

#define MAX_RECVBUF_LEN					4096+32
#define MAX_SENDBUF_LEN					4096+32

//websocket״̬ö��
enum WebsocketState
{
	beforCheck = 0,		// �ȴ�У�鵱ǰ�����Ƿ�websocket
	notWebSocket = 1,	//��ǰ���Ӳ���websocket
	handshake = 2,		// �����ڼ�
	transData = 3		// ���ݴ����ڼ�
};

//websocket ������
class WebSocketHelper
{
public:
	WebSocketHelper();
	~WebSocketHelper();

public:
	//�Ƿ���Ҫ����websocket����,���֣������
	bool NeedWebSocketOperate();
	//�Ƿ�������ֲ���
	bool NeedHandshake();
	//�Ƿ����������,���Դ�������
	bool CanTransData();
	//�������ֳɹ�
	void HandShakeSuccess();
	//����״̬������
	void Clear();
	//��������
	void ClearData();
	
	/*
	websocket ����
	@param char *recvData ���ν��յ����ݰ�
	@param char *sendData �������ֳɹ�Ҫ���͵����ݰ�
	@return �ɹ��򷵻�WEBSOCKET_SUCCESS,�ȴ����� WEBSOCKET_READING_DATA��ʧ��WEBSOCKET_ERROR
	*/
	int HandShake(char *recvData, ushort  dwRecvLen, char *sendData);
	
	/*
	websocket ���ղ���websocket���ݰ�
	@param char *recvData ���ν��յ����ݰ�
	@param ushort  dwRecvLen ���ν��յ����ݰ�����
	@param char *cmdData ����������
	@param ushort  *dwCmdLen ���������ݳ���
	@return �ɹ��򷵻�WEBSOCKET_SUCCESS,�ȴ����� WEBSOCKET_READING_DATA��ʧ��WEBSOCKET_ERROR
	*/
	int Unpack(char *recvData,ushort  dwRecvLen, char *cmdData, ushort  *dwCmdLen);
	
	/*
	websocket ��װwebsocket���ݰ�
	@param char *cmdData �������ݰ�
	@param char *sendData ��װ��Ҫ���͵����ݰ�	
	@param ushort *wSendlen ���������ݳ���
	@return �ɹ��򷵻�true
	*/
	bool Pack(const char *cmdData, ushort cmdLen, char *sendData, ushort *wSendlen);

private:
	//����websocket״̬
	void ChangeState(WebsocketState state);


private:
	WebsocketState m_wsState;
	//��ǰ�Ѿ����յ�������
	char m_szRecvBuf[MAX_SENDBUF_LEN];
	//�Ѿ����յ������ݳ���
	ushort m_wRecvedLen;	
	//�Ƿ����ڽ�������
	bool m_bRecivingData;
};

#endif //#ifndef _WEBSOCKET_HELPER_H_