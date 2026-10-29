import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_INVERTED,
    CONF_MODE,
    CONF_NUMBER,
    CONF_OUTPUT,
    CONF_PIN,
)

CODEOWNERS = ["@GitNik1"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

m5pm1_ns = cg.esphome_ns.namespace("m5pm1")
M5PM1Component = m5pm1_ns.class_("M5PM1Component", cg.Component, i2c.I2CDevice)
M5PM1GPIOPin = m5pm1_ns.class_("M5PM1GPIOPin", cg.GPIOPin)

CONF_M5PM1_ID = "m5pm1_id"
CONF_M5PM1 = "m5pm1"

CONF_CHARGE = "charge"
CONF_DCDC = "dcdc"
CONF_LDO = "ldo"
CONF_BOOST = "boost"
CONF_L3B = "l3b"
CONF_BATTERY_LOW_VOLTAGE_PROTECTION = "battery_low_voltage_protection"
CONF_I2C_SLEEP_TIMEOUT = "i2c_sleep_timeout"

# Actions
CONF_SECONDS = "seconds"
CONF_ACTION = "action"
CONF_MILLIVOLTS = "millivolts"
CONF_PULSES = "pulses"

TimerAction = m5pm1_ns.enum("TimerAction")
TIMER_ACTIONS = {
    "stop": TimerAction.TIM_ACTION_STOP,
    "flag": TimerAction.TIM_ACTION_FLAG,
    "reboot": TimerAction.TIM_ACTION_REBOOT,
    "power_on": TimerAction.TIM_ACTION_POWERON,
    "power_off": TimerAction.TIM_ACTION_POWEROFF,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(M5PM1Component),
            cv.Optional(CONF_CHARGE): cv.boolean,
            cv.Optional(CONF_DCDC): cv.boolean,
            cv.Optional(CONF_LDO): cv.boolean,
            cv.Optional(CONF_BOOST): cv.boolean,
            cv.Optional(CONF_L3B): cv.boolean,
            cv.Optional(CONF_BATTERY_LOW_VOLTAGE_PROTECTION): cv.All(
                cv.voltage, lambda v: int(v * 1000)
            ),
            cv.Optional(CONF_I2C_SLEEP_TIMEOUT): cv.int_range(min=0, max=15),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x6E))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_CHARGE in config:
        cg.add(var.set_charge_boot(1 if config[CONF_CHARGE] else 0))
    if CONF_DCDC in config:
        cg.add(var.set_dcdc_boot(1 if config[CONF_DCDC] else 0))
    if CONF_LDO in config:
        cg.add(var.set_ldo_boot(1 if config[CONF_LDO] else 0))
    if CONF_BOOST in config:
        cg.add(var.set_boost_boot(1 if config[CONF_BOOST] else 0))
    if CONF_L3B in config:
        cg.add(var.set_l3b_boot(1 if config[CONF_L3B] else 0))
    if CONF_BATTERY_LOW_VOLTAGE_PROTECTION in config:
        cg.add(var.set_battery_lvp_boot(config[CONF_BATTERY_LOW_VOLTAGE_PROTECTION]))
    if CONF_I2C_SLEEP_TIMEOUT in config:
        cg.add(var.set_i2c_sleep_boot(config[CONF_I2C_SLEEP_TIMEOUT]))


# ---------------------------------------------------------------------------
# GPIO expander pin
# ---------------------------------------------------------------------------


def _validate_pin_mode(value):
    return value


M5PM1_PIN_SCHEMA = pins.gpio_base_schema(
    M5PM1GPIOPin,
    cv.int_range(min=0, max=4),
    modes=[CONF_INPUT, CONF_OUTPUT],
    mode_validator=_validate_pin_mode,
    invertible=True,
).extend(
    {
        cv.Required(CONF_M5PM1): cv.use_id(M5PM1Component),
    }
)


@pins.PIN_SCHEMA_REGISTRY.register(CONF_M5PM1, M5PM1_PIN_SCHEMA)
async def m5pm1_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_M5PM1])
    cg.add(var.set_parent(parent))
    cg.add(var.set_pin(config[CONF_NUMBER]))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var


# ---------------------------------------------------------------------------
# Actions
# ---------------------------------------------------------------------------

_SIMPLE_ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.GenerateID(): cv.use_id(M5PM1Component)}
)


def _register_simple_action(name, action_class):
    @automation.register_action(
        name, action_class, _SIMPLE_ACTION_SCHEMA, synchronous=True
    )
    async def _to_code(config, action_id, template_arg, args):
        parent = await cg.get_variable(config[CONF_ID])
        return cg.new_Pvariable(action_id, template_arg, parent)


_register_simple_action("m5pm1.shutdown", m5pm1_ns.class_("ShutdownAction", automation.Action))
_register_simple_action("m5pm1.reboot", m5pm1_ns.class_("RebootAction", automation.Action))
_register_simple_action(
    "m5pm1.enter_download_mode", m5pm1_ns.class_("DownloadModeAction", automation.Action)
)
_register_simple_action(
    "m5pm1.feed_watchdog", m5pm1_ns.class_("FeedWatchdogAction", automation.Action)
)

SetTimerAction = m5pm1_ns.class_("SetTimerAction", automation.Action)


@automation.register_action(
    "m5pm1.set_timer",
    SetTimerAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5PM1Component),
            cv.Required(CONF_SECONDS): cv.templatable(cv.positive_int),
            cv.Optional(CONF_ACTION, default="power_on"): cv.enum(TIMER_ACTIONS, lower=True),
        }
    ),
    synchronous=True,
)
async def set_timer_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    seconds = await cg.templatable(config[CONF_SECONDS], args, cg.uint32)
    cg.add(var.set_seconds(seconds))
    cg.add(var.set_action(config[CONF_ACTION]))
    return var


SetBatteryLvpAction = m5pm1_ns.class_("SetBatteryLvpAction", automation.Action)


@automation.register_action(
    "m5pm1.set_battery_lvp",
    SetBatteryLvpAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5PM1Component),
            cv.Required(CONF_MILLIVOLTS): cv.templatable(cv.int_range(min=2000, max=4000)),
        }
    ),
    synchronous=True,
)
async def set_battery_lvp_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    mv = await cg.templatable(config[CONF_MILLIVOLTS], args, cg.uint16)
    cg.add(var.set_millivolts(mv))
    return var


Aw8737aPulseAction = m5pm1_ns.class_("Aw8737aPulseAction", automation.Action)


@automation.register_action(
    "m5pm1.aw8737a_pulse",
    Aw8737aPulseAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5PM1Component),
            cv.Required(CONF_PIN): cv.templatable(cv.int_range(min=0, max=4)),
            cv.Required(CONF_PULSES): cv.templatable(cv.int_range(min=0, max=3)),
        }
    ),
    synchronous=True,
)
async def aw8737a_pulse_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    pin = await cg.templatable(config[CONF_PIN], args, cg.uint8)
    pulses = await cg.templatable(config[CONF_PULSES], args, cg.uint8)
    cg.add(var.set_pin(pin))
    cg.add(var.set_pulses(pulses))
    return var
