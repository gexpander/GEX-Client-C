//
// Created by MightyPork on 2017/12/12.
//

#ifndef GEX_CLIENT_GEX_CLIENT_H
#define GEX_CLIENT_GEX_CLIENT_H

#ifndef GEX_H
#error "Include gex.h instead!"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "TinyFrame.h"
#include "gex_defines.h"

/**
 * Bind a report listener. The listener is called with a message object when
 * a spontaneous report is received. If no known unit matched the report,
 * a dummy unit is provided to avoid NULL access.
 *
 * @param unit - the handled unit
 * @param lst - the listener
 */
void GEX_SetUnitReportListener(GexUnit *unit, GexEventListener lst);

/**
 * Bind a defualt report listener (used if a report is not handled by any unit report listener).
 *
 * @param gex - client
 * @param lst - the listener
 */
void GEX_SetDefaultReportListener(GexClient *gex, GexEventListener lst);

/**
 * Get the bare TinyFrame instance
 * @param gex - GEX client
 * @return TinyFrame instance pointer
 */
TinyFrame *GEX_GetTF(GexClient *gex);

/**
 * Get the dummy SYSTEM unit. Can be used for bulk read/write that's
 * not targeted at a particular unit.
 *
 * @param gex - client
 * @return the SYSTEM unit
 */
GexUnit *GEX_SysUnit(GexClient *gex);

/**
 * Get a unit, or NULL
 *
 * @param gex - gex
 * @param name - unit name
 * @param type - unit type, used to verify it's the right type
 */
GexUnit *GEX_GetUnit(GexClient *gex, const char *name, const char *type);

/**
 * Initialize the GEX client
 *
 * @param device - device, e.g. /dev/ttyACM0
 * @param timeout_ms - read timeout in ms (allowed only multiples of 100 ms, others are rounded)
 * @return an allocated client instance
 */
GexClient *GEX_Init(const char *device, uint32_t timeout_ms);

/**
 * Poll for new messages
 * @param gex - client
 */
void GEX_Poll(GexClient *gex);

/**
 * Safely release all resources used up by GEX_Init()
 *
 * @param gex - the allocated client structure
 */
void GEX_DeInit(GexClient *gex);

#endif //GEX_CLIENT_GEX_CLIENT_H
