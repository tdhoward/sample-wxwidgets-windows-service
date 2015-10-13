/*

service.cpp
    Contains all the stuff for
    installing, starting, managing, stopping and uninstalling the service

Credits:
    Ideas and example code were taken from:
    https://forums.wxwidgets.org/viewtopic.php?f=20&t=4375
    http://www.codeproject.com/Articles/499465/Simple-Windows-Service-in-Cplusplus



*/

#include <wx/stdpaths.h>
#include <wx/wx.h>
#include <wx/evtloop.h>
#include <wx/event.h>
#include "service.h"
#include "logs.h"


SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle;

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
void WINAPI ServiceCtrlHandler(DWORD opcode);

SC_HANDLE m_globalSCM;


SERVICE_TABLE_ENTRY  ServiceTable[] =
{
    { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain },
    { NULL, NULL }
};


IMPLEMENT_APP_NO_MAIN(MainApp)

HINSTANCE g_hInstance;
HINSTANCE g_hPrevInstance;
wxCmdLineArgType g_lpCmdLine;
int g_nCmdShow;

svcTimer *svc_timer;  // optional, if you want the service to do something every x seconds

extern "C" int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    wxCmdLineArgType lpCmdLine,
    int nCmdShow)
{
    init_logger(LOG_DEBUG);  // init the logger, asking it to record to the LOG_DEBUG level
    log(LOG_DEBUG,"WinMain: Start");

    if(strstr(lpCmdLine, "--svc")==NULL)     // didn't find it
    {
        log(LOG_DEBUG,"WinMain: Didn't find --svc in argv");
        return wxEntry(hInstance, hPrevInstance, lpCmdLine, nCmdShow);  // calls OnInit()
    }
    else   // if we found it, start the service
    {
        log(LOG_DEBUG,"WinMain: Found --svc in argv");
        g_hInstance = hInstance;
        g_hPrevInstance = hPrevInstance;
        g_lpCmdLine = lpCmdLine;
        g_nCmdShow = nCmdShow;
        if(!StartServiceCtrlDispatcher (ServiceTable))
        {
            log(LOG_ERROR,"WinMain: StartServiceCtrlDispatcher returned error!");
            return GetLastError();
        }
    }
    log(LOG_DEBUG,"WinMain: End");
    close_logger();
    return 0;
}


// you can override wxAppConsole::OnEventLoopEnter() in order to do initialization
// of functions that require the event loop to be running

// OnInit should return true if wxWidgets is supposed to keep on processing,
// or false to exit.  However, if there is no parent frame created in OnInit,
// wxWidgets will still automatically exit, unless you call SetExitOnFrameDelete(false).
bool MainApp::OnInit()
{
    log(LOG_DEBUG,"OnInit: Start");
    wxString usage = "--install\n--uninstall";
    log(LOG_DEBUG,"OnInit: argc is %d", argc);
    if(argc < 2)
    {
        wxMessageDialog(NULL, usage, SERVICE_NAME, wxOK).ShowModal();
        log(LOG_DEBUG,"OnInit: End");
        close_logger();
        return false;
    }

    wxString command = argv[1];
    if(command=="--svc")        // INIT THE REAL SERVICE HERE
    {
        log(LOG_DEBUG,"OnInit: Starting service loop");

        // Set wxApp::SetExitOnFrameDelete to false to keep processing
        SetExitOnFrameDelete(false);

        // START TIMERS  (optional)
        // start a timer to perform some work every x seconds
        svc_timer = new svcTimer();
        if(!svc_timer->Start(2000,wxTIMER_CONTINUOUS))    // every 2 seconds
        {
            log(LOG_DEBUG,"OnInit: Error starting svc_timer!");
            log(LOG_DEBUG,"OnInit: End");
            close_logger();
            return false;
        }

        // BIND EVENT HANDLERS
        Bind( wxEVT_THREAD, &MainApp::EvtHndlr, this, EvtID);


        // if successful, let the SCM know that we are running
        // this should be after the service has been initialized
        log(LOG_DEBUG,"OnInit: Service loop started");
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 0;

        if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            log(LOG_ERROR,"OnInit: SetServiceStatus returned an error setting SERVICE_RUNNING.");
            log(LOG_DEBUG,"OnInit: End");
            close_logger();
            return false;
        }

        return true;  // this allows wxWidgets mainloop and event handlers to run
    }
    else if(command=="--install")
    {
        log(LOG_DEBUG,"OnInit: Trying to install...");
        if(isInstalled())
        {
            log(LOG_INFO,"OnInit: Already installed");
            wxMessageDialog(NULL, "Service is already installed", SERVICE_NAME, wxOK).ShowModal();
        }
        else
        {
            install();
        }
    }
    else if(command=="--uninstall")
    {
        log(LOG_DEBUG,"OnInit: Trying to uninstall...");
        if(!isInstalled())
        {
            log(LOG_INFO,"OnInit: Already uninstalled");
            wxMessageDialog(NULL, "Service is not installed.", SERVICE_NAME, wxOK).ShowModal();
        }
        else
        {
            uninstall();
        }
    }
    else
    {
        log(LOG_INFO,"OnInit: Unknown option");
        wxMessageDialog(NULL, usage, SERVICE_NAME, wxOK).ShowModal();
    }

    log(LOG_DEBUG,"OnInit: End");
    close_logger();
    return false;
}



/*
wxApp::OnExit is called when the application exits but before wxWidgets
cleans up its internal structures. You should delete all wxWidgets objects
that you created by the time OnExit finishes.
*/
int MainApp::OnExit()
{
    log(LOG_DEBUG,"OnExit: Start");

    // stop the service timer
    svc_timer->Stop();
    delete svc_timer;

    // notify the SCM that we have stopped.
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
        log(LOG_ERROR,"OnExit: SetServiceStatus returned error!");

    log(LOG_DEBUG,"OnExit: End");
    return 0;
}

void MainApp::EvtHndlr( wxThreadEvent &evt )
{
    log(LOG_DEBUG,"MainApp::EvtHndlr: Start");
    switch(evt.GetInt())
    {
        case E_EXIT:
            //  Attempt to gracefully close everything
            log(LOG_DEBUG,"MainApp::EvtHndlr: Event called for exit");
            log(LOG_DEBUG,"MainApp::EvtHndlr: End");
            ExitMainLoop();  // tell wxWidgets we're done
            return;

        default:
            evt.Skip();
            log(LOG_DEBUG,"MainApp::EvtHndlr: Unknown event code %d", evt.GetInt());
            log(LOG_DEBUG,"MainApp::EvtHndlr: End");
            return;
    }
}


bool MainApp::isInstalled(void)
{
    log(LOG_DEBUG,"isInstalled: Start");
    bool found;
    openSCM();
    SC_HANDLE svc = OpenService(
        m_globalSCM,
        SERVICE_NAME,  //TEXT("SampleService"),
        SERVICE_ALL_ACCESS);
    if(svc)
    {
        log(LOG_DEBUG,"isInstalled: found = true");
        found=true;
        CloseServiceHandle(svc);
    }
    else
    {
        log(LOG_DEBUG,"isInstalled: found = false");
        found=false;
    }
    closeSCM();
    log(LOG_DEBUG,"isInstalled: End");
    return found;
}

bool MainApp::openSCM(void)
{
    log(LOG_DEBUG,"openSCM: Start");
    m_globalSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (m_globalSCM == NULL)
    {
        log(LOG_ERROR,"openSCM: Error %d", GetLastError());
        log(LOG_DEBUG,"openSCM: End");
        return false;
    }
    else
    {
        log(LOG_DEBUG,"openSCM: End");
        return true;
    }
}

void MainApp::closeSCM(void)
{
    CloseServiceHandle(m_globalSCM);
}

void MainApp::install(void)
{
    log(LOG_DEBUG,"install: Start");
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();

    openSCM();
    SC_HANDLE schService = CreateService(
        m_globalSCM,
        SERVICE_NAME,  // lpServiceName
        SERVICE_NAME,  // lpDisplayName
        SERVICE_ALL_ACCESS,  // dwDesiredAccess
        SERVICE_WIN32_OWN_PROCESS, // dwServiceType  --  add  | SERVICE_INTERACTIVE_PROCESS if using gui
        SERVICE_AUTO_START,  // dwStartType  --  Could also be SERVICE_DEMAND_START to start the service manually
                                                // since SERVICE_AUTO_START starts the service upon startup
        SERVICE_ERROR_NORMAL,  // dwErrorControl
        wxString::Format("\"%s\" --svc", exePath),  // lpBinaryPathName
        NULL,  // lpLoadOrderGroup
        NULL,  // lpdwTagId
        NULL,  // lpDependencies  -- Pointer to a double null-terminated array of null-separated names of services that the system must start before this service.
        NULL,  // lpServiceStartName
        NULL); // lpPassword
    if(schService == NULL)
        log(LOG_ERROR,"install: createSvc error %d", GetLastError());
    else
        log(LOG_DEBUG,"install: Service installed.");

    closeSCM();
    log(LOG_DEBUG,"install: End");
}

void MainApp::uninstall(void)
{
    log(LOG_DEBUG,"uninstall: Start");
    openSCM();
    SC_HANDLE svc = OpenService(
        m_globalSCM,
        SERVICE_NAME,
        SERVICE_ALL_ACCESS);
    SERVICE_STATUS status;
    ControlService(svc, SERVICE_CONTROL_STOP, &status);  // instruct the service to stop
    if(!DeleteService(svc))
        log(LOG_ERROR,"uninstall: DeleteService error %d", GetLastError());
    else
        log(LOG_DEBUG,"uninstall: Service uninstalled.");

    CloseServiceHandle(svc);
    closeSCM();
    log(LOG_DEBUG,"uninstall: End");
}

// starts the service.  called by StartServiceCtrlDispatcher()
void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    log(LOG_DEBUG,"ServiceMain: Start");

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, &ServiceCtrlHandler);
    if(!g_StatusHandle)
    {
        log(LOG_ERROR,"ServiceMain: ServiceCtrlHandler error!");
        log(LOG_ERROR,"ServiceMain: End");
        return;
    }


    g_ServiceStatus.dwServiceType        = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted   = 0;
    g_ServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode      = NO_ERROR;  // (0)
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint         = 0;
    g_ServiceStatus.dwWaitHint           = 0;

    // to indicate an error on exit,
//    g_ServiceStatus.dwWin32ExitCode      = ERROR_SERVICE_SPECIFIC_ERROR;  // this should be set
//    g_ServiceStatus.dwServiceSpecificExitCode = -1;  // so that this value is returned

    if(SetServiceStatus(g_StatusHandle, &g_ServiceStatus))
    {
        log(LOG_DEBUG,"ServiceMain: Calling wxEntry()...");
        wxEntry(g_hInstance, g_hPrevInstance, g_lpCmdLine, g_nCmdShow);  // This calls OnInit()   // should this be wxEntryStart() ?
    }
    else
        log(LOG_ERROR,"ServiceMain: SetServiceStatus failed while starting the service!");

    log(LOG_DEBUG,"ServiceMain: End");
    return;
}

/*
Receives control requests from the SCM and handles them appropriately
*/
void WINAPI ServiceCtrlHandler (DWORD opcode)
{
    log(LOG_DEBUG,"ServiceCtrlHandler: Start");
    DWORD status;

    wxThreadEvent* evt;

    switch(opcode)
    {
        case SERVICE_CONTROL_SHUTDOWN:  // system is shutting down, so we need to stop.
            log(LOG_DEBUG,"ServiceCtrlHandler: SERVICE_CONTROL_SHUTDOWN");

        case SERVICE_CONTROL_STOP:  // service has been politely asked to stop.
            if(opcode == SERVICE_CONTROL_STOP)
                log(LOG_DEBUG,"ServiceCtrlHandler: SERVICE_CONTROL_STOP");
            if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
                break;
            g_ServiceStatus.dwControlsAccepted = 0;
            g_ServiceStatus.dwWin32ExitCode = 0;
            g_ServiceStatus.dwCurrentState  = SERVICE_STOP_PENDING;
            g_ServiceStatus.dwCheckPoint    = 0;  // for lengthy starts, stops (while pending), you can increment dwCheckPoint to show progress
            g_ServiceStatus.dwWaitHint      = 0;

            if (!SetServiceStatus (g_StatusHandle, &g_ServiceStatus))
            {
                status = GetLastError();
                log(LOG_ERROR,"ServiceCtrlHandler: SetServiceStatus error! Error %d", status);
            }

            // send the exit prog request
            evt = new wxThreadEvent(wxEVT_THREAD, EvtID);
            evt->SetInt(5);
            wxTheApp->QueueEvent(evt);

            log(LOG_DEBUG,"ServiceCtrlHandler: End");
            return;

        case SERVICE_CONTROL_INTERROGATE:
            log(LOG_DEBUG,"ServiceCtrlHandler: SERVICE_CONTROL_INTERROGATE");
            break;

        default:
            log(LOG_DEBUG,"ServiceCtrlHandler: Unhandled opcode.");
            break;
    }

    if (!SetServiceStatus (g_StatusHandle,  &g_ServiceStatus))
    {
        status = GetLastError();
        log(LOG_ERROR,"ServiceCtrlHandler: Error setting service status! Error %d", status);
    }

    log(LOG_DEBUG,"ServiceCtrlHandler: End");
    return;
}


void svcTimer::Notify()
{
    log(LOG_DEBUG,"svcTimer::Notify: Start");


    //  DO SOMETHING PRODUCTIVE HERE
    // Note: you can't Beep() in a service on Windows 7!


    log(LOG_DEBUG,"svcTimer::Notify: End");
    return;
}
