// PeerNetNode.cpp                                         (c) DIR, 2019
//
//              ���������� �������
// CNamedPipe   - ����������� �����
// CPeerNetNode - ���� ������������ ����
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include <strsafe.h>
#include "PeerNetDlg.h" //#include "PeerNetNode.h" //#include "BlkChain.h"

#ifdef _DEBUG
 #undef THIS_FILE
 static char THIS_FILE[]=__FILE__;
 #define new DEBUG_NEW
#endif

//  ���������� ����������� ������� ������
//----------------------------------------------------------------------
CRITICAL_SECTION  g_cs; //����������� ������

//======================================================================
// ����������� ������ CNamedPipe -
//  ������� ��� ������ � ������������ ��������
//----------------------------------------------------------------------

//  ������������� ����������� ��������� ������
//----------------------------------------------------------------------

// ����������� � ����������
CNamedPipe::CNamedPipe(CPeerNetNode *pArentNode,
                       WORD iCncNode/* = UNKNOWN_NODE*/)
{
  m_pArentNode = pArentNode;    //��������� ������������� �������
  m_pThread = NULL;
  m_iCncNode = iCncNode;
  m_hPipe = INVALID_HANDLE_VALUE;
  for (int i = 0; i < EVENTCOUNT; i++)
    m_hEvents[i] = INVALID_HANDLE_VALUE;
  m_bUsy = false;       //���� "����� �����"
  m_pQueue = NULL;      //������������� ������� ������ �� ��������
  m_nRead = 0;          //���������� �������� ���������
  m_nWritten = 0;       //���������� ������������ ���������
  m_eStatus = PS_None;          //��������� ������
  ClearError();
  memset(m_szErrorMes, 0, sizeof(m_szErrorMes));
}
CNamedPipe::~CNamedPipe()
{
  for (int i = 0; i < EVENTCOUNT; i++) {
    if (m_hEvents[i] > 0)
      ::CloseHandle(m_hEvents[i]);
  }
  if (m_pQueue) { delete m_pQueue;  m_pQueue = NULL; }
}

// ������� ������ ������� ������� ������������ ������
UINT  CNamedPipe::SendRequest()
{
  // �������� ������� "������ � ������� ���� k �����"
  BOOL  bSuccess = ::ResetEvent(m_hEvents[EvReqstReady]);
  if (!bSuccess) {
    // ������ ������ ResetEvent(): ��������� ��� ������ � ������� 
    //��������� �� ������.
    ProcessError(NULL, _T("::ResetEvent(hEvReqstReady)"));
    return 9;
  }
  // ������� ������ ������� ������
  DWORD  dwBytes = (DWORD)RequestBytes(), cbWritten;
  bSuccess = ::WriteFile(
    m_hPipe,        //����� ������
    RequestMessage(),   //����� ������� pNP->m_chRequest
    dwBytes,        //����� ������� � ������ m_wRequestBytes
    &cbWritten,     //���� ��������
    NULL            //not overlapped
  );
  if (!bSuccess || (cbWritten != dwBytes)) {
    // ������ �������� ������� ������� ������
    ProcessError(NULL, _T("SendRequest() ::WriteFile()"));
    return 7;
  }
  return 0;
} // CNamedPipe::SendRequest()

// ������� ����� ������� ������� ������������ ������
UINT  CNamedPipe::ReceiveReply(bool bNoEvent/* = false*/)
//bNoEvent - ���� "�� ���������� ������� EvRepReceivd"
{
  BOOL bSuccess;  bool bLoop = false;  UINT nRet = 0;
  TMessageBuf *pmbReply = NULL;  DWORD cbRead;
  while (true) {
    // ������ ������ ������ � �������� �����
    if (pmbReply == NULL)  pmbReply = &m_mbReply;
    bSuccess = ::ReadFile(
      m_hPipe,                  //pipe handle
      pmbReply->MessBuffer(),   //����� ������ ��� ����� ������
      (DWORD)BufferSize(),      //������ ������ � ������
      &cbRead,                  //number of bytes read
      NULL                      //not overlapped
    );
	pmbReply->_wMessBytes = (WORD)cbRead;
    if (bSuccess)  break;
    // ��������, ���� ������ - ��������
    if (SysErrorCode() == ERROR_MORE_DATA) {
      // ��� �� ������, � ������� ��������� - ������ ���� ������ ������� 
      //� �������� �����, ��������� � ������������ (���������� "�����")
      if (!bLoop) {
        // ������ ������ - �������� ������������ ����� ��� "������"
        pmbReply = new TMessageBuf();  bLoop = true;
      }
      bSuccess = TRUE;
    }
    if (!bSuccess) {
      // ������ ��� ��������� ������ �������
      ProcessError(NULL, _T("::ReadFile()"));
      nRet = 5;  break;
    }
  } //while (bLoop);
  if (!bNoEvent) {
    // ��������� ������� "����� ������� �������"
    bSuccess = ::SetEvent(m_hEvents[EvRepReceivd]);
    if ((nRet == 0) && !bSuccess) {
      // ������ ������ SetEvent(): ��������� ��� ������ � ������� 
      //��������� �� ������.
      ProcessError(NULL, _T("::SetEvent(hEvRepReceivd)"));
      nRet = 10;
    }
  }
  // ������� ������������ �����
  if (bLoop)  delete pmbReply;
  return nRet;
} // CNamedPipe::ReceiveReply()

// ������� ������ ������� ������� ������������ ������ � �������� �����.
//����� ���������� ����� � ������������ ������ � ������� �������.
TMessageBuf * CNamedPipe::RequestAndReply(TMessageBuf &mbRequest)
//mbRequest - ������� ����� � ��������
{
  bool b;  TMessageBuf *pmbRequest;
  if (m_bUsy) {
    // ����� ����� - ��������� ��������� � �������
    if (m_pQueue == NULL)  m_pQueue = new CCyclicQueue();
    b = m_pQueue->Put(mbRequest);
    return NULL;
  }
  m_bUsy = true;
  do {
    // ��������� ����� �������
    if (m_pQueue == NULL || m_pQueue->IsEmpty())
      // ������� ������� ����� - ����� ��������� �� ���������
      m_mbRequest = mbRequest;  //��������� ����� �������
    else {
      // ������� ������� �� ����� - ����� ��������� �� �������
      b = m_pQueue->Get(pmbRequest);
      m_mbRequest = *pmbRequest;  //��������� ����� �������
    }
#ifdef _TRACE
    TraceW(mbRequest.Message(), false);
#endif
    // ��������� ������� "������ � ������� �����"
    BOOL bSuccess = ::SetEvent(m_hEvents[EvReqstReady]);
    if (!bSuccess) {
      // ������ ������ SetEvent(): ��������� ��� ������ � ������� 
      //��������� �� ������.
      ProcessError(NULL, _T("::SetEvent(hEvReqstReady)"));
      continue; //return NULL;
    }
    // ����� ������� "����� ������� �������"
#ifdef _TRACE
    TraceW(_T("::WaitForSingleObject(hEvRepReceivd)"), false);
#endif
    DWORD dwWait = ::WaitForSingleObject(
      m_hEvents[EvRepReceivd], PIPE_TIMEOUT //INFINITE
    );
    if (dwWait != WAIT_OBJECT_0) {
      // ������ �������� ������� //== WAIT_FAILED
      if (dwWait == WAIT_TIMEOUT) {
        swprintf_s(m_szErrorMes, ERMES_BUFSIZE,
                   _T("::WaitForSingleObject(hEvRepReceivd) returned. %s"),
                   _T("The time-out interval elapsed"));
      }
      else {
        swprintf_s(m_szErrorMes, ERMES_BUFSIZE,
                   _T("::WaitForSingleObject(hEvRepReceivd) %s %d"),
                   _T("failed with error code"), SysErrorCode());
      }
#ifdef _TRACE
      TraceW(m_szErrorMes);
#endif
      //return NULL;
    }
    // �������� ������� "����� ������� �������"
    bSuccess = ::ResetEvent(m_hEvents[EvRepReceivd]);
    if (!bSuccess) {
      // ������ ������ ResetEvent(): ��������� ��� ������ � ������� 
      //��������� �� ������.
      ProcessError(NULL, _T("::ResetEvent(hEvRepReceivd)"));
      //return NULL;
    }
    m_bUsy = (m_pQueue != NULL) && !m_pQueue->IsEmpty();
  } while (m_bUsy);
  // ������� ����� �������
#ifdef _TRACE
  TraceW(m_mbReply.Message(), false);
#endif
  TMessageBuf *pmb = new TMessageBuf(m_mbReply.MessBuffer(), m_mbReply.MessageBytes());
  return pmb;
} // CNamedPipe::RequestAndReply()

// ���� ��������� � ��������� ��� ���������� ������. ���� � ������ 
//m_szErrorMes ��� ���������, �� ��������� ��������� ��������� �� ������
LPTSTR  CNamedPipe::ErrorMessage(LPTSTR lpszFunction/* = NULL*/)
//lpszFunction - ��� �������, ��� ������ ������� ��������� ������
{
  if (m_szErrorMes[0] == _T('\0'))
  {
    TCHAR chMsgBuf[CHARBUFSIZE];    //����� ���������� ���������
    memset(chMsgBuf, 0, sizeof(chMsgBuf));
    //m_dwSysError = ::GetLastError();    the last-error code
    //Retrieve the system error message
    DWORD dwLeng = ::FormatMessage(
      //FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_IGNORE_INSERTS |
      FORMAT_MESSAGE_FROM_SYSTEM,   //DWORD - ����� ������ ���������
      NULL,         //LPCVOID - The location of the message definition
      m_dwSysError, //DWORD - ����� ��������� ������ //_ENGLISH
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //DWORD dwLanguageId, _ENGLISH_US
      chMsgBuf,     //LPTSTR - ����� ���������
      0, NULL       //��������� ������ ���������� - ���
    );
    //������������ ��������� �� ������ � ������ m_szErrorMes
    if (lpszFunction == NULL)
      StringCchPrintf(m_szErrorMes, ERMES_BUFSIZE,
                      _T("System error %ld occur: %s"),
                      m_dwSysError, chMsgBuf);
    else
      StringCchPrintf(m_szErrorMes, ERMES_BUFSIZE,
                      _T("%s failed with error %ld: %s"),
                      lpszFunction, m_dwSysError, chMsgBuf);
  }
  return m_szErrorMes;
} // CNamedPipe::ErrorMessage

// ���������� ������
void  CNamedPipe::ProcessError(LPTSTR lpszMesg, LPTSTR lpszFunction/* = NULL*/)
//lpszMesg - �������������� ���������, ������� ����� �������
//            ����� ���������� �� ������
//lpszFunction - ��� �������, ��� ������ ������� ��������� ������
{
  if (!Success()) {
    if (lpszMesg) {
#ifdef _TRACE
      TraceW(lpszMesg, false);
#endif
    }
    if (lpszFunction) {
#ifdef _TRACE
      TraceW(ErrorMessage(lpszFunction));
#endif
    }
    ClearError();
  }
} // CNamedPipe::ProcessError()

#ifdef _TRACE
//static  ������� �������������� ���������
void  CNamedPipe::TraceW(LPTSTR lpszMes, bool bClearAfter/* = true*/)
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  if ((DWORD)m_hPipe == 0xfeeefeee || (DWORD)m_pThread == 0xfeeefeee)  return;
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("PipeHndl=0x%04X,ThreadID=0x%04X: %s"),
             (UINT)m_hPipe, m_pThread->m_nThreadID, lpszMes);
  //EnterCriticalSection(&g_cs);
  m_pArentNode->TraceW(szMessBuf, bClearAfter);
  //LeaveCriticalSection(&g_cs);
} // CNamedPipe::TraceW()
#endif

// ����� ����������� ������ CNamedPipe ---------------------------------


////////////////////////////////////////////////////////////////////////
// ����� CCyclicQueue - ����������� ������� �������� ������ DTB

// ����������� � ����������
CCyclicQueue::CCyclicQueue()
{
  memset(m_mbMsg, 0, sizeof(m_mbMsg));  //�������� ������ �������
  m_iHead  = 0;     //"������" ������� ������ �������
  m_iTail  = 0;     //"�����" ������� ������ �������
  m_nCount = 0;     //���������� ������ � �������
} // CCyclicQueue::CCyclicQueue()

//public:
// ��������� ���� � �������
bool  CCyclicQueue::Put(TMessageBuf &mbMsg)
{
  if (m_nCount == CLCQUEUESIZE)  return false;
  m_mbMsg[m_iTail].PutMessage(mbMsg.Message());
  Increment(m_iTail);  m_nCount++;
  return true;
} // CCyclicQueue::Put()

// ������� ���� �� �������
bool  CCyclicQueue::Get(TMessageBuf *&pmb)
{
  if (m_nCount == 0)  return false;
  pmb =  m_mbMsg + m_iHead;
  Increment(m_iHead);  m_nCount--;
  return true;
} // CCyclicQueue::Get()

// �������� ���������� ��������� �������
void  CCyclicQueue::Increment(int &iNdx)
{
  iNdx++;  iNdx &= CLCQUEUEMASK;
} // CCyclicQueue::Increment()

// ����� ����������� ������ CCyclicQueue -------------------------------

//volatile CNamedPipe * m_pNPListen;    //NP � ��������� Listen static

////////////////////////////////////////////////////////////////////////
// ����������� ������ CPeerNetNode - ���� ������������ ����

//  ������������� ����������� ��������� ������
//----------------------------------------------------------------------

// ����������� � ����������
CPeerNetNode::CPeerNetNode(CPeerNetDialog *pMain, CBlockChain *pBlkChain)
{
  BOOL b = InitializeCriticalSectionAndSpinCount(&g_cs, 0x80000400);
  m_pMainWin = pMain;       //���� ���������
  m_pBlkChain = pBlkChain;  //������ ����-����
  m_pSrvThread = NULL;      //����� ������� Listen
  m_nNodes = 0;
  m_iOwnNode = UNKNOWN_NODE;    //"����� ������������ ���� ���� ����������"
  memset(m_pSrvNPs, 0, sizeof(m_pSrvNPs));
  memset(m_pCliNPs, 0, sizeof(m_pCliNPs));
#ifdef _TRACE
  m_pTraceFile = NULL;
#endif
}
CPeerNetNode::~CPeerNetNode()
{
  ClearNode();
  DeleteCriticalSection(&g_cs);
} // CPeerNetNode::~CPeerNetNode()

//public:
// ���������������� ���� ������������ ����. ����� ���������� ��������:
//true - �������� ����������, false - ��������� ������
bool  CPeerNetNode::StartupNode()
{
  WORD iNode;  bool b, bRet = true;  UINT nRet = 0;
  if (IsNodeRunning())  return false;
#ifdef _TRACE
  TraceW(_T("StartupNode()-enter"), false);
#endif
  m_iOwnNode = UNKNOWN_NODE;
  // ��������� ������� ��� ���� ������������ ����� ����
  for (m_nNodes = iNode = 0; iNode < NODECOUNT; iNode++) {
    // ����� StartNPClient(iNode), ���� ����� �� ������� ������� ������, 
    //������ ������ CNamedPipe �� ������� ������� � ������� ��������� �� 
    //���� � ������ m_pCliNPs[iNode]:
    b = StartNPClient(iNode);
    if (!b && m_iOwnNode == UNKNOWN_NODE)
      // � ���� ��� ���� � ������� iNode - ������� � ������ ���������
      m_iOwnNode = iNode;
  }
  // �������� ����� "������" ���� - ������ ��������� ����� ���� ����
  if (m_iOwnNode == UNKNOWN_NODE) {
    // ��� �� ������ ���������� ������ ���� ���� - ������ ����������, 
    //���������� ����
    ShutdownNode();
  }
  else {
	// ���������� ����� "������" ���� � ��������� ����
    m_pMainWin->ShowOwnNodeNumber(m_iOwnNode);
	// ���� ������ ��������������� ������������
    bRet = ((m_iOwner = m_pBlkChain->GetAuthorizedUser()) != UNAUTHORIZED);
    if (bRet) {
      m_bTerm = false;
#ifdef _TRACE
      InitTracing();  //������� �������������� ����
#endif
      // ��������� ��������� ����� "������" ���� m_iOwnNode
      bRet = StartNPServer();
      if (bRet)  m_nNodes++;  //� ����� "������" ����� �������� "����" ����
    }
  }
#ifdef _TRACE
  TraceW(_T("StartupNode()-exit"), false);
#endif
  return bRet;
} // CPeerNetNode::StartupNode()

//public:
// ���������� ������ ���� ������������ ����
void  CPeerNetNode::ShutdownNode()
{
#ifdef _TRACE
  TraceW(_T("ShutdownNode()-enter"), false);
#endif
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNP;  UINT nRet;
  TMessageBuf mbRequest;
  // ���������� ��� ��������� ���� ���� �� �������� ������� ���� m_iOwnNode
  for (WORD iNode = 0; iNode < NODECOUNT; iNode++) {
    if (iNode == m_iOwnNode || (pNP = m_pCliNPs[iNode]) == NULL)
      continue;
    // ������� ������� ���� iNode ������� ������������ � ����� m_iOwnNode
    swprintf_s(szMessBuf, ERMES_BUFSIZE, _T("TRM %02d%02d"),
               m_iOwnNode, pNP->m_iCncNode);    //������������
    pNP->m_mbRequest.PutMessage(szMessBuf);     //���������
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("ShutdownNode(): Request sent: %s"),
               pNP->m_mbRequest.Message());
    pNP->TraceW(szMessBuf);
#endif
    // ������� ������ �� ������������ (����� �� �����)
    nRet = pNP->SendRequest();
  } // for
  // ���������� ����� ������� Listen: NPServerThreadProc()
  if (m_iOwnNode != UNKNOWN_NODE && m_pNPListen)
  {
    m_bTerm = true;     //������� ���� "���������� ����� ������� Listen"
    // ������������ ����� ������
    TCHAR  chPipeName[NAMEBUFSIZE];    //����� ��� ����� ������
    swprintf_s(chPipeName, NAMEBUFSIZE, PIPENAME_FRMT, m_iOwnNode);
    // ���������� ���������� �� "�����" ��������, ����� ������� ����� 
    //������� �� ��������
    HANDLE  hPipe = ::CreateFile(
      chPipeName,       //��� ������
      GENERIC_READ |    //��� �������: ������ (���� �� �������)
      GENERIC_WRITE,    // � ������ (�������� �������)
      0, NULL,          //no sharing, default security attributes
      OPEN_EXISTING,    //opens existing pipe
      0, NULL           //default attributes, no template file
    );
    // ��������� ����� �� ������
    CNamedPipe *pNPListen = (CNamedPipe *)m_pNPListen;
    if (!::DisconnectNamedPipe(pNPListen->m_hPipe)) {
      //������ ������������ ������� � ��������
      pNPListen->ProcessError(
        NULL, _T("::DisconnectNamedPipe(m_pNPListen->m_hPipe)")
      );
    }
    // Close the handles to the pipe instance
    BOOL bOK = ::CloseHandle(pNPListen->m_hPipe);
    delete m_pNPListen;  m_pNPListen = NULL;
    //m_iOwnNode = UNKNOWN_NODE;  m_nNodes = 0;
  }
#ifdef _TRACE
  TraceW(_T("ShutdownNode()-exit"), false);
  // ������� �������������� ����
  if (m_pTraceFile) {
    fclose(m_pTraceFile);  m_pTraceFile = NULL;
  }
#endif
  m_pBlkChain->CloseRegisters();
  ClearNode();
} // CPeerNetNode::ShutdownNode()

//  ��� ������� ���� ������� ��������� ���������� NP, ����� ���������.
//� ����� �� �������� ����������� ����� ������ �� ����������� ����� �� 
//�������� ���� kNode �������� ��������� ����� ������, ������� �� �����
//�������� ����� ����, � ������� �� ������ (iNode), ��� ��� �� ��� ������ 
//����� ��� �� ��������.
//  ����� ������������ ���� iNode ������������ ����� �������� ����������
//������� ��� ���� ������������ �������� �����.
//  ������������� ���������� ������ �� �������� ���� kNode �������������� ���
//��������� �� � ��� ������ Instance ��������� SCL <iNode><kNode><iUser>, ��� 
//iNode-����� ������������ ����, kNode-����� ��������� ����, �������� 
//����������� ����� Instance, iUser-����� ������������ � ���� iNode.

//  ��������� (���������� � ����������) ������������� ������� ��
//�������� �������� ���� � ������ ������� ��� ������� ������ �������
void  CPeerNetNode::ConnectRemoteClient(WORD iNode, WORD iOwner,
                                        CNamedPipe *pNP)
//iNode  - ������ ������������ ��������� ����
//iOwner - ������ ������������-��������� ��������� ����
//pNP    - ��������� ����� ������ ���� kNode, ������� �� ������ � ����� iNode
{
  EnterCriticalSection(&g_cs);
  // ������������� ���������� ������
  pNP->m_iCncNode = iNode;
  //pNP->m_iCncOwner = iOwner;
  m_pSrvNPs[iNode] = pNP;   //��������� ������ ���� kNode
  LeaveCriticalSection(&g_cs);
  // ��������� ������ �������, ������� ���������� � �������� ���� iNode
  StartNPClient(iNode);
  m_pMainWin->AddRemoteNode(iNode, iOwner);
} // CPeerNetNode::ConnectRemoteClient()

// ��������� (���������� � ����������) ������������ ������� �� �������� 
//�������� ���� �� ������ ������� ��� �������� ��������� ����
void  CPeerNetNode::DisconnectRemoteClient(WORD iNode)
{
  m_pMainWin->DeleteRemoteNode(iNode); //������� ���� �� ������
  EnterCriticalSection(&g_cs);
  m_pSrvNPs[iNode] = NULL;
  m_nNodes--;
  LeaveCriticalSection(&g_cs);
} // CPeerNetNode::DisconnectRemoteClient()

//public:
// ������� ���� ������ ������� ��������� ������������ ������ � �������� �����.
//����� ���������� ����� � ������������ ������ � ������� �������.
TMessageBuf * CPeerNetNode::RequestAndReply(WORD iNode, TMessageBuf &mbRequest)
//iNode[in]     - ����� ������������������ ���� ����
//mbRequest[in] - ����� � ��������
{
#ifdef _TRACE
  TraceW(_T("CPeerNetNode::RequestAndReply()-enter"), false);
#endif
  CNamedPipe *pNP = m_pCliNPs[iNode];   //����� NP �� ������ ����
  if (pNP == NULL)
  {
    // ���������� ������ - ������ �������������� �����
    TCHAR szMessBuf[ERMES_BUFSIZE];
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("CPeerNetNode::RequestAndReply(): NP %d not exists"), iNode);
#ifdef _TRACE
    TraceW(szMessBuf);
#endif
    return NULL;
  }
  return pNP->RequestAndReply(mbRequest);
} // CPeerNetNode::RequestAndReply()

// ��������� ��������� � ���� ������ � ������ ��������� "�� �����"
void  CPeerNetNode::WrapData(WORD iNodeTo, TMessageBuf &mbRequest)
//iNodeTo[in]       - ������ ����-���������� ���������
//mbRequest[in/out] - ����� ��������� (�������)
{
  TCHAR szReqst[CHARBUFSIZE];
  // ������������ ������ ��������� DTB iikkdddd
  swprintf_s(szReqst, CHARBUFSIZE, _T("DTB %02d%02d%04d"),
             m_iOwnNode, iNodeTo, mbRequest.MessageBytes());
  // �������� ������ ���� ������ � ���������� ������� ������
  WORD wLeng = (WORD)_tcslen(szReqst),  //������ ������ � �������� (��� 0)
       nSize = wLeng * sizeof(TCHAR);   //������ ������ � ������ (��� 0)
  memmove(mbRequest.Message() + wLeng,
          mbRequest.Message(), mbRequest.MessageBytes());
  memcpy(mbRequest.Message(), szReqst, nSize);
  mbRequest._wMessBytes += nSize;       //��������������� ����� ���������
} // CPeerNetNode::WrapData()

// ����������� ���� ������ � ��������� � ������ ��������� "�� �����"
void  CPeerNetNode::UnwrapData(WORD &iNodeFrom, TMessageBuf *pmbReply)
//iNodeFrom[out]  - ������ ����-����������� ���������
//mbReply[in/out] - ����� ��������� (������ �� ������)
{
  iNodeFrom = ToNumber(pmbReply->Message() + 4, 2);
  if (pmbReply->Compare(_T("DTB"), 3) != 0)  return;    //��� �� DTB
  // ������ ������ "�������� ��������", ���� ������ � '\0'
  WORD nSize = ToNumber(pmbReply->Message() + 8, 4);    //���� ������ � '\0'
  // �������� ���������� ������ ����� �� nSize ����
  memmove(pmbReply->Message(), pmbReply->Message() + 12, nSize);
  pmbReply->_wMessBytes = nSize; //����� �����
  // �������� �����
  memset(pmbReply->MessBuffer()+pmbReply->_wMessBytes, 0, 12*sizeof(TCHAR));
} // CPeerNetNode::UnwrapData()

//private:
// ��������� ��������� ������� ��������� ���� ���� ---------------------
bool  CPeerNetNode::StartNPClient(WORD iNode)
//iNode - ����� (������) ���� ����
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("StartNPClient(): Node %d: NP Client starting"), iNode);
  TraceW(szMessBuf);
#endif
  // ��������������� ������ ���������� ������������ ������
  CNamedPipe *pNP = new CNamedPipe(this, iNode);
  // ������������ ����� ������
  TCHAR  chPipeName[NAMEBUFSIZE];    //����� ��� ����� ������
  swprintf_s(chPipeName, NAMEBUFSIZE, PIPENAME_FRMT, iNode);
  // ���������� ������� ������� ����������� �����, ���� �� ����������.
  //������� ������������ �������, ���� ����� �����.
  while (true) {
    pNP->m_hPipe = ::CreateFile(
      chPipeName,       //��� ������
      GENERIC_READ |    //��� �������: ������ (���� �� �������)
      GENERIC_WRITE,    // � ������ (�������� �������)
      0, NULL,          //no sharing, default security attributes
      OPEN_EXISTING,    //opens existing pipe
      0, NULL           //default attributes, no template file
    );
    // ��� �������� �������� ������ ����� �� �����
    if (pNP->m_hPipe != INVALID_HANDLE_VALUE) {
#ifdef _TRACE
      TraceW(_T("StartNPClient(): ::CreateFile() returns OK"), false);
#endif
      break;
    }
    if (pNP->SysErrorCode() != ERROR_PIPE_BUSY) {
      // ������ �������� ������, �������� �� ERROR_PIPE_BUSY, - �����
      if (pNP->SysErrorCode() == ERROR_FILE_NOT_FOUND) {
        // ������ ERROR_FILE_NOT_FOUND - ��� ���� iNode
#ifdef _TRACE
        swprintf_s(szMessBuf, ERMES_BUFSIZE,
                   _T("- Net node #%d not found"), iNode);
        TraceW(szMessBuf);
#endif
        pNP->ClearError();
      }
      else
        pNP->ProcessError(_T("- Could not open pipe"), _T("::CreateFile()"));
      goto TraceErr;
    }
    // ��� ���������� ������ ������ - ����� �������� �����
    if (!::WaitNamedPipe(chPipeName, PIPE_TIMEOUT)) { //5000 ms
      pNP->ProcessError(_T("- Could not open pipe, time-out elapsed"),
                        _T("::CreateFile()"));
      //pNP->m_dwSysError = ERRCODE_BASE + 2;
      goto TraceErr;
    }
  } //while
  // ���������� �����������!
  //������� ������� ����� ���������� ������� - HANDLE m_hEvents[]:
  //0) m_hEvents[EvTermnReqst]-����� ������� "��������� ����������"
  //1) m_hEvents[EvReqstReady]-����� ������� "����� ������"
  //2) m_hEvents[EvRepReceivd]-����� ������� "������� �����"
  for (int i = 0; i < EVENTCOUNT; i++) {
    pNP->m_hEvents[i] = ::CreateEvent(
      NULL,     //default security attribute
      FALSE,    //TRUE: manual reset event, FALSE: auto-reset event
      FALSE,    //initial state = nonsignaled
      NULL      //unnamed event object
    );
    if (pNP->m_hEvents[i] == NULL) {
      // ������ �������� ������� Event:
      //����������� ���� ������ � ������������ ��������� �� ������.
      pNP->ProcessError(NULL, _T("::CreateEvent()"));
      goto TraceErr;
    }
  } // for
  // �������� ������������ ������ ������� ������������ ������
  pNP->m_pThread = AfxBeginThread(NPClientThreadProc, pNP);
  if (pNP->m_pThread) {
#ifdef _TRACE
    // �������������� ���������
    TraceW(_T("AfxBeginThread(NPClientThreadProc) returns OK."), false);
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("    hThread = 0x%08X, nThreadID = 0x%08X"),
               (UINT)pNP->m_pThread->m_hThread, pNP->m_pThread->m_nThreadID);
    TraceW(szMessBuf);
#endif
    // � ���� ���� ���� � ������� iNode � ������� ��������� ������� 
    //��� ������ � ���  - ������� � ������ �����-��������
    EnterCriticalSection(&g_cs);
    m_pCliNPs[iNode] = pNP;
    m_nNodes++; //������� �������� ����� ����, � �������� ����������� �����
    LeaveCriticalSection(&g_cs);
    //m_pMainWin->AddToNodeList(iNode);
  }
  else {
    // ������ �������� ������������ ������ �������.
    //����������� ���� ������ � ������������ ��������� �� ������
    pNP->ProcessError(NULL, _T("AfxBeginThread(NPClientThreadProc)"));
TraceErr:
    if (pNP != NULL) { delete pNP;  pNP = NULL; }
  }
  return (pNP != NULL);
} // CPeerNetNode::StartNPClient()

// ��������� ��������� ����� ���� ������������ ���� --------------------
bool  CPeerNetNode::StartNPServer()
//iNode - ����� ������������������ ���� ����
{
#ifdef _TRACE
  TraceW(_T("StartNPServer()-enter"), false);
#endif
  TCHAR szMessBuf[ERMES_BUFSIZE];  bool bRet;
  // ������� ����������� ����� �������, ���������� ����������� 
  //������������ ������ (����� Listen)
  m_pSrvThread = AfxBeginThread(NPServerThreadProc, this);
  if (m_pSrvThread) {
#ifdef _TRACE
    // �������������� ���������
    TraceW(
      _T("AfxBeginThread(NPServerThreadProc) returns OK."), false
    );
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("    hThread = 0x%08X, nThreadID = 0x%08X"),
               (UINT)m_pSrvThread->m_hThread, m_pSrvThread->m_nThreadID);
    TraceW(szMessBuf);
#endif
    bRet = true;
  }
  else
  {
    // ������ �������� ��������� ������������ ������ �������.
    DWORD dwSysError = ::GetLastError();
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("%s failed with error code %d"),
               _T("AfxBeginThread(NPServerThreadProc)"), dwSysError);
#ifdef _TRACE
    TraceW(szMessBuf);
#endif
    bRet = false;
  }
#ifdef _TRACE
  TraceW(_T("StartNPServer()-exit"), false);
#endif
  return bRet;
} // CPeerNetNode::StartNPServer()

// ��������� ������ (Thread) ���������� ����������� ������������ ������
//static
UINT __cdecl  CPeerNetNode::NPServerThreadProc(LPVOID lpvParam)
//lpvParam - ��������� ������� CPeerNetNode
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNPListen;
  CPeerNetNode *pNetNode = (CPeerNetNode *)lpvParam;
  if (pNetNode == NULL)  return 1;  //�������� �������� lpvParam
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NPServerThreadProc()-enter, nThreadID = 0x%08X"),
             pNetNode->m_pSrvThread->m_nThreadID);
  pNetNode->TraceW(szMessBuf);
#endif
  // ������������ ����� ������
  BOOL  bSuccess;
  TCHAR  chPipeName[NAMEBUFSIZE];   //����� ��� ����� ������
  swprintf_s(chPipeName, NAMEBUFSIZE, PIPENAME_FRMT, pNetNode->m_iOwnNode);
  WORD iNode;  bool bNode = pNetNode->GetFirstNode(iNode);
  // ���� �������� ����������� ������� ����������� ������������ ������ 
  //�� ���� ����������� �������� � ������� ����
  while (true) {
    pNPListen = new CNamedPipe(pNetNode);
    pNetNode->m_pNPListen = pNPListen;
    ASSERT(pNPListen);
    /* ������� ������� ����� �������� ������� - HANDLE m_hEvents[]:
    //0) m_hEvents[EvSaveListen]-����� ������� "��������� NP Listen"
    //1) m_hEvents[EvNewListen] -����� ������� "��������� ������� NP Listen"
    //2) m_hEvents[EvInstActive]-����� ������� "����� Instance �������"
    for (int i = 0; i < EVENTCOUNT; i++) {
      pNPListen->m_hEvents[i] = ::CreateEvent(
        NULL,     //default security attribute
        FALSE,    //TRUE: manual reset event, FALSE: auto-reset event
        FALSE,    //initial state = nonsignaled
        NULL      //unnamed event object
      );
      if (pNPListen->m_hEvents[i] == NULL) {
        // ������ �������� ������� Event:
        //����������� ���� ������ � ������������ ��������� �� ������.
        pNPListen->ProcessError(NULL, _T("::CreateEvent()"));
        goto TraceErr;
      }
    } // for */
    //�������� ���������� ������������ ������
    pNPListen->m_hPipe = ::CreateNamedPipe(
      chPipeName,               //pipe name
      PIPE_ACCESS_DUPLEX,       //read/write access
      PIPE_TYPE_MESSAGE |       //message type pipe
      PIPE_READMODE_MESSAGE |   //message-read mode
      PIPE_WAIT,                //blocking mode
      PIPE_UNLIMITED_INSTANCES, //max. instances
      BUFSIZE, BUFSIZE,         //output and input buffer size
      0, NULL                   //client time-out, default security attribute
    );
    if (pNPListen->m_hPipe == INVALID_HANDLE_VALUE) {
      pNPListen->ProcessError(NULL, _T("::CreateNamedPipe()"));
      return 2; //���������� ������ � ����� ������
    }           // �������� ���������� ������������ ������
#ifdef _TRACE
    // �������������� ���������
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
      _T("NPServerThreadProc(): %s Pipe handle = 0x%08X"),
      _T("::CreateNamedPipe() returns OK."), (UINT)pNPListen->m_hPipe);
    pNetNode->TraceW(szMessBuf);
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("Wait for the client of node #%d to connect..."),
               pNPListen->m_iCncNode);
    pNetNode->TraceW(szMessBuf);
#endif
    if (bNode) {
      pNetNode->NotifyNode(iNode);
      bNode = pNetNode->GetNextNode(iNode);
    }
    //----- ����� ���������� �� ������� ������� -----//
    bSuccess = ::ConnectNamedPipe(pNPListen->m_hPipe, NULL);
    if (bSuccess) {
      // ���������� �� ������� ������� ������� �����������,
      if (pNetNode->m_bTerm)  break;    //�� ������ ���� �������� ���� -
    }                                   // ��������� ��������� �����
    else {
      // ���� ConnectNamedPipe ���������� 0, � GetLastError ���������� 
      //��� ERROR_PIPE_CONNECTED, �� ��� ����� �������� ����������.
      bSuccess = (::GetLastError() == ERROR_PIPE_CONNECTED) ? TRUE : FALSE;
    }
    if (!bSuccess) {
      pNPListen->ProcessError(NULL, _T("::ConnectNamedPipe()"));
      ::CloseHandle(pNPListen->m_hPipe);
      return 3; //���������� ������ � ����� ������
    }           // ���������� ������� � ��������
    // �������� ���������� ������� � ��������
#ifdef _TRACE
    // �������������� ���������
    pNetNode->TraceW(
      _T("NPServerThreadProc(): ::ConnectNamedPipe() returns OK"), false
    );
#endif
    //CWinThread * ������� � ��������� ����� ���������� ������������ ������
    pNPListen->m_pThread =
      AfxBeginThread(NPInstanceThreadProc, pNPListen);
    if (!pNPListen->m_pThread) {
      // ������ �������� ������������ ������ ���������� ������������ ������.
      //����������� ���� ������ � ������������ ��������� �� ������
      pNPListen->ProcessError(
        NULL, _T("AfxBeginThread(NPInstanceThreadProc)")
      );
      return 4; //���������� �������� ������ NPServerThreadProc
    }           // � ����� ������ �������� ������ NPInstanceThreadProc
    // �������� �������� ������������ ������ ���������� ������������ ������
#ifdef _TRACE
    // �������������� ���������
    pNetNode->TraceW(
      _T("AfxBeginThread(NPInstanceThreadProc) returns OK."), false
    );
    swprintf_s(
      szMessBuf, ERMES_BUFSIZE,
      _T("    hThread = 0x%08X, nThreadID = 0x%08X"),
      (UINT)pNPListen->m_pThread->m_hThread, pNPListen->m_pThread->m_nThreadID
    );
    pNetNode->TraceW(szMessBuf);
#endif
  } //while()
//TraceErr:
#ifdef _TRACE
  pNetNode->TraceW(_T("NPServerThreadProc()-exit"), false);
#endif
  return 0;     //�������� ���������� ������
} // CPeerNetNode::NPServerThreadProc()

// ��������� ������ (Thread) ���������� ������������ ������
//static
UINT __cdecl  CPeerNetNode::NPInstanceThreadProc(LPVOID lpvParam)
//lpvParam - ��������� ������� CNamedPipe
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNP = (CNamedPipe *)lpvParam;
  if (pNP == NULL || !pNP->IsKindOf(RUNTIME_CLASS(CNamedPipe)))
    return 1;   //�������� �������� lpvParam
#ifdef _TRACE
  pNP->TraceW(_T("NPInstanceThreadProc()-enter"), false);
#endif
  UINT nRet = 0;  bool bStartClient = false;  BOOL bSuccess;
  ECommandCode eCmd;  WORD iFrom, iTo, iOwner;
  DWORD dwBytesRead, dwReplyBytes, dwWritten;
  // ���� ������ � ��������, ����������� ������ ��������� ������
  while (true) {
    // ����� � ��������� ������ �������
#ifdef _TRACE
    pNP->TraceW(_T("NPInstanceThreadProc(): waiting ::ReadFile()"), false);
#endif
    bSuccess = ::ReadFile(
      pNP->m_hPipe,             //handle to pipe
      pNP->RequestMessage(),    //buffer to receive data m_chRequest
      BUFSIZE,                  //size of buffer
      &dwBytesRead,             //number of bytes read
      NULL                      //not overlapped I/O
    );
    if (!bSuccess || (dwBytesRead == 0)) {
      // ������ ������ �� ������
      pNP->ProcessError(NULL, _T("::ReadFile()"));
      nRet = 5;  break;     //��������� ������
    }
    pNP->m_mbRequest._wMessBytes = (WORD)dwBytesRead;
    // ������ ������� ��������
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("NPInstanceThreadProc(): ::ReadFile(): %s"),
               pNP->RequestMessage());
    pNP->TraceW(szMessBuf);
#endif
    eCmd = ParseMessage(pNP->m_mbRequest, iFrom, iTo);  //��������� �������
    ASSERT(eCmd != CmC_NONE);
    //ASSERT(iFrom == pNP->m_iCncNode);
    if (eCmd == CmC_SCL) {
      // ������� �������� ���� <From> - ���������� ��������� ���� � ������� 
      //���� ������� ������� �� ������ <From>
      iOwner = ToNumber(pNP->m_mbRequest.Message() + 8, 2);
      //pNP->m_pArentNode->ConnectRemoteClient(iFrom, iOwner, pNP);
      bStartClient = true;
    }
    else if (eCmd == CmC_TRM)   //������ Terminate
    {
      // ���������� ������������ � �������� ����� � ����������
      pNP->m_pArentNode->DisconnectRemoteClient(iFrom); //pNP->m_iCncNode
      // ������� NP ������� ���� iFrom � ...
      CNamedPipe *pCliNP = pNP->m_pArentNode->m_pCliNPs[iFrom];
      //��������� � �� ������� EvTermnReqst ��� ���������� ����������� ������
      bSuccess = ::SetEvent(pCliNP->m_hEvents[EvTermnReqst]);
      if (!bSuccess) {
        // ������ ������ SetEvent(): ��������� ��� ������ � ������� 
        //��������� �� ������.
        pCliNP->ProcessError(NULL, _T("::SetEvent(EvTermnReqst)"));
      }
      break;    //����� �� ������ Terminate �� �����
    }
    // �������� ����� �� ������
    if (!pNP->m_pArentNode->GetAnswerToRequest(pNP))
    { nRet = 6;  break; }   //������
    // �������� ����� � �����
    dwReplyBytes = (DWORD)pNP->m_mbReply.MessageBytes();
    bSuccess = ::WriteFile(
      pNP->m_hPipe,         //handle to pipe
      pNP->ReplyMessage(),  //buffer to write from
      dwReplyBytes,         //number of bytes to write
      &dwWritten,           //number of bytes written
      NULL                  //not overlapped I/O
    );
    if (!bSuccess || dwReplyBytes != dwWritten) {
      pNP->ProcessError(NULL, _T("::WriteFile()"));
      nRet = 7;  break;
    }
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("NPInstanceThreadProc(): ::WriteFile(): %s"),
               pNP->ReplyMessage());
    pNP->TraceW(szMessBuf);
#endif
    if (bStartClient) {
      // ���������� � ���������� ������������� ������� �� �������� 
      //���� <From> � ������ ������� ��� ��� �������
      pNP->m_pArentNode->ConnectRemoteClient(iFrom, iOwner, pNP);
      bStartClient = false;
    }
  } //while
  // �������� ������������ ������
  //Flush the pipe to allow the client to read the pipe's contents
  // before disconnecting. Then disconnect the pipe, and close the
  // handle to this pipe instance.
  try {
    bSuccess = ::FlushFileBuffers(pNP->m_hPipe);
    bSuccess = ::DisconnectNamedPipe(pNP->m_hPipe);
    bSuccess = ::CloseHandle(pNP->m_hPipe);  //� ��� ��� �������� ������
  }
  catch (...) {
    ;
  }
#ifdef _TRACE
  // �������������� ���������
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NPInstanceThreadProc() of pipe #%d exited with code %d"),
             pNP->m_iCncNode, nRet);
  pNP->TraceW(szMessBuf);
#endif
  // ���������� ������������ � �������� ����� � ����������
  //pNP->m_pArentNode->DisconnectRemoteClient(pNP->m_iCncNode);
  delete pNP;
  return nRet;
} // CPeerNetNode::NPInstanceThreadProc()

// ��������� ������ (Thread) ������� ������������ ������
//static
UINT __cdecl  CPeerNetNode::NPClientThreadProc(LPVOID lpvParam)
//lpvParam - ��������� ������� CNamedPipe
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNP = (CNamedPipe *)lpvParam;
  if (pNP == NULL || !pNP->IsKindOf(RUNTIME_CLASS(CNamedPipe)))
    return 1;   //�������� �������� lpvParam
#ifdef _TRACE
  pNP->TraceW(_T("NPClientThreadProc()-enter"), false);
#endif
  UINT nRet = 0;  DWORD dwWait;
  // ���� ������ � �������� ���� pNP->m_iCncNode
  while (true) {
    // ����� ����� �� ���� �������:
    //EvTermnReqst(0) - "��������� �����" 
    //EvReqstReady(1) - "������ � ������� �����"
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("Wait for a request to node #%d or termination signal"),
               pNP->m_iCncNode);
    pNP->TraceW(szMessBuf);
#endif
    // dwWait shows which event was signaled
    dwWait = ::WaitForMultipleObjects( 
      2, pNP->m_hEvents,    //number and array of event objects
      FALSE, INFINITE       //����� �� ��� ������� �����, waits indefinitely
    );
    int i = dwWait - WAIT_OBJECT_0; //determines which event
    if (i < 0 || 1 < i) {
      // ������ �������� �������
      pNP->ProcessError(
        NULL, _T("::WaitForMultipleObjects(EvTermnReqst|hEvReqstReady)")
      );
      nRet = 8;  break;
    }
    if (i == 0)  break; //��� ������� EvTermnReqst - ��������� �����
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("EvReqstReady signal got from node #%d to node #%d"),
               pNP->m_pArentNode->m_iOwnNode, pNP->m_iCncNode);
    pNP->TraceW(szMessBuf);
#endif
    // ������� ������ "������ �����" - �������� ������ �������
    if ((nRet = pNP->SendRequest()) != 0 ||
        // ���� ��� ������� ������ TERMINATE, ��������� �����
        pNP->m_mbRequest.Compare(_T("TRM"), 3) == 0)  break;
    // ����� � �������� ����� ������� (�� �� �� TRM)
    if ((nRet = pNP->ReceiveReply()) != 0)  break;
  } //while
  pNP->m_pArentNode->m_pCliNPs[pNP->m_iCncNode] = NULL;
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NPClientThreadProc()-exit. Return code %d"), nRet);
  pNP->TraceW(szMessBuf);
#endif
  delete pNP;
  return nRet;
} // CPeerNetNode::NPClientThreadProc()

// ��������� ������. ����� ���������� ��� ��������.
//static
ECommandCode  CPeerNetNode::ParseMessage(TMessageBuf &mbRequest,
                                         WORD &wFrom, WORD &wTo)
//mbRequest[in] - ������ �� ����� � ��������
//wFrom[out]    - ������ �� ����� ���������� ���� ����
//wTo[out]      - ������ �� ����� ��������� ���� ����
{
  static LPTSTR sCmnds[] = {
//������             ��� �������
/* 0 */ _T("ACK"),  //1: Acknowledge  - ���������/�����������
/* 1 */ _T("DTB"),  //2: Data block   - ���� ������
/* 2 */ _T("SCL"),  //3: Start client - ��������� ������
/* 3 */ _T("TRM"),  //4: Terminate    - ������� �����
  };
  static WORD n = sizeof(sCmnds) / sizeof(LPTSTR);
  for (WORD i = 0; i < n; i++) {
    if (mbRequest.Compare(sCmnds[i], 3) == 0) {
      // ������� ����������, �������� ���������
      wFrom = ToNumber(mbRequest.Message() + 4, 2);   //<From>
      wTo = ToNumber(mbRequest.Message() + 6, 2);     //<To>
      return (ECommandCode)(i + 1);
    }
  }
  return CmC_NONE;   //������� �� ����������
} // CPeerNetNode::ParseMessage()

// ��������� First-Next �������� �������� ����� ����
bool  CPeerNetNode::GetNextNode(WORD &iNode, bool bAllNodes/* = false*/,
                                bool bFirst/* = false*/)
//iNode[out]    - ������ ���������� ����
//bAllNodes[in] - true ="���������� ��� ����",
//                false="���������� ���, ����� ������"
//bFirst[in]    - true="������ � ������", false="����������"
{
  static WORD i;
  if (bFirst)  i = 0;
  while (i < NODECOUNT) {
    if ((bAllNodes && i == m_iOwnNode) ||
        (i != m_iOwnNode && m_pCliNPs[i] != NULL)) {
      iNode = i++;  return true;
    }
    i++;
  }
  return false;
} // CPeerNetNode::GetNextNode()

// ������������� �������� ������� � �����
//static
WORD  CPeerNetNode::ToNumber(LPTSTR lpsNum, WORD wLeng)
{
  WORD n = 0;  TCHAR c;
  for (WORD i = 0; i < wLeng; i++) {
    c = lpsNum[i];
    if (isdigit(c))  n = n * 10 + (c - _T('0'));
  }
  return n;
} // CPeerNetNode::ToNumber()

// ������������ ����� �� ������ �������. ����� ���������� ��������: 
//true - �������� ����������, false - ���������������� ������ ���������
bool  CPeerNetNode::GetAnswerToRequest(CNamedPipe *pNP)
//pNP - ��������� ������� CNamedPipe
{
  TCHAR szMessBuf[CHARBUFSIZE], *pszFrmt;
  WORD iNodeFrom, iNodeTo;  bool b = true;
  ECommandCode eCmd = ParseMessage(pNP->m_mbRequest, iNodeFrom, iNodeTo);
  //ASSERT(iNodeFrom == pNP->m_iCncNode &&
  //       iNodeTo == pNP->m_pArentNode->m_iOwnNode);
  if (eCmd == CmC_SCL) {
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("SCL %02d%02d%02d"), iNodeTo, iNodeFrom, m_iOwner);
  }
  else {
    if (eCmd == CmC_DTB) {
      EnterCriticalSection(&g_cs);
      b = m_oDTBQueue.Put(pNP->m_mbRequest);    //��������� DTB � �������
      //b = m_pBlkChain->GetReplyOnDTB(pNP->m_mbRequest, szMessBuf);
      LeaveCriticalSection(&g_cs);
	  b = (pNP->m_pArentNode->m_pMainWin->PostMessage(WM_DTBQUEUED) == TRUE);
    }
    //else  b = false;
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("ACK %02d%02d"), iNodeTo, iNodeFrom);
  }
  pNP->m_mbReply.PutMessage(szMessBuf); //���������
  return b;
} // CPeerNetNode::GetAnswerToRequest()

// ���������� �������� ���� � ������ "������" ���� � �������� �����
void  CPeerNetNode::NotifyNode(WORD iNode)
{
  TMessageBuf  mbRequest;           //����� ������� �������
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNP = m_pCliNPs[iNode]; //���������� NP, ��������� � ����� iNode
  //������� � ����� ������ Start CLient.
  swprintf_s(szMessBuf, ERMES_BUFSIZE, _T("SCL %02d%02d%02d"),
             m_iOwnNode, iNode, m_iOwner);
  mbRequest.PutMessage(szMessBuf);
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NotifyNode(): Request sent: %s"), mbRequest.Message());
  pNP->TraceW(szMessBuf);
#endif
  // ������� ������ � �������� �����
  pNP->m_mbRequest = mbRequest;     //��������� ������ � �����
  UINT nRet = pNP->SendRequest();
  // ����� � �������� ����� �������
  nRet = pNP->ReceiveReply(true);
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NotifyNode(): Reply received: %s"),
             pNP->m_mbReply.Message());
  pNP->TraceW(szMessBuf);
#endif
  WORD iOwner = ToNumber(pNP->m_mbReply.Message() + 8, 2);
  m_pMainWin->AddRemoteNode(iNode, iOwner);
} // CPeerNetNode::NotifyNode()

// �������� ����
void  CPeerNetNode::ClearNode()
{
  int i;
  m_pSrvThread = NULL;
  m_iOwnNode = UNKNOWN_NODE;  m_nNodes = 0;
  for (i = 0; i < NODECOUNT; i++)
    if (m_pSrvNPs[i]) { delete m_pSrvNPs[i];  m_pSrvNPs[i] = NULL; }
  for (i = 0; i < NODECOUNT; i++)
    if (m_pCliNPs[i]) { delete m_pCliNPs[i];  m_pCliNPs[i] = NULL; }
} // CPeerNetNode::ClearNode()

#ifdef _TRACE
// ���������� ������� �����������:
// ���������������� �������� �����������
void  CPeerNetNode::InitTracing()
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  // ������������ ��� ��������������� �����
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("%sPNNtrace_%d.log"), TRACEFILEPATH, m_iOwnNode);
  errno_t err = _wfopen_s(&m_pTraceFile, szMessBuf, _T("wt"));
  TraceW(_T("(c) DIR, 2019. Peer Net Node tracing file"), false);
} // CPeerNetNode::InitTracing()

// ������� �������������� ���������
void  CPeerNetNode::TraceW(LPTSTR lpszMes, bool bClearAfter/* = true*/)
{
  //EnterCriticalSection(&g_cs);
  SYSTEMTIME st;  TCHAR szMessBuf[ERMES_BUFSIZE];
  GetSystemTime(&st);
  swprintf_s(szMessBuf, ERMES_BUFSIZE, _T("%02d:%02d:%02d.%03d - %s"),
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, lpszMes);
  m_pMainWin->ShowTrace(szMessBuf);
  if (CPeerNetNode::m_pTraceFile != NULL) {
    // �������������� ���� ������
    fwprintf_s(CPeerNetNode::m_pTraceFile, _T("%s\n"), szMessBuf);
  }
  if (bClearAfter)  *lpszMes = L'\0';   //������� ������ ���������
  //LeaveCriticalSection(&g_cs);
} // CPeerNetNode::TraceW()
#endif

// ����� ����������� ������ CPeerNetNode -------------------------------
