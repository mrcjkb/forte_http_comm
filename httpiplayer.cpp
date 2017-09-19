/*******************************************************************************
*
* BSD 3-Clause License
*
* Copyright (c) 2017, Marc Jakobi
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* * Neither the name of the copyright holder nor the names of its
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
* FORTE License
*
* Copyright (c) 2010-2014 fortiss, TU Wien ACIN and Profactor GmbH.
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*
*
********************************************************************************
* IP Com Layer used by the HTTP Com Layer.
* This class is similar to the CIPComLayer, with some changes for performance improvement.
* Opening the connection, sending and receiving data all occurs in the sendData() method.
* This is done because mose HTTP servers have high requirements for speed that currently cannot be
* met by FORTE's standard CIPComLayer.
********************************************************************************/
#include "httpiplayer.h"
#include "../../arch/devlog.h"
#include "commfb.h"
#include "comlayersmanager.h"
#include "../../core/datatypes/forte_dint.h"
#include <stdio.h>
#include <string.h>

using namespace forte::com_infra;

CHttpIPComLayer::CHttpIPComLayer(CComLayer* pa_poUpperLayer, CCommFB* pa_poComFB) :
	CComLayer(pa_poUpperLayer, pa_poComFB),
	kTimeOutS(20),
	m_nSocketID(CIPComSocketHandler::scm_nInvalidSocketDescriptor),
	// m_nListeningID(CIPComSocketHandler::scm_nInvalidSocketDescriptor),
	m_eInterruptResp(e_Nothing),
	m_unBufFillSize(0){
}

CHttpIPComLayer::~CHttpIPComLayer() {
}

void CHttpIPComLayer::closeConnection() {
	closeSocket(&m_nSocketID);
	// closeSocket(&m_nListeningID);
	m_eConnectionState = e_Disconnected;
}

EComResponse CHttpIPComLayer::sendData(void *pa_pvData, unsigned int pa_unSize) {
	if (0 != m_poFb) {
		switch (m_poFb->getComServiceType()) {
		case e_Server:
			// TODO Implement HTTP server behaviour
			// if (0
			//	>= CIPComSocketHandler::sendDataOnTCP(m_nSocketID, static_cast<char*>(pa_pvData), pa_unSize)) {
			//	closeSocket(&m_nSocketID);
			//	m_eConnectionState = e_Listening;
			//	eRetVal = e_InitTerminated;
			//}
			break;
		case e_Client:
		{
			const char* requestCache = static_cast<char*>(pa_pvData);
			time_t start = time(0); // for timeout
			time_t endWait = start + kTimeOutS;
			// Loop runs until the end of the time out period or until the HTTP response has been received completely
			bool activeAttempt = true;
			while (start < endWait && activeAttempt) {
				m_unBufFillSize = 0;
				char request[CHttpComLayer::kAllocSize];
				memcpy(request, requestCache, CHttpComLayer::kAllocSize);
				if (e_InitOk != openConnection()) {
					activeAttempt = false;
					m_eInterruptResp = e_ProcessDataSendFailed;
					DEVLOG_INFO("Opening HTTP connection failed\n");
				} else {
					DEVLOG_INFO("Sending request on TCP\n");
					if (0 >= CIPComSocketHandler::sendDataOnTCP(m_nSocketID, request, pa_unSize)) {
						m_eInterruptResp = e_ProcessDataSendFailed;
						activeAttempt = false;
						DEVLOG_INFO("Sending request on TCP failed\n");
					}
					else {
						// Wait for peer to close connection because it may not contain Content-length in header
						// and may not support chunked encoding
						// TODO: Implement detection of content-length and/or chunk detection to break out of loop early
						while (e_Connected == m_eConnectionState && start < endWait) {
							handledConnectedDataRecv();
							start = time(0);
						}
						m_eInterruptResp = m_poTopLayer->recvData(m_acRecvBuffer, m_unBufFillSize);
						if (e_ProcessDataRecvFaild != m_eInterruptResp) {
							DEVLOG_INFO("HTTP parsed successfully\n");
							activeAttempt = false;
						}
					}
				}
			}
			if (start >= endWait) { // Timeout?
				m_eInterruptResp = e_ProcessDataSendFailed;
				DEVLOG_INFO("HTTP response Timeout exceeded\n");
			}
			// Close connection to deinitialize socket
			EComResponse eBackup = m_eInterruptResp;
			closeConnection(); // This sets m_eInterruptedResp to e_InitTerminated
			m_eInterruptResp = eBackup;
			break;
		}
		case e_Publisher:
		case e_Subscriber:
			// Do nothing as HTTP does not use UDP
			break;
		}
	}
	// Ensure event output is sent (sendData does not trigger e_ProcessDataOk events for clients/servers/subscribers)
	DEVLOG_INFO("Interrupting CommFB\n");
	m_poFb->interruptCommFB(this);
	DEVLOG_INFO("Forcing CommFB CNF event\n");
	CEventChainExecutionThread *poEventChainExecutor = m_poFb->getEventChainExecutor();
	m_poFb->receiveInputEvent(cg_nExternalEventID, *poEventChainExecutor);
	DEVLOG_INFO("CommFB CNF event executed\n");
	// Make sure sendData() does not trigger additional INIT- event in case of failure
	return e_Nothing;
}

EComResponse CHttpIPComLayer::processInterrupt() {
	return m_eInterruptResp;
}

EComResponse CHttpIPComLayer::recvData(const void*, unsigned int) {
	// Data is received in the sendData method.
	return e_Nothing;
}

EComResponse CHttpIPComLayer::openConnection(char *pa_acLayerParameter) {
	// For HTTP client: cache parameters and return INIT+ event
	mParams[0] = 0;
	strncpy(mParams, pa_acLayerParameter, strlen(pa_acLayerParameter) + 1);
	return e_InitOk;
}

EComResponse CHttpIPComLayer::openConnection() {
	// Copy params to new char* and perform the same actions as in CIPComLayer
	char pa_acLayerParameter[CHttpComLayer::kAllocSize];
	strncpy(pa_acLayerParameter, mParams, strlen(mParams) + 1);
	EComResponse eRetVal = e_InitInvalidId;
	char *acPort = strchr(pa_acLayerParameter, ':');
	if (0 != acPort) {
		*acPort = '\0';
		++acPort;

		TForteUInt16 nPort = static_cast<TForteUInt16>(forte::core::util::strtoul(acPort, 0, 10));

		CIPComSocketHandler::TSocketDescriptor nSockDes =
			CIPComSocketHandler::scm_nInvalidSocketDescriptor;
		m_eConnectionState = e_Connected;

		switch (m_poFb->getComServiceType()) {
		case e_Server:
			// TODO: HTTP server has not yet been implemented
			//nSockDes = m_nListeningID =
			//	CIPComSocketHandler::openTCPServerConnection(pa_acLayerParameter, nPort);
			//m_eConnectionState = e_Listening;
			break;
		case e_Client:
			nSockDes = m_nSocketID =
				CIPComSocketHandler::openTCPClientConnection(pa_acLayerParameter, nPort);
			break;
		case e_Publisher:
		case e_Subscriber:
			break;
		}

		if (CIPComSocketHandler::scm_nInvalidSocketDescriptor != nSockDes) {
			eRetVal = e_InitOk;
		}
		else {
			m_eConnectionState = e_Disconnected;
		}
	}
	return eRetVal;
}

void CHttpIPComLayer::closeSocket(CIPComSocketHandler::TSocketDescriptor *pa_nSocketID) {
	if (CIPComSocketHandler::scm_nInvalidSocketDescriptor != *pa_nSocketID) {
		CIPComSocketHandler::getInstance().removeComCallback(*pa_nSocketID);
		CIPComSocketHandler::closeSocket(*pa_nSocketID);
		*pa_nSocketID = CIPComSocketHandler::scm_nInvalidSocketDescriptor;
	}
}

void CHttpIPComLayer::handledConnectedDataRecv() {
	if ((cg_unIPLayerRecvBufferSize - m_unBufFillSize) <= 0) {
		// If buffer is full, clear and return
		m_unBufFillSize = 0;
		m_eInterruptResp = e_ProcessDataRecvFaild;
		DEVLOG_INFO("HTTP recv buffer full\n");
		return;
	}
	if (CIPComSocketHandler::scm_nInvalidSocketDescriptor != m_nSocketID) {
		// TODO: sync buffer and bufFillSize
		int nRetVal = 0;
		switch (m_poFb->getComServiceType()) {
		case e_Server:
		case e_Client:
			DEVLOG_INFO("Attempting to receive data from TCP\n");
			nRetVal =
				CIPComSocketHandler::receiveDataFromTCP(m_nSocketID, &m_acRecvBuffer[m_unBufFillSize], cg_unIPLayerRecvBufferSize
					- m_unBufFillSize);
			break;
		case e_Publisher:
		case e_Subscriber:
			// Do nothing because HTTP does not use UDP
			break;
		}
		switch (nRetVal) {
		case 0:
			m_eInterruptResp = e_InitTerminated;
			DEVLOG_INFO("Connection closed by peer\n");
			closeConnection();
			if (e_Server == m_poFb->getComServiceType()) {
				//Move server into listening mode again
				m_eConnectionState = e_Listening;
			}
			break;
		case -1:
			m_eInterruptResp = e_ProcessDataRecvFaild;
			DEVLOG_INFO("Failed to receive data from TCP\n");
			break;
		default:
			//we successfully received data
			DEVLOG_INFO("Successfully received data from TCP\n");
			m_unBufFillSize += nRetVal;
			m_eInterruptResp = e_ProcessDataOk;
			break;
			}
		}
}