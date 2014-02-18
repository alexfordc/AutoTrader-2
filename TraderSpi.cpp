#include <iostream>
#include <stdio.h>
using namespace std;

#include ".\ThostTraderApi\ThostFtdcTraderApi.h"
#include ".\ThostTraderApi\ThostFtdcUserApiDataType.h"

#include "Defines.h"
#include "TraderSpi.h"

#include <wx/log.h>

using namespace std;

#pragma warning(disable : 4996)

// ���ò���
extern wxLog* CreateWxLogTarget(const wxString& log_file);


CTraderSpi::CTraderSpi(wxEvtHandler* handler)
    :logTarget_(NULL)
    ,reqID_(0), frontID_(-1)
    ,pTraderApi_(NULL)
    ,winHandler_(handler)
{
}

CTraderSpi::~CTraderSpi()
{
    if (logTarget_) delete logTarget_;

    if (pTraderApi_) pTraderApi_->Release();
}

CThostFtdcInstrumentField CTraderSpi::GetInstrumentById(const string& id) const
 {
     CThostFtdcInstrumentField tmp = {0};
     map<string, CThostFtdcInstrumentField>::const_iterator it = instrumentData_.find(id);
     if (it != instrumentData_.end()) tmp = it->second;

     return tmp;
 }

CThostFtdcInstrumentField CTraderSpi::GetInstrumentByLabel(const string& label) const
 {
     CThostFtdcInstrumentField tmp = {0};
     map<string, string>::const_iterator it1 = instrumentName2ID_.find(label);
     
     if (it1 == instrumentName2ID_.end()) return tmp;

     string id = it1->second;
     map<string, CThostFtdcInstrumentField>::const_iterator it2 = instrumentData_.find(id);
     if (it2 == instrumentData_.end()) return tmp;

     return it2->second;
 }

void CTraderSpi::SetInvestorInfo(const char* investor, const char* pwd)
{
    investor_ = investor;
    password_ = pwd;
}

void CTraderSpi::DisplayMsg(const wxString& msg, int type)
{
    if (winHandler_)
    {
        wxCommandEvent evt(EVENT_MAIN_WIN_SHOW_MSG);
        evt.SetInt(type);
        evt.SetString(msg);
        winHandler_->AddPendingEvent(evt);
    }
    else
    {
       msgDisplayer_.push(string(msg.c_str()));    
    }
       
    wxLogVerbose(msg);
}

void CTraderSpi::ReqUserLogin(const char* investor, const char* pwd)
{
    investor_ = investor;
    password_ = pwd;

    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, broker_.c_str());
    strcpy(req.UserID, investor_.c_str());
    strcpy(req.Password, password_.c_str());
    int iResult = pTraderApi_->ReqUserLogin(&req, ++reqID_);

    DisplayMsg(wxString::Format("--->>> �����û���¼����: %s", ((iResult == 0) ? "�ɹ�" : "ʧ��")));
}

void CTraderSpi::ReqUserConnect(const char* front, const char* broker)
{
    // registerfront() takes non-const parameter, stupid.
    char front_addr[256] = {0};

    sprintf(front_addr,"tcp://%s",front);

    if (pTraderApi_) pTraderApi_->Release();
     
    // ��ʼ��UserApi
    pTraderApi_ = CThostFtdcTraderApi::CreateFtdcTraderApi();			// ����UserApi

    pTraderApi_->RegisterSpi(this);			// ע���¼���
    pTraderApi_->SubscribePublicTopic(THOST_TERT_RESTART);					// ע�ṫ����
    pTraderApi_->SubscribePrivateTopic(THOST_TERT_RESTART);					// ע��˽����

	pTraderApi_->RegisterFront(front_addr);							// connect
	pTraderApi_->Init();

    frontAddr_ = front;
    broker_ = broker;
}

// �������޼�
void CTraderSpi::ReqOrderInsert(const char* instrument, double price,
        int director, int offset, int volume)
{
    CThostFtdcInputOrderField f;
    memset(&f, 0, sizeof(f));
    f.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;	//1Ͷ��
    f.ContingentCondition = THOST_FTDC_CC_Immediately;//��������
    f.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    f.IsAutoSuspend = 0;
    f.TimeCondition =  THOST_FTDC_TC_GFD;		//***�������1  ������Ч3***
    f.VolumeCondition = THOST_FTDC_VC_AV;	//��������1  ��С����2  ȫ���ɽ�3
    f.MinVolume = 1;

    strcpy(f.InvestorID, investor_.c_str());
    strcpy(f.UserID, investor_.c_str());
    strcpy(f.BrokerID, broker_.c_str());

    strcpy(f.InstrumentID, instrument);	//��Լ

    if (price)
        f.OrderPriceType = THOST_FTDC_OPT_LimitPrice;		//***�����1  �޼�2***
    else
        f.OrderPriceType = THOST_FTDC_OPT_AnyPrice;

    f.LimitPrice = price;				//***�۸�***

    if(director == 0)
        f.Direction = THOST_FTDC_D_Buy;			//��
    else
        f.Direction = THOST_FTDC_D_Sell;			//��

    if(offset == 0)
        f.CombOffsetFlag[0] = THOST_FTDC_OF_Open;//����
    else if(offset == 1)
        f.CombOffsetFlag[0] = THOST_FTDC_OF_Close;	//ƽ��
    else
        f.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;	//ƽ��

    f.VolumeTotalOriginal = volume;//����

    sprintf(f.OrderRef, "%d", ++orderRef_);//OrderRef
    pTraderApi_->ReqOrderInsert(&f, reqID_++);	//����

    OrderEntity entity;

    entity.orderRef_ = orderRef_ - 1;
    entity.reqID_    = reqID_ - 1;
    entity.orderStartTime_ = time(NULL);

    string key = wxString::Format("%s|%d|%d|%d", f.InstrumentID, sessionID_, frontID_, orderRef_ - 1).c_str();
    orderSend_[key] = entity;

    DisplayMsg(wxString::Format("����:%s", f.OrderRef));
}

//����-�м�
void CTraderSpi::ReqOrderInsert(const char* instrument, int director, int offset, int volume)
{
    ReqOrderInsert(instrument, 0, director, offset, volume);
}

//����
void CTraderSpi::ReqOrderAction(const char* instrument, int session, int frontid, const char* orderref)
{
    CThostFtdcInputOrderActionField f;
    memset(&f, 0, sizeof(f));
    f.ActionFlag = THOST_FTDC_AF_Delete; 	//THOST_FTDC_AF_Modify
    strcpy(f.BrokerID, broker_.c_str());
    strcpy(f.InvestorID, investor_.c_str());

    strcpy(f.InstrumentID, instrument);
    f.SessionID = session;
    f.FrontID = frontid;
    strcpy(f.OrderRef, orderref);

    pTraderApi_->ReqOrderAction(&f, ++reqID_);

    OrderEntity entity;

    entity.orderRef_ = atoi(orderref);
    entity.reqID_    = reqID_ - 1;
    entity.orderStartTime_ = time(NULL);

    string key = wxString::Format("%s|%d|%d|%d", f.InstrumentID, session, frontid, atoi(orderref)).c_str();
    orderDelete_[key] = entity;
}


/*
 ��ѯ����ע�����
 1) �ۺϽ���ƽ̨���Բ�ѯ�����������ƣ��Խ���ָ��û�����ơ�
 2) �������;�Ĳ�ѯ���������µĲ�ѯ��
 3) 1 �������������1����ѯ��
 4) ����ֵ��-2����ʾ��δ�������󳬹��������
 5) ��-3����ʾ��ÿ�뷢�������������������

*/
//��ֲ�
void CTraderSpi::ReqQryInvestorPosition()
{
    CThostFtdcQryInvestorPositionField f;
    memset(&f, 0, sizeof(f));
    strcpy(f.BrokerID, broker_.c_str());
    strcpy(f.InvestorID, investor_.c_str());
    pTraderApi_->ReqQryInvestorPosition(&f, ++reqID_);
}

//��ֲ���ϸ
void CTraderSpi::ReqQryInvestorPositionDetail()
{
    CThostFtdcQryInvestorPositionDetailField f;
    memset(&f, 0, sizeof(f));
    strcpy(f.BrokerID, broker_.c_str());
    strcpy(f.InvestorID, investor_.c_str());
    pTraderApi_->ReqQryInvestorPositionDetail(&f, ++reqID_);
}

//���ʽ�
void CTraderSpi::ReqQryTradingAccount()
{
    CThostFtdcQryTradingAccountField f;
    memset(&f, 0, sizeof(f));
    strcpy(f.BrokerID, broker_.c_str());
    strcpy(f.InvestorID, investor_.c_str());
    int iResult = pTraderApi_->ReqQryTradingAccount(&f, ++reqID_);
    DisplayMsg(wxString::Format("--->>> �����ѯ�ʽ��˻�: %sbankSettingPanel_", ((iResult == 0) ? "�ɹ�" : "ʧ��")));
}

//��ǩԼ����
void CTraderSpi::ReqQryAccountregister()
{
    CThostFtdcQryAccountregisterField f;
    memset(&f, 0, sizeof(f));
    strcpy(f.BrokerID, broker_.c_str());
    strcpy(f.AccountID, investor_.c_str());

    pTraderApi_->ReqQryAccountregister(&f, ++reqID_);
}

//�������ʻ�
void CTraderSpi::ReqQueryBankAccountMoneyByFuture(const char* bankID, const char* bankPWD, const char* accountPWD)
{
    CThostFtdcReqQueryAccountField f;
    memset(&f, 0, sizeof(f));

    strcpy(f.BankID, bankID);
    strcpy(f.BankBranchID, "0000");		//������
    strcpy(f.BankPassWord, bankPWD);

    strcpy(f.BrokerID, broker_.c_str());
    strcpy(f.AccountID, investor_.c_str());
    strcpy(f.Password, accountPWD);
    f.SecuPwdFlag = THOST_FTDC_BPWDF_BlankCheck;	//���ĺ˶�
    strcpy(f.CurrencyID, "RMB"); 					//���֣�RMB-����� USD-��Բ HKD-��Ԫ

    pTraderApi_->ReqQueryBankAccountMoneyByFuture(&f, ++reqID_);
}

//��ת����
void CTraderSpi::ReqTransferByFuture(const char* bankID, const char* bankPWD, const char* accountPWD, double amount, bool f2B)
{
    CThostFtdcReqTransferField f;
    memset(&f, 0, sizeof(f));

    strcpy(f.BankID, bankID);
    strcpy(f.BankBranchID, "0000");		//������
    strcpy(f.BankPassWord, bankPWD);

    strcpy(f.BrokerID, broker_.c_str());
    strcpy(f.AccountID, investor_.c_str());
    strcpy(f.Password, accountPWD);
    f.SecuPwdFlag = THOST_FTDC_BPWDF_BlankCheck;	//���ĺ˶�
    strcpy(f.CurrencyID, "RMB"); 					//���֣�RMB-����� USD-��Բ HKD-��Ԫ

    f.TradeAmount = amount;
    if(f2B)	//��ת��
        pTraderApi_->ReqFromFutureToBankByFuture(&f, ++reqID_);
    else	//��ת��
        pTraderApi_->ReqFromBankToFutureByFuture(&f, ++reqID_);
}

// event handlers.
void CTraderSpi::OnFrontConnected()
{
	//cerr << "--->>> " << "OnFrontConnected" << endl;
    logTarget_ = CreateWxLogTarget("AutoTraderApp_TraderSpiLog_");
#if wxCHECK_VERSION(2,9,1)
    wxLog::SetThreadActiveTarget(logTarget_);
#else
	wxLog::SetActiveTarget(logTarget_);
#endif
    DisplayMsg(wxString::Format("--->>>  OnFrontConnectedbankSettingPanel_"));
	///�û���¼����

    ReqUserLogin(investor_.c_str(), password_.c_str());
}

void CTraderSpi:: OnFrontDisconnected(int nReason)
{
	DisplayMsg(wxString::Format("--->>> OnFrontDisconnectedbankSettingPanel_"));
	DisplayMsg(wxString::Format("--->>> Reason = %dbankSettingPanel_", nReason));
}

void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	// cerr << "--->>> " << "OnRspUserLogin" << endl;
	DisplayMsg(wxString::Format("--->>> OnRspUserLogin"));
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		// ����Ự����
		frontID_ = pRspUserLogin->FrontID;
		sessionID_ = pRspUserLogin->SessionID;
		// int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
		// iNextOrderRef++;
		// sprintf(ORDER_REF, "%d", iNextOrderRef);
		///��ȡ��ǰ������
		// cerr << "--->>> ��ȡ��ǰ������ = " << pUserApi->GetTradingDay() << endl;
		// wxLogVerbose("--->>> ��ȡ��ǰ������ = %s", pTraderApi_->GetTradingDay());
		///Ͷ���߽�����ȷ��
		// ReqSettlementInfoConfirm();

        orderRef_ = atoi(pRspUserLogin->MaxOrderRef);
        CThostFtdcQrySettlementInfoConfirmField f;
        memset(&f, 0, sizeof(f));
        strcpy(f.BrokerID, broker_.c_str());
        strcpy(f.InvestorID, investor_.c_str());
        pTraderApi_->ReqQrySettlementInfoConfirm(&f, ++reqID_);
                
        DisplayMsg("login done", MAIN_WIN_MSG_LOGIN);
	}
}

void CTraderSpi::ReqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, broker_.c_str());
	strcpy(req.InvestorID, investor_.c_str());
	int iResult = pTraderApi_->ReqSettlementInfoConfirm(&req, ++reqID_);
	DisplayMsg(wxString::Format("--->>> Ͷ���߽�����ȷ��: %s", ((iResult == 0) ? "�ɹ�" : "ʧ��")));
}

void CTraderSpi::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
        CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if(this->IsErrorRspInfo(pRspInfo))
    {
        DisplayMsg(wxString::Format("��ѯ����ȷ�ϴ���"));
    }
    else
    {
        tradingDay_ = pTraderApi_->GetTradingDay();
        if((pSettlementInfoConfirm) && strcmp(pSettlementInfoConfirm->ConfirmDate, tradingDay_.c_str()) == 0)
        {            
            // δȷ�Ͻ��㣬��Ҫȷ�ϲ��ܽ��б�Ĳ���
            DisplayMsg(wxString::Format("����δȷ�ϣ������������ȷ��..."));
              
            CThostFtdcSettlementInfoConfirmField f;
            memset(&f, 0, sizeof(f));
            strcpy(f.BrokerID, broker_.c_str());
            strcpy(f.InvestorID, investor_.c_str());

            pTraderApi_->ReqSettlementInfoConfirm(&f, ++reqID_);
        }
        else
        {            
            // �����������ȷ��            
            DisplayMsg(wxString::Format("������ȷ�ϣ��鿴�����..."));

            CThostFtdcQrySettlementInfoField f;
            memset(&f, 0, sizeof(f));
            strcpy(f.BrokerID, broker_.c_str());
            strcpy(f.InvestorID, investor_.c_str());

            wxThread::Sleep(1000);// CTP requires one query per second.
            pTraderApi_->ReqQrySettlementInfo(&f, ++reqID_);
        }
    }
}

//�������Ϣ
void CTraderSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo,
                                   CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if(pSettlementInfo)
    {
        settlementInfo_ += pSettlementInfo->Content;
     
        wxString msg = wxString::Format("������Ϣ�������գ�%s��, ������(%d), ���͹�˾(%s), Ͷ���߱��(%s),\
                                        ���(%d), ����(%d)", pSettlementInfo->TradingDay, pSettlementInfo->SettlementID,
                                        pSettlementInfo->BrokerID, pSettlementInfo->InvestorID, pSettlementInfo->InvestorID,
                                        pSettlementInfo->SequenceNo, pSettlementInfo->Content);

        DisplayMsg(msg);
    }
        
    if (bIsLast)
    {
        wxThread::Sleep(1000);// CTP requires one query per second.
        ReqQryInstrument();
    }
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	// cerr << "--->>> " << "OnRspSettlementInfoConfirm" << endl;
	DisplayMsg(wxString::Format("--->>> OnRspSettlementInfoConfirm"));
	if (bIsLast)
	{
        wxThread::Sleep(1000);// CTP requires one query per second.
        ReqQryInstrument();
	}
}

void CTraderSpi::ReqQryInstrument()
{
    // wxThread::Sleep(500);
	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
	// strcpy(req.InstrumentID, INSTRUMENT_ID);
	int iResult = pTraderApi_->ReqQryInstrument(&req, ++reqID_);
	// cerr << "--->>> �����ѯ��Լ: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;

    wxString msg;

    if (iResult == -1) msg = "��������ʧ��";
    else if (iResult == -2) msg = "δ�������󳬹������";
    else if (iResult == -3) msg = "ÿ�뷢�����������������";
    else msg = "�ɹ�";

	DisplayMsg(wxString::Format("--->>> �����ѯ��Լ: %s", msg.c_str()));
}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	// cerr << "--->>> " << "OnRspQryInstrument" << endl;
	DisplayMsg(wxString::Format("--->>> OnRspQryInstrument"));

    if (IsErrorRspInfo(pRspInfo)) return;

    wxString label = wxString::Format("��Լ��:%s,��Լ����:%s,������:%s",
        pInstrument->InstrumentID, pInstrument->InstrumentName, pInstrument->ExchangeID);

    DisplayMsg(label, MAIN_WIN_MSG_INSTRUMENT);

    instrumentName2ID_[string(label.c_str())] = pInstrument->InstrumentID;
    instrumentData_[pInstrument->InstrumentID] = *pInstrument;

	if (bIsLast)
	{
		///�����ѯ��Լ
        // wxThread::Sleep(1000);// CTP requires one query per second.
		// ReqQryTradingAccount();
	}
}

void CTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	// cerr << "--->>> " << "OnRspQryTradingAccount" << endl;
	DisplayMsg(wxString::Format("--->>> OnRspQryTradingAccount"));
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		///�����ѯͶ���ֲ߳�
		// ReqQryInvestorPosition();

        //��̬Ȩ��=���ս���-������+�����
        double preBalance = pTradingAccount->PreBalance
            - pTradingAccount->Withdraw + pTradingAccount->Deposit;
        //��̬Ȩ��=��̬Ȩ��+ ƽ��ӯ��+ �ֲ�ӯ��- ������
        double current = preBalance + pTradingAccount->CloseProfit
            + pTradingAccount->PositionProfit - pTradingAccount->Commission;

        wxString format = _T("\n�����ʽ�  ƽ��ӯ��  �ֲ�ӯ��  ��̬Ȩ��  ��̬Ȩ��  ռ�ñ�֤��  �����ʽ�  ���ն�\n\
            %f  %f  %f  %f  %f  %f  %f  %f");

        wxString msg = wxString::Format(format, pTradingAccount->Available,
                pTradingAccount->CloseProfit, pTradingAccount->PositionProfit,
                preBalance, current, pTradingAccount->CurrMargin,
                (pTradingAccount->FrozenMargin + pTradingAccount->FrozenCommission),
                (current == 0? 0 : (pTradingAccount->CurrMargin / current * 100)));

        DisplayMsg(msg);
    }
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	// cerr << "--->>> " << "OnRspQryInvestorPosition" << endl;
	DisplayMsg(wxString::Format("--->>> OnRspQryInvestorPosition"));

    if (pInvestorPosition)
    {
        string key = wxString::Format("%s|%d|%d", pInvestorPosition->InstrumentID, pInvestorPosition->PosiDirection, pInvestorPosition->PositionDate).c_str();

        positionData_[key] = *pInvestorPosition;
    }

    if (bIsLast)
    {
        wxString format = _T("%s  %s  %s  %d  %f  %f\n");
        wxString msg = _T("\n�ֲ֣�\n��Լ  ����  ����  �ֲ�  �ɱ�  �ֲ�ӯ��\n");
        map<string, CThostFtdcInvestorPositionField>::iterator it = positionData_.begin();

        while (it != positionData_.end())
        {
            CThostFtdcInvestorPositionField f = it->second;
            wxString str = wxString::Format(format, 
                    f.InstrumentID,
                    (f.PosiDirection == THOST_FTDC_PD_Long? _T("��") : _T("��")),
                    (f.PositionDate == THOST_FTDC_PSD_History? _T("���") : _T("���")),
                    f.Position,
                    (f.PositionCost),// / instrumentData_[pInvestorPosition->InstrumentID].VolumeMultiple
                    f.PositionProfit);

            msg += str;
            ++it;
        }

        DisplayMsg(msg);
    }
}

void CTraderSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail,
										CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if(pInvestorPositionDetail)
    {
        positionDetailData_[atoi(pInvestorPositionDetail->TradeID)] = *pInvestorPositionDetail;
    }

    if(bIsLast)
    {
        positionData_.clear();
        wxString format = _T("%s  %s  %s  %s  %d  %f  %f\n");
        wxString msg = _T("\n�ֲ���ϸ:\n�ɽ����  ��Լ  ����  ����  ����  ���ּ�  �ֲ�ӯ��\n");
        map<int, CThostFtdcInvestorPositionDetailField>::iterator it = positionDetailData_.begin();

        while (it != positionDetailData_.end())
        {
            CThostFtdcInvestorPositionDetailField f = it->second;
            wxString str = wxString::Format(format,
                       f.TradeID, f.InstrumentID,
                       (f.Direction == THOST_FTDC_D_Buy? _T("��"):_T("��")),
                       (strcmp(f.OpenDate, tradingDay_.c_str()) == 0? _T("���"):_T("���")),
                       f.Volume, f.OpenPrice,
                       f.PositionProfitByDate);

            msg += str;
            ++it;

            // TODO

            /*
            //�ֲֻ���
            double multi = instrumentData_[f.InstrumentID].VolumeMultiple;
            string positionID = wxString::Format("%s|%d|%d", f.InstrumentID,
                    f.Direction, (strcmp(f.OpenDate, tradingDay_.c_str())==0)).c_str();

            map<string, CThostFtdcInvestorPositionField>::iterator iter = positionData_.find(positionID.c_str());
            CThostFtdcInvestorPositionField pf;
            if(iter == positionData_.end())
            {
                memset(&pf, 0, sizeof(pf));
                strcpy(pf.InstrumentID, f.InstrumentID);
                pf.PosiDirection = (f.Direction == THOST_FTDC_D_Buy ? THOST_FTDC_PD_Long: THOST_FTDC_PD_Short);
                pf.PositionDate = (strcmp(f.OpenDate, tradingDay_.c_str()) == 0? THOST_FTDC_PSD_Today : THOST_FTDC_PSD_History);
                pf.Position = 0;//f.Volume;
                pf.PositionCost = 0;//f.OpenPrice;
                pf.PositionProfit = 0;//f.PositionProfitByDate
                pf.TodayPosition = 0;
                pf.YdPosition = 0;
            }
            else
            {
                pf = positionData_[positionID];
            }

            pf.Position  += f.Volume;

            if(pf.PositionDate == THOST_FTDC_PSD_History)
            {
                pf.YdPosition += f.Volume;
                pf.PositionCost += f.Volume * f.LastSettlementPrice * multi;
                //�ֲ�ӯ����Ҫ�Լ�����detailʼ��Ϊ0
                pf.PositionProfit += (f.Direction == THOST_FTDC_PD_Long? 1 : -1) *
                                     (dicTick[f.InstrumentID].LastPrice - f.LastSettlementPrice) * f.Volume * multi;
            }
            else
            {
                pf.TodayPosition += f.Volume;
                pf.PositionCost += f.Volume * f.OpenPrice * multi;
                //�ֲ�ӯ����Ҫ�Լ�����detailʼ��Ϊ0
                pf.PositionProfit += (f.Direction == THOST_FTDC_PD_Long? 1 : -1) *
                                     (dicTick[f.InstrumentID].LastPrice - f.OpenPrice) * f.Volume * multi;
            }
            

            positionData_[positionID] = pf;
            */
        }

        // show(msg, 1);

        DisplayMsg(msg);

        //��ʾ�ֲֻ���
        format = "%s  %s  %s  %f  %f  %f\n";
        msg = "\n��Լ  ����  ����  �ֲ�  �ɱ�  �ֲ�ӯ��\n";

        map<string, CThostFtdcInvestorPositionField>::iterator iter = positionData_.begin();
        while (iter != positionData_.end())
        {
            CThostFtdcInvestorPositionField& f = iter->second;
            wxString str = wxString::Format(format,
                       f.InstrumentID, (f.PosiDirection == THOST_FTDC_PD_Long?"��":"��"),
                       (f.PositionDate == THOST_FTDC_PSD_History?"���":"���"), f.Position,
                       (f.PositionCost / f.Position), // / instrumentData_[f.InstrumentID].VolumeMultiple),
                       f.PositionProfit);

            msg += str;
            ++it;
        }

        DisplayMsg(msg);
    }
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	DisplayMsg(wxString::Format("--->>> ��������:%s", pRspInfo->ErrorMsg));
	// cerr << "--->>> " << "OnRspOrderInsert" << endl;
	// IsErrorRspInfo(pRspInfo);
}

void CTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    DisplayMsg(wxString::Format("--->>> OnRspOrderAction"));
    if (pInputOrderAction)
    {  
        wxString key = wxString::Format(_T("%s|%d|%d|%s,����״̬:%s"), pInputOrderAction->InstrumentID, 
            pInputOrderAction->SessionID, pInputOrderAction->FrontID, pInputOrderAction->OrderRef, pRspInfo->ErrorMsg);
        DisplayMsg(key);
    }

    // cerr << "--->>> " << "OnRspOrderAction" << endl;
    // IsErrorRspInfo(pRspInfo);
}

///����֪ͨ
void CTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    // cerr << "--->>> " << "OnRtnOrder"  << endl;
	DisplayMsg(wxString::Format("--->>> OnRtnOrder"));

    string key = wxString::Format("%s|%d|%d|%s", pOrder->InstrumentID, pOrder->SessionID, pOrder->FrontID, pOrder->OrderRef).c_str();
        
    wxString msg = wxString::Format("�������:%s@@���͹�˾���:%d@#@״̬:%s",
            key.c_str(), pOrder->BrokerOrderSeq, pOrder->StatusMsg);

    DisplayMsg(msg, MAIN_WIN_MSG_ORDER);

    std::map<std::string, OrderEntity>::iterator it = orderSend_.find(key.c_str());
    if (it != orderSend_.end())
    {
        time_t t2 = time(NULL);
        OrderEntity entity = orderSend_[key];

        // TODO calc time span.
        orderSend_.erase(it);
    }
}

///�ɽ�֪ͨ
void CTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    DisplayMsg(wxString::Format("�ɽ����:%s, broker���%d, �ɽ�ʱ��%s", pTrade->TradeID, pTrade->BrokerOrderSeq, pTrade->TradeTime));
}

void CTraderSpi::OnRspQryAccountregister(CThostFtdcAccountregisterField* pAccountregister,
                                    CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if(IsErrorRspInfo(pRspInfo))
    {
        DisplayMsg(wxString::Format(wxString::Format("��ǩԼ���д���:%s", pRspInfo->ErrorMsg)));
    }
    else if(pAccountregister)
    {
        string bank = "";
        if(pAccountregister->BankID[0] == THOST_FTDC_BF_ABC)
            bank = "ũҵ����";
        else if(pAccountregister->BankID[0] == THOST_FTDC_BF_BC)
            bank = "�й�����";
        else if(pAccountregister->BankID[0] == THOST_FTDC_BF_BOC)
            bank = "��ͨ����";
        else if(pAccountregister->BankID[0] == THOST_FTDC_BF_CBC)
            bank = "��������";
        else if(pAccountregister->BankID[0] == THOST_FTDC_BF_ICBC)
            bank = "��������";
        else if(pAccountregister->BankID[0] == THOST_FTDC_BF_Other)
            bank = "��������";

        string bankID = string(pAccountregister->BankAccount);
        bankID = bankID.substr(strlen(pAccountregister->BankAccount)-4, 4);
        DisplayMsg(wxString::Format(wxString::Format("%d:%s:%s", pAccountregister->BankID[0], bank, bankID)));
    }
}

void CTraderSpi::OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField* pNotifyQueryAccount)
{
    DisplayMsg(wxString::Format("�������:%s, ��ȡ���:%s", pNotifyQueryAccount->BankUseAmount,
             pNotifyQueryAccount->BankFetchAmount));
}

void CTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
    DisplayMsg(wxString::Format("--->>> OnHeartBeatWarning"));
    DisplayMsg(wxString::Format("--->>> nTimerLapse = %d", nTimeLapse));
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    DisplayMsg(wxString::Format("--->>> OnRspError"));
	IsErrorRspInfo(pRspInfo);
}

bool CTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	// ���ErrorID != 0, ˵���յ��˴������Ӧ
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult)
		DisplayMsg(wxString::Format("--->>> ErrorID=%d, ErrorMsg=%s", pRspInfo->ErrorID, pRspInfo->ErrorMsg));
	return bResult;
}

bool CTraderSpi::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

