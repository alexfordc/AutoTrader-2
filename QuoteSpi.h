#ifndef _FEATURE_TRADING_QUOTE_H_
#define _FEATURE_TRADING_QUOTE_H_
//#pragma once
#include "ThostApi/ThostFtdcMdApi.h"
#include "Defines.h"
#include <time.h>
#include <map>
#include <queue>
#include <wx/string.h>
#include <wx/event.h>
#include <wx/log.h>
#include <wx/thread.h>


class QuoteSpi : public CThostFtdcMdSpi
{
public:
    QuoteSpi(wxEvtHandler* handler, std::map<std::string, CThostFtdcInstrumentField>&);

private:
	///����Ӧ��
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast);

	///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
	///@param nReason ����ԭ��
	///        0x1001 �����ʧ��
	///        0x1002 ����дʧ��
	///        0x2001 ����������ʱ
	///        0x2002 ��������ʧ��
	///        0x2003 �յ�������
	virtual void OnFrontDisconnected(int nReason);
		
	///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
	///@param nTimeLapse �����ϴν��ձ��ĵ�ʱ��
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	virtual void OnFrontConnected();
	
	///��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///��������Ӧ��
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///ȡ����������Ӧ��
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///�������֪ͨ
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
	virtual void SaveTickInfo(CThostFtdcDepthMarketDataField* f);
public:
	void ReqUserLogin(const std::string& investor, const std::string& password);
	void SubscribeMarketData(char* instIdList);
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
	void DisplayMsg(const wxString& msg, int type = MAIN_WIN_MSG_RUNTIME_LOG);
	void SetLoginInfo(const char* investor, const char* password);
	CThostFtdcDepthMarketDataField GetMarketDataByInstrumentId(const char* id) const;

private:
	std::string investor_;
	std::string password_;
	std::string tradingDay_;

	mutable wxMutex tickLocker_;
	std::map<std::string, CThostFtdcDepthMarketDataField> tickData_;
	const std::map<std::string, CThostFtdcInstrumentField> instrumentData_;

	wxEvtHandler* mainWin_;
    CThostFtdcMdApi* pUserApi;
};

#endif