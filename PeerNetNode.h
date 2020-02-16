// PeerNetNode.h                                           (c) DIR, 2019
//
//              ���������� �������:
// CNamedPipe   - ����������� �����
// CPeerNetNode - ���� ������������ ����
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

#ifdef _DEBUG
 #define  _TRACE        //�������� �����������
#else
 #undef  _TRACE
#endif

//======================================================================
// ���������� ������ CNamedPipe -
//  ������� ��� ������ � ������������ ��������
//----------------------------------------------------------------------

#define  NODECOUNT      7       //���� ���������� ����� ������������ ����
#define  NAMEBUFSIZE    32      //������ ������ ����� ������ � ��������
#define  CHARBUFSIZE    256     //������ ������ ��������� � ��������
#define  BUFSIZE        (CHARBUFSIZE*sizeof(TCHAR)) //������ ������ ����� � ������
#define  ERMES_BUFSIZE  256     //������ ������ ����� �� ������ � ������
#define  UNKNOWN_NODE   99      //"�����" ������������ ���� ����
#define  UNAUTHORIZED   0xFFFF  //���������������� ������������
#define  ERRCODE_BASE   800     //���� ����������� ����� ������
// ������ ��� ������������ ����� ������
#define  PIPENAME_FRMT  _T("\\\\.\\pipe\\PeerNetNP_%d")
#ifdef _TRACE
 // ������������ �������� ��� �������������� ������
 //#define  TRACEFILEPATH  _T("D:\\Programming\\NamedPipe\\PeerNetNode\\")
 #define  TRACEFILEPATH  _T("..\\")
#endif
// ������� ������ � �������������
#define  PIPE_TIMEOUT   2000
#define  CLCQUEUESIZE   8       //������ ����������� �������
#define  CLCQUEUEMASK   0x0007  //����� ��������� ������������ �������
#define  WM_DTBQUEUED   (WM_USER+1) //���������: ���� DTB � �������

// ��������� ������������ ������
enum EPipeStatus {
  PS_None = 0,          //�������������������� ��������� ������
  PS_Created,           //����� ������
  PS_Listen,            //�������� ������� �� ���������� �� �������
  PS_Connected,         //����� �����������
  PS_RequestWaiting,    //�������� ������� �� �������
  PS_ReplySending,      //���������� � ����������� ������ �������
  PS_Disconnected,      //����� ���������
};

// �������������� (����) ������
enum ECommandCode {
  CmC_NONE = 0,     //������� �� ����������
  CmC_ACK,          //1: Acknowledge        - ���������/�����������
  CmC_DTB,          //2: Data block         - ���� ������
  CmC_SCL,          //3: Start client       - ��������� ������
  CmC_TRM,          //4: Terminate          - ������� �����
};

#define  EVENTCOUNT    3    //���������� ������� ������������� � ������ NP
// ��� ���������� �������
#define  EvTermnReqst  0    //������ ������� "��������� ����������"
#define  EvReqstReady  1    //������ ������� "����� ������"
#define  EvRepReceivd  2    //������ ������� "������� �����"
// ��� ��������� �������
#define  EvSaveListen  0    //������ ������� "��������� NP Listen"
#define  EvNewListen   1    //������ ������� "��������� ������� ����� NP Listen"
#define  EvInstActive  2    //������ ������� "����� Instance �������"

// ������ ��� ������ � ����������� �������
enum EPipeError {
  PE_OK = 0,  //��� ������
  PE_Create,  //������ �������� ������������ ������
  PE_Connect, //������ ���������� �� ������������ ������
  PE_Read,    //������ ������ ������ �� ������
  PE_Write,   //������ ������ ������ � �����
};

// ����������� ��� "����� ��������� ��� ������ �� ����"
struct TMessageBuf {
  TCHAR  _chMess[CHARBUFSIZE];  //����� ��������� (TCHAR=wchar_t)
  WORD   _wMessBytes;   //����� ����� ��������� � ������ (������ � '\0')
  // ������������:
  // ����������� �� ���������
  TMessageBuf() {
    memset(_chMess, 0, sizeof(_chMess));  _wMessBytes = 0;
  }
  // ���������������� �����������
  TMessageBuf(BYTE *pData, WORD wLeng) { Put(pData, wLeng); }
  TMessageBuf(LPTSTR lpszMes) { PutMessage(lpszMes); }
  // ���� ������ ������ � ������
  WORD    BufferSize() { return (WORD)BUFSIZE; }
  // ���� ��������� ������ ��� ��������� �������
  BYTE  * MessBuffer() { return (BYTE *)_chMess; }
  // ���� ��������� ���������, ������������ � ������
  LPTSTR  Message() { return _chMess; }
  // ���� ����� ��������� � ��������
  WORD    MessageLength() { return (WORD)_tcslen(_chMess); }
  // ���� ����� ����� ��������� � ������ ������ � '\0'
  WORD    MessageBytes() { return _wMessBytes; }
  // ��������� � ����� ���� ������
  void    Put(BYTE *pData, WORD wLeng)
  {
    _wMessBytes = min(wLeng, sizeof(_chMess));
    memcpy(_chMess, pData, _wMessBytes);
    // �������� �����
    if (sizeof(_chMess) > _wMessBytes)
      memset(MessBuffer()+_wMessBytes, 0, sizeof(_chMess)-_wMessBytes);
  }
  // ��������� � ����� ��������� ������
  void    PutMessage(LPTSTR lpszMes)
  {
    WORD wLeng = ((WORD)_tcslen(lpszMes) + 1) * sizeof(TCHAR);
    //_tcscpy_s(_chMess, CHARBUFSIZE-1, lpszMes);
    Put((BYTE *)lpszMes, wLeng);
    _chMess[CHARBUFSIZE-1] = _T('\0');  //��� ��������, ����� �������� ������
  }                                     // �� ��������� � ������
  // �������� ������� ��������� � ���������� � ������
  int     Compare(LPTSTR lpszMes, WORD nChars = 0)
  //lpszMes - ������ �������� ���������
  //nChars  - ���������� ������������ ��������; ���� �������� ������
  //           (nChars == 0), �� ���������� ��� ������
  {
    return (nChars == 0) ? _tcscmp(_chMess, lpszMes) :
                           _tcsncmp(_chMess, lpszMes, nChars);
  }
}; // End of struct TMessageBuf declaration ----------------------------

extern CRITICAL_SECTION  g_cs; //����������� ������

////////////////////////////////////////////////////////////////////////
// ����� CCyclicQueue - ����������� ������� ������ ���������

class CCyclicQueue
{
  // ������
  TMessageBuf  m_mbMsg[CLCQUEUESIZE];   //������� ������ ���������
  int          m_iHead;     //������ ������ �������
  int          m_iTail;     //������ ������ �������
  int          m_nCount;    //���������� ������ � �������
public:
  // ����������� � ����������
  CCyclicQueue();
  ~CCyclicQueue() {}
  bool  IsEmpty() { return (m_nCount == 0); }
  bool  Put(TMessageBuf &mbMsg);
  bool  Get(TMessageBuf *&pmbMsg);
private:
  void  Increment(int &iNdx);
}; // End of class CCyclicQueue declaration ----------------------------

class CPeerNetNode;
class CPeerNetDialog;
class CBlockChain;

////////////////////////////////////////////////////////////////////////
// ����� CPeerNetNode - ���� ������������ ����

class CNamedPipe : public CObject
{
public:
  CPeerNetNode *m_pArentNode;   //��������� ������������� ������� CPeerNetNode
  CWinThread   *m_pThread;      //��������� ������, ���������� � ������ NP
  // ����� ������������������ ���� ���� (���������������� ����� NP),
  //�� ����� ������� ��������� ������� CNamedPipe � ��������
  //CPeerNetNode::m_pSrvNPs[NODECOUNT] � CPeerNetNode::m_pCliNPs[NODECOUNT]:
  WORD          m_iCncNode;
  HANDLE        m_hPipe;        //����� (����������) ���������� NP
  HANDLE        m_hEvents[EVENTCOUNT];  //������ ������� ������������� �������
  //m_hEvents[0]-hEvTermnReqst;   //����� ������� "��������� ����������"
  //m_hEvents[1]-hEvReqstReady;   //����� ������� "����� ������"
  //m_hEvents[2]-hEvRepReceivd;   //����� ������� "������� �����"
//protected:
  bool          m_bUsy;         //���� "����� �����"
  CCyclicQueue *m_pQueue;       //������������� ������� ������ �� ��������
  TMessageBuf   m_mbRequest;    //����� ������� � �������
  TMessageBuf   m_mbReply;      //����� ������ �������
  DWORD         m_nRead;        //���������� �������� ���������
  DWORD         m_nWritten;     //���������� ������������ ���������
  EPipeStatus   m_eStatus;      //��������� ������
  EPipeError    m_eRrorCode;    //��� ������
  DWORD  m_dwSysError;          //��������� ����� ������
  TCHAR  m_szErrorMes[ERMES_BUFSIZE];   //����� ��������� �� ������

public:
  CNamedPipe(CPeerNetNode *pArentNode, WORD iCncNode = UNKNOWN_NODE);
  virtual ~CNamedPipe();

  // ���� ������ ������ � ������
  WORD    BufferSize() { return (WORD)BUFSIZE; }
  // ���� ��������� ������ �������
  BYTE  * RequestBuffer() { return m_mbRequest.MessBuffer(); }
  // ���� ��������� ��������� � ������ �������
  LPTSTR  RequestMessage() { return m_mbRequest.Message(); }
  // ���� ����� ������� � �������� (��� ����� ������������ 0)
  WORD    RequestLength() { return m_mbRequest.MessageLength(); }
  // ���� ����� ������� � ������ � ������ ������������ 0
  WORD    RequestBytes() { return m_mbRequest.MessageBytes(); }
  // ���� ��������� ������ ������
  BYTE  * ReplyBuffer() { return m_mbReply.MessBuffer(); }
  // ���� ��������� ��������� � ������ ������
  LPTSTR  ReplyMessage() { return m_mbReply.Message(); }
  // ���� ����� ������ � �������� (��� ����� ������������ 0)
  WORD    ReplyLength() { return m_mbReply.MessageLength(); }
  // ���� ����� ������ � ������ � ������ ������������ 0
  WORD    ReplyBytes() { return m_mbReply.MessageBytes(); }
  // ������� ������ ������� ������� ������������ ������
  UINT    SendRequest();
  // ������� ����� ������� ������� ������������ ������
  UINT    ReceiveReply(bool bNoEvent = false);
  // ������� ������ ������� ������� ������������ ������ � �������� �����
  TMessageBuf * RequestAndReply(TMessageBuf &mbRequest);
  // ������ ��������� ������
  DWORD   SysErrorCode() {
    if (m_dwSysError == 0)  m_dwSysError = ::GetLastError();
    return m_dwSysError;
  }
  bool    Success() { return (m_eRrorCode == PE_OK && SysErrorCode() == 0); }
  void    ClearError() {
    m_eRrorCode = PE_OK;  m_dwSysError = 0;  m_szErrorMes[0] = _T('\0');
  }
  LPTSTR  ErrorMessage(LPTSTR lpszFunction = NULL);
  void    ProcessError(LPTSTR lpszMesg, LPTSTR lpszFunction = NULL);
//private:
#ifdef _TRACE
  //static  ������� �������������� ���������
  void    TraceW(LPTSTR lpszMes, bool bClearAfter = true);
#endif
}; // End of class CNamedPipe declaration ------------------------------

////////////////////////////////////////////////////////////////////////
// ����� CPeerNetNode - ���� ������������ ����

class CPeerNetNode
{
public:
  CPeerNetDialog  * m_pMainWin;     //������ �������� ���� ���������
  CBlockChain     * m_pBlkChain;    //������ ������ ����-����
  volatile CNamedPipe * m_pNPListen;    //NP � ��������� Listen static
  CWinThread  * m_pSrvThread;   //����� ������� Listen
  WORD          m_nNodes;       //���������� �������� ����� ����
  WORD          m_iOwnNode;     //������ "������" ���� ���� (0,1,..,n-1)
  WORD          m_iOwner;       //������ ������������-��������� ����
  CNamedPipe  * m_pSrvNPs[NODECOUNT];   //��������� NP ����
//WORD          m_nSrvNPs;  //���������� ���������� ������������ ��������� NPs
  CNamedPipe  * m_pCliNPs[NODECOUNT];   //���������� NP ����
  CCyclicQueue  m_oDTBQueue;    //����������� ������� �������� DTB
//HANDLE        m_hMayStartClient;      //����� (����������) �������
//TList         m_FreeNodes;            //������ "���������" ������� �����
  bool   m_bTerm;           //���� "���������� ����� ������� Listen"
//bool   m_bSrvStarting;    //���� ������ ������� �������
#ifdef _TRACE
//static CFile  m_oTraceFile;   //�������������� ����
  FILE * m_pTraceFile;  //�������������� ����
#endif

  // ����������� � ����������
  CPeerNetNode(CPeerNetDialog *pMain, CBlockChain *pBlkChain);
  ~CPeerNetNode();

  // ������� ������
  // -------------------------------------------------------------------
  // ���������������� ������ ���� ������������ ����
  bool  StartupNode();
  // ���������� ������ ���� ������������ ����
  void  ShutdownNode();
  // ������� ���� ������ ������� ��������� ������������ ������ � �������� �����
  TMessageBuf * RequestAndReply(WORD iNodeTo, TMessageBuf &mbRequest);
  // ��������� ��������� � ���� ������
  void  WrapData(WORD iNodeTo, TMessageBuf &mbRequest);
  // ����������� ���� ������ � ���������
  void  UnwrapData(WORD &iNodeFrom, TMessageBuf *pmbReply);
  // ������� �� ���� ����?
  bool  IsNodeRunning() { return (m_nNodes > 0); }
  // ���� �� ������ ��� �����������? ��������
  bool  IsQuorumPresent() { return true; } //(m_nNodes > (NODECOUNT >> 1))
  // ��������� First-Next �������� �������� ����� ����
  bool  GetFirstNode(WORD &iNode, bool bAllNodes = false)
  { return GetNextNode(iNode, bAllNodes, true); }
  bool  GetNextNode(WORD &iNode, bool bAllNodes = false, bool bFirst = false);
  // ������������� �������� ������� � �����
  static WORD  ToNumber(LPTSTR lpsNum, WORD wLeng);
#ifdef _TRACE
  // ���������� ������� �����������:
  // ���������������� �������� �����������
  void  InitTracing();
  // ������� �������������� ���������
  void  TraceW(LPTSTR lpszMes, bool bClearAfter = true);
#endif

private:
  // ���������� ������
  // -------------------------------------------------------------------
  // ��������� ��������� ������� ��������� ���� ����
  bool  StartNPClient(WORD iNode);
  // ��������� ��������� ������� ��������� ���� ����
  bool  StartNPServer(); //CNamedPipe *WORD iNode
  // ��������� ������ (Thread) ������� ������������ ������ ��� �������� 
  //������� �� ���������� �� ������� ������ �������
  static UINT __cdecl  NPServerThreadProc(LPVOID lpvParam);
  // ��������� ������ (Thread) ���������� ������������ ������
  static UINT __cdecl  NPInstanceThreadProc(LPVOID lpvParam);
  // ��������� ������ (Thread) ������� ������������ ������
  static UINT __cdecl  NPClientThreadProc(LPVOID lpvParam);
  // ��������� ������. ����� ���������� ��� ��������.
  static ECommandCode  ParseMessage(TMessageBuf &mbRequest,
                                    WORD &wFrom, WORD &wTo);
  // ������������ ����� �� ���������� ������ �������
  bool  GetAnswerToRequest(CNamedPipe *pNP);
  // ���������� �������� ���� � ������ "������" ����
  void  NotifyNode(WORD iNode);
  // ��������� (���������� � ����������) ������������� ������� ��
  //�������� �������� ���� � ������ ������� ��� ������� ������ �������
  void  ConnectRemoteClient(WORD iNode, WORD iOwner, CNamedPipe *pNP);
  // ��������� (���������� � ����������) ������������ ������� �� �������� 
  //�������� ���� �� ������ ������� ��� �������� ��������� ����
  void  DisconnectRemoteClient(WORD iNode);
  // �������� ����
  void  ClearNode();

}; // End of class CPeerNetNode declaration ----------------------------
