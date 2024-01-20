#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zephyr/stats/stats.h>
#ifdef CONFIG_MCUMGR_GRP_STAT
#include <zephyr/mgmt/mcumgr/grp/stat_mgmt/stat_mgmt.h>
#endif

/***************************************************************************************************
 * LOGs
 ***************************************************************************************************/
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(zbox);


/***************************************************************************************************
 * SMP Server Statistics Management
 ***************************************************************************************************/
// Define an example stats group
STATS_SECT_START(smp_server_stats)
STATS_SECT_ENTRY(counter_stat)
STATS_SECT_END;

// Assign a name to the `counter_stat` stat.
STATS_NAME_START(smp_server_stats)
STATS_NAME(smp_server_stats, counter_stat)
STATS_NAME_END(smp_server_stats);

// Define an instance of the stats group.
STATS_SECT_DECL(smp_server_stats) smp_server_stats;

/***************************************************************************************************
 * Switch between GREEN_BIN and BLUE_BIN
 ***************************************************************************************************/
// Choose one LED to blink
// #define GREEN_BIN
#define BLUE_BIN

#ifdef GREEN_BIN

#define GREEN_LED_NODE DT_NODELABEL(green_led_4)
static const struct gpio_dt_spec led_dt_spec = GPIO_DT_SPEC_GET_OR(GREEN_LED_NODE, gpios, {0});

#elif defined(BLUE_BIN)

#define BLUE_LED_NODE DT_NODELABEL(blue_led_6)
static const struct gpio_dt_spec led_dt_spec = GPIO_DT_SPEC_GET_OR(BLUE_LED_NODE, gpios, {0});

#endif

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

int main(void)
{
    int rc = STATS_INIT_AND_REG(smp_server_stats, STATS_SIZE_32, "smp_server_stats");

    if (rc < 0)
    {
        LOG_ERR("Error initializing stats system [%d]", rc);
        return 0;
    }

    if (!gpio_is_ready_dt(&led_dt_spec))
    {
        return 0;
    }

    rc = gpio_pin_configure_dt(&led_dt_spec, GPIO_OUTPUT_ACTIVE);
    if (rc < 0)
    {
        return 0;
    }

    while (1)
    {
        rc = gpio_pin_toggle_dt(&led_dt_spec);
        if (rc < 0)
        {
            return 0;
        }
        k_msleep(SLEEP_TIME_MS);
        STATS_INC(smp_server_stats, counter_stat);
    }
    return 0;
}
