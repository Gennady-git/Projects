// BlkChain.h                                              (c) DIR, 2019
//
//              ���������� ������� ������ ��������
//
// CBlockChain - ������ ��������
//
////////////////////////////////////////////////////////////////////////

#pragma once

//#include "afxtempl.h"
#include "FileBase.h"

// ��������� ������ ���������. �������� ������ � �������� TCHAR.
#define  CMDFIELDSIZ  4     //������ ���� �������
#define  LONGNUMSIZE  8     //������ ������ ����������
#define  SHORTNUMSIZ  2     //������ ���� ������ ��� ���������� ������
#define  AMOUNT_SIZE  NAMESBUFSIZ   //������ ���� �����, ��� � FileBase.h
#define  TRANSNUMOFF  CMDFIELDSIZ   //�������� ����� TransNo, Node
#define  HASHFROMOFF  (TRANSNUMOFF+LONGNUMSIZE) //�������� ���� HashFrom
// _HASHSIZE ���������� � FileBase.h
#ifdef DBGHASHSIZE  //���������� � ������ FileBase.h
 #define  HASHTO_OFFS  (HASHFROMOFF+_HASHSIZE/sizeof(TCHAR)-1) //�������� ���� HashTo
 #define  AMOUNT_OFFS  (HASHTO_OFFS+_HASHSIZE/sizeof(TCHAR)-1) //�������� ���� Amount
#else
 #define  HASHTO_OFFS  (HASHFROMOFF+_HASHSIZE) //�������� ���� HashTo
 #define  AMOUNT_OFFS  (HASHTO_OFFS+_HASHSIZE) //�������� ���� Amount
#endif
#define  HASHPREVOFF  (AMOUNT_OFFS+AMOUNT_SIZE) //�������� ���� HashPrev

#define  VOTECOLNUM   6     //���������� �������� � ������� �����������
#define  TRANCOLNUM   4     //���������� �������� � ������� ����������
// ��� ���������� ������ ���������� (UNICODE):
#define  KEYCONTNAME  TEXT("Peer Net Key Container")

// �������������� (����) ��������� ������� DTB
enum ESubcmdCode {
  ScC_ATR = 11, //Add in Transaction Log - ����� �������� � ������ ����������
  ScC_GTB,      //Get Transaction log Block - ���� ���� ������� ����������
  ScC_TAQ,      //TransAction reQuest    - ������ ����������
  ScC_TAR,      //TransAction Reply      - ����� �� ������ ����������
  ScC_TBL,      //Transaction log BLock  - ���� ����������
  ScC_TRA,      //execute TRansAction    - ��������� ����������
};
// ����������� ��� "��������� ��������� (����������)"
struct TSubcmd {
  ESubcmdCode  _eCode;              //��� ����������
  TCHAR  _szNode[SHORTNUMSIZ+1];    //����� ���� (2)
  TCHAR  _szBlock[SHORTNUMSIZ+1];   //����� ��� ���������� ������ (2)
  TCHAR  _szTrans[LONGNUMSIZE+1];   //����� ���������� (8)
  BYTE   _chHashFrom[_HASHSIZE];    //���-��� ��� ����� �����-�����������
  BYTE   _chHashTo[_HASHSIZE];      //���-��� ��� ����� �����-����������
  TCHAR  _szAmount[NAMESBUFSIZ];    //+1 ����� ���������� (��� ����� �������)
  BYTE   _chHashPrev[2*HASHSIZE+2]; //���-��� ����������� ����� � ���� ������
  // ��������� ���������
  void  Clear() {
    memset(&_eCode, 0, sizeof(TSubcmd));
  }
  // ���� ����� ���� (2 ����)
  void  SetNode(LPTSTR pszNode) {
    memcpy(_szNode, pszNode, SHORTNUMSIZ * sizeof(TCHAR));
    _szNode[SHORTNUMSIZ] = _T('\0');
  }
  WORD  GetNode() {
    DWORD n;
    swscanf_s(_szNode, _T("%02d"), &n);
    return (WORD)n;
  }
  // ���� ����� ��� ���������� ������ (2 ����)
  void  SetBlockNo(LPTSTR pszNo) {
    memcpy(_szBlock, pszNo, SHORTNUMSIZ * sizeof(TCHAR));
    _szBlock[SHORTNUMSIZ] = _T('\0');
  }
  WORD  GetBlockNo() {
    DWORD n;
    swscanf_s(_szBlock, _T("%02d"), &n);
    return (WORD)n;
  }
  // ���� ����� ���������� (8 ����)
  void  SetTransNo(LPTSTR pszNo) {
    memcpy(_szTrans, pszNo, LONGNUMSIZE * sizeof(TCHAR));
    _szTrans[LONGNUMSIZE] = _T('\0');
  }
  WORD  GetTransNo() {
    DWORD n;
    swscanf_s(_szTrans, _T("%08d"), &n);
    return (WORD)n;
  }
  // ���� ���-��� ���������������� ����� (128 ����) (��� ������ (2 ����))
  //������������-�����������
  void  SetHashFrom(BYTE *cHash) {
#ifdef DBGHASHSIZE
    memcpy(_chHashFrom, cHash, _HASHSIZE-2);  //��� ������������ _T('\0')
    memset(_chHashFrom + _HASHSIZE-2, 0, 2);  //���������� ��������� _T('\0')
#else
    memcpy(_chHashFrom, cHash, _HASHSIZE);
#endif
  }
  // ���� ���-��� ���������������� ����� (128 ����) (��� ������ (2 ����))
  //������������-����������
  void  SetHashTo(BYTE *cHash) {
#ifdef DBGHASHSIZE
    memcpy(_chHashTo, cHash, _HASHSIZE-2);
    memset(_chHashTo + _HASHSIZE-2, 0, 2);
#else
    memcpy(_chHashTo, cHash, _HASHSIZE);
#endif
  }
  // ����� ���������� (��� ������� �������) (15+1 ����)
  void  SetAmount(LPTSTR pszSum) {
    memcpy(_szAmount, pszSum, (NAMESBUFSIZ-1)*sizeof(TCHAR));
    _szAmount[NAMESBUFSIZ-1] = _T('\0');
  }
  double  GetAmount() {
    double rSum;
    swscanf_s(_szAmount, _T("%lf"), &rSum);
    return rSum;
  }
  // ���� ���-��� � ���� ������������� �������� ����� ���������� (256+1 ����)
  void  SetHashPrev(BYTE *cHash) {
    memcpy(_chHashPrev, cHash, 2*HASHSIZE);
    memset(_chHashPrev + 2*HASHSIZE, 0, 2);
  }
}; // struct TSubcmd

// ����������� ��� "������ ������� ����������� �����������"
struct TVotingRes {
  WORD    _iUser;       //�������� ���� (������ � �������)
  WORD    _nTrans;      //����� ����� ����������
  WORD    _iUserFrom;   //������������ "���" (������ � �������)
  WORD    _iUserTo;     //������������ "����" (������ � �������)
  double  _rSum;        //����� ���������� ��� ������� ������� �����
  // ���������������� ���������
  void  Init(WORD iUser) {
    memset(&_iUser, 0, sizeof(TVotingRes));
    _iUser = iUser;
  }
};

class CPeerNetDialog;
class CPeerNetNode;

// ���������� ������ CBlockChain - ������ ��������
////////////////////////////////////////////////////////////////////////

class CBlockChain
{
  CPeerNetDialog  * m_pMainWin;     //������� ���� ���������
  CPeerNetNode    * m_paNode;       //���� ������������ ����
  CCrypto           m_aCrypt;       //������ ������������
  CUserRegister     m_rUsers;       //������� �������������
  CBalanceRegister  m_rBalances;    //������� ������� ��������
  CTransRegister    m_rTransacts;   //������� ����������
  // ������� ������������ iNode->iUser, � ����� ������� ����������� �����:
  TVotingRes  m_tVoteRes[NODECOUNT];
  WORD        m_nVote;          //���������� ��������������� �����
  WORD        m_iNodes[NODECOUNT];      //������� ������������ iUser->iNode
  WORD        m_nActTrans;      //"����������" ����� ����� ����������
  WORD        m_iActNodes[NODECOUNT];   //������ "����������" ����� ����
  WORD        m_nActNodes;      //���������� "����������" ����� � ������
  WORD        m_iActNode;       //������� ������� ������ "����������" �����
  //bool         m_bTrans;        //���� "���������� ����������� ����������"
  TMessageBuf  m_mbTRA;         //����������� ��������� TRA
  // ��������� ������������ �������� ����������
  WORD         m_iSrcNode;      //����-�������� ��� ������������
  //WORD         m_nTransBlock;   //����� ���������� ����� ����������
  WORD         m_nAbsentBlks;   //���������� ����������� ������ ����������
public:
  // ����������� � ����������
  CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode);
  ~CBlockChain(void);
  // ������� ������
  // -------------------------------------------------------------------
  // ���� ��������� �������� �������������
  CUserRegister    * UserRegister() { return &m_rUsers; }
  // ���� ��������� �������� ������� ��������
  CBalanceRegister * BalanceRegister() { return &m_rBalances; }
  // ���� ��������� �������� ����������
  CTransRegister   * TransRegister() { return &m_rTransacts; }
  // ��������� ��������������� ������ ������������
  WORD  AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd);
  WORD  AuthorizationCheck(CString sLogin, CString sPasswd);
  // ������������ ������������
  bool  Authorize(WORD iUser);
  // ���� ��������������� ������������ (��������� ����)
  WORD  GetAuthorizedUser() { return m_rUsers.OwnerIndex(); }
  // �������� ����������� ������������
  void  CancelAuthorization();
  // ������� ������������ � ����� ����
  void  TieUpUserAndNode(WORD iUser, WORD iNode);
  // �������� ������������ �� ���� ����
  void  UntieUserAndNode(WORD iNode);
  // ���� ������������ ��������� ����
  WORD  GetNodeOwner(WORD iNode) { return m_tVoteRes[iNode]._iUser; }
  // ���������� ��������� DTB �� ������� (���������� ��������� �������)
  bool  ProcessDTB();
  // ������������ ����������
  bool  StartTransaction(WORD iUserTo, double rAmount);
  // ����������� ���������?
  bool  IsTheVoteOver() { return (m_nVote == m_paNode->m_nNodes); }
  // ���������������� ���������� �����������
  bool  AnalyzeVoting();
  // ������������ ��������� TRA �� ��������� ���������� �� ������� �����������
  void  CreateTRAMessage();
  // �������� ���������� ����������
  //void  ExecuteTransaction(); //TMessageBuf &mbTAR
  // �������� ���������� �� ���� ����
  bool  ExecuteTransactionOnLocalNode(TMessageBuf &mbTRA);
  // ������� �������� ��
  void  CloseRegisters();
private:
  // ���������� ������
  // -------------------------------------------------------------------
  // ���������������� ��� ������� � ������
  //bool  Initialize();
  // ������������ ���������� ������� ��� ���������� ����������
  void  CreateTransactSubcommand(LPTSTR pszMsg, LPTSTR pszSbcmd,
                                 WORD nTrans, WORD iUserFrom,
                                 WORD iUserTo, double rAmount);
  // ������� ��������� ����������� ������ ��������� ���� 
  //� ��������� � ���� ������, ��� ��������� ������
  void  RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest);
  // ����������� ����� ������ ���� �� ��������� ����������� ������
  TMessageBuf * EmulateReply(TMessageBuf &mbRequest);
  // ��������� ���������. ����� ���������� ��� ���������� � ��������� �����.
  ESubcmdCode  ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd);
  // ��������� ����� � ������� �����������
  bool  AddInVotingTable(WORD iNodeFrom, TMessageBuf *pmbReply);
  // ��������������� ���� ������� ����������
  bool  AskTransactionBlock(); //WORD iNodeFrom, WORD nBlocks
  // ���������� ��������� GTB "���� ���� �������� ����������".
  //� ����� ����������� ��������� TBL.
  bool  GetTransactionBlock(WORD nBlock, TMessageBuf &mbGTBL);
  // ���������� ��������� TAQ "������ ����������".
  //� ����� ����������� ��������� TAR.
  bool  TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbTAQR);
  // �������� � ������� ���������� ����� ���� (��� ������������).
  bool  AddTransactionBlock(TMessageBuf &mbTBL);
  // ��������������� ������� ������� ����� �� �������� �����
  void  CorrectBalance(WORD iUser, double rAmount);
  // ��������� ����������� ������� ������� �����
  void  CalculateBalance(WORD iUser);
  // ����� ������������ ����� ���������� � ������� �����������
  WORD  GetActualTransactOrd();
  // ��������� ������ ���������� �����
  void  BuildActualNodeList(WORD nTransAct);
  // ������� � ���������� ����������� ���� � ������ (����������� �������)
  void  NextActualNode();
};

// End of class CBlockChain declaration --------------------------------
