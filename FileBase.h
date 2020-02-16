// FileBase.h                                              (c) DIR, 2019
//
//              ���������� ������� �������� ���� ������
//
// CFileBase        - ������� ����� ��� ��������� �������� "���� ������"
// CUserRegister    - ������� �������������
// CBalanceRegister - ������� ������� ��������
// CTransRegister   - ������� ����������
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include "afxtempl.h"
#include "Crypto.h"

// ���������� ������ CFileBase -
// ������� ����� ��� ��������� �������� "���� ������"
////////////////////////////////////////////////////////////////////////

#define  DBGHASHSIZE  1 //���������� �����: ������� ����� ������ ���-����
//#undef   DBGHASHSIZE

#define  FILENAMESIZ  16    //������ ������ ����� ����� � ��������
#define  FOLDERNMSIZ  128   //������ ������ ����� ���. �������� � ��������
// ������ ������ ��� ������ ������������ ����� � ��������:
#define  FILEPATH     (FOLDERNMSIZ+FILENAMESIZ)
#define  FILESIGNSIZ  40    //������ ������ ��������� ����� � ������
#define  FILESIGNBAS  "(c) DIR, 2019."  //��������� ����� �� ���������
// ������� ������� �� ��������� ��� ������ ��.
//������� (.) ��������� �������, � ������� ��������� ����� *.h � *.cpp �������
#define  FOLDERDEFLT  _T("..\\Data\\")

class CFileBase
{
  static TCHAR * m_sErrMes[];   //������ ��������� �� �������
protected:
  CCrypto * m_paCrypt;              //��������� ������� ������������
  TCHAR  m_sFileName[FILENAMESIZ];  //������ ����� �����
  TCHAR  m_sFolder[FOLDERNMSIZ];    //������� ������� ��� ������ �������� ��
  // ����������� ����� ��������� �����
  char   m_sFileSign[FILESIGNSIZ];  //��������� �����
  WORD   m_nItemSize;               //������ �������� ������
  WORD   m_nItemCount;              //���������� ��������� ������
  // ������� ����������
  WORD   m_iOwner;      //������ ������������-��������� ������ �������
  WORD   m_nOwner;      //����� ������������-��������� ������ �������
  FILE * m_pFile;       //��������� �����
  bool   m_bLoaded;     //���� "������ �������� �� �����"
  bool   m_bChanged;    //���� "������ ������"
  WORD   m_nErr;        //��� ������
public:
  // ����������� � ����������
  CFileBase(CCrypto *paCrypt, LPTSTR pszFileNameBase,
            WORD nItemSize);
  ~CFileBase(void);
  // ������� ������
  // ���������, ��������������� �� ������������?
  bool  IsThereSignedInUser() { return (m_iOwner != 0xFFFF); }
  // ���� ������ � ������� ������������, "���������" �������
  WORD  OwnerIndex() { return m_iOwner; }
  // ���� ����� ������������, "���������" �������
  WORD  OwnerOrd() { return m_nOwner; }
  // ������ ������ � ����� ������������, "���������" �������
  void  SetOwner(WORD iOwner, WORD nOwner)
  { m_iOwner = iOwner;  m_nOwner = nOwner; }
  // �������� (�������) ������������-��������� �������
  void  ClearOwner() { m_iOwner = 0xFFFF;  m_nOwner = 0; }
  // ���� ���������� ������� � ����������� ������� (��������)
  WORD  ItemCount() { return m_nItemCount; }
  // ������ ������� ������� ��� ���������� �������� ��
  void  SetFolder(LPTSTR sFolder = NULL);
  // ������� ��������?
  bool  IsLoaded() { return (m_nItemCount > 0); }
  // ��������� ���� �� ��� ���������� ������ ������������
  virtual bool  Load();
  // ��������� ������ ������� � ����� �� ��� ���������� ������ ������������
  virtual bool  Save(bool bDoSave = false);
  // �������� ������� ������ � ������ ��
  virtual bool  Add(void *pItem, WORD nLeng = 0);
  // ����� ������� ������ � ������� ��
  virtual WORD  Find(void *pItem, WORD nLeng = 0);
  // ���������� ���� ���������
  void    SetChanged(bool bChng = true) { m_bChanged = bChng; }
  // ������ ��� ������
  void    SetError(WORD nErr) { m_nErr = nErr; }
  // ��������� ��������� ������
  bool    Error() { return (m_nErr != 0); }
  // ���� ��������� �� ������ � �������� ��������� ������
  TCHAR * ErrorMessage() {
    WORD nErr = m_nErr;  m_nErr = 0;  return m_sErrMes[nErr];
  }
protected:
  // ������, ��������� � ����������� �������
  // ���� ������� ��������� ����� ��
  virtual char * GetSignaturePattern(WORD &nLeng);
  char * GetSignaturePattern(WORD &nLeng, char *psIgn2);
  // ��������� ���� ��������� ����� ��
  void   InitSignature();
  // ��������� ���������, ����������� �� ����� ��
  bool   CheckSignature();
private:
  // ������ ������� ��������� �������
  void  Initialize(LPTSTR sFileName, WORD nItemSize);
  // ��������� � ����� ������ ������������ ����� ��
  void  FillFilePath(LPTSTR pszFilePath, WORD nSize);
};

// End of class CFileBase declaration ----------------------------------


// ���������� ������ CUserRegister - ������� �������������
////////////////////////////////////////////////////////////////////////

#define  NAMESBUFSIZ  16        //������ ����� ���
#define  HASHSIZE     128       //������ ���-�����
#define  USERSIGNATR  " User Register." //���� ����� ��������� �����
#ifdef DBGHASHSIZE  //���������� � ������ FileBase.h
 #define  _HASHSIZE   6     //������ ������ ������ ���-����
#else
 #define  _HASHSIZE  HASHSIZE   //������� �����: �������� ���-���
#endif

// ����������� ��� "������ ������� �������������"
struct TUser {
  char   _sName[NAMESBUFSIZ];   //�������� ��� ������������: user01,user02,..
  TCHAR  _szLogin[NAMESBUFSIZ];     //��������������� ��� (Unicode)
  TCHAR  _szPasswd[NAMESBUFSIZ];    //������ (Unicode)
  BYTE   _chLogHash[HASHSIZE];      //���-��� ���������������� �����
  //BYTE   _chPwdHash[HASHSIZE];  //���-��� ������
  // ��������� ���������
  void  Clear() {
    memset(_sName, 0, sizeof(TUser));
  }
  void  SetName(char *sName) {
    int  len = min(strlen(sName), NAMESBUFSIZ-1);
    memcpy(_sName, sName, len);
    // ��������� ������ (��� ������� ���� ����)
    memset(_sName+len, 0, NAMESBUFSIZ-len);
  }
  WORD  GetUserOrd() {
    DWORD nOrd;
    sscanf_s(_sName+4, "%d", &nOrd);
    return (WORD)nOrd;
  }
  void  SetLogin(LPTSTR pszLogin) {
    int  len = min(_tcslen(pszLogin), NAMESBUFSIZ-1);
    memcpy(_szLogin, pszLogin, len * sizeof(TCHAR));
    // ��������� ������ (��� ������� ���� ����)
    memset(_szLogin+len, 0, (NAMESBUFSIZ-len) * sizeof(TCHAR));
  }
  void  SetPassword(LPTSTR pszPasswd) {
    int  len = min(_tcslen(pszPasswd), NAMESBUFSIZ-1);
    memcpy(_szPasswd, pszPasswd, len * sizeof(TCHAR));
    // ��������� ������ (��� ������� ���� ����)
    memset(_szPasswd+len, 0, (NAMESBUFSIZ-len) * sizeof(TCHAR));
  }
  void  SetLoginHash(BYTE *cLogHash, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chLogHash, cLogHash, nLeng);
  }
/*  void  SetPasswordHash(BYTE *cPwdHash, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chPwdHash, cPwdHash, nLeng);
  } */
}; // struct TUser

typedef CTypedPtrArray<CPtrArray, TUser *> CUserArray;

class CUserRegister : public CFileBase
{
  CUserArray  m_aUsers;     //������� �������������
public:
  // ����������� � ����������
  CUserRegister(CCrypto *paCrypt);
  ~CUserRegister(void);
  // ������� ������
  // ��������� ��������������� ������ ������������
  bool  AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd, WORD &iUser);
  // ������ ������, � �� ���� � ����� ������������-��������� �������
  bool  SetOwner(WORD iUser);
  // ���� ������ ������� ������������� �� �������
  TUser * GetAt(WORD iUser)
    { return (0 <= iUser && iUser < m_nItemCount) ? m_aUsers[iUser] : NULL; }
  // ���� ��� ������������ �� ��� �������
  bool  UserName(WORD iUser, LPTSTR pszName);
  // ���� ������ ������������ �� ��� �����
  WORD  GetUserByName(LPTSTR pchName, WORD nOffset = 4);
  // ���� ������ ������������ �� ��� ���-����
  WORD  GetUserByLoginHash(BYTE *pchLogHash)
    { return Find(pchLogHash, HASHSIZE); }
  // ������������� ������� ������������� � ������
  void  GenerateUserArray(WORD nUsers);
  // ��������� ���� ��
  virtual bool  Load();
  // ��������� ������ ������� � ����� ��
  virtual bool  Save(bool bDoSave = false);
  // �������� ������� ������ � ������ ��
  virtual bool  Add(void *pItem, WORD nLeng = 0);
  // ����� ������� ������ � ������� ��
  virtual WORD  Find(void *pItem, WORD nLeng = 0);
protected:
  // ���� ������� ��������� ����� ��
  virtual char * GetSignaturePattern(WORD &nLeng);
private:
  // ������������� ������ ������� ������������� � ������
  TUser * GenerateUser(WORD nUser);
  // �������� ������� ������������� � ������
  void  ClearUserArray();
};

// End of class CUserRegister declaration ------------------------------


// ���������� ������ CBalanceRegister - ������� ������� ��������
////////////////////////////////////////////////////////////////////////

#define  BLNCSIGNATR  " Balance Register."  //���� ����� ��������� �����

// ����������� ��� "������ ������� ��������"
struct TBalance {
  char  _sName[NAMESBUFSIZ];    //�������� ��� ������������: user01,user02,..
  BYTE  _chLogHash[HASHSIZE];   //���-��� ���������������� �����
  char  _sBalance[NAMESBUFSIZ];     //����� �������� �������
  // ��������� ���������
  void  Clear() {
    memset(_sName, 0, sizeof(TBalance));
  }
  void  SetName(char *sName) {
    int  len = min(strlen(sName), NAMESBUFSIZ-1);
    memcpy(_sName, sName, len);
    // ��������� ������ (��� ������� ���� ����)
    memset(_sName+len, 0, NAMESBUFSIZ-len);
  }
  void  SetLoginHash(BYTE *cLogHash, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chLogHash, cLogHash, nLeng);
  }
  void  SetBalance(double rSum) {
    sprintf_s(_sBalance, NAMESBUFSIZ, "%.2f", rSum);
    int  len = min(strlen(_sBalance), NAMESBUFSIZ-1);
    // ��������� ������ (��� ������� ���� ����)
    memset(_sBalance+len, 0, NAMESBUFSIZ-len);
  }
  double  GetBalance() {
    double rSum = 0.0;
    sscanf_s(_sBalance, "%lf", &rSum);
    return rSum;
  }
}; // struct TBalance

typedef CTypedPtrArray<CPtrArray, TBalance *> CBalanceArray;

class CBalanceRegister : public CFileBase
{
  CBalanceArray  m_aBalances;   //������� ������� ��������
public:
  // ����������� � ����������
  CBalanceRegister(CCrypto *paCrypt);
  ~CBalanceRegister(void);
  // ������� ������
  // ������������� ������� ������� �������� � ������
  void  GenerateBalanceArray(CUserRegister *poUserReg, double rSum);
  // ���� ������ ������� ������� �������� �� �������
  TBalance * GetAt(WORD iUser) {
    return (0 <= iUser && iUser < m_nItemCount) ? m_aBalances[iUser] : NULL;
  }
  // ��������� ���� �� ��� ���������� ������ ������������
  virtual bool  Load();
  // ��������� ������ ������� � ����� ��
  virtual bool  Save(bool bDoSave = false);
  // �������� ������� ������ � ������ ��
  virtual bool  Add(void *pItem, WORD nLeng = 0);
  // ����� ������� ������ � ������� ��
  virtual WORD  Find(void *pItem, WORD nLeng = 0);
protected:
  // ���� ������� ��������� ����� ��
  virtual char * GetSignaturePattern(WORD &nLeng);
private:
  // ������������� ������ ������� ������� �������� � ������
  TBalance * GenerateBalance(TUser *ptUser, double rSum);
  // �������� ������� ������������� � ������
  void  ClearBalanceArray();
};

// End of class CBalanceRegister declaration ---------------------------


// ���������� ������ CTransRegister - ������� ����������
////////////////////////////////////////////////////////////////////////

#define  TRANSIGNATR  " Transaction Register."  //���� ����� ��������� �����

// ����������� ��� "������ ������� ����������"
struct TTransact {
  char  _szTrnsOrd[NAMESBUFSIZ];    //0x0000 ���������� ����� ����������
  BYTE  _chHashSrc[HASHSIZE];       //0x0010 ���-��� ��� ����� "���"
  BYTE  _chHashDst[HASHSIZE];       //0x0090 ���-��� ��� ����� "����"
  char  _szAmount[NAMESBUFSIZ];     //0x0110 ����� ���������� "�������"
  BYTE  _chPrevHash[HASHSIZE];      //0x0120 ���-��� ����������� �����
  // ��������� ���������              0x01a0 ������ ���������
  void  Clear() {
    memset(this, 0, sizeof(TTransact)); //_szTrnsOrd
  }
  void  SetTransOrd(WORD nOrd) {
    sprintf_s(_szTrnsOrd, NAMESBUFSIZ, "%d", nOrd);
    int  len = min(strlen(_szTrnsOrd), NAMESBUFSIZ-1), size = NAMESBUFSIZ;
    // ��������� ������ (��� ������� ���� ����)
    memset(_szTrnsOrd+len, 0, size-len);
  }
  WORD  GetTransOrd() {
    DWORD nOrd;
    sscanf_s(_szTrnsOrd, "%d", &nOrd);
    return (WORD)nOrd;
  }
  void  SetHashSource(BYTE *cHashSrc, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chHashSrc, cHashSrc, nLeng);
  }
  void  SetHashDestination(BYTE *cHashDst, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chHashDst, cHashDst, nLeng);
  }
  void  SetAmount(double rSum) {
    sprintf_s(_szAmount, NAMESBUFSIZ, "%.2lf", rSum);
    int  len = min(strlen(_szAmount), NAMESBUFSIZ-1), size = NAMESBUFSIZ;
    // ��������� ������ (��� ������� ���� ����)
    memset(_szAmount+len, 0, size-len);
  }
  double  GetAmount() {
    double rSum;
    sscanf_s(_szAmount, "%lf", &rSum);
    return rSum;
  }
  void  SetPrevHash(BYTE *cPrevHash, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chPrevHash, cPrevHash, nLeng);
  }
}; // struct TTransact

typedef CTypedPtrArray<CPtrArray, TTransact *> CTransactArray;

class CTransRegister : public CFileBase
{
  BYTE  m_chLastHash[HASHSIZE]; //���-��� ��������� ����������
  CUserRegister * m_paUsers;    //��������� ������� �������������
  CTransactArray  m_aTrans;     //������� ����������
public:
  // ����������� � ����������
  CTransRegister(CCrypto *paCrypt, CUserRegister *paUsers);
  ~CTransRegister(void);
  // ������� ������
  // ���� ������ ������� ���������� �� �������
  TTransact * GetAt(WORD iTrans) {
    return (0 <= iTrans && iTrans < m_nItemCount) ? m_aTrans[iTrans] : NULL;
  }
  // ������������� ���-��� � ���������� �������������
  void  HashToSymbols(char *pcBuff, BYTE btHash[]);
  // ������������� ���������� ������������� � ���-���
  void  SymbolsToHash(char *pcBuff, BYTE btHash[]);
  // ��������� ���� �� ��� ���������� ������ ������������
  virtual bool  Load();
  // ��������� ������ ������� � ����� ��
  virtual bool  Save(bool bDoSave = false);
  // �������� ������� ������ � ������ ��
  virtual bool  Add(void *pItem, WORD nLeng = 0);
  // ����� ������� ������ � ������� ��
  virtual WORD  Find(void *pItem, WORD nLeng = 0);
  // ���� ����� ����� ����������
  WORD  GetNewTransactOrd() { return m_nItemCount+1; }
  // ������� ����� ���� ������� ����������
  TTransact * CreateTransactBlock(WORD iUserFrom,
                                  WORD iUserTo, double rAmount);
protected:
  // ���� ������� ��������� ����� ��
  virtual char * GetSignaturePattern(WORD &nLeng);
private:
  // �������� ������� ���������� � ������
  void  ClearTransactArray();
};

// End of class CTransRegister declaration -----------------------------
