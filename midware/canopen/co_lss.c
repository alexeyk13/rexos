#include "canopens_private.h"
#include "can.h"

void lss_send_msg(CO* co, uint32_t type);

//------- LSS MASTER ---------------
static inline void lss_send_master_msg(CO* co, uint32_t type)
{
    co->out_msg.id = MLSS_ADRESS;
    lss_send_msg(co, type);
}

static inline void lss_msend_fss_bitcheck(CO* co)
{
    co->out_msg.data.lo = co->lss.remote_id << 8;
    co->out_msg.data.hi = (co->lss.remote_id >> 24);
    co->out_msg.data.b5 = co->lss.bit_checked;
    lss_send_master_msg(co, LSS_IDENT_FASTSCAN);
}

void lss_init_find(CO* co, uint32_t bit_cnt, uint32_t lss_id)
{
    timer_start_ms(co->timers.lss, LSS_DELAY_MS);
    co->lss.response = false;
    co->lss.bit_checked = bit_cnt;
    co->lss.remote_id = lss_id & (0xFFFFFFFF << bit_cnt);
    if (bit_cnt)
    {
        co->lss.state = LSS_RESET;
        co->out_msg.data.lo = 0;
        co->out_msg.data.hi = 0x8000;
        lss_send_master_msg(co, LSS_IDENT_FASTSCAN);
    } else
    {
        co->lss.state = LSS_TEST;
        lss_msend_fss_bitcheck(co);
    }
}

void lss_init_set_id(CO* co, uint32_t id)
{
    co->lss.state = LSS_SET_ID;
    timer_start_ms(co->timers.lss, LSS_DELAY_MS);
    co->out_msg.data.lo = id << 8;
    co->out_msg.data.hi = 0;
    lss_send_master_msg(co, LSS_CONF_NODE_ID);
}

static inline void lss_master_request(CO* co)
{
    if (co->rec_msg.data_len != 8)
        return;
    switch (co->lss.state)
    {
    case LSS_SET_ID:
        if (co->rec_msg.data.b0 != LSS_CONF_NODE_ID)
            return;
        timer_stop(co->timers.lss, COT_LSS, HAL_CANOPEN);
        co->out_msg.data.lo = 0;
        co->out_msg.data.hi = 0;
        lss_send_master_msg(co, LSS_SM_GLOBAL);
        ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LSS_SET_ID), 0, 0, 0);
        break;
    case LSS_TEST:
        if (co->rec_msg.data.b0 != LSS_IDENT_SLAVE)
            return;
        timer_stop(co->timers.lss, COT_LSS, HAL_CANOPEN);
        ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LSS_FOUND), co->lss.remote_id, 0, 0);
        break;
    default:
        co->lss.response = true;
        break;
    }
}

void lss_timeout(CO* co)
{
    switch (co->lss.state)
    {
    case LSS_SET_ID: // not answer
        ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LSS_SET_ID), 1, 1, 1);
        return;
    case LSS_TEST: // not found
        ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LSS_FOUND), 0, 1, 1);
        return;
    case LSS_RESET:
        if (co->lss.response)
        {
            co->lss.state = LSS_BIT_CHECK;
        } else
        {
            ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LSS_FOUND), 0, 1, 1);
            return;
        }
        break;
    case LSS_BIT_CHECK:
        if (co->lss.bit_checked == 0)
        {
            if (co->lss.response)
            {
                ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LSS_FOUND), co->lss.remote_id, 0, 0);
                return;
            } else
            {
                co->lss.state = LSS_TEST;
                co->lss.remote_id |= 0x01;
                break;
            }
        }
        if (!co->lss.response)
            co->lss.remote_id |= 1 << co->lss.bit_checked;
        co->lss.bit_checked--;
        break;
    default:
        return;
    } // switch
    co->lss.response = false;
    lss_msend_fss_bitcheck(co);
    timer_start_ms(co->timers.lss, LSS_DELAY_MS);
}

//------- LSS SLAVE ---------------
static inline void lss_slave_replay(CO* co, uint8_t type)
{
    co->out_msg.data.hi = 0;
    co->out_msg.data.lo = 0;
    co->out_msg.id = SLSS_ADRESS;
    lss_send_msg(co, type);
}

static inline void lss_slave_request(CO* co)
{
    uint8_t cmd = co->rec_msg.data.b0;
    uint8_t bit_checked = co->rec_msg.data.b5;
    uint32_t id = (co->rec_msg.data.lo >> 8) | (co->rec_msg.data.hi << 24);
    if (co->co_state == Operational)
        return;
    if (co->lss.state == LSS_CONFIGURATION)
        switch (cmd)
        // LSS_CONFIGURATION
        {
        case LSS_CONF_NODE_ID:
            co->id = co->rec_msg.data.b1;
            lss_slave_replay(co, LSS_CONF_NODE_ID);
            return;
        case LSS_SM_GLOBAL:
            co->lss.state = LSS_WAITING;
            ipc_post_inline(process_get_current(), HAL_CMD(HAL_CANOPEN, IPC_CO_INIT), 0, 0, 0);
            return;
        default:
            return;
        }
// LSS_WAITING
    if (cmd != LSS_IDENT_FASTSCAN)
        return;
    if (bit_checked == 0x80) // RESET
    {
        lss_slave_replay(co, LSS_IDENT_SLAVE);
        return;
    }
    if (bit_checked > 31)
        return;
    if ((co->lss.id & (0xFFFFFFFF << bit_checked)) == id)
    {
        lss_slave_replay(co, LSS_IDENT_SLAVE);
        if (bit_checked == 0)
        {
            co->lss.state = LSS_CONFIGURATION;
            co_led_change(co, LED_CONFIG);
        }
    }
}

//------- COMMON ---------------
void lss_send_msg(CO* co, uint32_t type)
{
    co->out_msg.data_len = 8;
    co->out_msg.rtr = 0;
    co->out_msg.data.b0 = type;
    can_write(&co->out_msg, &co->can_state);
}
void lss_request(CO* co)
{
#if (CO_LSS_MASTER)
    if((co->rec_msg.id == SLSS_ADRESS) && (co->rec_msg.data_len ==8)) lss_master_request(co);
#else
    if ((co->rec_msg.id == MLSS_ADRESS) && (co->rec_msg.data_len == 8))
        lss_slave_request(co);
#endif
}

