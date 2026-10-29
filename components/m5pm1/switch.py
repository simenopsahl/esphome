import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_PIN, CONF_TYPE

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1Switch = m5pm1_ns.class_("M5PM1Switch", switch.Switch, cg.Component)
SwitchType = m5pm1_ns.enum("M5PM1SwitchType")

TYPES = {
    "charge": SwitchType.SWITCH_CHARGE,
    "dcdc": SwitchType.SWITCH_DCDC,
    "ldo": SwitchType.SWITCH_LDO,
    "boost": SwitchType.SWITCH_BOOST,
    "led_en_level": SwitchType.SWITCH_LED_LEVEL,
    "ldo_hold": SwitchType.SWITCH_LDO_HOLD,
    "boost_hold": SwitchType.SWITCH_BOOST_HOLD,
    "gpio_hold": SwitchType.SWITCH_GPIO_HOLD,
}


def _validate(config):
    if config[CONF_TYPE] == "gpio_hold" and CONF_PIN not in config:
        raise cv.Invalid("'pin' is required when type is 'gpio_hold'")
    if config[CONF_TYPE] != "gpio_hold" and CONF_PIN in config:
        raise cv.Invalid("'pin' is only valid when type is 'gpio_hold'")
    return config


CONFIG_SCHEMA = cv.All(
    switch.switch_schema(M5PM1Switch)
    .extend(
        {
            cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component),
            cv.Required(CONF_TYPE): cv.enum(TYPES, lower=True),
            cv.Optional(CONF_PIN): cv.int_range(min=0, max=4),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    _validate,
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(config[CONF_TYPE]))
    if CONF_PIN in config:
        cg.add(var.set_pin(config[CONF_PIN]))
