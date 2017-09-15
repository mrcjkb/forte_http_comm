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

#ifndef _HTTPIPCOMLAYER_H_
#define _HTTPIPCOMLAYER_H_

#include "ipcomlayer.h"
#include "httplayer.h"

namespace forte {

  namespace com_infra {

    class CHttpIPComLayer : public CComLayer {
      public:
		CHttpIPComLayer(CComLayer* pa_poUpperLayer, CCommFB* pa_poComFB);
        virtual ~CHttpIPComLayer();

		EComResponse sendData(void *pa_pvData, unsigned int pa_unSize); // top interface, called from top
		EComResponse recvData(const void *pa_pvData, unsigned int pa_unSize);

		EComResponse processInterrupt();

	protected:
		void closeConnection();

	private:
		static void closeSocket(CIPComSocketHandler::TSocketDescriptor *pa_nSocketID);

		/** Connection time out in s */
		const double kTimeOutS;

		EComResponse openConnection(char *pa_acLayerParameter);
		EComResponse openConnection();
		void handledConnectedDataRecv();

		CIPComSocketHandler::TSocketDescriptor m_nSocketID;
		// CIPComSocketHandler::TSocketDescriptor m_nListeningID; //!> to be used by server type connections. there the m_nSocketID will be used for the accepted connection.
		CIPComSocketHandler::TUDPDestAddr m_tDestAddr;
		EComResponse m_eInterruptResp;
		char m_acRecvBuffer[cg_unIPLayerRecvBufferSize];
		unsigned int m_unBufFillSize;
		/** HTTP connection parameters */
		char mParams[CHttpComLayer::kAllocSize];
    };

  }

}

#endif /*_HTTPIPCOMLAYER_H_*/
