/** \file ssWiChannel.hpp
 *  \brief Abstract class for the communication channel
 *
 */

#ifndef __SHARED_SLOTTED_WIRELESS_CHANNEL_HPP__
#define __SHARED_SLOTTED_WIRELESS_CHANNEL_HPP__


#include "ssWi.hpp"


/** \brief Abstract class for a communication channel
 *
 */
class ssWiChannel
{

protected:

    /** \brief Initialize the protocol
     *
     * \param c ssWi channel
     * \param TXRate how may times trans every second
     * \param RXRate how may time rx every second
     * \return true if the channel is ready, false otherwise
     */
    bool _init (ssWiChannel* c, int TXRate, int RXRate) {
        return ssWi_init(c, TXRate, RXRate);
    }

public:

    /** \brief Initialize ssWi on this channel
     *
     *  \param TXRate number of transmissions per second
     *  \param RXRate number of receptions per second
     *  \return true if ssWi has been initialized succefully, false otherwise
     */
    virtual bool init (int TXRate, int RXRate) = 0;

    /** \brief read from the socket
     *
     *  \param msg buffer where to write the read message
     *  \return the number of read bytes
     */
    virtual int read (char* msg) = 0;

    /** \brief write to the socket
     *
     *  \param msg buffer with the message to send
     *  \param n number of bytes to send
     */
    virtual void write (const char* msg, int n) = 0;

};

#endif //__SHARED_SLOTTED_WIRELESS_CHANNEL_HPP__
