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
#include "httpparser.h"
#include <stdio.h>
#include <string.h>

using namespace forte::com_infra;

CHttpParser::CHttpParser() {
	setExpectedRspCode("HTTP/1.1 200 OK");
}

CHttpParser::~CHttpParser(){
}

void CHttpParser::createGetRequest(char* dest, const char* params) {
	char ipParams[kAllocSize]; // address + port
	char path[kAllocSize]; // path for HTTP request
	sscanf(params, "%99[^/]/%511s[^/n]", ipParams, path);
	strcpy(dest, "GET /");
	strcat(dest, path);
	CHttpParser::addHost(dest, ipParams);
	CHttpParser::addRequestEnding(dest);
}

void CHttpParser::createPutRequest(char* dest, const char* params, const char* data) {
	char ipParams[kAllocSize]; // address + port
	char path[kAllocSize]; // path for HTTP request
	sscanf(params, "%99[^/]/%511s[^/n]", ipParams, path);
	strcpy(dest, "PUT /");
	strcat(dest, path);
	CHttpParser::addHost(dest, ipParams);
	strcat(dest, "\r\nContent-type: text/html\r\nContent-length: ");
	char contentLength[kAllocSize];
	sprintf(contentLength, "%zu", strlen(data));
	strcat(dest, contentLength);
	CHttpParser::addRequestEnding(dest);
	strcat(dest, data);
}

void CHttpParser::addHost(char* dest, const char* host) {
	strcat(dest, " HTTP/1.1\r\nHost: ");
	strcat(dest, host);
}

void CHttpParser::addRequestEnding(char* dest) {
	strcat(dest, "\r\n\r\n");
}

bool CHttpParser::parseGetResponse(char* dest, char* src) {
	if (CHttpParser::isOKresponse(src)) {
		// Extract data from HTTP GET respnse char
		char* data = strstr(src, "\r\n\r\n");
		if (0 != data) {
			data += 4;
			sscanf(data, "%1499s[^/n])", dest);
		}
		return true;
	}
	CHttpParser::getHttpResponseCode(dest, src);
	return false;
}

bool CHttpParser::parsePutResponse(char* dest, char* src) {
	if (CHttpParser::isOKresponse(src)) {
		strcpy(dest, mExpectedRspCode);
		return true;
	}
	CHttpParser::getHttpResponseCode(dest, src);
	return false;
}

void CHttpParser::getHttpResponseCode(char* dest, char* src) {
	char* tmp = strtok(src, "\r\n");
	if (tmp != 0) {
		memcpy(dest, tmp, strlen(tmp) + 1);
	}
}

bool CHttpParser::isOKresponse(char* response) {
	return 0 != strstr(response, mExpectedRspCode);
}

void CHttpParser::setExpectedRspCode(const char* rsp) {
	strcpy(mExpectedRspCode, rsp);
}