/** \file xbee.hpp
 *  \brief header of the ssWi channel using the XBee module
 *
 */
 
#ifndef __XBEE_MODULE_HPP__
#define __XBEE_MODULE_HPP__

#include "mbed.h"

#include "MODSERIAL.h"

#include <ssWiChannel.hpp>

#include "string"


class XBeeAddress
{
    string low;
    string high;

public:

    XBeeAddress () : low(""), high("") {}

    XBeeAddress (string low, string high) {
        this->low = low;
        this->high = high;
    }       
    
    string getLowAddr () {
        return low;
    }
    
    string getHighAddr () {
        return high;
    }
};


class XBeeBroadcastAddress: public XBeeAddress
{
public:
    XBeeBroadcastAddress () : XBeeAddress("00FFFF", "0") {}
};


class XBeeModule: public ssWiChannel
{

    MODSERIAL xbee;
    XBeeAddress local;

    bool status;

    bool executeWithOk (const char* cmd);
    void executeWithRes (const char* cmd, char* res);
    void readResponse (char* msg);

    bool _getLocalAddr ();
    bool _setChannel (int channel);
    bool _setPanID (int id);
 public:

    XBeeModule (PinName tx, PinName rx, int panID, int channel);

    /*
     * specific for the XBee module
     */
    XBeeAddress getLocalAddress () {
        return local;
    }

    bool setDstAddress (XBeeAddress addr);

    XBeeAddress getDstAddress ();

    int getChannel ();
    
    int getPanID ();


    /*
     * inherited from ssWiChannel
     */
    virtual bool init (int TXRate, int RXRate) {
        return _init(this, TXRate, RXRate);
    }

    virtual int read (char* msg);
    
    virtual void write (const char* msg, int n);
    
};

#endif //__XBEE_MODULE_HPP__
