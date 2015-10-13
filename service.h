// service.h
#include <wx/wx.h>

#define SERVICE_NAME  L"My Sample WX Service"


// arbitrary event ID so that our event handler is not processing every event generated
//  by wxWidgets.
#define EvtID  42

// Also arbitrary event numbers, just so we can keep track of what we are signalling
#define E_EXIT 5
// add more here as necessary


class MainApp: public wxApp
{
    private:
        bool openSCM(void);
        void closeSCM(void);
        bool isInstalled(void);
        void install(void);
        void uninstall(void);
        void EvtHndlr(wxThreadEvent &);
    public:
        virtual bool OnInit();
        virtual int OnExit();
};


//  timer used for doing service operations
class svcTimer : public wxTimer
{
    public:
        void Notify();  // called every x seconds
};
