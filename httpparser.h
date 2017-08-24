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
* Class for parsing HTTP requests and responses
********************************************************************************/

#ifndef _HTTPIPPARSER_H_
#define _HTTPIPPARSER_H_



namespace forte {

namespace com_infra {

		class CHttpParser {
		public:
			CHttpParser();
			virtual ~CHttpParser();

			/**
			* Generates a HTTP GET request. The request is written to the "dest" input.
			* params: host and path, e.g., 192.168.1.144:80/example/path
			*/
			virtual void createGetRequest(char* dest, const char* params);
			/**
			* Generates a HTTP PUT request. The request is written to the "dest" input.
			* params: host and path, e.g., 192.168.1.144:80/example/path
			* data: Data to be written to the server location.
			*/
			virtual void createPutRequest(char* dest, const char* params, const char* data);
			/**
			* Extracts data from a response to a HTTP GET request stored in src and writes it to dest.
			* If the respones header is not HTTP OK, the header is copied instead.
			* Returns true if the response indicates a successful request.
			*/
			virtual bool parseGetResponse(char* dest, char* src);
			/**
			* Extracts the header from an HTTP response.
			* Returns true if the response indicates a successful request.
			*/
			static bool parsePutResponse(char* dest, char* src);


		private:

			/** Appends the host to the HTTP request */
			static void addHost(char* dest, const char* host);
			/** Appends the ending "\r\n\r\n" to the HTTP request */
			static void addRequestEnding(char* dest);

			/** Size with which to allocate char arrays */
			static const size_t kAllocSize = 512;
		};
	}

}

#endif /* _HTTPIPPARSER_H_ */
