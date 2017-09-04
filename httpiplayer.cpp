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
* This class is exactly the same as the CIPComLayer, but the processInterrupt() method
* is overloaded to allow server-side disconnections without notifying the function block.
********************************************************************************/
#include "httpiplayer.h"
#include "../../arch/devlog.h"
#include "commfb.h"
#include "comlayersmanager.h"
#include "../../core/datatypes/forte_dint.h"
#include <stdio.h>
#include <string.h>

using namespace forte::com_infra;

CHttpIPComLayer::CHttpIPComLayer(CComLayer* pa_poUpperLayer, CCommFB* pa_poComFB, char* pa_acLayerParameter) :
	CIPComLayer(pa_poUpperLayer, pa_poComFB),
	mNumRequestAttempts(0) {
	// Copy params for later use in sendData()
	memcpy(mParams, pa_acLayerParameter, strlen(pa_acLayerParameter) + 1);
}

CHttpIPComLayer::~CHttpIPComLayer(){
}

EComResponse forte::com_infra::CHttpIPComLayer::sendData(void * pa_pvData, unsigned int pa_unSize) {
	mRspReceived = false;
	mNumRequestAttempts += 1;
	char* request = static_cast<char*> (pa_pvData);
	memcpy(mLastRequest, request, kAllocSize);
	openConnection();
	return CIPComLayer::sendData(pa_pvData, pa_unSize);
}

EComResponse CHttpIPComLayer::recvData(const void *pa_pvData, unsigned int) {
	mRspReceived = true;
	mNumRequestAttempts = 0;
	EComResponse eRetVal = e_Nothing;
	eRetVal = CIPComLayer::recvData(pa_pvData, 0);
	while (!mRspReceived && e_ProcessDataOk != eRetVal && mNumRequestAttempts <= kMaxRequestAttempts) {
		resetConnection();
		CIPComLayer::recvData(mRecvBuffer, 0);
	}
	return eRetVal;
}

EComResponse CHttpIPComLayer::processInterrupt() {
	EComResponse eRetVal = CIPComLayer::processInterrupt();
	// Close the connection to avoid further processInterrupt() calls
	closeConnection();
	while (!mRspReceived && e_ProcessDataOk != eRetVal && mNumRequestAttempts <= kMaxRequestAttempts) {
		eRetVal = e_Nothing;
		resetConnection();
		CIPComLayer::recvData(mRecvBuffer, 0);
	}
	if (0 == eRetVal) {
		eRetVal = e_Nothing;
	}
	return eRetVal;
}

void forte::com_infra::CHttpIPComLayer::resetConnection() {
	m_eConnectionState = e_Disconnected;
	closeConnection();
	// Re-open connection and wait for response from peer if the expected response has not been received.
	if (!mRspReceived && mNumRequestAttempts <= kMaxRequestAttempts && 0 != mLastRequest) {
		EComResponse eRetVal = sendData(mLastRequest, strlen(mLastRequest) + 1);
		if (e_ProcessDataOk != eRetVal) {
			Sleep(0);
		}
	}
	else {
		mNumRequestAttempts = 0; // reset number of request attemts and do nothing.
	}
}

EComResponse forte::com_infra::CHttpIPComLayer::openConnection() {
	char ipParams[kAllocSize]; // address + port for CIPComLayer
	memcpy(ipParams, mParams, kAllocSize); // Create copy so that data is not lost
	EComResponse eRetVal = CComLayer::openConnection(ipParams, "\0"); // open connection of bottom layer
	if (e_InitOk == eRetVal) { // unsuccessful connection
		m_eConnectionState = e_Connected;
	}
	return eRetVal;
}
