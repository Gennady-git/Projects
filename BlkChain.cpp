// BlkChain.cpp                                            (c) DIR, 2019
//
//              ���������� ������� ������ ��������
//
// CBlockChain - ������ ��������
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include "PeerNetDlg.h" //#include "PeerNetNode.h" //#include "BlkChain.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif

//      ���������� ������ CBlockChain
////////////////////////////////////////////////////////////////////////
//      ������������� ����������� ��������� ������
//

// ����������� � ����������
CBlockChain::CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode)
           : m_aCrypt(KEYCONTNAME),
             m_rUsers(&m_aCrypt),
             m_rBalances(&m_aCrypt),
             m_rTransacts(&m_aCrypt, &m_rUsers)
{
  m_pMainWin = pMainWin;    //������� ���� ���������
  m_paNode = paNode;        //���� ������������ ����
  CancelAuthorization();    //�������� ������ �����������
  //m_bTrans = false;         //���� "���������� ����������� ����������"
}
CBlockChain::~CBlockChain(void)
{
} // CBlockChain::~CBlockChain()

//public:
// ������� ������
// -------------------------------------------------------------------
// ������������� �����. ��������� ��������������� ������ �
//���������������� ������������
WORD  CBlockChain::AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd)
{
  WORD iUser;
  bool b = m_rUsers.IsLoaded();
  if (!b)  b = m_rUsers.Load();
  if (b)  b = m_rUsers.AuthorizationCheck(pszLogin, pszPasswd, iUser);
  else {
    // ������ �������� �������� �������������
    ; //TraceW(m_rUsers.ErrorMessage());
  }
  return b ? iUser : UNAUTHORIZED;
}
WORD  CBlockChain::AuthorizationCheck(CString sLogin, CString sPasswd)
{
  return AuthorizationCheck(sLogin.GetBuffer(), sPasswd.GetBuffer());
} // CBlockChain::AuthorizationCheck()

// ������������ ������������
bool  CBlockChain::Authorize(WORD iUser)
{
  // ������������� ������� ������������
  if (!m_aCrypt.IsInitialized())  m_aCrypt.Initialize();
  bool b = m_rUsers.IsLoaded();
  if (b) {  //����� ������� ������������� m_rUsers ��� ������ ���� ��������!
    m_rUsers.SetOwner(iUser);
    // ��������� ������� ������� ��������
    m_rBalances.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rBalances.Load();
  }
  if (b) {
    // ��������� ������� ����������
    m_rTransacts.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rTransacts.Load();
  }
  return b;
} // CBlockChain::Authorize()

// �������� ����������� ������������ � �������� ������ �������
void  CBlockChain::CancelAuthorization()
{
  m_rUsers.ClearOwner();
  m_rBalances.ClearOwner();
  m_rTransacts.ClearOwner();
  memset(m_tVoteRes, 0xFF, sizeof(m_tVoteRes));
  memset(m_iNodes, 0xFF, sizeof(m_iNodes));
  memset(m_iActNodes, 0, sizeof(m_iActNodes));
  m_nActNodes = 0;
  m_iActNode = 0xFFFF;
} // CBlockChain::CancelAuthorization()

// ������� ������������ � ���� ���� (����� ����������� � ������� ����)
void  CBlockChain::TieUpUserAndNode(WORD iUser, WORD iNode)
{
  ASSERT(0 <= iUser && iUser < NODECOUNT && 0 <= iNode && iNode < NODECOUNT);
  m_iNodes[iUser] = iNode;
  m_tVoteRes[iNode].Init(iUser);
} // CBlockChain::TieUpUserAndNode()

// �������� ������������ �� ���� ����
void  CBlockChain::UntieUserAndNode(WORD iNode)
{
  if (0 <= iNode && iNode < NODECOUNT)
  {
    WORD iUser = m_tVoteRes[iNode]._iUser;
    memset(m_tVoteRes+iNode, 0xFF, sizeof(TVotingRes));
    m_iNodes[iUser] = 0xFFFF;
  }
} // CBlockChain::UntieUserAndNode()

// ���������� ��������� DTB �� ������� (���������� ��������� �������)
bool  CBlockChain::ProcessDTB()
{
  TMessageBuf *pmbDTB;  bool b;
  EnterCriticalSection(&g_cs);
  b = m_paNode->m_oDTBQueue.Get(pmbDTB);    //���� DTB
  LeaveCriticalSection(&g_cs);
  if (!b)  return b;
  // ����������� ��������� DTB � ����� ��� ���������
  TMessageBuf  tMessBuf(pmbDTB->MessBuffer(), pmbDTB->MessageBytes());
  WORD iNodeFrom;
  // ������� ���������� �� DTB, ���������� iNodeFrom
  m_paNode->UnwrapData(iNodeFrom, &tMessBuf);
  TSubcmd tCmd;
  ESubcmdCode eSbcmd = ParseMessage(tMessBuf, tCmd);
  switch (eSbcmd) {
case ScC_ATR:   //Add in Transaction Register
    // ��������������� � ���� (tCmd._szNode) (tCmd._szBlock) ������ 
    //�������� ����������
    m_iSrcNode = tCmd.GetNode();        //����-�������� ��� ������������
    m_nAbsentBlks = tCmd.GetBlockNo();  //����� ����������� ������ ����������
    b = AskTransactionBlock();
    break;
case ScC_GTB:   //Get Transaction Register Block
    b = GetTransactionBlock(tCmd.GetBlockNo(), tMessBuf); //����� - TBL
    RequestNoReply(iNodeFrom, tMessBuf);
    break;
case ScC_TAQ:   //TransAction reQuest
    b = TransActionreQuest(tCmd, tMessBuf);             //����� - TAR
    RequestNoReply(iNodeFrom, tMessBuf);
    break;
case ScC_TAR:   //TransAction request Replay
    // ����� ��������� ���� �� ������ ���������� ������� 
    //� ������� �����������
    b = AddInVotingTable(iNodeFrom, &tMessBuf);
    break;
case ScC_TBL:   //Transaction BLock
	// ������� ���� ���������� - �������� � ������� ����������
    b = AddTransactionBlock(tMessBuf);
    //if (m_bTrans) {
    // ���������� ����������� ����������
    //}
    if (m_nAbsentBlks > 0)
      b = AskTransactionBlock();
    else
      ExecuteTransactionOnLocalNode(m_mbTRA);
    break;
case ScC_TRA:   //execute TRansAction
	if (tCmd.GetTransNo() == m_rTransacts.GetNewTransactOrd())
      // ������� ���������� �������� - �������� ���������� �� ���� ����
      b = ExecuteTransactionOnLocalNode(tMessBuf);
	else {
      // �������� ���������� ���������� �� ������������ �������� ���������� ...
      m_mbTRA = tMessBuf;   //��������� ��������� TRA
      // ... � ���� ��������� ����������� ����� ����������
      m_iSrcNode = iNodeFrom;
      //m_nTransBlock = m_rTransacts.GetNewTransactOrd();
      m_nAbsentBlks = tCmd.GetTransNo() - m_rTransacts.GetNewTransactOrd();
      b = AskTransactionBlock();
	}
case 0:         //��� �� ������� - ��������� �� �����
    break;
  } //switch
  return b;
} // CBlockChain::ProcessDTB()

// ������������ ����������
bool  CBlockChain::StartTransaction(WORD iUserTo, double rAmount)
//iUserTo - ������ ������������ "����"
//rAmount - ����� ����������
{
  // ��������� ����������� ������� ���������� ����������
  ASSERT(0 <= iUserTo && iUserTo < NODECOUNT);
  TBalance *ptBalance = m_rBalances.GetAt(iUserTo);
  double rBalance = ptBalance->GetBalance();
  if (rAmount > rBalance) {
    m_rBalances.SetError(7);    //����� ���������� ��������� �������
    m_pMainWin->ShowMessage(m_rBalances.ErrorMessage());
    return false;
  }
  if (!m_paNode->IsQuorumPresent())  return false;  //��� �������
  //m_bTrans = true;              //���� "���������� ����������� ����������"
  // ������������ ���������� TAQ "��������� ����������"
  WORD nTrans = m_rTransacts.GetNewTransactOrd();   //����� ����� ����������
  WORD iUserFrom = m_rUsers.OwnerIndex();           //������ ��������� ����
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CreateTransactSubcommand(szMessBuf, _T("TAQ"),
                           nTrans, iUserFrom, iUserTo, rAmount);
  TMessageBuf mbRequest, *pmbTAR = NULL;
  WORD iNodeTo;  bool b;
  m_nActTrans = m_nVote = 0;    //������������� �����������
  // ���� ����������� - 
  //��������� ������� TAQ ���� ����� ���� (������� ����)
  for (b = m_paNode->GetFirstNode(iNodeTo, true);
       b; b = m_paNode->GetNextNode(iNodeTo, true)) {
    mbRequest.PutMessage(szMessBuf);        //������� � �����
    if (iNodeTo == m_paNode->m_iOwnNode) {
      pmbTAR = EmulateReply(mbRequest);
      // "�����" ������ ���� ������� - ������� � ������� �����������
      b = AddInVotingTable(iNodeTo, pmbTAR);
	  delete pmbTAR;  pmbTAR = NULL;
    }
    else
      RequestNoReply(iNodeTo, mbRequest);   //������ mbRequest!
  }
  return true;
} // CBlockChain::StartTransaction()

// ���������������� ���������� �����������
bool  CBlockChain::AnalyzeVoting()
{
  m_nActTrans = GetActualTransactOrd(); //���������� ����� ����������
  BuildActualNodeList(m_nActTrans); //��������� ������ ���������� �����
  // ������������ ��������� TRA ���������� ����������
  CreateTRAMessage(); //����������� m_mbTRA
  bool b, bActual;  //������� ������������ "������" ����
  TCHAR chMess[CHARBUFSIZE];  TMessageBuf mbRequest;
  // ���� �� ������� ������� �����������
  for (WORD i = 0; i < m_nVote; i++) {
    b = m_tVoteRes[i]._nTrans == m_nActTrans;   //i-� ���� ��������?
    if (i == m_paNode->m_iOwnNode)
      bActual = b;      //������������ "������" ����
    else if (b) {
      // �������� ���� i �������� - ������� ��� ��������� TRA
      mbRequest.PutMessage(m_mbTRA.Message());  //������� � �����
      // ������� ���� i
      RequestNoReply(i, mbRequest); //������ mbRequest!
    }
    else {
      // �������� ���� i �� �������� - ������� ��� ��������� ATR.
      // ������������ ��������� ATR ...
      swprintf_s(chMess, CHARBUFSIZE, _T("ATR %02d%02d"),
                 m_iActNodes[m_iActNode], m_nActTrans - m_tVoteRes[i]._nTrans);
      NextActualNode(); //� ���������� ����������� ����
      mbRequest.PutMessage(chMess); //������� � �����
      // ������� ���� i
      RequestNoReply(i, mbRequest); //������ mbRequest!
    }
  }
  return bActual;
} // CBlockChain::AnalyzeVoting()

// ������������ ��������� TRA �� ��������� ���������� �� ������� �����������
void  CBlockChain::CreateTRAMessage()
{
  TCHAR szMess[CHARBUFSIZE];
  WORD iNode     = m_paNode->m_iOwnNode,            //�������� ����
       nTrans    = m_nActTrans,                     //����� ����������
       iUserFrom = m_tVoteRes[iNode]._iUserFrom,    //������������ "���"
       iUserTo   = m_tVoteRes[iNode]._iUserTo;      //������������ "����"
  double rAmount = m_tVoteRes[iNode]._rSum;         //����� ����������
  CreateTransactSubcommand(szMess, _T("TRA"),
                           nTrans, iUserFrom, iUserTo, rAmount);
  m_mbTRA.PutMessage(szMess);   //������� � �����
} // CBlockChain::CreateTRAMessage()

/* �������� ���������� (��������� � "�����" ������ ������� �����������)
void  CBlockChain::ExecuteTransaction()
{
  / * ������������ ��������� TRA
  TCHAR szMess[CHARBUFSIZE];
  WORD iNode     = m_paNode->m_iOwnNode,            //�������� ����
       nTrans    = m_nActTrans, //m_tVoteRes[iNode]._nTrans ����� ����������
       iUserFrom = m_tVoteRes[iNode]._iUserFrom,    //������������ "���"
       iUserTo   = m_tVoteRes[iNode]._iUserTo,      //������������ "����"
       iNodeTo;
  double rAmount = m_tVoteRes[iNode]._rSum;         //����� ����������
  CreateTransactSubcommand(szMess, _T("TRA"),
                           nTrans, iUserFrom, iUserTo, rAmount); * /
  bool b;  WORD iNodeTo;
  TMessageBuf mbTRA(m_mbTRA.Message());
  // ��������� ��������� TRA �� �������� ���� ����
  for (b = m_paNode->GetFirstNode(iNodeTo);
       b; b = m_paNode->GetNextNode(iNodeTo)) {
    mbTRA.PutMessage(m_mbTRA.Message());    //������� � �����
    RequestNoReply(iNodeTo, mbTRA);     //������ mbTRA!
  }
  if (m_rTransacts.GetNewTransactOrd() == m_nActTrans) {
    // ������� ���������� �������� - �������� ���������� �� ���� ����
    m_pMainWin->ApproveTransaction();   //���������� ��������� ����������
    mbTRA.PutMessage(m_mbTRA.Message());    //������� � �����
    b = ExecuteTransactionOnLocalNode(mbTRA);
  }
/ *  else
    m_mbTRA = mbTRA;    //��������� ��������� TRA
} // CBlockChain::ExecuteTransaction() */

// �������� ���������� �� ���� ����
bool  CBlockChain::ExecuteTransactionOnLocalNode(TMessageBuf &mbTRA)
//iUserFrom - ������ ������������-�����������
//iUserTo   - ������ ������������-����������
//rAmount   - ����� ����������
{
  TSubcmd tCmd;
  if (ParseMessage(mbTRA, tCmd) != ScC_TRA)  return false;
  // ������� ��������� ����������
  WORD nTrans = tCmd.GetTransNo(),
#ifdef DBGHASHSIZE
  iUserFrom = CPeerNetNode::ToNumber((LPTSTR)tCmd._chHashFrom, SHORTNUMSIZ),
  iUserTo = CPeerNetNode::ToNumber((LPTSTR)tCmd._chHashTo, SHORTNUMSIZ);
#else
  iUserFrom = m_rUsers.GetUserByLoginHash(tCmd._chHashFrom),
  iUserTo = m_rUsers.GetUserByLoginHash(tCmd._chHashTo);
#endif
  double rAmount = tCmd.GetAmount();
/*  if (m_bTrans) {
    VERIFY(nTrans == m_rTransacts.GetNewTransactOrd());
    // ����� � ������� ����������� ����� ����������� ����������
    for (WORD i = 0; i < m_nVote; i++)
      if (m_tVoteRes[i]._iUser == m_tVoteRes[i]._iUserFrom) {
        rAmount = m_tVoteRes[i]._rSum;  break;
      }
    VERIFY(rAmount == rAmount0);
    m_bTrans = false; //���� ����������� ����������� ����������, �������� ����
  } */
  // ������������ ���� ����������
  TTransact *ptRansact =
    m_rTransacts.CreateTransactBlock(iUserFrom, iUserTo, rAmount);
  m_rTransacts.Add(ptRansact);  //�������� � ������� ����������
  m_pMainWin->AddInTransactTable(ptRansact);    //����������
  // ��������������� ������� �������������
  CorrectBalance(iUserFrom, -rAmount);
  CorrectBalance(iUserTo, rAmount);
  return true;
} // CBlockChain::ExecuteTransactionOnLocalNode()

// ������� �������� ��
void  CBlockChain::CloseRegisters()
{
  m_rBalances.Save();   //������� ������� ��������
  m_rTransacts.Save();  //������� ����������
} // CBlockChain::CloseRegisters()

//private:
// ���������� ������
// -------------------------------------------------------------------
// ������������ ���������� �������, ������ ��� ���������� ����������
void  CBlockChain::CreateTransactSubcommand(
  LPTSTR pszMsg, LPTSTR pszSbcmd, WORD nTrans,
  WORD iUserFrom, WORD iUserTo, double rAmount)
{
  // ������������ ���������� ����������
#ifdef DBGHASHSIZE
  // ���������� �������
  swprintf_s(pszMsg, CHARBUFSIZE, _T("%s %08d%02d%02d%015.2f"),
             pszSbcmd, nTrans, iUserFrom, iUserTo, rAmount);
#else
  // ������� �������
  swprintf_s(pszMsg, CHARBUFSIZE, _T("%s %08d"), pszSbcmd, nTrans);
  TUser *ptUserFrom = m_rUsers.GetAt(iUserFrom),
        *ptUserTo = m_rUsers.GetAt(iUserTo);
  memcpy(pszMsg+HASHFROMOFF, ptUser->_chLogHash, HASHSIZE);
  memcpy(pszMsg+HASHTO_OFFS, ptUser->_chLogHash, HASHSIZE);
  swprintf_s(pszMsg+AMOUNT_OFFS, CHARBUFSIZE-AMOUNT_OFFS,
             _T("%015.2f"), rAmount);
#endif
} // CBlockChain::CreateTransactSubcommand()

// ������� ��������� ����������� ������ ��������� ���� 
//� ��������� � ���� ������, ��� ��������� ������
void  CBlockChain::RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest)
//iNode     - ������ ����-��������
//mbRequest - ����� ���������
{
  TMessageBuf *pmbReply = NULL;
  WORD iNodeFrom;
  m_paNode->WrapData(iNodeTo, mbRequest);   //��������� ���������� � DTB
  // ������� ��������� ���� iNodeTo
  pmbReply = m_paNode->RequestAndReply(iNodeTo, mbRequest);
  if (pmbReply) {       //����� �������, �� ����� ���� ������ ACK
	// ����������� ���������� �� DTB �/��� ���������� iNodeFrom
    if (pmbReply->MessageBytes() > 0) {
      m_paNode->UnwrapData(iNodeFrom, pmbReply);
      ASSERT(iNodeFrom == iNodeTo);
    }
    delete pmbReply;    //����� �� ������� ���������
  }
} // RequestNoReply()

// ����������� ����� ������ ���� �� ��������� ����������� ������
TMessageBuf * CBlockChain::EmulateReply(TMessageBuf &mbTAQ)
{
  // ������������ ���������� TAR "��������� ����������" �� ����� 
  //���������� TAQ
  TMessageBuf *pmbTAR = new TMessageBuf(mbTAQ.Message());
  memcpy(pmbTAR->Message(), _T("TAR"), 3*sizeof(TCHAR));
  return pmbTAR;
} // CBlockChain::EmulateReply()

// ��������� ���������. ����� ���������� ��� ���������� � ��������� �����.
ESubcmdCode  CBlockChain::ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd)
//mbMess[in] - ���������
//tCmd[out]  - ��������� �������, ��������� ���������
{
  static LPTSTR sCmds[] = {
//������             ��� ����������
/* 0 */ _T("ATR"),  //ScC_ATR = 11, Add in Transact Reg - �������� � ������ ��
/* 1 */ _T("GTB"),  //ScC_GTB,      Get Trans log Block - ���� ���� ������� ��
/* 2 */ _T("TAQ"),  //ScC_TAQ,      TransAction reQuest - ������ ����������
/* 3 */ _T("TAR"),  //ScC_TAR,      TransAction Reply   - ����� �� ������ ����
/* 4 */ _T("TBL"),  //ScC_TBL,      Transact log BLock  - ���� ����������
/* 5 */ _T("TRA"),  //ScC_TRA,      execute TRansAction - ��������� ����������
  };
  static WORD n = sizeof(sCmds) / sizeof(LPTSTR);
  WORD i = 0;
  for ( ; i < n; i++) {
    if (mbMess.Compare(sCmds[i], 3) == 0) {
      tCmd._eCode = (ESubcmdCode)(ScC_ATR + i);  goto SbcmdFound;
    }
  }
  tCmd._eCode = (ESubcmdCode)0;
SbcmdFound:
  // ���������� ����������, ����������� ���������
  switch (tCmd._eCode) {
case ScC_ATR:   //Add in Transaction Register
    // ����� (������) ����
    tCmd.SetNode(mbMess.Message() + CMDFIELDSIZ);
    // ���������� ������
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ+SHORTNUMSIZ);
    break;
case ScC_GTB:   //Get Transaction Register Block
    // ����� ����� �� ����� ����
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ);
    break;
case ScC_TAQ:   //TransAction reQuest
case ScC_TAR:   //TransAction Reply
case ScC_TRA:   //execute TRansAction
    // ����� ����������
    tCmd.SetTransNo(mbMess.Message() + TRANSNUMOFF);
    // ���-��� �����������
    tCmd.SetHashFrom(mbMess.MessBuffer() + HASHFROMOFF*sizeof(TCHAR));
    // ���-��� ����������
    tCmd.SetHashTo(mbMess.MessBuffer() + HASHTO_OFFS*sizeof(TCHAR));
    // ����� ����������
    tCmd.SetAmount(mbMess.Message() + AMOUNT_OFFS);
    break;
case ScC_TBL:   //Transact register BLock
    // ����� ����������
    tCmd.SetTransNo(mbMess.Message() + TRANSNUMOFF);
    // ���-��� �����������
    tCmd.SetHashFrom(mbMess.MessBuffer() + HASHFROMOFF*sizeof(TCHAR));
    // ���-��� ����������
    tCmd.SetHashTo(mbMess.MessBuffer() + HASHTO_OFFS*sizeof(TCHAR));
    // ����� ����������
    tCmd.SetAmount(mbMess.Message() + AMOUNT_OFFS);
    // ���-��� ����������� ����� ����������
    tCmd.SetHashPrev(mbMess.MessBuffer() + HASHPREVOFF*sizeof(TCHAR));
    break;
  } //switch
  return tCmd._eCode;   //���������� �� ����������
} // CBlockChain::ParseMessage()

// ��������� ����� � ������� �����������
bool  CBlockChain::AddInVotingTable(WORD iNodeFrom, TMessageBuf *pmbTAR)
{
  TSubcmd tCmd;
  bool b = (ParseMessage(*pmbTAR, tCmd) == ScC_TAR);
  if (!b)  return b;
  WORD nTrans, iUser = m_tVoteRes[iNodeFrom]._iUser;
  m_tVoteRes[iNodeFrom]._nTrans = tCmd.GetTransNo();
  m_tVoteRes[iNodeFrom]._iUserFrom =
    m_rUsers.GetUserByName((LPTSTR)tCmd._chHashFrom, 0);
/* #ifdef DBGHASHSIZE
#else
    m_rUsers.GetUserByLoginHash(tCmd._chHashFrom),
#endif */
  m_tVoteRes[iNodeFrom]._iUserTo =
    m_rUsers.GetUserByName((LPTSTR)tCmd._chHashTo, 0);
/* #ifdef DBGHASHSIZE
#else
    m_rUsers.GetUserByLoginHash(tCmd._chHashTo),
#endif */
  m_tVoteRes[iNodeFrom]._rSum = tCmd.GetAmount();
  CString sNode, sTrans, sAmount;
  TCHAR szOwner[NAMESBUFSIZ], szUserFrom[NAMESBUFSIZ],
        szUserTo[NAMESBUFSIZ], *pszFields[VOTECOLNUM];
  sNode.Format(_T("���� #%d"), iNodeFrom+1);
  pszFields[0] = sNode.GetBuffer();     //����� ����
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUser, szOwner);
  pszFields[1] = szOwner;               //��� ��������� ����
  pszFields[2] = tCmd._szTrans;         //����� ����������
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserFrom, szUserFrom);
  pszFields[3] = szUserFrom;            //��� ������������-�����������
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserTo, szUserTo);
  pszFields[4] = szUserTo;              //��� ������������-����������
  sAmount.Format(_T("%.2lf"), m_tVoteRes[iNodeFrom]._rSum);
  pszFields[5] = sAmount.GetBuffer();   //�����
  m_pMainWin->AddInVotingTable(pszFields);
  m_nVote++;    //������� ��������������� �����
  if (!IsTheVoteOver())  return b;  //��� �� ��� �������������
  // ������������� ��� - ���������������� ���������� �����������
  b = AnalyzeVoting();  //��� ��� ����� ���������� � ������ ��� �����
  if (!b) {
    // ��������� ���� ���������� - ��������� ������� ���������� ���������� 
    //���� ������������ ������� ����������
    m_pMainWin->ApproveTransaction(1);      //���������� ���������� ����������
    m_iSrcNode = m_iActNodes[m_iActNode];   //����-�������� ��� ������������
    NextActualNode();   //� ���������� ����������� ����
    m_nAbsentBlks = m_nActTrans - m_tVoteRes[m_paNode->m_iOwnNode]._nTrans;
    b = AskTransactionBlock();  //������ ������������ ���������� ����
    return b;
  }
  // ��������� ���� �������� - ����� ����� �������� ����������
  b = ExecuteTransactionOnLocalNode(m_mbTRA);
    /* ����� "����" ������ � ������� �����������  TVotingRes
    if (iNodeFrom != m_paNode->m_iOwnNode) {
      sTrans.Format(_T("%08d"), m_tVoteRes[m_paNode->m_iOwnNode]._nTrans);
      sAmount.Format(_T("%015.2lf"), m_tVoteRes[m_paNode->m_iOwnNode]._rSum);
      memcpy(pmbTAR->_chMess+TRANSNUMOFF,
             sTrans.GetBuffer(), LONGNUMSIZE*sizeof(TCHAR));
      memcpy(pmbTAR->_chMess+AMOUNT_OFFS,
             sAmount.GetBuffer(), NAMESBUFSIZ*sizeof(TCHAR));
    }
    ExecuteTransaction(*pmbTAR); */
  return b;
} // CBlockChain::AddInVotingTable()

// ��������� ���� ���������� ��� ������������ �������� ����������
bool  CBlockChain::AskTransactionBlock()
{
  ASSERT(m_nAbsentBlks > 0);
  // ������������ ��������� GTB ...
  TMessageBuf mbGTB;  TCHAR szMess[CHARBUFSIZE];
  swprintf_s(szMess, CHARBUFSIZE, _T("GTB %02d"), m_rTransacts.GetNewTransactOrd());
  mbGTB.PutMessage(szMess);     //������� � �����
  // ... ������� ��������� ����
  RequestNoReply(m_iSrcNode, mbGTB);
  m_nAbsentBlks--;
  return true;
} // CBlockChain::AskTransactionBlock()

// ���������� ��������� GTB "���� ���� �������� ����������".
//� ����� ����������� ��������� TBL.
bool  CBlockChain::GetTransactionBlock(WORD nBlock, TMessageBuf &mbGTBL)
//nBlock[in]  - ����� �������������� ����� (1,2,..)
//mbGTBL[out] - ����� ���������: �� ����� GTB, �� ������ TBL
{
  TCHAR szMess[CHARBUFSIZE];    //����� ��������� (TCHAR=wchar_t)
  TTransact *ptRansact = m_rTransacts.GetAt(nBlock - 1);
  WORD iUserFrom = m_rUsers.GetUserByLoginHash(ptRansact->_chHashSrc),
       iUserTo = m_rUsers.GetUserByLoginHash(ptRansact->_chHashDst),
       nLeng = HASHPREVOFF*sizeof(TCHAR)+HASHSIZE*2;
  CreateTransactSubcommand(szMess, _T("TBL"), nBlock,
                           iUserFrom, iUserTo, ptRansact->GetAmount());
  m_rTransacts.HashToSymbols((char *)(szMess+HASHPREVOFF),
                             ptRansact->_chPrevHash);
  mbGTBL.Put((BYTE *)szMess, nLeng);
  return true;
} // CBlockChain::GetTransactionBlock()

// ���������� ��������� TAQ "������ ����������".
//� ����� ����������� ��������� TAR.
bool  CBlockChain::TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbTAQR)
//tCmd[out]      - ��������� �������, ��������� ��������� TAQ
//mbTAQR[in/out] - ����� ���������: �� ����� TAQ, �� ������ TAR
{
  LPTSTR pszMsg = mbTAQR.Message();  DWORD n;
  swscanf_s((LPTSTR)tCmd._chHashFrom, _T("%02d"), &n);
  WORD nTrans = m_rTransacts.GetNewTransactOrd(), iUserFrom = (WORD)n;
  TBalance *ptBalnc = m_rBalances.GetAt(iUserFrom);
  double rAmount = ptBalnc->GetBalance();
  TCHAR chMess[CHARBUFSIZE];
  swprintf_s(chMess, CHARBUFSIZE, _T("TAR %08d%015.2f"), nTrans, rAmount);
  n = CMDFIELDSIZ + LONGNUMSIZE;    //�������� �������� rAmount � chMess
  memcpy(mbTAQR.Message(), chMess, sizeof(TCHAR)*n);
  memcpy(mbTAQR.Message() + AMOUNT_OFFS,
         chMess + n, sizeof(TCHAR)*NAMESBUFSIZ);
  return true;
} // CBlockChain::TransActionreQuest()

// �������� � ������� ���������� ����� ���� (��� ������������).
bool  CBlockChain::AddTransactionBlock(TMessageBuf &mbTBL)
{
  TSubcmd tCmd;
  bool b = (ParseMessage(mbTBL, tCmd) == ScC_TBL);
  if (!b)  return b;
  WORD nTrans = tCmd.GetTransNo(),
       iUserFrom = m_rUsers.GetUserByName((LPTSTR)tCmd._chHashFrom, 0),
       iUserTo = m_rUsers.GetUserByName((LPTSTR)tCmd._chHashTo, 0);
  double rAmount = tCmd.GetAmount();
/*
  char  _szTrnsOrd[NAMESBUFSIZ];    //0x0000 ���������� ����� ����������
  BYTE  _chHashSrc[HASHSIZE];       //0x0010 ���-��� ��� ����� "���"
  BYTE  _chHashDst[HASHSIZE];       //0x0090 ���-��� ��� ����� "����"
  char  _szAmount[NAMESBUFSIZ];     //0x0110 ����� ���������� "�������"
  BYTE  _chPrevHash[HASHSIZE];      //0x0120 ���-��� ����������� �����
*/
  TTransact tRansact;  TUser *ptUser;
  b = (nTrans == m_rTransacts.GetNewTransactOrd());
  if (b) {
    tRansact.SetTransOrd(nTrans);
    ptUser = m_rUsers.GetAt(iUserFrom);
    tRansact.SetHashSource(ptUser->_chLogHash, HASHSIZE);
    ptUser = m_rUsers.GetAt(iUserTo);
    tRansact.SetHashDestination(ptUser->_chLogHash, HASHSIZE);
    m_rTransacts.SymbolsToHash((char *)(tCmd._chHashPrev),
                               tRansact._chPrevHash);
    tRansact.SetAmount(rAmount);
    m_rTransacts.Add(&tRansact);    //�������� � ������� ����������
/*  if (m_bTrans) {
      // ����������� "����" ���������� - ��������������� ������� �����������
      m_tVoteRes[m_paNode->m_iOwnNode]._nTrans = nTrans;
      if (m_rTransacts.GetNewTransactOrd() == m_nActTrans) {
        // ������� ���������� ��������� �������������� - 
        //�������� � �������� ����������
        m_pMainWin->ApproveTransaction();   //���������� ��������� ����������
        ExecuteTransaction();
      }
    } */
    m_pMainWin->AddInTransactTable(&tRansact);    //����������
  }
  else {
    // ������ - ��������� ������������������ ������ ���-����
    ASSERT(FALSE);
  }
  return b;
} // CBlockChain::AddTransactionBlock()

// ��������������� ������� ������� ����� ������������ �� �������� �����
void  CBlockChain::CorrectBalance(WORD iUser, double rTurnover)
//iUser     - ������ ������������
//rTurnover - ����� ������� (+/-)
{
  TBalance *ptBalnc = m_rBalances.GetAt(iUser);
  double rSum = ptBalnc->GetBalance();  //������� �������
  rSum += rTurnover;
  ptBalnc->SetBalance(rSum);  m_rBalances.SetChanged();
  // ���� ���� ������ - ����������
  if (iUser == m_paNode->m_iOwner)  m_pMainWin->ShowBalance(rSum);
} // CBlockChain::CorrectBalance()

// ��������� ����������� ������� ������� ����� ��������� ������������
void  CBlockChain::CalculateBalance(WORD iUser)
{
  double rSum = 1000.0, rTurnover;
  WORD iUserFrom, iUserTo, n = m_rTransacts.ItemCount();
  TTransact *ptRansact;
  // �������� ����� �������� ����������
  for (WORD i = 0; i < n; i++) {
    ptRansact = m_rTransacts.GetAt(i);
    iUserFrom = m_rUsers.GetUserByLoginHash(ptRansact->_chHashSrc),
    iUserTo = m_rUsers.GetUserByLoginHash(ptRansact->_chHashDst);
	if (iUserFrom == iUser || iUserTo == iUser) {
      rTurnover = ptRansact->GetAmount();
      if (iUserFrom == iUser)  rTurnover = -rTurnover;
      rSum += rTurnover;
	}
  }
  TBalance *ptBalnc = m_rBalances.GetAt(iUser);
  ptBalnc->SetBalance(rSum);  m_rBalances.SetChanged();
  // ���� ���� ������ - ����������
  if (iUser == m_paNode->m_iOwner)  m_pMainWin->ShowBalance(rSum);
} // CBlockChain::CalculateBalance()

// ����� ������������ ����� ���������� � ������� �����������
WORD  CBlockChain::GetActualTransactOrd()
{
  WORD iAct = 0;
  for (WORD i = 1; i < m_nVote; i++)
    if (m_tVoteRes[i]._nTrans > m_tVoteRes[iAct]._nTrans) {
      iAct = i;
    }
  return m_tVoteRes[iAct]._nTrans;
} // CBlockChain::GetActualTransactOrd()

// ��������� ������ ���������� �����
void  CBlockChain::BuildActualNodeList(WORD nTransAct)
{
  WORD i;
  for (m_nActNodes = 0, i = 0; i < m_nVote; i++)
    if (m_tVoteRes[i]._nTrans == nTransAct) {
      m_iActNodes[m_nActNodes++] = i;
    }
  m_iActNode = 0;   //�� ������ ������� ������ "����������" �����
} // CBlockChain::BuildActualNodeList()

// ������� � ���������� ����������� ���� � ������ (����������� �������)
void  CBlockChain::NextActualNode()
{
  m_iActNode++;
  if (m_iActNode == m_nActNodes)  m_iActNode = 0;
} // CBlockChain::NextActualNode()

// End of class CBalanceRegister definition ----------------------------
