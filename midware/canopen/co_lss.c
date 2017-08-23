#include "canopens_private.h"
#include "can.h"
#include <string.h>
#include "stdio.h"

#pragma pack(push,1)
typedef struct {
    uint8_t cmd;
    uint32_t id;
    uint8_t bit_check;
    uint8_t lss_sub;
    uint8_t lss_next;
} LSS_PKT;
#pragma pack(pop)

void lss_send_msg(CO* co, uint32_t type);

//------- LSS MASTER ---------------
static inline void lss_found(CO* co)
{
    ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LSS_FOUND), 11111, 0, 0);
    printf("LSS: vendor:%x product:%x version:%x serial:%x \n ",
            co->lss.req.lss_addr.vendor, co->lss.req.lss_addr.product,
            co->lss.req.lss_addr.version, co->lss.req.lss_addr.serial);
}
static inline void lss_not_found(CO* co)
{
    ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LSS_FOUND), 0, 1, 1);
    printf("No unconfigurated nodes\n");
}

static inline void lss_send_master_msg(CO* co, uint32_t type)
{
    co->out_msg.id = MLSS_ADRESS;
    co->lss.response = false;
    lss_send_msg(co, type);
}

static inline void lss_msend_fss_bitcheck(CO* co)
{
    LSS_PKT* ptr = (LSS_PKT*)&co->out_msg.data;
    ptr->id = co->lss.req.lss_addr.data[co->lss.curr_pos];//co->lss.remote_id;
    ptr->bit_check = co->lss.bit_checked;
    ptr->lss_sub = co->lss.curr_pos;
    ptr->lss_next = co->lss.curr_pos;

    if (co->lss.state == LSS_TEST)
        ptr->lss_next++;
    lss_send_master_msg(co, LSS_IDENT_FASTSCAN);
}

void lss_init_find(CO* co, IO* io)
{
    LSS_PKT* ptr = (LSS_PKT*)&co->out_msg.data;
    memcpy((uint8_t*)&co->lss.req, io_data(io), sizeof(LSS_FIND));
    timer_start_ms(co->timers.lss, LSS_DELAY_MS);
    co->lss.curr_pos = co->lss.req.start_pos;
    co->lss.state = LSS_RESET;
    co->out_msg.data.lo = 0;
    co->out_msg.data.hi = 0;
    ptr->bit_check = 0x80;
    ptr->lss_sub = co->lss.req.start_pos;
    lss_send_master_msg(co, LSS_IDENT_FASTSCAN);
}

void lss_init_set_id(CO* co, uint32_t id)
{
    co->lss.state = LSS_SET_ID;
    timer_start_ms(co->timers.lss, LSS_DELAY_MS);
    co->out_msg.data.lo = id << 8;
    co->out_msg.data.hi = 0;
    lss_send_master_msg(co, LSS_CONF_NODE_ID);
}

static inline void lss_init_pos(CO* co)
{
    uint8_t bit_cnt = co->lss.req.pos[co->lss.curr_pos];
//    co->lss.response = false;
    co->lss.bit_checked = bit_cnt;
    co->lss.req.lss_addr.data[co->lss.curr_pos] &= (0xFFFFFFFF << bit_cnt);
    if (bit_cnt)
        co->lss.state = LSS_BIT_CHECK;
    else
    co->lss.state = LSS_TEST;

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
        co->lss.response = true;
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
    case LSS_RESET:
        if (co->lss.response)
            lss_init_pos(co);
        else
        {
            lss_not_found(co);
            return;
        }
        break;
    case LSS_TEST:
        if (!co->lss.response)
        {
            lss_not_found(co);
            return;
        } else
        if (co->lss.curr_pos >= 3)
        {
            lss_found(co);
            return;
        } else
        {
            co->lss.curr_pos++;
            lss_init_pos(co);
        }
        break;
    case LSS_BIT_CHECK:
        if ((co->lss.response) && (co->lss.curr_pos >= 3) && (co->lss.bit_checked == 0))
        {
            lss_found(co);
            return;
        }
        if (!co->lss.response)
            co->lss.req.lss_addr.data[co->lss.curr_pos] |= 1 << co->lss.bit_checked;
        if (co->lss.bit_checked == 0)
            co->lss.state = LSS_TEST;
        else
        {
            co->lss.bit_checked--;
        }
        break;
    default:
        printf("Wrong state in lss\n");
        return;
    } // switch
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
    LSS_PKT* ptr = (LSS_PKT*)&co->rec_msg.data;
    uint8_t cmd = ptr->cmd;
    uint8_t bit_checked = ptr->bit_check;
    uint32_t id = ptr->id;
    if (co->co_state == Operational)
        return;
    if (co->lss.state == LSS_CONFIGURATION)
        switch (cmd)
        // LSS_CONFIGURATION
        {
        case LSS_CONF_NODE_ID:
            co->id = co->rec_msg.data.b1;
            ipc_post_inline(process_get_current(), HAL_CMD(HAL_CANOPEN, IPC_CO_ID_CHANGED), 0, 0, 0);
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
        co->lss.curr_pos = ptr->lss_sub;
        lss_slave_replay(co, LSS_IDENT_SLAVE);
        return;
    }
    if (bit_checked > 31)
        return;
    if (co->lss.curr_pos != ptr->lss_sub)
        return;
    if ((co->lss.lss_id.data[ptr->lss_sub] & (0xFFFFFFFF << bit_checked)) == id)
    {
        lss_slave_replay(co, LSS_IDENT_SLAVE);
        co->lss.curr_pos = ptr->lss_next;
        if ((bit_checked == 0) && (ptr->lss_sub == 3))
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

