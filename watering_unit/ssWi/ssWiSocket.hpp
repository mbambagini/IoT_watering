/** \file ssWiSocket.hpp
 *  \brief Header for the communication socket
 *
 */

#ifndef __SHARED_SLOTTED_WIRELESS_SOCKET_HPP__
#define __SHARED_SLOTTED_WIRELESS_SOCKET_HPP__


#include "ssWiTypes.hpp"


/** \brief Socket to communciate through ssWi
 *
 * It is not possible to instanciate directly a ssWiSocket, use the static
 * method createSocket
 */
class ssWiSocket
{
    /** \brief Port identifier */
    PortID _id;

    /** \brief Hidden constructor */
    ssWiSocket(PortID id) : _id(id) {}

public:

    /** \brief create a new socket
     * 
     * If the network is not inizialized yet, the method returns false
     *
     * \param id port identifier to connect the socket with
     * \return the created socket
     */
    static ssWiSocket* createSocket(PortID id);

    /** \brief read the last value read through the network on such socket
     *
     * \return the read value
     */
    PortValue read ();

    /** \brief write a new value to be sent through the socket
     *
     * \param value value to be sent
     */
    void write (PortValue value);

};


#endif //__SHARED_SLOTTED_WIRELESS_SOCKET_HPP__
