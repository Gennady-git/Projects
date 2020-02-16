// Crypto.h                                                (c) DIR, 2019
//
//              ���������� ������
// CCrypto - ��������� ������� ������������
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include <Wincrypt.h>

// ���������� ������� �������������� ���������
#define _TRACE
//#undef _TRACE

#define  NAMEMAXSIZE  64    //������ ������ ��� ����� ���������� ������
#define  KEYBUFFSIZE  150   //������ ������ ��� ������ ����������
#define  SIGNBUFSIZE  150   //������ ������ ��� �������� ������� (���-����)

// ������������ ��� "��� ����� ����������"
enum EKeyKind {
  EKK_Signature = 1,    //signature key
  EKK_Exchange,         //exchange key
};

// ���������� ������ CCrypto - ��������� ������� ������������
//
class CCrypto
{
  static TCHAR  m_szContainerName[NAMEMAXSIZE]; //the name of the container
  HCRYPTPROV  m_hCryptProv; //handle for the cryptographic provider context
  HCRYPTKEY   m_hSignKey;   //public/private signature key pair handle
  HCRYPTKEY   m_hExchKey;   //public/private exchange key pair handle
  BYTE  m_chSignKey[KEYBUFFSIZE];   //�������� ���� ����������
  BYTE  m_chSignature[SIGNBUFSIZE]; //�������� ������� ������
public:
  // ����������� � ����������
  CCrypto(LPTSTR pszContainerName = NULL);
  ~CCrypto();

  // ������� ������
  // ���������������� ������
  bool  Initialize();
  // ���������, ��������������� �� ������
  bool  IsInitialized() {
	return (m_hCryptProv != NULL &&
            m_hSignKey != NULL && m_hExchKey != NULL);
  }
  // ���� ���������� ��������� ���������� ������������
  HCRYPTPROV  AcquireContext();
  //HCRYPTPROV  CryptoProviderContext() { return m_hCryptProv; }
  // ���� ������������� ��������� ����� ����������
  BYTE * ExportKey(EKeyKind eKeyKind, DWORD &dwSize);
  // ���� ������� (���-���) ����� ������
  BYTE * GetDataSignature(BYTE *pchData, DWORD dwDataLeng, DWORD &dwSignLeng);
  // ����: ���� ������, �������� ���� ���������� � ������� (���-���). 
  //���������, ����� �� ������� (���-���) ����� ������.
  static bool  VerifySignature(BYTE *pchData, DWORD dwDataLeng,
                               BYTE *pchPubSignKey, DWORD dwKeyLeng,
                               BYTE *pchSignature, DWORD dwSignLeng);
  // ���������� ������
private:
  HCRYPTKEY  GetSignatureKey();
  HCRYPTKEY  GetExchangeKey();
};

// End of class CCrypto declaration ------------------------------------
