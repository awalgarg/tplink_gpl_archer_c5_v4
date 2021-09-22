/*
 * pcap-septel.c: Packet capture interface for Intel Septel card
 *
 * The functionality of this code attempts to mimic that of pcap-linux as much
 * as possible.  This code is only needed when compiling in the Intel/Septel
 * card code at the same time as another type of device.
 *
 * Authors: Gilbert HOYEK (gil_hoyek@hotmail.com), Elias M. KHOURY
 * (+961 3 485343);
 *
 * @(#) $Header: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/lib/libpcap-1.0.0/pcap-septel.h#1 $
 */

pcap_t *septel_create(const char *device, char *ebuf);

