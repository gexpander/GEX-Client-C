//
// Created by MightyPork on 2017/12/22.
//

#include <stdio.h>
#include <assert.h>
#include "utils/hexdump.h"
#include "utils/payload_parser.h"
#include "utils/payload_builder.h"

#define GEX_H
#include "gex_bulk.h"
#include "gex_internal.h"
#include "gex_message_types.h"

/**
 * Bulk read from a unit, synchronous
 *
 * @param unit - the unit to target. If system_unit is used, a raw query will be sent (no unit preamble and cmd is TYPE)
 * @param bulk - the bulk transport info struct
 * @return actual number of bytes received.
 */
uint32_t GEX_BulkRead(GexUnit *unit, GexBulk *bulk)
{
    // We ask for the transfer. This is unit specific and can contain information that determines the transferred data
    // if unit callsign is 0, it's the system unit and we use raw query without the unit prefix
    GexMsg resp0 = GEX_QueryEx(unit, bulk->req_cmd,
                               bulk->req_data, bulk->req_len,
                               0, false, // frame_id, response
                               unit->callsign == 0);

    if (resp0.type == MSG_ERROR) {
        fprintf(stderr, "Bulk read rejected! %.*s\n", resp0.len, (char*)resp0.payload);
        return 0;
    }

    if (resp0.type != MSG_BULK_READ_OFFER) {
        fprintf(stderr, "Bulk read failed, response not BULK_READ_OFFER!\n");
        return 0;
    }

    // Check how much data is available
    PayloadParser pp = pp_start(resp0.payload, resp0.len, NULL);
    uint32_t total = pp_u32(&pp);
    assert(pp.ok);

    total = MIN(total, bulk->capacity);

    uint32_t at = 0;
    while (at < total) { // +1 makes sure we read one past end and trigger the END OF DATA msg
        uint8_t buf[10];
        PayloadBuilder pb = pb_start(buf, 10, NULL);
        pb_u32(&pb, TF_MAX_PAYLOAD_RX); // This selects the chunk size.

        GexMsg resp = GEX_QueryEx(unit, MSG_BULK_READ_POLL, buf,
                                  (uint32_t) pb_length(&pb), resp0.session, true, true);

        if (resp.type == MSG_ERROR) {
            fprintf(stderr, "Bulk read failed! %.*s\n", resp.len, (char *) resp.payload);
            return 0;
        }

        if (resp.type == MSG_BULK_DATA || resp.type == MSG_BULK_END) {
            memcpy(bulk->buffer+at, resp.payload, resp.len);
            at += resp.len;

            // quit if we're done
            if (resp.type == MSG_BULK_END) {
                return at;
            }
        } else {
            fprintf(stderr, "Bulk read failed! Bad response type.\n");
            return 0;
        }
    }

    return at;
}


/**
 * Bulk write to a unit, synchronous
 *
 * @param unit - the unit to target. If system_unit is used, raw query with cmd as TYPE will be sent.
 * @param bulk - the bulk transport info struct
 * @return true on success
 */
bool GEX_BulkWrite(GexUnit *unit, GexBulk *bulk)
{
    // We ask for the transfer. This is unit specific
    GexMsg resp0 = GEX_QueryEx(unit,
                               bulk->req_cmd, bulk->req_data, bulk->req_len,
                               0, false,
                               unit->callsign == 0);

    if (resp0.type == MSG_ERROR) {
        fprintf(stderr, "Bulk write rejected! %.*s\n", resp0.len, (char*)resp0.payload);
        return false;
    }

    if (resp0.type != MSG_BULK_WRITE_OFFER) {
        fprintf(stderr, "Bulk write failed, response not MSG_BULK_WRITE_OFFER!\n");
        return false;
    }

    PayloadParser pp = pp_start(resp0.payload, resp0.len, NULL);
    uint32_t max_size = pp_u32(&pp);
    uint32_t max_chunk = pp_u32(&pp);
    assert(pp.ok);

    if (max_size < bulk->len) {
        fprintf(stderr, "Write not possible, not enough space.\n");

        // Inform GEX we're not going to do it
        GEX_SendEx(unit, MSG_BULK_ABORT,
                   NULL, 0,
                   resp0.session, true,
                   true);

        return false;
    }

    uint32_t at = 0;
    while (at < bulk->len) {
        uint32_t chunk = MIN(max_chunk, bulk->len - at);

        GexMsg resp = GEX_QueryEx(unit, ((at+chunk) >= bulk->len) ? MSG_BULK_END : MSG_BULK_DATA,
                                  bulk->buffer + at, chunk,
                                  resp0.session, true,
                                  true);
        at += chunk;

        if (resp.type == MSG_ERROR) {
            fprintf(stderr, "Bulk write failed! %.*s\n", resp.len, (char *) resp.payload);
            return false;
        }

        if (resp.type != MSG_SUCCESS) {
            fprintf(stderr, "Bulk write failed! Bad response type.\n");
            return false;
        }
    }

    return true;
}
