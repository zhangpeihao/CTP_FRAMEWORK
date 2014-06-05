#include"MdUserApi.h"
#include<iostream>
#include"toolkit.h"
#include"CLock.h"
using namespace std;
#define _CTP_SEPS_ ",;"
MdUserApi::MdUserApi(void)
{
    m_pMdUserApi = NULL;
    m_iRequestID = 0;
   
    InitializeCriticalSection(&m_csMapInstrumentIDs);

}

MdUserApi::~MdUserApi(void)
{
    Disconnect();
}

bool MdUserApi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bRet)
    {
        cout << "����ID: " << pRspInfo->ErrorID << "  ������Ϣ�� " << pRspInfo->ErrorMsg << endl;
    }
    return bRet;
}

void MdUserApi::Connect(const string& strPath,
    const string& strAddresses,
    const string& strBrokerId,
    const string& strInvestorId,
    const string& strPassword)
{
    m_strBrokerId = strBrokerId;
    m_strInvestorId = strInvestorId;
    m_strPassword = strPassword;
    char *pPath = new char[strPath.size() + 20];
    sprintf(pPath, "%s\\Md\\", strPath.c_str());
    makedirs(pPath);
    m_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi(pPath, (strAddresses.find("udp://") != strAddresses.npos));
    delete[] pPath;

    if (m_pMdUserApi)
    {
        m_pMdUserApi->RegisterSpi(this);

        size_t len = strAddresses.length() + 1;
        char* buf = new char[len];
        strncpy(buf, strAddresses.c_str(), len);
        

        char *token = strtok(buf, _CTP_SEPS_);

        while (token)
        {
            if (strlen(token) > 0)
            {
                char *pch = strstr(token, "udp://");
                if (pch)
                {
                    strncpy(pch, "tcp://", 6);
                }
               m_pMdUserApi->RegisterFront(token);
            }
            token = strtok(NULL, _CTP_SEPS_);
        }
        delete[] buf;

        m_pMdUserApi->Init();

    }//end 
    


}

void MdUserApi::ReqUserLogin()
{
    if (NULL == m_pMdUserApi)
        return;
    CThostFtdcReqUserLoginField request = { 0 };

    strncpy(request.BrokerID, m_strBrokerId.c_str(), sizeof(TThostFtdcBrokerIDType));
    strncpy(request.UserID, m_strInvestorId.c_str(), sizeof(TThostFtdcInvestorIDType));
    strncpy(request.Password, m_strPassword.c_str(), sizeof(TThostFtdcPasswordType));

    //ֻ����һ���õ���m_iRequestID��û�б�Ҫÿ������m_iRequestID����0��ʼ
    m_pMdUserApi->ReqUserLogin(&request, ++m_iRequestID);
    
}

void MdUserApi::Disconnect()
{
    if (m_pMdUserApi)
    {
        m_pMdUserApi->RegisterSpi(NULL);
        m_pMdUserApi->Release();
        m_pMdUserApi = NULL;
    }
}

void MdUserApi::Subscribe(const string& szInstrumentIDs)
{
    if (NULL == m_pMdUserApi)
        return;

    vector<char*> vct;

    size_t len = szInstrumentIDs.length() + 1;
    char* buf = new char[len];
    strncpy(buf, szInstrumentIDs.c_str(), len);

    CLock cl(&m_csMapInstrumentIDs);

    char* token = strtok(buf, _CTP_SEPS_);
    while (token)
    {
        size_t l = strlen(token);
        if (l>0)
        {
            //��Լ�Ѿ����ڣ������ٶ��ģ�����ζ���Ҳû��ϵ
            m_setInstrumentIDs.insert(token);//��¼�˺�Լ���ж���
            vct.push_back(token);
        }
        token = strtok(NULL, _CTP_SEPS_);
    }

    if (vct.size()>0)
    {
        //ת���ַ�������
        char** pArray = new char*[vct.size()];
        for (size_t j = 0; j<vct.size(); ++j)
        {
            pArray[j] = vct[j];
        }

        //����
        m_pMdUserApi->SubscribeMarketData(pArray, (int)vct.size());

        delete[] pArray;
    }

    //�ͷ��ڴ�
    delete[] buf;
}

void MdUserApi::Subscribe(const set<string> &setInstrumentIDs)
{
    if (NULL == m_pMdUserApi)
        return;

    string szInstrumentIDs;
    for (set<string>::iterator i = setInstrumentIDs.begin(); i != setInstrumentIDs.end(); ++i)
    {
        szInstrumentIDs.append(*i);
        szInstrumentIDs.append(";");
    }

    if (szInstrumentIDs.length()>1)
    {
        Subscribe(szInstrumentIDs);
    }
}

void MdUserApi::Unsubscribe(const string& strInstrumentIDs)
{
    if (NULL == m_pMdUserApi)
        return;

    vector<char*> vct;
    size_t len = strInstrumentIDs.length() + 1;
    char* buf = new char[len];
    strncpy(buf, strInstrumentIDs.c_str(), len);

    CLock cl(&m_csMapInstrumentIDs);

    char* token = strtok(buf, _CTP_SEPS_);
    while (token)
    {
        size_t l = strlen(token);
        if (l>0)
        {
            //��Լ�Ѿ������ڣ�������ȡ�����ģ������ȡ��Ҳûʲô��ϵ
            m_setInstrumentIDs.erase(token);
            vct.push_back(token);
        }
        token = strtok(NULL, _CTP_SEPS_);
    }

    if (vct.size()>0)
    {
        //ת���ַ�������
        char** pArray = new char*[vct.size()];
        for (size_t j = 0; j<vct.size(); ++j)
        {
            pArray[j] = vct[j];
        }

        //����
        m_pMdUserApi->UnSubscribeMarketData(pArray, (int)vct.size());

        delete[] pArray;
    }

    //�ͷ��ڴ�
    delete[] buf;
}

void MdUserApi::OnFrontConnected()
{
    //���ӳɹ����Զ������¼
    ReqUserLogin();
}

void MdUserApi::OnFrontDisconnected(int nReason)
{
    CThostFtdcRspInfoField RspInfo;
    //����ʧ�ܷ��ص���Ϣ��ƴ�Ӷ��ɣ���Ҫ��Ϊ��ͳһ���
    RspInfo.ErrorID = nReason;
    GetOnFrontDisconnectedMsg(&RspInfo);
}

void MdUserApi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (!IsErrorRspInfo(pRspInfo)
        && pRspUserLogin)
    {

        //�п��ܶ����ˣ������Ƕ������������¶���
        set<string> mapOld = m_setInstrumentIDs;//�����ϴζ��ĵĺ�Լ
        //Unsubscribe(mapOld);//�����Ѿ������ˣ�û�б�Ҫ��ȡ������
        Subscribe(mapOld);//����
    }
    else
    {
        cout << "��½����˳ɹ�" << endl;
    }
}


void MdUserApi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    //��ģ��ƽ̨��������������ᴥ��������Ҫ�Լ�ά��һ���Ѿ����ĵĺ�Լ�б�
    if (!IsErrorRspInfo(pRspInfo)
        && pSpecificInstrument)
    {
        CLock cl(&m_csMapInstrumentIDs);

        m_setInstrumentIDs.insert(pSpecificInstrument->InstrumentID);
    }
}

void MdUserApi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    //ģ��ƽ̨��������������ᴥ��
    if (!IsErrorRspInfo(pRspInfo)
        && pSpecificInstrument)
    {
        CLock cl(&m_csMapInstrumentIDs);

        m_setInstrumentIDs.erase(pSpecificInstrument->InstrumentID);
    }
}

//����ص����ñ�֤�˺������췵��
void MdUserApi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    cout << pDepthMarketData->LastPrice << endl;
}


void MdUserApi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}