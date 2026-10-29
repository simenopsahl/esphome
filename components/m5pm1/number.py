import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_TYPE,
    CONF_UNIT_OF_MEASUREMENT,
    UNIT_HERTZ,
    UNIT_SECOND,
)

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1Number = m5pm1_ns.class_("M5PM1Number", number.Number, cg.Component)
NumberType = m5pm1_ns.enum("M5PM1NumberType")

# type -> (enum, min, max, step, unit)
NUMBER_TYPES = {
    "battery_lvp": (NumberType.NUMBER_BATTERY_LVP, 2000, 4000, 10, "mV"),
    "watchdog_timeout": (NumberType.NUMBER_WATCHDOG_TIMEOUT, 0, 255, 1, UNIT_SECOND),
    "pwm_frequency": (NumberType.NUMBER_PWM_FREQUENCY, 1, 65535, 1, UNIT_HERTZ),
}

CONFIG_SCHEMA = (
    number.number_schema(M5PM1Number)
    .extend(
        {
            cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component),
            cv.Required(CONF_TYPE): cv.one_of(*NUMBER_TYPES, lower=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    enum_val, mn, mx, step, unit = NUMBER_TYPES[config[CONF_TYPE]]
    config.setdefault(CONF_UNIT_OF_MEASUREMENT, unit)
    var = await number.new_number(
        config, min_value=mn, max_value=mx, step=step
    )
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(enum_val))
