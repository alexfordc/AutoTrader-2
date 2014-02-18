#pragma once
#include "Defines.h"
#include ".\ThostTraderApi\ThostFtdcTraderApi.h"

#include <time.h>
#include <map>
#include <queue>
#include <wx/string.h>

#include <wx/event.h>

class wxLog;

struct OrderEntity
{
   int reqID_;
   int orderRef_;
   time_t orderStartTime_;
   time_t orderDoneTime_;
};

class CTraderSpi : public CThostFtdcTraderSpi
{
public:

    explicit CTraderSpi(wxEvtHandler* handler = NULL);
    virtual ~CTraderSpi();


    void SetInvestorInfo(const char* investor, const char* pwd);
    	///�û���¼����
    void ReqUserLogin(const char*, const char*);

    void ReqUserConnect(const char* front, const char* broker);

	///Ͷ���߽�����ȷ��
    void ReqSettlementInfoConfirm();
    ///�����ѯ��Լ
    void ReqQryInstrument();
    ///�����ѯ�ʽ��˻�
    void ReqQryTradingAccount();
    ///�����ѯͶ���ֲ߳�
    void ReqQryInvestorPosition();
    ///����¼������
    void ReqOrderInsert(const char* instrument, double price,
        int director, int offset, int volume);
    void ReqOrderInsert(const char* instrument, int director, 
        int offset, int volume);

    void ReqQryInvestorPositionDetail();
	///������������
	void ReqOrderAction(const char* instrument, int session, int frontid, const char* orderref);
    void ReqQryAccountregister();
    void ReqQueryBankAccountMoneyByFuture(const char* bankID, const char* bankPWD, const char* accountPWD);
    void ReqTransferByFuture(const char* bankID, const char* bankPWD, const char* accountPWD, double amount, bool f2B);

private:
    //���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
    virtual void OnFrontConnected();

    ///��¼������Ӧ
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///Ͷ���߽�����ȷ����Ӧ
    virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
    ///�����ѯ��Լ��Ӧ
    virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    //�����ѯ�ʽ��˻���Ӧ
    virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯͶ���ֲ߳���Ӧ
    virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///����¼��������Ӧ
    virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///��������������Ӧ
    virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///����Ӧ��
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
    ///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
    virtual void OnFrontDisconnected(int nReason);
		
    ///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
    virtual void OnHeartBeatWarning(int nTimeLapse);
	
    virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
        CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
    ///�����ѯͶ���߽�������Ӧ
    virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, 
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail,
										CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspQryAccountregister(CThostFtdcAccountregisterField* pAccountregister,
                                    CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField* pNotifyQueryAccount);
    ///����֪ͨ
    virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

    ///�ɽ�֪ͨ
    virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);
        
    CThostFtdcInstrumentField GetInstrumentById(const std::string& id) const;
    CThostFtdcInstrumentField GetInstrumentByLabel(const std::string& label) const;

private:

    // �Ƿ��յ��ɹ�����Ӧ
    bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
    // �Ƿ����ڽ��׵ı���
    bool IsTradingOrder(CThostFtdcOrderField *pOrder);

    void DisplayMsg(const wxString& msg, int type = MAIN_WIN_MSG_RUNTIME_LOG);    

private:

    wxLog* logTarget_;

    int reqID_;
    int	orderRef_;	//��������
    TThostFtdcFrontIDType	frontID_;	//ǰ�ñ��
    TThostFtdcSessionIDType	sessionID_;	//�Ự���

    std::string investor_;
    std::string password_;
    std::string broker_;
    std::string frontAddr_;
    std::string tradingDay_;
    std::string settlementInfo_;

    CThostFtdcTraderApi* pTraderApi_;


    // all the following data are accessed from separated thread
    // don't access them from GUI thread.
    std::map<int, CThostFtdcInvestorPositionDetailField> positionDetailData_;
    std::map<std::string, CThostFtdcInstrumentField> instrumentData_;
    std::map<std::string, OrderEntity> orderSend_;
    std::map<std::string, OrderEntity> orderDelete_;
    std::map<std::string, OrderEntity> orderDone_;
    std::map<std::string, CThostFtdcInvestorPositionField> positionData_;
    std::map<std::string, std::string> instrumentName2ID_;

    std::queue<std::string> msgDisplayer_;

    wxEvtHandler* winHandler_;
};

