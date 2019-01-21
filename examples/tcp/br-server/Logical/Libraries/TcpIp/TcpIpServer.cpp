/** @file TcpIpServer.cpp
 *  @brief Implementation of the TcpIpServer Class
 */
#include "TcpIpServer.h"

#define LINGER_ON (1)

/**
*	@brief disable all function blocks
*/
void TcpIpServer::deinit()
{
	TcpClose_0.enable = false;
	TcpClose(&TcpClose_0);
	TcpOpen_0.enable = false;
	TcpOpen(&TcpOpen_0);
	TcpServer_0.enable = false;
	TcpServer(&TcpServer_0);
	TcpIoctl_0.enable = false;
	TcpIoctl(&TcpIoctl_0);
	TcpRecv_0.enable = false;
	TcpRecv(&TcpRecv_0);
	TcpSend_0.enable = false;
	TcpSend(&TcpSend_0);
	tcp_step = 0;
}

/**
*	@brief initialize server
*/
void TcpIpServer::init()
{
	this->_status = BUSY;
	static const struct tcpLINGER_typ linger_opt = {LINGER_ON, 0};			
	switch(tcp_step) {
		case 0:
			deinit();
			this->cOutBuf = 0;
			tcp_step = 1;
			break;
		case 1:
			/* Open TCP Socket */
			TcpOpen_0.pIfAddr = 0;
			TcpOpen_0.port = this->serverPort;
			TcpOpen_0.options = tcpOPT_REUSEADDR;
			TcpOpen_0.enable = true;
			TcpOpen(&TcpOpen_0);
			if (TcpOpen_0.status == ERR_OK) {
				this->serverID = TcpOpen_0.ident;
				tcp_step = 2;
			} else if (TcpOpen_0.status == ERR_FUB_BUSY) {
				// intentionally blank
			} else {
				tcp_step = 0;
				this->_status = ERROR;
			}
			break;
		case 2:
			/* set server options */
			/* these ensure that the connection gets closed directly, without waiting for timeouts */
			/* see BuR AsTCP Help files for more information */
				
			TcpIoctl_0.ident = this->serverID;
			TcpIoctl_0.ioctl = tcpSO_LINGER_SET;
			TcpIoctl_0.pData = (unsigned long) &linger_opt;
			TcpIoctl_0.datalen = sizeof(linger_opt);
			TcpIoctl_0.enable = true;
			TcpIoctl(&TcpIoctl_0);
			if (TcpIoctl_0.status == ERR_OK) {
				//this->outlen = TcpIoctl_0.outlen;
				tcp_step = 3;
			} else if (TcpIoctl_0.status == ERR_FUB_BUSY) {
				// intentionally blank
			} else {
				tcp_step = 0;
				this->_status = ERROR;
			}
			break;
		case 3:
			/* start server and wait for client */
			TcpServer_0.ident = this->serverID;
			TcpServer_0.backlog = 1;
			TcpServer_0.pIpAddr = (unsigned long) this->clientIp;
			TcpServer_0.enable = true;
			TcpServer(&TcpServer_0);
			if (TcpServer_0.status == ERR_OK) {
				this->clientID = TcpServer_0.identclnt;
				//this->clientPort = TcpServer_0.portclnt;
				tcp_step = 4;
			} else if (TcpServer_0.status == ERR_FUB_BUSY) {
				//intentionally blank
			} else {
				tcp_step = 0;
				this->_status = ERROR;
			}
			break;
		case 4:
			/* set same options for client */
			TcpIoctl_0.ident = this->clientID;
			TcpIoctl_0.ioctl = tcpSO_LINGER_SET;
			TcpIoctl_0.pData = (unsigned long) &linger_opt;
			TcpIoctl_0.datalen = sizeof(linger_opt);
			TcpIoctl_0.enable = true;
			TcpIoctl(&TcpIoctl_0);
			if (TcpIoctl_0.status == ERR_OK) {
			//	this->client_outlen = TcpIoctl_0.outlen;
				this->_status = READY;
				tcp_step = 0;
			} else if (TcpIoctl_0.status == ERR_FUB_BUSY) {
				// intentionally blank
			} else {
				tcp_step = 0;
				this->_status = ERROR;
			}
			break;
		default:
			break;
	}
}

/**
 * @brief reads incoming data from client
 * data gets saved to inBuffer, the data length is written to inBufferLen
 */
TcpIpServer::Status TcpIpServer::read()
{
	TcpRecv_0.ident = this->clientID;
	TcpRecv_0.pData = (unsigned long) this->inBuffer;
	TcpRecv_0.datamax = sizeof(this->inBuffer);
	TcpRecv_0.flags = 0;
	TcpRecv_0.enable = true;
	TcpRecv(&TcpRecv_0);
	if (TcpRecv_0.status == ERR_OK)
	{
		this->inBufferLen = TcpRecv_0.recvlen;
		if (this->inBufferLen == 0)
		{
			/* client already disconnected */
			return STOP;
		}
		else
			return READY;
	}
	else if (TcpRecv_0.status == ERR_FUB_BUSY)
		;
	else if (TcpRecv_0.status == tcpERR_NO_DATA)
	{
		/* no data received */
		this->inBufferLen = 0;
		return READY;
	}
	else if (TcpRecv_0.status == tcpERR_NOT_CONNECTED)
	{
		/* connection closed */
		return STOP;
	}
	else
	{       
		return ERROR;
	}
	return BUSY;
}

/**
 * @brief write data from the current outBuffer to the client
 */
TcpIpServer::Status TcpIpServer::write()
{
	TcpSend_0.ident = this->clientID;
	TcpSend_0.pData = (unsigned long) this->outBuffer[cOutBuf];
	TcpSend_0.datalen = this->outBufferLen[cOutBuf];
	TcpSend_0.flags = 0;
	TcpSend_0.enable = true;
	TcpSend(&TcpSend_0);
	if (TcpSend_0.status == ERR_OK)
	{
		this->outBufferLen[cOutBuf] = 0;
		cOutBuf = !cOutBuf;
		return READY;
	}
	else if (TcpSend_0.status == ERR_FUB_BUSY)
		;
	else if (TcpSend_0.status == tcpERR_NOT_CONNECTED)
	{
		/* connection closed */
		return STOP;        
	}
	else
		return ERROR;
	return BUSY;
}

/**
 * @brief writes outgoing frames to the currently unused outBuffer
 */
void TcpIpServer::handleFrame(Frame frame)
{
	unsigned char w = !this->cOutBuf; 
	this->outBuffer[w][this->outBufferLen[w]++] = frame.data.id;
	for (int i = 0; i < MAX_PAYLOAD; ++i) {
		this->outBuffer[w][this->outBufferLen[w]++] = frame.data.payload[i];
	}
}
	
void TcpIpServer::sync()
{
	this->_status = BUSY;
	switch (tcp_step) {
		case 0:
			/**< Lese von Client*/
			this->_status_sub = read();
			switch (this->_status_sub) {
				case READY:
					if (this->inBufferLen > 0) {
						unsigned char *pointer = this->inBuffer;
						while (pointer - this->inBuffer < (signed long)this->inBufferLen) {
							Frame frame(pointer++[0]);
							for (int i = 0; i < MAX_PAYLOAD; ++i)
								frame.data.payload[i] = *pointer++;
							this->transp->handleFrame(frame);
						}
						this->inBufferLen = 0;
					}
					/**< Sende Daten falls vorhanden*/
					if (1){//this->outBufferLen[this->cOutBuf] > 0) {
						tcp_step = 1;
					} else {
						tcp_step = 0;
						_status = READY;
					}
					break;
				case BUSY:
					// intentionally blank
					break;
				case STOP:
					tcp_step = 0;
					_status = STOP;
					break;
				default:			
					tcp_step = 0;
					_status = ERROR;
					break;			
			}
			break;
		case 1:
			/**< Schreibe Daten an Client*/
			_status_sub = write();
			switch (_status_sub) {
				case READY:
					tcp_step = 0;
					_status = READY;
					break;	
				case BUSY:
					// intentionally blank
					break;
				case STOP:
					tcp_step = 0;
					_status = STOP;
					break;
				default:
					tcp_step = 0;
					_status = ERROR;
					break;
			}
			break;
		default:
			break;
	}
}

void TcpIpServer::closeSockets()
{
	_status = BUSY;
	switch (tcp_step)
	{
		case 0:
			/**< Schlie�e Client Socket*/
			TcpClose_0.ident = this->clientID;
			TcpClose_0.how = 0;
			TcpClose_0.enable = true;
			TcpClose(&TcpClose_0);
			if (TcpClose_0.status == ERR_OK)
			{
				tcp_step = 1;
			}
			else if (TcpClose_0.status == ERR_FUB_BUSY)
				;
			else
			{
				tcp_step = 255;
				_status = ERROR;
			}
			break;
		case 1:
			/**< Schlie�e Server*/
			TcpClose_0.ident = this->serverID;
			TcpClose_0.how = 0;
			TcpClose_0.enable = true;
			TcpClose(&TcpClose_0);
			if (TcpClose_0.status == ERR_OK)
			{
				_status = READY;
				tcp_step = 0;
			}
			else if (TcpClose_0.status == ERR_FUB_BUSY)
				;
			else
			{
				tcp_step = 255;
				_status = ERROR;
			}
			break;
		default:
			break;
	}  
}

void TcpIpServer::registerListener(Comm *t)
{
	this->transp = t;
}

void TcpIpServer::resetBuffs(void)
{
	this->outBufferLen[0] = 0;
	this->outBufferLen[1] = 0;
	this->inBufferLen = 0;
	for (int i = 0; i < 255 * (MAX_PAYLOAD + 1); ++i) {
		this->outBuffer[0][i] = 0;
		this->outBuffer[1][i] = 0;
		this->inBuffer[i] = 0;
	}
}
	
