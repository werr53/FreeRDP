/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Security Support Provider Interface (SSPI) Tests
 *
 * Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <freerdp/freerdp.h>
#include <freerdp/utils/stream.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/hexdump.h>

#include "test_sspi.h"
#include <freerdp/auth/sspi.h>

#define NTLM_PACKAGE_NAME		"NTLM"

int init_sspi_suite(void)
{
	sspi_GlobalInit();
	return 0;
}

int clean_sspi_suite(void)
{
	sspi_GlobalFinish();
	return 0;
}

int add_sspi_suite(void)
{
	add_test_suite(sspi);

	add_test_function(EnumerateSecurityPackages);
	add_test_function(QuerySecurityPackageInfo);
	add_test_function(AcquireCredentialsHandle);
	add_test_function(InitializeSecurityContext);

	return 0;
}

void test_EnumerateSecurityPackages(void)
{
	uint32 cPackages;
	SECURITY_STATUS status;
	SEC_PKG_INFO* pPackageInfo;

	status = EnumerateSecurityPackages(&cPackages, &pPackageInfo);

	if (status == SEC_E_OK)
	{
		int index;

		printf("\nEnumerateSecurityPackages (%d):\n", cPackages);

		for (index = 0; index < cPackages; index++)
		{
			printf("\"%s\", \"%s\"\n",
					pPackageInfo[index].Name, pPackageInfo[index].Comment);
		}
	}

	FreeContextBuffer(pPackageInfo);
}

void test_QuerySecurityPackageInfo(void)
{
	SECURITY_STATUS status;
	SEC_PKG_INFO* pPackageInfo;

	status = QuerySecurityPackageInfo("NTLM", &pPackageInfo);

	if (status == SEC_E_OK)
	{
		printf("\nQuerySecurityPackageInfo:\n");
		printf("\"%s\", \"%s\"\n", pPackageInfo->Name, pPackageInfo->Comment);
	}
}

const char* test_User = "User";
const char* test_Domain = "Domain";
const char* test_Password = "Password";

void test_AcquireCredentialsHandle(void)
{
	SECURITY_STATUS status;
	CRED_HANDLE credentials;
	SEC_TIMESTAMP expiration;
	SEC_AUTH_IDENTITY identity;
	SECURITY_FUNCTION_TABLE* table;
	SEC_PKG_CREDENTIALS_NAMES credential_names;

	table = InitSecurityInterface();

	identity.User = (uint16*) xstrdup(test_User);
	identity.UserLength = sizeof(test_User);
	identity.Domain = (uint16*) xstrdup(test_Domain);
	identity.DomainLength = sizeof(test_Domain);
	identity.Password = (uint16*) xstrdup(test_Password);
	identity.PasswordLength = sizeof(test_Password);
	identity.Flags = SEC_AUTH_IDENTITY_ANSI;

	status = table->AcquireCredentialsHandle(NULL, NTLM_PACKAGE_NAME,
			SECPKG_CRED_OUTBOUND, NULL, &identity, NULL, NULL, &credentials, &expiration);

	if (status == SEC_E_OK)
	{
		status = table->QueryCredentialsAttributes(&credentials, SECPKG_CRED_ATTR_NAMES, &credential_names);

		if (status == SEC_E_OK)
		{
			printf("\nQueryCredentialsAttributes: %s\n", credential_names.sUserName);
		}
	}
}

void test_InitializeSecurityContext(void)
{
	uint32 cbMaxLen;
	uint32 fContextReq;
	void* output_buffer;
	CTXT_HANDLE context;
	uint32 pfContextAttr;
	SECURITY_STATUS status;
	CRED_HANDLE credentials;
	SEC_TIMESTAMP expiration;
	SEC_PKG_INFO* pPackageInfo;
	SEC_AUTH_IDENTITY identity;
	SECURITY_FUNCTION_TABLE* table;
	SEC_BUFFER* p_sec_buffer;
	SEC_BUFFER output_sec_buffer;
	SEC_BUFFER_DESC output_sec_buffer_desc;

	table = InitSecurityInterface();

	status = QuerySecurityPackageInfo(NTLM_PACKAGE_NAME, &pPackageInfo);

	printf("pPackageInfo: 0x%08X ppPackageInfo:0x%08X\n", pPackageInfo, &pPackageInfo);

	if (status != SEC_E_OK)
	{
		printf("QuerySecurityPackageInfo status: 0x%08X\n", status);
		return;
	}

	cbMaxLen = pPackageInfo->cbMaxToken;

	identity.User = (uint16*) xstrdup(test_User);
	identity.UserLength = sizeof(test_User);
	identity.Domain = (uint16*) xstrdup(test_Domain);
	identity.DomainLength = sizeof(test_Domain);
	identity.Password = (uint16*) xstrdup(test_Password);
	identity.PasswordLength = sizeof(test_Password);
	identity.Flags = SEC_AUTH_IDENTITY_ANSI;

	status = table->AcquireCredentialsHandle(NULL, NTLM_PACKAGE_NAME,
			SECPKG_CRED_OUTBOUND, NULL, &identity, NULL, NULL, &credentials, &expiration);

	if (status != SEC_E_OK)
	{
		printf("AcquireCredentialsHandle status: 0x%08X\n", status);
		return;
	}

	fContextReq = ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_DELEGATE;

	output_buffer = xmalloc(cbMaxLen);

	output_sec_buffer_desc.ulVersion = 0;
	output_sec_buffer_desc.cBuffers = 1;
	output_sec_buffer_desc.pBuffers = &output_sec_buffer;

	output_sec_buffer.cbBuffer = cbMaxLen;
	output_sec_buffer.BufferType = SECBUFFER_TOKEN;
	output_sec_buffer.pvBuffer = output_buffer;

	status = table->InitializeSecurityContext(&credentials, NULL, NULL, fContextReq, 0, 0, NULL, 0,
			&context, &output_sec_buffer_desc, &pfContextAttr, &expiration);

	if (status != SEC_I_CONTINUE_NEEDED)
	{
		printf("InitializeSecurityContext status: 0x%08X\n", status);
		return;
	}

	printf("cBuffers: %d ulVersion: %d\n", output_sec_buffer_desc.cBuffers, output_sec_buffer_desc.ulVersion);

	p_sec_buffer = &output_sec_buffer_desc.pBuffers[0];

	printf("BufferType: 0x%04X cbBuffer:%d\n", p_sec_buffer->BufferType, p_sec_buffer->cbBuffer);

	freerdp_hexdump((uint8*) p_sec_buffer->pvBuffer, p_sec_buffer->cbBuffer);

	table->FreeCredentialsHandle(&credentials);

	FreeContextBuffer(pPackageInfo);
}
















