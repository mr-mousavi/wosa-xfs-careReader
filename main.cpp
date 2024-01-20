#include "widget.h"

#include <QApplication>
#include <windows.h>
#include "Xfsapi.h"
#include "zf_log.h"
#include "Xfsidc.h"

static HAPP hApp;
HSERVICE hService;
HWND hwnd;  //window handler, you need set qwidget::winId to hwnd


int initWosaStartUp();
int wfsSyncOpen(const char *logicalName, int timeout);
int wfsSyncRegister();
HRESULT wfsSyncExecute(int CMD, int timeout, LPVOID readData, WFSRESULT& wfsResult);
std::string getPanFromTrack2(const std::string & track2);
#define NUMBER_OF_CARD_TRACK 3

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();


    hwnd = (HWND)w.winId();  your main window from qt
    initWosaStartUp();

    const char *logicalName= "CardReader";//logical name of device
    int timeout =0; //بی نهایت
    wfsSyncOpen(logicalName,timeout);
     HRESULT r =wfsSyncRegister();
     WFSRESULT wfsResult;
     WORD readData = WFS_IDC_TRACK1 | WFS_IDC_TRACK2 | WFS_IDC_TRACK3;
    if(r == WFS_SUCCESS){
        wfsSyncExecute(WFS_CMD_IDC_READ_RAW_DATA,0,&readData,wfsResult);
    }

    if(wfsResult.lpBuffer) {
        if (wfsResult.hResult == WFS_SUCCESS) {
            LPWFSIDCCARDDATA *wfsIdcCardData=(LPWFSIDCCARDDATA*)wfsResult.lpBuffer;

            for(int i=0; i<NUMBER_OF_CARD_TRACK; i++) {
                if(wfsIdcCardData[i]->wStatus==WFS_IDC_DATAOK) {
                    ZF_LOGI(" wfsIdcCardData[%d]->wDataSource------>%d",i, wfsIdcCardData[i]->wDataSource);
                    ZF_LOGI(" wfsIdcCardData[%d]->wStatus---------->%d",i, wfsIdcCardData[i]->wStatus);
                    ZF_LOGI(" wfsIdcCardData[%d]->ulDataLength----->%d",i, wfsIdcCardData[i]->ulDataLength);
                    ZF_LOGI(" wfsIdcCardData[%d]->fwWriteMethod---->%d",i, wfsIdcCardData[i]->fwWriteMethod);
                    ZF_LOGI_MEM(wfsIdcCardData[i]->lpbData, wfsIdcCardData[i]->ulDataLength, "TRACK::");

                    if(wfsIdcCardData[i]->wDataSource==WFS_IDC_TRACK1) {
                        //only track1
                    }else if(wfsIdcCardData[i]->wDataSource==WFS_IDC_TRACK2) {

                            std::string pan=getPanFromTrack2((const char *)wfsIdcCardData[i]->lpbData);
                            char track2[wfsIdcCardData[i]->ulDataLength];
                            memcpy(track2,wfsIdcCardData[i]->lpbData,wfsIdcCardData[i]->ulDataLength);
                            track2[wfsIdcCardData[i]->ulDataLength]='\0';

                    }else if(wfsIdcCardData[i]->wDataSource==WFS_IDC_TRACK3) {
                        ZF_LOGI("WFS_IDC_TRACK3 IS NOT NECESSARY NOW,MAYBE WE USE IT LATER.");
                    }
                }else{
                    ZF_LOGI("TRACK[%d] is not WFS_IDC_DATAOK",i);
                }
            }//end of for.

        }else if(wfsResult.hResult==WFS_ERR_TIMEOUT) {
            ZF_LOGD("IDC WFS_ERR_TIMEOUT OCCURRED FOR READ_RAW_DATA");

        } else if(wfsResult.hResult== WFS_ERR_IDC_RETAINBINFULL) {
        ZF_LOGD("if its not swip card you should empty card bin ");
        } else {
            ZF_LOGD("error on idc.result");
        }
    }else {
        ZF_LOGW("wfsResult.lpBuffer is NULL,idc execute command failed.");
    }



    return a.exec();
}


int initWosaStartUp()
{
    int returnCode=0;
    HRESULT r;
    char MinMinor = 0
    , MinMajor = 1
    , MaxMinor = 2
    , MaxMajor = 2;
    DWORD Ver = (DWORD)MaxMajor | (DWORD)MaxMinor << 8 | (DWORD)MinMajor << 16 | (DWORD)MinMinor << 24; //66050
    WFSVERSION wfsVersion;// = { 3, 1, 3, "", "" };

    if ((r = WFSStartUp(Ver, &wfsVersion)) == WFS_SUCCESS) {
        ZF_LOGI("startup done.lets create app handle, result: %ld", r);
    } else {
        ZF_LOGE("startup failed. result: %ld", r);
        return -1;
    }
    if (WFSCreateAppHandle(&hApp) != WFS_SUCCESS) {
        ZF_LOGI("StartUp FAILED, result: %ld", r);
        return -2;
    } else {
        ZF_LOGI("WFSCreateAppHandle is WFS_SUCCESS, result: %ld", r);
        ZF_LOGI("Description---->%s", wfsVersion.szDescription);
    }
    return returnCode;
}


int wfsSyncOpen(const char *logicalName, int timeout)
{
    ZF_LOGI("wfsSyncOpen()");
    HRESULT r;
    LPSTR lpszAppID = (LPSTR)"Mousavi";
    DWORD dwTraceLevel = 0;
    char MinMinor = 0
    , MinMajor = 1
    , MaxMinor = 0
    , MaxMajor = 3;

    DWORD dwSrvcVersionsRequired = (DWORD)MaxMajor | (DWORD)MaxMinor << 8 |
                                   (DWORD)MinMajor << 16 | (DWORD)MinMinor << 24; // 01000103;
    WFSVERSION lpSrvcVersion;
    WFSVERSION lpSPIVersion;
    if ((r = WFSOpen((LPSTR)logicalName,hApp, lpszAppID, dwTraceLevel, timeout,
                     dwSrvcVersionsRequired, &lpSrvcVersion, &lpSPIVersion, &hService)) != WFS_SUCCESS) {
        return r;
    }
    return r;
}


int wfsSyncRegister()
{
    ZF_LOGI("wfsSyncRegister()");
    HRESULT r;
    DWORD dwEventClass = EXECUTE_EVENTS | USER_EVENTS | SYSTEM_EVENTS | SERVICE_EVENTS;
    r = WFSRegister(hService, dwEventClass, hwnd);
    if (r == WFS_SUCCESS) {
        ZF_LOGI("WFSRegister complete successfully, result: %ld", r);
    }else {
        ZF_LOGE("WFSRegister could not done, result: %ld", r);
    }
    return r;
}


HRESULT wfsSyncExecute(int CMD, int timeout, LPVOID readData, WFSRESULT& wfsResult)
{
    HRESULT r;
    LPWFSRESULT lpwfsResult=NULL;
    Sleep(50);
    r = WFSExecute(hService, CMD, readData, timeout, &lpwfsResult);

    if (r == WFS_SUCCESS) {
        ZF_LOGI("wfsSyncExecute execute successfully, result: %ld", r);
        wfsResult = *lpwfsResult;
    }else {
        ZF_LOGE("wfsSyncExecute could not execute, result: %ld", r);
    }
    WFSFreeResult(lpwfsResult);

    return r;
}



std::string getPanFromTrack2(const std::string & track2)
{
    std::string pan="";
    if(track2.size()>0) {
        for (uint8_t i = 0; i < track2.size() && track2.at(i) != '='; i++)
            pan.push_back(track2.at(i));
    }
    return pan;
}
