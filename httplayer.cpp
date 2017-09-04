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
#include "ipcomlayer.h"
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
	m_poBottomLayer = 0;
}

void CHttpComLayer::closeConnection(){
	m_eConnectionState = e_Disconnected;
	if (0 != m_poBottomLayer) {
		m_poBottomLayer->closeConnection();
	}
}

EComResponse forte::com_infra::CHttpComLayer::openConnection(char *pa_acLayerParameter){
	EComResponse eRetVal = e_InitInvalidId;
	if (0 != strchr(pa_acLayerParameter, ';')) {
		strtok(pa_acLayerParameter, ";");
		char* expectedRspCode = strtok(0, ";");
		mHttpParser.setExpectedRspCode(expectedRspCode);
	}
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
			m_poBottomLayer = new CIPComLayer(this, m_poFb);
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
	char ipParams[kAllocSize]; // address + port for CIPComLayer
	sscanf(mParams, "%99[^/]", ipParams); // Extract address & port from mParams
	EComResponse eRetVal = m_poBottomLayer->openConnection(ipParams, "\0"); // open connection of bottom layer
	if (e_InitOk == eRetVal) {
		m_eConnectionState = e_Connected;
	}
	else {
		m_eConnectionState = e_Disconnected;
	}
	return eRetVal;
}

EComResponse CHttpComLayer::sendData(void *pa_pvData, unsigned int pa_unSize){
  EComResponse eRetVal = e_ProcessDataOk;
  switch (m_poFb->getComServiceType()){
	  case e_Server:
		  // TODO: Currently not implemented.
		  eRetVal = e_Nothing;
		  break;
	  case e_Client:
	  {
		  // Send HTTP request
		  char request[kAllocSize];
		  if (e_GET == m_eRequestType) {
			  mHttpParser.createGetRequest(request, mParams);
		  }
		  else if (e_PUT == m_eRequestType) {
			  TConstIEC_ANYPtr apoSDs = static_cast<TConstIEC_ANYPtr > (pa_pvData);
			  if (!serializeData(apoSDs[0])) {
				  return e_Nothing; // Serializing failed
			  }
			  mHttpParser.createPutRequest(request, mParams, mReqData);
		  }
		  mLastRequest = new char[kAllocSize];
		  memcpy(mLastRequest, request, kAllocSize);
		  openConnection();
		  eRetVal = m_poBottomLayer->sendData(request, strlen(request) + 1);
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
		CIEC_ANY* apoRDs = m_poFb->getRDs();
		switch (m_poFb->getComServiceType()) {
		case e_Server:
			// TODO: Currently not implemented.
			break;
		case e_Client:
		{
			// Close the connection to avoid processInterrupt() call when peer closes connection
			m_poBottomLayer->closeConnection();
			m_eConnectionState = e_Disconnected;
			// Interpret HTTP response and set output status according to success/failure.
			char output[kAllocSize];
			bool success = true;
			if (e_GET == m_eRequestType) {
				success = mHttpParser.parseGetResponse(output, recvData);
			}
			else if (e_PUT == m_eRequestType) {
				success = mHttpParser.parsePutResponse(output, recvData);
			}
			if (success) {
				eRetVal = e_ProcessDataOk;
			}
			else {
				eRetVal = e_ProcessDataSendFailed;
			}
			// Set data output if possible
			if (m_poFb->getNumRD() > 0) {
				apoRDs->fromString(output);
			}
			break;
		}
		case e_Publisher:
		case e_Subscriber:
			// HTTP does not use UDP
			break;
		}
	}
	return eRetVal;
}

bool CHttpComLayer::serializeData(const CIEC_ANY& pa_roCIECData) {
	CIEC_STRING string;
	CIEC_ANY::EDataTypeID eDataType = pa_roCIECData.getDataTypeID();
	switch (eDataType) {
	case CIEC_ANY::e_BOOL:
		string = static_cast<CIEC_STRING>(BOOL_TO_STRING(static_cast<const CIEC_BOOL&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_SINT:
		string = static_cast<CIEC_STRING>(SINT_TO_STRING(static_cast<const CIEC_SINT&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_INT:
		string = static_cast<CIEC_STRING>(INT_TO_STRING(static_cast<const CIEC_INT&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_DINT:
		string = static_cast<CIEC_STRING>(DINT_TO_STRING(static_cast<const CIEC_DINT&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_LINT:
		string = static_cast<CIEC_STRING>(LINT_TO_STRING(static_cast<const CIEC_LINT&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_USINT:
		string = static_cast<CIEC_STRING>(USINT_TO_STRING(static_cast<const CIEC_USINT&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_UINT:
		string = static_cast<CIEC_STRING>(UINT_TO_STRING(static_cast<const CIEC_UINT&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_UDINT:
		string = static_cast<CIEC_STRING>(UDINT_TO_STRING(static_cast<const CIEC_UDINT&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_ULINT:
		string = static_cast<CIEC_STRING>(ULINT_TO_STRING(static_cast<const CIEC_ULINT&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_BYTE:
		string = static_cast<CIEC_STRING>(BYTE_TO_STRING(static_cast<const CIEC_BYTE&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_WORD:
		string = static_cast<CIEC_STRING>(WORD_TO_STRING(static_cast<const CIEC_WORD&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_DWORD:
		string = static_cast<CIEC_STRING>(DWORD_TO_STRING(static_cast<const CIEC_DWORD&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_LWORD:
		string = static_cast<CIEC_STRING>(LWORD_TO_STRING(static_cast<const CIEC_LWORD&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_TIME:
		string = static_cast<CIEC_STRING>(TIME_TO_STRING(static_cast<const CIEC_TIME&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_REAL:
		string = static_cast<CIEC_STRING>(REAL_TO_STRING(static_cast<const CIEC_REAL&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_LREAL:
		string = static_cast<CIEC_STRING>(LREAL_TO_STRING(static_cast<const CIEC_LREAL&>(pa_roCIECData)));
		break;
	case CIEC_ANY::e_WSTRING:
		string = static_cast<CIEC_STRING>(WSTRING_TO_STRING(static_cast<const CIEC_WSTRING&>(pa_roCIECData)));
		break;
	default:
		return false;
	}
	const char* data = string.getValue();
	memcpy(mReqData, data, strlen(data) + 1);
	return true;
}
