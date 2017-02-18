/** \file ssWi.hpp
 *  \brief internal functions for mananing the protocol
 *
 */

#ifndef __SHARED_SLOTTED_WIRELESS_HPP__
#define __SHARED_SLOTTED_WIRELESS_HPP__

#include "ssWiTypes.hpp"


/** \brief number of provided ports
 */
#define N_PORTS 256


class ssWiChannel;

/** \brief Initialize the ssWi protocol
 *
 * It is not possible to have two instances of this protocol at the same time. 
 *
 * \param c channel to be used for sending/receving data
 * \param rateTX transmission rate (how many time every second)
 * \param rateRX receiving rate (how many time every second)
 * \return true if the network has been correctly initialized, false otherwise
 *
 * \warning rx should be at least twice more frequent than tx (rateRX >= 2*rateTX)
 */
bool ssWi_init (ssWiChannel* c, int rateTX, int rateRX);

/** \brief disable the communication protocol
 *
 */
void ssWi_stop ();

/** \brief check if the communication port is open
 *
 * \param port port identified to check
 * \return true if the port is open, false otherwise
 */
bool ssWi_isActive (PortID port);

/** \brief open the specified port
 *
 * \param port port identified to open
 * \return true if the port has been opened, false otherwise
 */
bool ssWi_setPort (PortID port);

/** \brief free the specified port
 *
 * \param port port identified to close
 * \return true if the port has been closed, false otherwise
 */
bool ssWi_unsetPort (PortID port);


#endif //__SHARED_SLOTTED_WIRELESS_HPP__
