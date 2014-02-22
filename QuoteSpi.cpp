#include "QuoteSpi.h"
using namespace std;

QuoteSpi::QuoteSpi(wxEvtHandler* handler, map<string, CThostFtdcInstrumentField>& validInstrument)
	:pUserApi(NULL)
	,mainWin_(handler)
	,instrumentData_(validInstrument)
{
}

void QuoteSpi::DisplayMsg(const wxString& msg, int type)
{
    if (mainWin_)
    {
        wxCommandEvent evt(EVENT_MAIN_WIN_SHOW_MSG);
        evt.SetInt(type);
        evt.SetString(msg);
        mainWin_->AddPendingEvent(evt);
    }
	else
	{
        wxLogVerbose(msg);
	}
}

void QuoteSpi::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void QuoteSpi::OnFrontConnected()
{
    ReqUserLogin(investor_, password_);
}

void QuoteSpi::OnFrontDisconnected(int nReason)
{
	DisplayMsg("����Ͽ�");
}

void QuoteSpi::OnHeartBeatWarning(int nTimeLapse)
{
}

void QuoteSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
                           CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if(IsErrorRspInfo(pRspInfo))
    {
	    DisplayMsg(wxString::Format("��¼����:%s", pRspInfo->ErrorMsg));
	}
    else
    {
		tradingDay_ = pRspUserLogin->TradingDay;
    	DisplayMsg("�����¼�ɹ�!"); 	//
    }
}

void QuoteSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void QuoteSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

//���������Ӧ
void QuoteSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
	if (pDepthMarketData) SaveTickInfo(pDepthMarketData);
}

CThostFtdcDepthMarketDataField QuoteSpi::GetMarketDataByInstrumentId(const char* id) const
{
	CThostFtdcDepthMarketDataField data;
	memset(&data, 0, sizeof(data));

	{
        wxMutexLocker lock(tickLocker_);

	    map<string, CThostFtdcDepthMarketDataField>::const_iterator it = tickData_.find(id);
	    if (it == tickData_.end()) return data;

	    return it->second;
	}
}

//tickд���ı�
void QuoteSpi::SaveTickInfo(CThostFtdcDepthMarketDataField* f)
{
	wxMutexLocker lock(tickLocker_);

    //�Ϸ���Լ//�Ϸ�����
	map<string, CThostFtdcInstrumentField>::const_iterator it = instrumentData_.find(f->InstrumentID);
    if(it != instrumentData_.end() && f->LastPrice < f->UpperLimitPrice)
    {
        CThostFtdcInstrumentField instField;
        memset(&instField, 0, sizeof(instField));
        instField = it->second;	//��Լ��Ϣ

        strcpy(f->TradingDay, tradingDay_.c_str());			//����
        strcpy(f->ExchangeID, instField.ExchangeID);	//������

        if(f->AskPrice1 > f->UpperLimitPrice)	//�����йұߵ����
            f->AskPrice1 = f->LastPrice;
        if(f->BidPrice1 > f->UpperLimitPrice)
            f->BidPrice1 = f->LastPrice;

        if(instField.ExchangeID == "CZCE") //�ɽ��������
            f->Turnover *= instField.VolumeMultiple;
        else
            f->AveragePrice /= instField.VolumeMultiple;

        tickData_[string(f->InstrumentID)] = *f;	//����Tick

		return;
    }
}
