// FileBase.cpp                                            (c) DIR, 2019
//
//              ���������� ������� �������� ���� ������
//
// CFileBase        - ������� ����� ��� �������� �������� "���� ������"
// CUserRegister    - ������� �������������
// CBalanceRegister - ������� ������� ��������
// CTransRegister   - ������� ����������
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include "FileBase.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif

//      ���������� ������ CFileBase
////////////////////////////////////////////////////////////////////////
//      ������������� ����������� ��������� ������
//
//static ������ ��������� �� �������
TCHAR * CFileBase::m_sErrMes[] = {
/* 0 */ _T("OK"),
/* 1 */ _T("CFileBase: ������ �������� �����"),
/* 2 */ _T("CFileBase: ������ ������ �� �����"),
/* 3 */ _T("CFileBase: ���������������� ������ �����"),
/* 4 */ _T("CFileBase: ������ ������ � ����"),
/* 5 */ _T("CFileBase: ������ �������� �����"),
/* 6 */ _T("CUserRegister: ������� ������������� �� ��������"),
/* 7 */ _T("CBlockChain: ����� ������������� ���������� ��������� ������� �������"),
//* 8 * "",
//* 9 * "",
};

// ����������� � ����������
CFileBase::CFileBase(CCrypto *paCrypt, LPTSTR pszFileNameBase,
                     WORD nItemSize)
{
  m_paCrypt = paCrypt;  //��������� ������� ������������
  Initialize(pszFileNameBase, nItemSize);
  m_nItemCount = 0;     //���������� ��������� ������
  //m_iOwner = 0xFFFF;    ������ ������������-��������� ������ �������
  //m_nOwner = 0;         ����� ������������-��������� ������ �������
  m_pFile = NULL;       //��������� �����
  m_bLoaded = false;    //���� "������ �������� �� �����"
  m_bChanged = false;   //���� "������ ������"
  m_nErr = 0;           //��� ������
}
CFileBase::~CFileBase(void)
{
  if (m_pFile) {
    fclose(m_pFile);  m_pFile = NULL;
  }
} // CFileBase::~CFileBase()

// ������� ������
// ������ ������� ������� ��� ���������� �������� ��
void  CFileBase::SetFolder(LPTSTR sFolder/* = NULL*/)
{
  LPTSTR psz = (sFolder != NULL) ? sFolder : FOLDERDEFLT;
  int  len = min(_tcslen(psz), FOLDERNMSIZ-1);
  memcpy(m_sFolder, psz, len * sizeof(TCHAR));
  // ��������� ������ (��� ������� ���� ����)
  memset(m_sFolder+len, 0, (FOLDERNMSIZ-len) * sizeof(TCHAR));
} // CFileBase::SetFolder()

//virtual ������ �������� ����� �� ��� ���������� ������ ������������
bool  CFileBase::Load()
{
  TCHAR  sFilePath[FOLDERNMSIZ+FILENAMESIZ];
  size_t  nSize = FOLDERNMSIZ + FILENAMESIZ;
  // ���� ������ ������������ �����
  FillFilePath(sFilePath, nSize);
  m_nErr = 0;
  // ��������� � ��������� ��������� �����
  errno_t  err = _tfopen_s(&m_pFile, sFilePath, _T("r+b"));
  if (err != 0)
    { m_nErr = 1;  return false; }  //������ �������� �����
  // ��������� �� 1 ����� FILESIGNSIZ ���:
  size_t  nRead = fread(m_sFileSign, 1, FILESIGNSIZ, m_pFile);
  if (nRead != FILESIGNSIZ)
    { m_nErr = 2;  return false; }  //������ ������ �����
  if (!CheckSignature())
    { m_nErr = 3;  return false; }  //������������ ���������
  // ��������� m_nItemSize, m_nItemCount
  nRead = fread(&m_nItemSize, 1, sizeof(m_nItemSize), m_pFile);
  if (nRead != sizeof(m_nItemSize))
    { m_nErr = 2;  return false; }  //������ ������ �����
  nRead = fread(&m_nItemCount, 1, sizeof(m_nItemCount), m_pFile);
  if (nRead != sizeof(m_nItemCount))
    { m_nErr = 2;  return false; }  //������ ������ �����
  return  true;
} // CFileBase::Load()

//virtual  ��������� ������ ������� � ����� ��
bool  CFileBase::Save(bool bDoSave/* = false*/)
{
  TCHAR  sFilePath[FOLDERNMSIZ+FILENAMESIZ];
  size_t  nSize = FOLDERNMSIZ + FILENAMESIZ;
  // ���� ������ ������������ �����
  FillFilePath(sFilePath, nSize);
  m_nErr = 0;
  // ������� ���� �� ������
  errno_t  err = _tfopen_s(&m_pFile, sFilePath, _T("w+b"));
  if (err != 0)
    { m_nErr = 1;  return false; }  //������ �������� �����
  // �������� ��������� �����
  InitSignature();
  // �������� �� 1 ����� FILESIGNSIZ ���:
  size_t  nWrit = fwrite(m_sFileSign, 1, FILESIGNSIZ, m_pFile);
  if (nWrit != FILESIGNSIZ)
    { m_nErr = 4;  return false; }  //������ ������ �����
  // �������� m_nItemSize, m_nItemCount
  nWrit = fwrite(&m_nItemSize, 1, sizeof(m_nItemSize), m_pFile);
  if (nWrit != sizeof(m_nItemSize))
    { m_nErr = 4;  return false; }  //������ ������ �����
  nWrit = fwrite(&m_nItemCount, 1, sizeof(m_nItemCount), m_pFile);
  if (nWrit != sizeof(m_nItemCount))
    { m_nErr = 4;  return false; }  //������ ������ �����
  return true;
} // CFileBase::Save()

//virtual  �������� ������� ������ � ������ ��
bool  CFileBase::Add(void *pItem, WORD nLeng/* = 0*/)
{
  return true;
} // CFileBase::Add()

//virtual  ����� ������� ������ � ������� ��
WORD  CFileBase::Find(void *pItem, WORD nLeng/* = 0*/)
{
  return 0;
} // CFileBase::Find()

//protected:
//virtual ������������� �����. ���� ������� ��������� ����� ��.
char * CFileBase::GetSignaturePattern(WORD &nLeng)
//nLeng[out] - ������ ��������� ����� � ������
{
  static char *sFileSign = FILESIGNBAS;
  nLeng = strlen(sFileSign);    //��� ����� ������������ 0
  return sFileSign;
}
char * CFileBase::GetSignaturePattern(WORD &nLeng, char *psIgn2)
{
  static char  sFileSign[FILESIGNSIZ];  //����� ��� ��������� �����
  WORD  leng;
  char *psIgn = CFileBase::GetSignaturePattern(leng);
  if (leng > FILESIGNSIZ-1)  leng = FILESIGNSIZ-1;
  memcpy(sFileSign, psIgn, leng);       //����������� 1-� �����
  WORD  nRest = FILESIGNSIZ - leng,     //�������� ���� ���������
        leng2 = strlen(psIgn2);         //����� 2-� �����
  if (leng2 > nRest-1)  leng2 = nRest-1;
  memcpy(sFileSign+leng, psIgn2, leng2); //����������� 2-� �����
  leng += leng2;  sFileSign[leng] = '\0';
  nLeng = leng;
  return sFileSign;
} // CFileBase::GetSignaturePattern()

// ��������� ���� ��������� ����� ��
void  CFileBase::InitSignature()
{
  WORD  nLeng;
  char *psIgn = GetSignaturePattern(nLeng);
  if (nLeng > FILESIGNSIZ-1)  nLeng = FILESIGNSIZ-1;
  memcpy(m_sFileSign, psIgn, nLeng);
  // ��������� ������ (��� ������� ���� ����)
  memset(m_sFileSign+nLeng, 0, FILESIGNSIZ-nLeng);
} // CFileBase::InitSignature()

//virtual  ��������� ���������, ����������� �� ����� ��
bool  CFileBase::CheckSignature()
{
  WORD  nLeng;
  char *psIgn = GetSignaturePattern(nLeng);
  int  n = strncmp(m_sFileSign, psIgn, nLeng);
  return (n == 0);
} // CFileBase::CheckSignature()

//private:
// ������ ������� ��������� �������
void  CFileBase::Initialize(LPTSTR sFileName, WORD nItemSize)
{
  // ������� ������ ����� �����
  int len = min(_tcslen(sFileName), FILENAMESIZ-1); //����� ��������
  memcpy(m_sFileName, sFileName, len * sizeof(TCHAR));
  // ��������� ������ (��� ������� ���� ����)
  memset(m_sFileName+len, 0, (FILENAMESIZ-len) * sizeof(TCHAR));

  SetFolder();  //������� ��� �������� �������� �� �� ���������
  // ������ ������������-���������, ������������ ��� �����������:
  m_iOwner = 0xFFFF; //������ ������������-��������� � ������� �������������
  m_nOwner = 0;      //����� ������������-��������� ������������ �������
  m_nItemSize = nItemSize;  //������ �������� ������
} // CFileBase::Initialize()

// ��������� � ����� ������ ������������ ����� ��
void  CFileBase::FillFilePath(LPTSTR pszFilePath, WORD nSize)
//pszFilePath - ����� ��� ������ ������������
//nSize       - ������ ������ � ��������
{
  WORD  iLast = _tcslen(m_sFileName) - 1;
  if (m_sFileName[iLast] == _T('_'))
    // ��� ����� �������� ����� ������������
    swprintf_s(pszFilePath, nSize,
               _T("%s%s%02d.dat"), m_sFolder, m_sFileName, m_nOwner);
  else
    swprintf_s(pszFilePath, nSize,
               _T("%s%s.dat"), m_sFolder, m_sFileName);
} // CFileBase::FillFilePath()

// End of class CFileBase definition -----------------------------------


//      ���������� ������ CUserRegister
////////////////////////////////////////////////////////////////////////
//      ������������� ����������� ��������� ������
//

// ����������� � ����������
CUserRegister::CUserRegister(CCrypto *paCrypt)
             : CFileBase(paCrypt, _T("UserList"), sizeof(TUser))
{
}
CUserRegister::~CUserRegister(void)
{
  bool b = Save();
  ClearUserArray();
} // CUserRegister::~CUserRegister()

// ������� ������
// ---------------------------------------------------------------------
// ��������� ��������������� ������ ������������ � ���� ��� ������
bool  CUserRegister::AuthorizationCheck(LPTSTR pszLogin,
                                        LPTSTR pszPasswd, WORD &iUser)
{
  if (m_nItemCount == 0) { m_nErr = 6;  return false; }
  if (_tcslen(pszLogin) == 0 || _tcslen(pszPasswd) == 0)  return false;
  TUser *ptUser;  int nCmp;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptUser = m_aUsers[i];
    nCmp = _tcscmp(ptUser->_szLogin, pszLogin);
    if (nCmp == 0) {
      // Login ������ - �������� ������
      nCmp = _tcscmp(ptUser->_szPasswd, pszPasswd);
      if (nCmp == 0) {
        //m_iOwner = i;  m_nOwner = ptUser->GetUserOrd();
        //SetOwner(i);
        iUser = i;  return true;
      }
    }
  }
  return false; //�� ������
} // CUserRegister::AuthorizationCheck()

// ������ ������, � �� ���� � ����� ������������-��������� �������
bool  CUserRegister::SetOwner(WORD iUser)
{
  bool b = (0 <= iUser && iUser < m_nItemCount);
  if (b) {
    m_iOwner = iUser;
    m_nOwner = m_aUsers[iUser]->GetUserOrd();
  }
  return b;
} // SetOwner()

// ���� ��� ������������ �� ��� �������
bool  CUserRegister::UserName(WORD iUser, LPTSTR pszName)
{
  TUser *ptUser = GetAt(iUser);
  if (ptUser == NULL)  return false;
  CString s(ptUser->_sName);    //��������������� ANSI->Unicode
  _tcscpy_s(pszName, NAMESBUFSIZ, s.GetBuffer());
  return true;
} // CUserRegister::UserName()

// ���� ������ ������������ �� ��� ����� ���� userNN
WORD  CUserRegister::GetUserByName(LPTSTR pchName, WORD nOffset/* = 4*/)
{
  // ������ ������������� ������ ������������
  DWORD iUser;  _stscanf_s(pchName+nOffset, _T("%d"), &iUser);
  if (nOffset > 0)  iUser--;
  return (WORD)iUser;
/*  TUser *ptUser;  int nCmp;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptUser = m_aUsers[i];
    {
      CString sName(ptUser->_sName);
      if (sName.Compare(pchName) == 0)  return i;
    }
  }
  return 0xFFFF;    //�� ������ */
} // CUserRegister::GetUserByName()

// ������������� ������� ������������� � ������
void  CUserRegister::GenerateUserArray(WORD nUsers)
{
  TUser *ptUser;  WORD i, k;
  for (i = 1; i <= nUsers; i++) {
    ptUser = GenerateUser(i);
    if (ptUser) {
      k = m_aUsers.Add(ptUser);  m_nItemCount++;
    }
  }
  m_bChanged = true;
} // CUserRegister::GenerateUserArray()

//virtual ��������� ���� ��
bool  CUserRegister::Load()
{
  bool b = CFileBase::Load();
  if (b)
  {
    TUser *ptUser;  size_t nRead;  WORD i, k;
    for (i = 0; i < m_nItemCount; i++) {
      ptUser = new TUser;
      nRead = fread(ptUser, 1, sizeof(TUser), m_pFile);
      if (nRead != sizeof(TUser))
        { m_nErr = 2;  b = false;  break; }   //������ ������ �����
      k = m_aUsers.Add(ptUser);
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //������ �������� �����
    m_pFile = NULL;
  }
  m_bLoaded = b;  m_bChanged = false;
  return b;
} // CUserRegister::Load()

//virtual  ��������� ������ ������� � ����� ��
bool  CUserRegister::Save(bool bDoSave/* = false*/)
{
  if (!bDoSave && !m_bChanged)  return false;
  bool b = CFileBase::Save();
  if (b)
  {
    TUser *ptUser;  size_t nWrit;
    for (WORD i = 0; i < m_nItemCount; i++) {
      ptUser = m_aUsers[i];
      nWrit = fwrite(ptUser, 1, sizeof(TUser), m_pFile);
      if (nWrit != sizeof(TUser))
        { m_nErr = 4;  b = false;  break; }   //������ ������ �����
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //������ �������� �����
    m_pFile = NULL;
  }
  m_bChanged = !b;
  return b;
} // CUserRegister::Save()

//virtual  �������� ������� ������ � ������ ��
bool  CUserRegister::Add(void *pItem, WORD nLeng/* = 0*/)
{
  return true;
} // CUserRegister::Add()

// ����� ������ ������������ � ��������
//virtual
WORD  CUserRegister::Find(void *pItem, WORD nLeng/* = 0*/)
//pItem - ������� ������ (pchLogHash - ���-��� Login'�)
{
#ifdef DBGHASHSIZE  //���������� � FileBase.h
#else
  // ������ ������������� ������ ������������
  DWORD iUser;
  _stscanf_s((LPTSTR)pItem, _T("%d"), &iUser);
  return (WORD)iUser;
#endif
  // ������ �� �� ���-���� Login'�
  TUser *ptUser;  int nCmp;
  if (nLeng == 0)  return 0xFFFF;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptUser = m_aUsers[i];
    nCmp = memcmp(ptUser->_chLogHash, pItem, nLeng);
    if (nCmp == 0)  return i;
  }
  return 0xFFFF;    //�� ������
} // CUserRegister::Find()

//protected: -----------------------------------------------------------
//virtual  ���� ������� ��������� ����� ��
char * CUserRegister::GetSignaturePattern(WORD &nLeng)
//nLeng[out] - ������ ��������� ����� � ������
{
  return CFileBase::GetSignaturePattern(nLeng, USERSIGNATR);
} // CUserRegister::GetSignaturePattern()

//private: -------------------------------------------------------------
// ������������� ������ ������� �������������
TUser * CUserRegister::GenerateUser(WORD nUser)
//nUser[in] - ����� ������������
{
  static TCHAR szPwd[11] = _T("0246897531"); //������ ��� ������������� ������
  char sName[NAMESBUFSIZ];
  TCHAR ch, szLogin[NAMESBUFSIZ];
  TUser *ptUser = new TUser;
  sprintf_s(sName, NAMESBUFSIZ, "user%02d", nUser);  ptUser->SetName(sName);
  _stprintf_s(szLogin, NAMESBUFSIZ, _T("login%02d"), nUser);  ptUser->SetLogin(szLogin);
  ptUser->SetPassword(szPwd);
  // ����������� ��������� ������
  ch = szPwd[0];  memmove(szPwd, szPwd+1, 9*sizeof(TCHAR));  szPwd[9] = ch;
  // ����� ������ � '\0' � ������:
  DWORD dwLoginLen = (_tcslen(ptUser->_szLogin) + 1) * sizeof(TCHAR),
//      dwPasswLen = _tcslen(ptUser->_szPasswd) + 1, //����� ������ � '\0'
        dwHashSize;
  // ���� ���-��� Login'� ������������
  BYTE *pbDataHash = m_paCrypt->GetDataSignature(
    (BYTE *)ptUser->_szLogin, dwLoginLen, dwHashSize
  );
  //ASSERT(dwHashSize == HASHSIZE);
  ptUser->SetLoginHash(pbDataHash, (WORD)dwHashSize);
  /* ���� ���-��� ������ ������������
  pbDataHash = m_paCrypt->GetDataSignature(
    (BYTE *)ptUser->_szPasswd, dwPasswLen, dwHashSize
  );
  ASSERT(dwHashSize == HASHSIZE);
  ptUser->SetPasswordHash(pbDataHash, (WORD)dwHashSize); */
  return ptUser;
} // CUserRegister::GenerateUser()

// �������� ������� ������������� � ������
void  CUserRegister::ClearUserArray()
{
  TUser *ptUser;  WORD i;
  for (i = 0; i < m_nItemCount; i++) {
    ptUser = m_aUsers[i];
    if (ptUser) {
      delete ptUser;  ptUser = NULL;
    }
  }
  m_aUsers.RemoveAll();  m_nItemCount = 0;
} // CUserRegister::ClearUserArray()

// End of class CUserRegister definition -------------------------------


//      ���������� ������ CBalanceRegister
////////////////////////////////////////////////////////////////////////
//      ������������� ����������� ��������� ������
//

// ����������� � ����������
CBalanceRegister::CBalanceRegister(CCrypto *paCrypt)
                : CFileBase(paCrypt, _T("Balnc_"), sizeof(TBalance))
{
}
CBalanceRegister::~CBalanceRegister(void)
{
  bool b = Save();
  ClearBalanceArray();
} // CBalanceRegister::~CBalanceRegister()

// ������� ������
// ������������� ������� ������� �������� � ������
void  CBalanceRegister::GenerateBalanceArray(CUserRegister *poUserReg,
                                             double rSum)
//poUserReg - ��������� ������� ������������� � ������
//rSum      - �����, ������� ����� ��������� �� ����� �������������
{
  TUser *ptUser;  TBalance *ptBalance;  WORD iUser, k;
  for (iUser = 0; (ptUser = poUserReg->GetAt(iUser)) != NULL; iUser++) {
    ptBalance = GenerateBalance(ptUser, rSum);
    if (ptBalance) {
      k = m_aBalances.Add(ptBalance);  m_nItemCount++;
    }
  }
  m_bChanged = true;
} // CBalanceRegister::GenerateBalanceArray()

//virtual ��������� ���� ��
bool  CBalanceRegister::Load()
{
  bool b = CFileBase::Load();
  if (b)
  {
    TBalance *ptBalance;  size_t nRead;  WORD i, k;
    for (i = 0; i < m_nItemCount; i++) {
      ptBalance = new TBalance;
      nRead = fread(ptBalance, 1, sizeof(TBalance), m_pFile);
      if (nRead != sizeof(TBalance))
        { m_nErr = 2;  b = false;  break; }   //������ ������ �����
      k = m_aBalances.Add(ptBalance);
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //������ �������� �����
    m_pFile = NULL;
  }
  m_bLoaded = b;  m_bChanged = false;
  return b;
} // CBalanceRegister::Load()

//virtual  ��������� ������ ������� � ����� ��
bool  CBalanceRegister::Save(bool bDoSave/* = false*/)
{
  if (!bDoSave && !m_bChanged)  return false;
  bool b = CFileBase::Save();
  if (b)
  {
    TBalance *ptBalance;  size_t nWrit;
    for (WORD i = 0; i < m_nItemCount; i++) {
      ptBalance = m_aBalances[i];
      nWrit = fwrite(ptBalance, 1, sizeof(TBalance), m_pFile);
      if (nWrit != sizeof(TBalance))
        { m_nErr = 4;  b = false;  break; }   //������ ������ �����
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //������ �������� �����
    m_pFile = NULL;
  }
  m_bChanged = !b;
  return b;
} // CBalanceRegister::Save()

//virtual  �������� ������� ������ � ������ ��
bool  CBalanceRegister::Add(void *pItem, WORD nLeng/* = 0*/)
{
  return true;
} // CBalanceRegister::Add()

//virtual  ����� ������� ������ � ������� ��
WORD  CBalanceRegister::Find(void *pItem, WORD nLeng/* = 0*/)
{
  return 0;
} // CBalanceRegister::Find()

//protected:
//virtual  ���� ������� ��������� ����� ��
char * CBalanceRegister::GetSignaturePattern(WORD &nLeng)
//nLeng[out] - ������ ��������� ����� � ������
{
  return CFileBase::GetSignaturePattern(nLeng, BLNCSIGNATR);
} // CBalanceRegister::GetSignaturePattern()

//private:
// ������������� ������ ������� ������� �������� � ������
TBalance * CBalanceRegister::GenerateBalance(TUser *ptUser, double rSum)
{
  TBalance *ptBalance = new TBalance;
  memcpy(ptBalance->_sName, ptUser->_sName, NAMESBUFSIZ);
  memcpy(ptBalance->_chLogHash, ptUser->_chLogHash, HASHSIZE);
  ptBalance->SetBalance(rSum);
  return ptBalance;
} // CBalanceRegister::GenerateBalance()

// �������� ������� ������� �������� � ������
void  CBalanceRegister::ClearBalanceArray()
{
  TBalance *ptBalance;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptBalance = m_aBalances[i];
    if (ptBalance) {
      delete ptBalance;  ptBalance = NULL;
    }
  }
  m_aBalances.RemoveAll();  m_nItemCount = 0;
} // CBalanceRegister::ClearBalanceArray()

// End of class CBalanceRegister definition ----------------------------


//      ���������� ������ CTransRegister
////////////////////////////////////////////////////////////////////////
//      ������������� ����������� ��������� ������
//

// ����������� � ����������
CTransRegister::CTransRegister(CCrypto *paCrypt, CUserRegister *paUsers)
              : CFileBase(paCrypt, _T("Trans_"), sizeof(TTransact))
{
  memset(m_chLastHash, 0, HASHSIZE);
  m_paUsers = paUsers;  //��������� ������� �������������
}
CTransRegister::~CTransRegister(void)
{
  bool b = Save();
  ClearTransactArray();
} // CTransRegister::~CTransRegister()

// ������� ������
// ������������� ���-��� � ���������� �������������
void  CTransRegister::HashToSymbols(char *pcBuff, BYTE btHash[])
//pcBuff[out] - �������� ����� ������� 2*HASHSIZE ��� ����������
//btHash[in]  - ���-��� � �������� ������� ������� HASHSIZE
{
  char *pc = pcBuff;  BYTE hbt0, hbt1;
  for (int i = 0; i < HASHSIZE; i++) {
    hbt0 = btHash[i] & 0x0F;  hbt1 = (btHash[i] & 0xF0) >> 4;
	*pc++ = (hbt0 < 10) ? hbt0 + '0' : (hbt0 - 10) + 'a';
	*pc++ = (hbt1 < 10) ? hbt1 + '0' : (hbt1 - 10) + 'a';
  }
} // CTransRegister::HashToSymbols()

// ������������� ���������� ������������� � ���-���
void  CTransRegister::SymbolsToHash(char *pcBuff, BYTE btHash[])
//pcBuff[in]  - ������� ����� ������� 2*HASHSIZE � �������������� ���-����
//btHash[out] - �������� ������ ������� HASHSIZE ��� ���-����
{
  char *pc = pcBuff;  BYTE hbt0, hbt1;
  for (int i = 0; i < HASHSIZE && *pc; i++) {
    hbt0 = *pc++;
    if ('0' <= hbt0 && hbt0 <= '9')  hbt0 -= '0';
    else { hbt0 -= 'a';  hbt0 += 10; }
    hbt1 = *pc++;
    if ('0' <= hbt1 && hbt1 <= '9')  hbt1 -= '0';
    else { hbt1 -= 'a';  hbt1 += 10; }
    btHash[i] = (hbt1 << 4) & hbt0;
  }
} // CTransRegister::HashToSymbols()

//virtual ������ �������� ����� �� ��� ���������� ������ ������������
bool  CTransRegister::Load()
{
  bool  b = CFileBase::Load();  size_t nRead;
  if (b) {
    // ��������� ���-��� ��������� ����������
    // ��������� �� 1 ����� HASHSIZE ���:
    nRead = fread(m_chLastHash, 1, HASHSIZE, m_pFile);
    if (nRead != HASHSIZE)
      { m_nErr = 2;  b = false; }   //������ ������ �����
  }
  if (b)
  {
    TTransact *ptRansact;  WORD i, k;
    for (i = 0; i < m_nItemCount; i++) {
      ptRansact = new TTransact;
      nRead = fread(ptRansact, 1, sizeof(TTransact), m_pFile);
      if (nRead != sizeof(TTransact))
        { m_nErr = 2;  b = false;  break; } //������ ������ �����
      k = m_aTrans.Add(ptRansact);
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //������ �������� �����
    m_pFile = NULL;
  }
  m_bLoaded = b;  m_bChanged = false;
  return b;
} // CTransRegister::Load()

//virtual  ��������� ������ ������� � ����� ��
bool  CTransRegister::Save(bool bDoSave/* = false*/)
{
  if (!bDoSave && !m_bChanged)  return false;
  size_t nWrit;
  bool b = CFileBase::Save();
  if (b) {
    // �������� ���-��� ��������� ����������
    nWrit = fwrite(m_chLastHash, 1, HASHSIZE, m_pFile);
    if (nWrit != HASHSIZE)
      { m_nErr = 4;  b = false; }   //������ ������ �����
  }
  if (b)
  {
    TTransact *ptRansact;
    for (WORD i = 0; i < m_nItemCount; i++) {
      ptRansact = m_aTrans[i];
      nWrit = fwrite(ptRansact, 1, sizeof(TTransact), m_pFile);
      if (nWrit != sizeof(TTransact))
        { m_nErr = 4;  b = false;  break; } //������ ������ �����
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //������ �������� �����
    m_pFile = NULL;
  }
  m_bChanged = !b;
  return b;
} // CTransRegister::Save()

//virtual  �������� ����� ���� � ���-���� ����������
bool  CTransRegister::Add(void *pItem, WORD nLeng/* = 0*/)
//pItem - ��������� �������������� ���� ����������
{
  TTransact *ptRansact = (TTransact *)pItem;
  if (nLeng == 0)  nLeng = sizeof(TTransact);
  else
    ASSERT(nLeng == sizeof(TTransact));
  // �������� ���-��� ��������� ����������
  //memcpy(ptRansact->_chPrevHash, m_chLastHash, HASHSIZE);
  // �������� ���������� � ���-����
  WORD k = m_aTrans.Add(ptRansact);
  m_nItemCount++;
  // ��������� ���-��� ��������� ����������
  DWORD dwHashSize;
  BYTE *pbDataHash = m_paCrypt->GetDataSignature(
    (BYTE *)ptRansact, (DWORD)nLeng, dwHashSize
  );
  ASSERT(dwHashSize == HASHSIZE);
  memcpy(m_chLastHash, pbDataHash, HASHSIZE);
  m_bChanged = true;
  return true;
} // CTransRegister::Add()

//virtual  ����� ������� ������ � ������� ��
WORD  CTransRegister::Find(void *pItem, WORD nLeng/* = 0*/)
{
  return 0;
} // CTransRegister::Find()

// ������� ����� ���� ������� ����������
TTransact * CTransRegister::CreateTransactBlock(
  WORD iUserFrom, WORD iUserTo, double rAmount)
//iUserFrom - ������ ������������-�����������
//iUserTo   - ������ ������������-����������
//rAmount   - ����� ����������
{
  TTransact *ptRansact = new TTransact;
  TUser *ptUserFrom = m_paUsers->GetAt(iUserFrom);
  TUser *ptUserTo = m_paUsers->GetAt(iUserTo);
  // �������� ���-��� ��������� ����������
  memcpy(ptRansact->_chPrevHash, m_chLastHash, HASHSIZE);
  ptRansact->SetTransOrd(GetNewTransactOrd());  //����� ����������
  // �������� ���-��� Login'� ������������-�����������
  memcpy(ptRansact->_chHashSrc, ptUserFrom->_chLogHash, HASHSIZE);
  // �������� ���-��� Login'� ������������-����������
  memcpy(ptRansact->_chHashDst, ptUserTo->_chLogHash, HASHSIZE);
  ptRansact->SetAmount(rAmount);                //����� ����������
  return ptRansact;
} // CTransRegister::CreateTransactBlock()

//protected:
//virtual  ���� ������� ��������� ����� ��
char * CTransRegister::GetSignaturePattern(WORD &nLeng)
//nLeng[out] - ������ ��������� ����� � ������
{
  return CFileBase::GetSignaturePattern(nLeng, TRANSIGNATR);
} // CTransRegister::GetSignaturePattern()

// ��������� ���������, ����������� �� ����� ��
void  CTransRegister::ClearTransactArray()
{
  TTransact *ptRansact;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptRansact = m_aTrans[i];
    if (ptRansact) {
      delete ptRansact;  ptRansact = NULL;
    }
  }
  m_aTrans.RemoveAll();  m_nItemCount = 0;
} // CTransRegister::ClearTransactArray()

// End of class CTransRegister definition ------------------------------
