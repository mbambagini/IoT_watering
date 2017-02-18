/** \file ssWiPort.cpp
 *  \brief Implementation of the dual head port
 *
 */

#include "ssWiPort.hpp"

PortValue ssWiPort::getTXValue()
{
    PortValue tmp;
    mutexTX.lock();
    tmp = valueTX;
    modified = false;
    mutexTX.unlock();
    return tmp;
}

void ssWiPort::setTXValue(PortValue tmp)
{
    mutexTX.lock();
    valueTX = tmp;
    modified = true;
    mutexTX.unlock();
}

bool ssWiPort::isModified()
{
    bool tmp;
    mutexTX.lock();
    tmp = modified;
    mutexTX.unlock();
    return tmp;
}

PortValue ssWiPort::getRXValue()
{
    PortValue tmp;
    mutexRX.lock();
    tmp = valueRX;
    mutexRX.unlock();
    return tmp;
}

void ssWiPort::setRXValue(PortValue tmp)
{
    mutexRX.lock();
    valueRX = tmp;
    mutexRX.unlock();
}
