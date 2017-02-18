/** \file ssWiSocket.cpp
 *  \brief Implementation of the communication socket
 *
 */
 
#include "ssWiSocket.hpp"
#include "ssWiPort.hpp"
#include "ssWi.hpp"

#include <map>


extern std::map<int, ssWiPort> ports;


PortValue ssWiSocket::read () {
    return ports[_id].getRXValue();
}

void ssWiSocket::write (PortValue value) {
    ports[_id].setTXValue(value);
}

ssWiSocket* ssWiSocket::createSocket(PortID id)
{
    if (!ssWi_setPort(id))
        return NULL;
    return new ssWiSocket(id);
}
