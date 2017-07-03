#include "sys_config.h"
#include "process.h"
#include "stdio.h"
#include "sys.h"
#include "io.h"
#include "canopens_private.h"

#define CO_DEF_BAUDRATE 250000
#define CO_DEF_HEATBEART  2000 // in ms
#define CO_DEF_TIM_BUSRESTART  4000 // in ms

#include <string.h>

void co_led_change(CO* co, LED_STATE state)
{
    ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_LED_STATE), 0, state, 0);
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
    timer_start_ms(co->timers.heartbeat, CO_DEF_HEATBEART);
    co->out_msg.data_len = 1;
    co->out_msg.data.b0 = 0;
    co->out_msg.id = NODE_GUARD + co->id;
    can_write(&co->out_msg, &co->can_state);
    co->co_state = Operational;
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
}

static inline void init(CO* co)
{
    co->bus_state = BUS_RESET;
}

static inline void open(CO* co, IPC* ipc)
{
    co->device = ipc->process;
    co->timers.bus = timer_create(COT_BUS, HAL_CANOPEN);
    co->timers.heartbeat = timer_create(COT_HEARTBEAT, HAL_CANOPEN);
    co->timers.lss = timer_create(COT_LSS, HAL_CANOPEN);
    co->lss.id = ipc->param2;
    co->id = ipc->param3;
    if ((co->id == 0) || (co->id > 0x7F))
        co->id = 0xFF;
    co->co_state = Pre_operational; //TODO:
    uint32_t baud = ipc->param1;
    if (baud == 0)
        baud = CO_DEF_BAUDRATE;
    can_open(baud);
    co->bus_state = BUS_INIT;
    timer_start_us(co->timers.bus, 30 * 1000000 / baud);
    co->out_msg.rtr = 0;
#if (CO_DEBUG)
    printf("LSS id:%x \n", co->lss.id);
#endif

}

static inline void close(CO* co)
{

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
    if (co->bus_state != BUS_RUN)
        return;
    timer_start_ms(co->timers.heartbeat, CO_DEF_HEATBEART);

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

static inline void canopen_request(CO* co, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        open(co, ipc);
        break;
    case IPC_CLOSE:
        close(co);
        break;
    case IPC_CO_INIT:
        bus_init(co);
        break;
    case IPC_TIMEOUT:
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
        co->out_msg.id = SYNC + co->id;
        co->out_msg.data.lo = ipc->param2;
        co->out_msg.data.hi = 0;
        co->out_msg.data_len = 8;
        can_write(&co->out_msg, &co->can_state);
        break;
#if (CO_LSS_MASTER)
        case IPC_CO_LSS_FIND:
        lss_init_find(co, ipc->param2, ipc->param3);
        break;
        case IPC_CO_LSS_SET_ID:
        lss_init_set_id(co, ipc->param1);
        break;
#endif
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
        switch (co->rec_msg.id & MSK_FUNC)
        {
        case NMT:
            break;
        case PDO1rx:
            if ((co->rec_msg.id & 0x7F) != co->id)
                break;
            ipc_post_inline(co->device, HAL_CMD(HAL_CANOPEN, IPC_CO_PDO), co->rec_msg.data_len, co->rec_msg.data.b0,
                    co->rec_msg.data.b1);
            break;
        case LSS:
            lss_request(co);
            break;
        }
#if (CO_DEBUG)
        printf("received %d  %d\n", co->rec_msg.id, co->rec_msg.data.b0);
#endif
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
