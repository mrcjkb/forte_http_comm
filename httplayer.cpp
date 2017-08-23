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
* HTTP Com Layer
********************************************************************************/
#include "httplayer.h"
#include "httpiplayer.h"
#include "../../arch/devlog.h"
#include "commfb.h"
// TODO remove #include "comlayersmanager.h"
#include "../../core/datatypes/forte_dint.h"
#include <stdio.h>
#include <string.h>

using namespace forte::com_infra;

CHttpComLayer::CHttpComLayer(CComLayer* pa_poUpperLayer, CCommFB* pa_poComFB) :
    CComLayer(pa_poUpperLayer, pa_poComFB),
	mHttpParser(CHttpParser()){
}

CHttpComLayer::~CHttpComLayer(){
	delete m_poBottomLayer;
	m_poBottomLayer = nullptr;
}

void CHttpComLayer::closeConnection(){
	if (0 != m_poBottomLayer) {
		m_poBottomLayer->closeConnection();
		// Re-open connection and wait for response from peer if the expected response has not been received.
		if (!mRspReceived && mNumRequestAttempts <= kMaxRequestAttempts) {
			openConnection();
			m_poBottomLayer->sendData(mLastRequest, strlen(mLastRequest) + 1);
		}
		else {
			mNumRequestAttempts = 0; // reset number of request attemts and do nothing.
		}
	}
	m_eConnectionState = e_Disconnected;
}

EComResponse forte::com_infra::CHttpComLayer::openConnection(char *pa_acLayerParameter){
	EComResponse eRetVal = e_InitInvalidId;
	switch (m_poFb->getComServiceType()) {
	case e_Server:
		// TODO: Currently not implemented.
		eRetVal = e_ProcessDataInvalidObject;
		break;
	case e_Client:
	{
		// Determine request type according to function block data inputs. If the function block has a data input,
		// it sends a PUT request. Otherwise, it sends a GET request.
		eRetVal = e_InitOk;
		if (m_poFb->getNumSD() > 0) {
			m_eRequestType = e_PUT;
		}
		else {
			m_eRequestType = e_GET;
		}
		// Copy params for later use in sendData()
		memcpy(mParams, pa_acLayerParameter, strlen(pa_acLayerParameter) + 1);
		if (m_poBottomLayer == 0) {
			// TODO remove m_poBottomLayer = CComLayersManager::createCommunicationLayer("ip", this, m_poFb); // create bottom layer
			m_poBottomLayer = new CHttpIPComLayer(this, m_poFb);
			if (m_poBottomLayer == 0) {
				eRetVal = e_InitInvalidId;
				return eRetVal;
			}
		}
		m_eConnectionState = e_Disconnected;
		break;
	}
	case e_Publisher:
	case e_Subscriber:
		// HTTP does not use UDP
		eRetVal = e_ProcessDataInvalidObject;
		break;
	}
	return eRetVal;
}

EComResponse forte::com_infra::CHttpComLayer::openConnection() {
	EComResponse eRetVal = e_InitOk;
	char ipParams[kAllocSize]; // address + port for CIPComLayer
	sscanf(mParams, "%99[^/]", ipParams); // Extract address & port from mParams
	if (e_Disconnected == m_eConnectionState) {
		EComResponse eConnectRsp = m_poBottomLayer->openConnection(ipParams, "\0"); // open connection of bottom layer
		if (e_InitOk != eConnectRsp) { // unsuccessful connection
			return eConnectRsp;
		}
		m_eConnectionState = e_Connected;
	}
	return eRetVal;
}

EComResponse CHttpComLayer::sendData(void *pa_pvData, unsigned int pa_unSize){
  EComResponse eRetVal = e_ProcessDataOk;
  mNumRequestAttempts += 1;
  mRspReceived = false;
  switch (m_poFb->getComServiceType()){
      case e_Server:
		  // TODO: Currently not implemented.
		  eRetVal = e_Nothing;
		  break;
      case e_Client:
	  {
		  // Send HTTP request
		  if (e_GET == m_eRequestType) {
			char request[kAllocSize];
			  mHttpParser.createGetRequest(request, mParams);
			  mLastRequest = new char[kAllocSize];
			  memcpy(mLastRequest, request, kAllocSize);
			  eRetVal = openConnection();
			  eRetVal = m_poBottomLayer->sendData(request, strlen(request) + 1);
		  }
		  else if (e_PUT == m_eRequestType) {
			  // TODO: Implement sending of PUT request
		  }
		  break;
	  }
	  case e_Publisher:
	  case e_Subscriber:
		  // HTTP does not use UDP
		  eRetVal = e_Nothing;
		  break;
  }
  return eRetVal;
}

EComResponse CHttpComLayer::recvData(const void *pa_pvData, unsigned int pa_unSize){
	char* recvData = (char*) pa_pvData;
	EComResponse eRetVal = e_Nothing;
	if (m_poFb != 0) {
		CIEC_ANY *apoRDs = m_poFb->getRDs();
		switch (m_poFb->getComServiceType()) {
		case e_Server:
			// TODO: Currently not implemented.
			break;
		case e_Client:
		{
			// Close the connection to avoid processInterrupt() call
			m_poBottomLayer->closeConnection();
			m_eConnectionState = e_Disconnected;
			if (e_GET == m_eRequestType) {
				char output[kAllocSize];
				mHttpParser.parseGetResponse(output, recvData);
				apoRDs->fromString(output);
				eRetVal = e_ProcessDataOk;
			}
			// TODO handle response to PUT request (return e_ProcessDataOk if it was successful, otherwise e_Nothing)
			break;
		}
		case e_Publisher:
		case e_Subscriber:
			// HTTP does not use UDP
			break;
		}
	}
	mRspReceived = true;
	mNumRequestAttempts = 0; // reset
	return eRetVal;
}