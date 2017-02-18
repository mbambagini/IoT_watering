/** \file xbee.cpp
 *  \brief implementation of the ssWi channel using the XBee module
 *
 */
 
#include "xbee.hpp"

#include <sstream>


void XBeeModule::readResponse (char* msg)
{
    char c = 0;
    int i = 0;
    while (c!='\r')
        if (xbee.readable()) {
            c = xbee.getc();
            msg[i++] = c;
        }
}

bool XBeeModule::executeWithOk (const char* cmd)
{
    char msg[5];
    xbee.printf("%s", cmd);
    readResponse(msg);
    //printf("%s -> %s\n\r", cmd, msg);
    if (strncmp(msg, "OK\r", 3)!=0)
        return false;
    return true;
}

void XBeeModule::executeWithRes (const char* cmd, char* res)
{
    xbee.printf("%s", cmd);
    readResponse(res);
    //printf("\n\r%s -> %s\n\r", cmd, res);
}

XBeeModule::XBeeModule (PinName tx, PinName rx, int panID, int channel) : xbee(tx, rx, 64)
{
    status = false;
    wait(1);
    if (!_getLocalAddr())
        return;
    wait(1);
    if (!_setChannel(channel))
        return;
    wait(1);
    if (!_setPanID(panID))
        return;
    wait(1);
    status = true;
}

XBeeAddress XBeeModule::getDstAddress ()
{
    char tmp[10];
    std::stringstream s1, s2;
    string high, low;
    XBeeAddress addr;
    wait(1);
    if (!executeWithOk("+++"))
        return addr;

    wait(1);

    executeWithRes("ATDH \r", tmp);
    s1<<std::hex<<tmp;
    s1>>high;

    executeWithRes("ATDL \r", tmp);
    s2<<std::hex<<tmp;
    s2>>low;

    if (!executeWithOk("ATCN \r"))
        return addr;
    
    wait(1);
    return XBeeAddress(low, high);
}

bool XBeeModule::setDstAddress (XBeeAddress addr)
{
    char s[10];
    string low, high;
    wait(1);
    if (!executeWithOk("+++"))
        return false;
    wait(1);
    sprintf(s, "ATDH%s \r", addr.getHighAddr().c_str());
    //printf("%s\n\r", addr.getHighAddr().c_str());
    if (!executeWithOk(s))
        return false;

    sprintf(s, "ATDL%s \r", addr.getLowAddr().c_str());
    //printf("%s\n\r", addr.getLowAddr().c_str());
    if (!executeWithOk(s))
        return false;

    if (!executeWithOk("ATCN \r"))
        return false;

    return true;
}

bool XBeeModule::_getLocalAddr ()
{
    char tmp[10];
    string high, low;
    std::stringstream s1, s2;

    if (!executeWithOk("+++"))
        return false;
    executeWithRes("ATSH \r", tmp);
    s1<<std::hex<<tmp;
    s1>>high;
    executeWithRes("ATSL \r", tmp);
    s2<<std::hex<<tmp;
    s2>>low;
    if (!executeWithOk("ATCN \r"))
        return false;
    local = XBeeAddress(low, high);
    return true;
}


bool XBeeModule::_setChannel (int channel)
{
    char s[10];

    if (!executeWithOk("+++"))
        return false;
    wait(1);
    sprintf(s, "ATCH%d \r", channel);
    if (!executeWithOk(s))
        return false;
    if (!executeWithOk("ATCN \r"))
        return false;
    return true;
}

int XBeeModule::getChannel ()
{
    int channel;
    char s[10];

    if (!executeWithOk("+++"))
        return -1;
    wait(1);
    executeWithRes("ATCH \r", s);
    channel = atoi(s);
    if (!executeWithOk("ATCN \r"))
        return -1;
    wait(1);
    return channel;
}


bool XBeeModule::_setPanID (int panID)
{
    char s[10];
    if (!executeWithOk("+++"))
        return false;
    sprintf(s, "ATID%d \r", panID);
    if (!executeWithOk(s))
        return false;
    if (!executeWithOk("ATCN \r"))
        return false;
    return true;
}

int XBeeModule::getPanID ()
{
    int id;
    char s[10];

    if (!executeWithOk("+++"))
        return -1;
    executeWithRes("ATID \r", s);
    id = atoi(s);
    if (!executeWithOk("ATCN \r"))
        return -1;
    wait(1);
    return id;
}

int XBeeModule::read (char* msg)
{
    int i = 0;
    DigitalOut l(LED1);
    l = 1;
    while (xbee.readable())
        msg[i++] = xbee.getc();
    l = 0;
    return i;
}

void XBeeModule::write (const char* msg, int n)
{
    DigitalOut l(LED1);
    l = 1;
    for (int i=0; i<n; i++) {
        while(!xbee.writeable());
        xbee.putc(msg[i]);
    }
    l = 0;
}
