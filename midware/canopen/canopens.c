#include "sys_config.h"
#include "process.h"
#include "stdio.h"
#include "sys.h"
#include "io.h"
#include "canopens_private.h"

#define CO_DEF_BAUDRATE               250000
#define CO_DEF_HEATBEART              2000 // in ms
#define CO_DEF_TIM_BUSRESTART         4000 // in ms

#define CO_MIN_INHIBIT_TIME          5000 // in us

#define TIMER_PDO_MASK                (1UL <<31)
#define COT_PDO                       (TIMER_PDO_MASK +1)
#define COT_INHIBIT                   (TIMER_PDO_MASK +2)
#define CO_OD_ENTRY_EVENT_TIME_1TPDO   0x051800
#define CO_OD_ENTRY_INHIBIT_TIME       CO_OD_IDX(0x1800, 3)
#define CO_OD_ENTRY_ERROR_REG          CO_OD_IDX(0x1001, 0)

#define IS_MY_ID          ((co->rec_msg.id & 0x7F) == co->id)

#include <string.h>

static void pdo_start_timer(CO* co)
{
    CO_OD_ENTRY* entry = co_od_find_idx(co->od, CO_OD_ENTRY_EVENT_TIME_1TPDO);
    if (entry == NULL)
        return;
    if(entry->data)
        timer_start_ms(co->tpdo.timer, entry->data);
}

static void pdo_send(CO* co, uint32_t cob_id, uint32_t data, uint32_t len)
{
    if (co->co_state != Operational)
        return;
    co->out_msg.id = cob_id;
    co->out_msg.data.lo = data;
    co->out_msg.data.hi = 0;
    co->out_msg.data_len = len;
    can_write(&co->out_msg, &co->can_state);
    if((!co->tpdo.inhibit) && (co->tpdo.inhibit_time))
        {
            timer_start_us(co->tpdo.inhibit_timer, co->tpdo.inhibit_time);
            co->tpdo.inhibit = true;
        }
}

void pdo_data_changed(CO* co, CO_OD_ENTRY* entry, uint32_t data)
{
    int i;
    for (i = 0; i < CO_MAX_TPDO; i++)
    {
        if (entry != co->tpdo.od_var[i])
            continue;
        if(co->tpdo.inhibit)
        {
            co->tpdo.updated_msk |= (1 << i);
            continue;
        }
        pdo_send(co, co->tpdo.cob_id[i]->data, data, entry->len & 0xff);
        timer_stop(co->tpdo.timer, COT_PDO, HAL_CANOPEN);
        pdo_start_timer(co);
    }
}

uint32_t pdo_tpdo_param_change(CO* co, CO_OD_ENTRY* entry, uint32_t data)
{
    switch (entry->idx)
    {
    case CO_OD_ENTRY_EVENT_TIME_1TPDO:   //event timer first TPDO in  ms
        if((data < 100) && (data != 0))
            data = 100;
        entry->data = data;
        break;
    case CO_OD_ENTRY_INHIBIT_TIME:   // in us
        if((data < CO_MIN_INHIBIT_TIME) && (data != 0))
            data = CO_MIN_INHIBIT_TIME;
        entry->data = data;
        co->tpdo.inhibit_time = data;
        break;
    }

    return SDO_ERROR_OK;
}

void pdo_timeout(CO* co, uint32_t timer)
{
    int i;
    switch (timer)
    {
    case COT_PDO:
        for (i = 0; i < co->tpdo.count; i++)
            pdo_send(co, co->tpdo.cob_id[i]->data, co->tpdo.od_var[i]->data, co->tpdo.od_var[i]->len & 0xff);
        pdo_start_timer(co);
        break;
    case COT_INHIBIT:
        co->tpdo.inhibit = false;
        for (i = 0; i < co->tpdo.count; i++)
            if(co->tpdo.updated_msk & (1 << i))
                pdo_send(co, co->tpdo.cob_id[i]->data, co->tpdo.od_var[i]->data, co->tpdo.od_var[i]->len & 0xff);
        co->tpdo.updated_msk = 0;
        break;
    }
}

void pdo_init(CO* co)
{
    CO_OD_ENTRY* entry;
    int i;
    uint32_t idx;
    if(co->co_state != Operational)
    {
        co->tpdo.count = 0;
        co->rpdo.count = 0;
        return;
    }
//search TPDO
    for (i = 0; i < CO_MAX_TPDO; i++)
    {
        entry = co_od_find_idx(co->od, CO_OD_IDX(0x1800 + i, 1));
        if (entry == NULL)
            break;
        co->tpdo.cob_id[i] = entry;
        entry->data = (entry->data & ~MSK_ID) | co->id;
        entry = co_od_find_idx(co->od, CO_OD_IDX(0x1A00 + i, 1));
        if (entry == NULL)
            break;
        idx = (entry->data >> 16) | ((entry->data << 8) & 0x00FF0000);
        co->tpdo.od_var[i] = co_od_find_idx(co->od, idx);
        if (co->tpdo.od_var[i] == NULL)
            break;
    }
    co->tpdo.count = i;
//search RPDO
    for (i = 0; i < CO_MAX_RPDO; i++)
    {
        entry = co_od_find_idx(co->od, CO_OD_IDX(0x1400 + i, 1));
        if (entry == NULL)
            break;
        entry->data = (entry->data & ~MSK_ID) | co->id;
        co->rpdo.cob_id[i] = entry;
        entry = co_od_find_idx(co->od, CO_OD_IDX(0x1600 + i, 1));
        if (entry == NULL)
            break;
        idx = (entry->data >> 16) | ((entry->data << 8) & 0x00FF0000);
        co->rpdo.od_var[i] = co_od_find_idx(co->od, idx);
        if (co->rpdo.od_var[i] == NULL)
            break;
    }
    co->rpdo.count = i;
}

void pdo_bus_init(CO* co)
{
    pdo_init(co);
    pdo_start_timer(co);
}

void pdo_bus_error(CO* co)
{
    timer_stop(co->tpdo.timer, COT_PDO, HAL_CANOPEN);
}

static inline bool pdo_is_rec_rpdo(CO* co)
{
    int i;
    uint32_t data;
    for (i = 0; i < co->rpdo.count; i++)
    {
        if (co->rec_msg.id == co->rpdo.cob_id[i]->data)
        {
            data = co->rec_msg.data.lo;
            if (co->rec_msg.data_len < (co->rpdo.od_var[i]->len & 0xFF))
                break;
            co->rpdo.od_var[i]->data = data;
            ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_OD_CHANGE),
                    co->rpdo.od_var[i]->idx, data, 0);
            return true;
        }
    }
    return false;
}

static inline void pdo_tpdo_req(CO* co, uint32_t id)
{
    int i;
    for (i = 0; i < CO_MAX_TPDO; i++)
    {
        if (co->tpdo.cob_id[i]->data == id)
        {
            pdo_send(co, id, co->tpdo.od_var[i]->data, co->tpdo.od_var[i]->len & 0xff);
            return;
        }
    }
}
//----------------------------------------
void co_led_change(CO* co, LED_STATE state)
{
    ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LED_STATE), 0, state, 0);
}

static void heartbeat_init(CO* co)
{
    CO_OD_ENTRY* entry = co_od_find_idx(co->od, CO_OD_ENTRY_HEARTBEAT_TIME);
    if (co->bus_state != BUS_RUN)
        return;
    if (entry == NULL)
        return;
    if (entry->data)
        timer_start_ms(co->timers.heartbeat, entry->data);
}

static inline void bus_init(CO* co) // can bus ready after init or restore after bus error
{
    if (co->id > 0x80)
    {
        co_led_change(co, LED_NO_CONFIG);
        return;
    }
// node ID valid, change state to OPERATE
    co_led_change(co, LED_OPERATE);
    co->out_msg.data_len = 1;
    co->out_msg.data.b0 = 0;
    co->out_msg.id = NODE_GUARD + co->id;
    can_write(&co->out_msg, &co->can_state);
    co->co_state = Operational;
    heartbeat_init(co);
    pdo_init(co);
}

static inline void bus_error(CO* co)
{
    timer_start_ms(co->timers.bus, CO_DEF_TIM_BUSRESTART);
    timer_stop(co->timers.heartbeat, COT_HEARTBEAT, HAL_CANOPEN);
    LED_STATE led = LED_BUSOFF;
    if (co->can_state.error == ARBITRATION_LOST)
        led = LED_LOST;
    if (co->can_state.state == INIT)
        led = LED_INIT;
    co_led_change(co, led);
    pdo_bus_error(co);
}

static inline void init(CO* co)
{
    co->bus_state = BUS_RESET;
    memset(co, 0, sizeof(CO));
    co->od = NULL;
    co->od_size = 0;
}

static inline void open(CO* co, IPC* ipc) // HANDLE device, IO* io)
{
    IO* io = (IO*)ipc->param2;
    CO_OD_ENTRY* od;
    if (co->od)
    {
        error (ERROR_ALREADY_CONFIGURED);
        return;
    }
    co->device = ipc->process;

    co->od_size = io->data_size;
    co->od = malloc(co->od_size);
    memcpy(co->od, io_data(io), co->od_size);

    co->timers.bus = timer_create(COT_BUS, HAL_CANOPEN);
    co->timers.heartbeat = timer_create(COT_HEARTBEAT, HAL_CANOPEN);
    co->timers.lss = timer_create(COT_LSS, HAL_CANOPEN);

    co->lss.lss_id.vendor = co_od_get_u32(co->od, 0x1018, 1);
    co->lss.lss_id.product = co_od_get_u32(co->od, 0x1018, 2);
    co->lss.lss_id.version = co_od_get_u32(co->od, 0x1018, 3);
    co->lss.lss_id.serial = co_od_get_u32(co->od, 0x1018, 4);
    co->id = ipc->param3;
    if ((co->id == 0) || (co->id > 0x7F))
        co->id = 0xFF;
    co->co_state = Pre_operational; //TODO:
    uint32_t baud = ipc->param1;
    if (baud == 0)
        baud = CO_DEF_BAUDRATE;
    can_open(baud);
    co->bus_state = BUS_INIT;
    timer_start_us(co->timers.bus, 30 * 1000000ul / baud);
    co->out_msg.rtr = 0;
    co->tpdo.timer = timer_create(COT_PDO, HAL_CANOPEN);
    co->tpdo.inhibit_timer = timer_create(COT_INHIBIT, HAL_CANOPEN);
    pdo_init(co);

    od = co_od_find_idx(co->od, CO_OD_ENTRY_INHIBIT_TIME);
    if(od)
    {
        if( (od->data > 0) && (od->data < CO_MIN_INHIBIT_TIME) )
            od->data = CO_MIN_INHIBIT_TIME;
        co->tpdo.inhibit_time  = od->data;
    }else
        co->tpdo.inhibit_time  = 0;

#if (CO_DEBUG)
    printf("LSS serial:%x \n", co->lss.lss_id.serial);
#endif

}

static inline void close(CO* co)
{
    free(co->od);
}

static inline void bus_state_change(CO* co)
{
    can_get_state(&co->can_state);
    switch (co->can_state.state)
    {
    case NORMAL:
        case PASSIVE_ERR:
        if (co->can_state.error == ARBITRATION_LOST)
        {
            co->bus_state = BUS_ERROR;
            bus_error(co);
        } else
        {
            co->bus_state = BUS_RUN;
            bus_init(co);
        }
        break;
    case INIT:
        case BUSOFF:
        if (co->bus_state == BUS_ERROR)
            return;
        co->bus_state = BUS_ERROR;
        bus_error(co);
    default:
        #if (CO_DEBUG)
        printf("bus_state_change error\n");
#endif
        return;
    }
#if (CO_DEBUG)
    printf("bus_state_changed to %u can state^%u\n", co->bus_state, co->can_state.state);
#endif
    ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_CHANGE_STATE), 0, co->bus_state, co->can_state.state);
}

static inline void heartbeat_timeout(CO* co)
{
    heartbeat_init(co);
    co->out_msg.id = NODE_GUARD + (co->id & MSK_ID);
    co->out_msg.data.b0 = co->co_state;
    co->out_msg.data_len = 1;
    can_write(&co->out_msg, &co->can_state);

#if (CO_DEBUG)
    printf("HEARTBEAT\n");
#endif

}

static inline void bus_timeout(CO* co)
{
    switch (co->bus_state)
    {
    case BUS_INIT:
        bus_state_change(co);
        break;
    case BUS_ERROR:
        can_clear_error();
        bus_state_change(co);
        break;
    default:
        break;
    }
}
//----------- SDO --------------
static void co_sdo_slave_replay(CO* co, uint8_t cmd, uint32_t data)
{
    SDO_PKT* pkt = (SDO_PKT*)&co->out_msg.data;
    co->out_msg.data_len = 8;
    co->out_msg.id = SDOtx + co->id;
    co->out_msg.rtr = 0;
    co->out_msg.data.hi = co->rec_msg.data.hi;
    co->out_msg.data.lo = co->rec_msg.data.lo;
    pkt->cmd = cmd;
    pkt->data = data;
    can_write(&co->out_msg, &co->can_state);
}

static inline uint32_t co_sdo_save_od(CO* co, CO_OD_ENTRY* entry, uint32_t data)
{
    uint32_t size;
    if (data != SDO_SAVE_MAGIC)
        return SDO_ERROR_STORE;
    size = co_od_size(co->od)*sizeof(CO_OD_ENTRY);
    ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_SAVE_OD), 0, CO_OD_SUBINDEX((entry->idx)) , size);
    return 0;
}

static inline uint32_t co_sdo_comm_changed(CO* co, CO_OD_ENTRY* entry, uint32_t data)
{
    if ((CO_OD_INDEX(entry->index) & 0xFF00) == 0x1800)
    {
        return pdo_tpdo_param_change(co, entry, data);
    }
    switch(CO_OD_INDEX(entry->idx))
    {
    case CO_OD_INDEX_SAVE_OD:
        return co_sdo_save_od(co, entry, data);
    case CO_OD_INDEX_RESTORE_OD:

        ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_RESTORE_OD), 0, CO_OD_SUBINDEX((entry->idx)), 0);
        return SDO_ERROR_OK;
    }

    switch (entry->idx)
    {
    case CO_OD_ENTRY_HEARTBEAT_TIME:
        if ((entry->data == 0) && (data != 0))
        {
            entry->data = data;
            heartbeat_init(co);
        }
        break;
    }
    entry->data = data;
    return SDO_ERROR_OK;
}

static inline void co_sdo_slave_request(CO* co)
{
    SDO_PKT* pkt = (SDO_PKT*)&co->rec_msg.data;
    CO_OD_ENTRY* entry = co_od_find(co->od, pkt->index, pkt->subindex);
    if (entry == NULL)
    {
        co_sdo_slave_replay(co, SDO_CMD_ABORT, SDO_ERROR_NOT_EXIST);
        return;
    }
    switch (pkt->cmd & SDO_CMD_MSK)
    {
    case SDO_CMD_DOWNLOAD_REQ:
        if (CO_OD_IS_RW(entry))
        {
            uint32_t err_code = 0;
            if( CO_OD_INDEX(entry->index) < CO_OD_FIRST_USER_INDEX)
                err_code = co_sdo_comm_changed(co, entry, pkt->data);
            else
            {
                pdo_data_changed(co, entry, pkt->data);
                ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_OD_CHANGE), entry->idx, pkt->data, 0);
                entry->data = pkt->data;
            }
//            = co_sdo_changed(co, entry, pkt->data);
            if (err_code == SDO_ERROR_OK)
                co_sdo_slave_replay(co, SDO_CMD_DOWNLOAD_RESP, 0);
            else
            co_sdo_slave_replay(co, SDO_CMD_ABORT, err_code);
        } else
        co_sdo_slave_replay(co, SDO_CMD_ABORT, SDO_ERROR_READONLY);
        break;
    case SDO_CMD_UPLOAD_REQ:
        co_sdo_slave_replay(co, SDO_CMD_UPLOAD_RESP | 0x03 | ((4 - (entry->len & 0x0F)) << 2), entry->data);
        break;
    default:
        break;
    }

}
#if (CO_LSS_MASTER)
static inline void co_sdo_master_replay(CO* co)
{
    SDO_PKT* pkt = (SDO_PKT*)&co->rec_msg.data;
    uint8_t id = co->rec_msg.id & 0x7F;
    ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_SDO_RESULT), id, pkt->cmd, pkt->data);
}

static inline void co_sdo_init_request(CO* co, IPC* ipc)
{
    SDO_PKT* pkt = (SDO_PKT*)&co->out_msg.data;
    co->out_msg.data_len = 8;
    co->out_msg.id = SDOrx + (ipc->param1 >>24);
    co->out_msg.rtr = 0;
    pkt->index = ipc->param1 & 0xFFFF;
    pkt->subindex = (ipc->param1 >>16) & 0xFF;
    switch((CO_SDO_TYPE)ipc->param2)
    {
        case CO_SDO_U8:
        pkt->cmd = 0x2F;
        pkt->data = ipc->param3 & 0xFF;
        break;
        case CO_SDO_U16:
        pkt->cmd = 0x2B;
        pkt->data = ipc->param3 & 0xFFFF;
        break;
        case CO_SDO_U32:
        pkt->cmd = 0x23;
        pkt->data = ipc->param3;
        break;
        case CO_SDO_GET:
        pkt->cmd = 0x40;
        pkt->data = 0;
        break;
        default:
        return;
    }
    can_write(&co->out_msg, &co->can_state);
}
#endif // CO_LSS_MASTER

static void co_nmt_request(CO* co)
{
    CAN_MSG* msg = &co->rec_msg;
    if( (msg->data_len != 2) || (msg->data.b1 != co->id) )
        return;
    switch(msg->data.b0)
    {
    case NMT_RESET_NODE:
        ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_HARDW_RESET), 0, 0, 0);
        break;
    }
}

//-----------------
static inline void can_received(CO* co)
{
#if (CO_DEBUG)
    printf("received %d  %d\n", co->rec_msg.id, co->rec_msg.data.b0);
#endif
    if ((co->rec_msg.id & MSK_FUNC) == LSS)
    {
        lss_request(co);
        return;
    }
    if (co->co_state != Operational)
        return;
    if (co->rec_msg.rtr)
    {
        pdo_tpdo_req(co, co->rec_msg.id);
        return;
    }
    if (pdo_is_rec_rpdo(co))
        return;
    switch (co->rec_msg.id & MSK_FUNC)
    {
#if (CO_LSS_MASTER)
        case SDOtx:         // from slave(server) to master(client)
        co_sdo_master_replay(co);
        break;
#endif // CO_LSS_MASTER
    case NMT:
        co_nmt_request(co);
        break;
    case SDOrx:         // from master(client) to slave(server)
//        if ((!IS_MY_ID) || (co->rec_msg.data_len != 8))
        if (!IS_MY_ID)
            break;
        co_sdo_slave_request(co);
        break;
    }
}
static inline void co_get_OD(CO* co, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
    io_data_append(io, co->od, ipc->param3);
    ipc->param3 = io->data_size;
}

static inline void co_fault(CO* co, uint32_t error_code)
{
    CO_OD_ENTRY* entry= co_od_find_idx(co->od, CO_OD_ENTRY_ERROR_REG);
    if(entry)
    {
        if(error_code)
            entry->data = 1;
        else
            entry->data = 0;
    }
    if (co->co_state != Operational)
        return;
    co->out_msg.id = SYNC + co->id;
    co->out_msg.data.lo = error_code;
    co->out_msg.data.hi = 0;
    co->out_msg.data_len = 8;
    can_write(&co->out_msg, &co->can_state);

}

static inline void canopen_request(CO* co, IPC* ipc)
{
    CO_OD_ENTRY* entry;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        open(co, ipc);         // ipc->process, (IO*)ipc->param2);
        break;
    case IPC_CLOSE:
        close(co);
        break;
    case IPC_CO_INIT:
        bus_init(co);
        break;
    case IPC_CO_ID_CHANGED:
        bus_init(co);
        pdo_start_timer(co);
//        pdo_init(co);
        break;
    case IPC_TIMEOUT:
        if (ipc->param1 & TIMER_PDO_MASK)
        {
            pdo_timeout(co, ipc->param1);
            break;
        }
        switch (ipc->param1)
        {
        case COT_BUS:
            bus_timeout(co);
            break;
        case COT_HEARTBEAT:
            heartbeat_timeout(co);
            break;
        case COT_LSS:
            lss_timeout(co);
            break;
        }
        break;
    case IPC_CO_FAULT:
        co_fault(co, ipc->param2);
        break;

    case IPC_CO_GET_OD:
        co_get_OD(co,ipc);
        break;
    case IPC_CO_OD_CHANGE:
        entry = co_od_find_idx(co->od, ipc->param1);
        if (entry)// if(CO_OD_INDEX(entry->idx) >= 0x2000)
        {
            pdo_data_changed(co, entry, ipc->param2);
            entry->data = ipc->param2;
        }
//            co_sdo_changed(co, entry, ipc->param2);
        break;

#if (CO_LSS_MASTER)
        case IPC_CO_LSS_FIND:
        lss_init_find(co, (IO*)ipc->param2);
        break;
        case IPC_CO_LSS_SET_ID:
        lss_init_set_id(co, ipc->param1);
        break;
        case IPC_CO_SDO_REQUEST:
        co_sdo_init_request(co, ipc);
        break;
        case IPC_CO_SEND_PDO:
        if (co->co_state != Operational)
        break;
        co->out_msg.id = PDO1tx + co->id + (((ipc->param1 - 1) & 0x03) << 8);
        co->out_msg.data.lo = ipc->param3;
        co->out_msg.data.hi = ipc->param2;
        co->out_msg.data_len = ipc->param1 >> 16;
        can_write(&co->out_msg, &co->can_state);
        break;

#endif // CO_LSS_MASTER
    default:
        error (ERROR_NOT_SUPPORTED);
    }
}

static inline void can_request(CO* co, IPC* ipc)
{
    can_state_decode(ipc, &co->can_state);
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_CAN_BUS_ERROR:
        bus_state_change(co);
        break;
    case IPC_CAN_TXC:
        if ((co->can_state.error == ARBITRATION_LOST) | (co->can_state.error == BUSOFF_ABORT))
            bus_state_change(co);
        break;
    case IPC_READ:
        if (co->bus_state != BUS_RUN)
            bus_state_change(co);
        can_decode(ipc, &co->rec_msg);
        can_received(co);
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
    }
}

void canopens_main()
{
    IPC ipc;
    CO co;
    init(&co);
#if (CO_DEBUG)
    open_stdout();
#endif //CO_DEBUG

    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_CAN:
            can_request(&co, &ipc);
            break;
        case HAL_CANOPEN:
            canopen_request(&co, &ipc);
            break;
        default:
            error (ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}
