/** \file ssWiPort.hpp
 *  \brief Header for introducing the dual head port
 *
 */

#ifndef __SHARED_SLOTTED_WIRELESS_PORT_HPP__
#define __SHARED_SLOTTED_WIRELESS_PORT_HPP__

#include "rtos.h"

#include "ssWiTypes.hpp"


/** \brief Internal type which represents a logical flow
 *
 *  Ports are used internally to model a virtual memory are of the network.
 *  Note that a port is a dual gate memory area which means that if you write
 *  on it, the reading operation does not read that value but the one received
 *  through the network
 */
class ssWiPort
{
    /** \brief receiving buffer */
    PortValue valueRX;
    /** \brief receiving mutex */
    Mutex mutexRX;

    /** \brief transmission buffer */
    PortValue valueTX;
    /** \brief transmission mutex */
    Mutex mutexTX;

    /** \brief modification flag (if true a new value has to be sent) */
    bool modified;

public:

    /** \brief constructor */
    ssWiPort () {
        modified = false;
        valueTX = 0;
        valueRX = 0;
    }

    /** \brief get the value to be sent
     */
    PortValue getTXValue();
    
    /** \brief write a value to be sent
     */
    void setTXValue(PortValue tmp);
    
    /** \brief true if there is a value to be sent
     */
    bool isModified();

    /** \brief Read the last received value
     */
    PortValue getRXValue();
    
    /** \brief Write the last received value
     */
    void setRXValue(PortValue tmp);

};

#endif //__SHARED_SLOTTED_WIRELESS_PORT_HPP__
